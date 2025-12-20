#include "web_server.h"
#include "web_bridge.h"

#include <stdlib.h>
#include <stdio.h>
#include <thread>
#include <atomic>
#include <mutex>
#include <vector>
#include <string>

#include "httplib.h"
#include "nlohmann/json.hpp"
#include "ixwebsocket/IXWebSocketServer.h"

// Constants and forward declaration for preset operations
// (preset.h not included to avoid raylib/Windows header conflicts)
// These values must match preset.h definitions
#ifndef MAX_PRESET_FILES
#define MAX_PRESET_FILES 32
#endif

int PresetListFiles(const char* directory, char outFiles[][PRESET_PATH_MAX], int maxFiles);

using json = nlohmann::json;

struct WebServer {
    httplib::Server httpServer;
    ix::WebSocketServer* wsServer;
    std::thread httpThread;
    std::atomic<bool> running;
    std::string webRoot;
    int httpPort;
    int wsPort;
    AppConfigs* configs;

    std::mutex commandMutex;
    std::vector<std::string> commandQueue;
};

static void BroadcastToClients(ix::WebSocketServer* wsServer, const char* message)
{
    if (wsServer == NULL || message == NULL) {
        return;
    }
    auto clients = wsServer->getClients();
    for (auto& client : clients) {
        client->send(message);
    }
}

WebServer* WebServerInit(const char* webRoot, int port)
{
    WebServer* ws = new (std::nothrow) WebServer();
    if (ws == NULL) {
        return NULL;
    }

    ws->webRoot = webRoot;
    ws->httpPort = port;
    ws->wsPort = port + 1;  // WebSocket on port 8081
    ws->configs = NULL;
    ws->running = false;
    ws->wsServer = NULL;

    return ws;
}

void WebServerUninit(WebServer* server)
{
    if (server == NULL) {
        return;
    }

    if (server->running) {
        // Stop WebSocket server
        if (server->wsServer != NULL) {
            server->wsServer->stop();
            delete server->wsServer;
        }

        // Stop HTTP server
        server->httpServer.stop();
        if (server->httpThread.joinable()) {
            server->httpThread.join();
        }
    }

    delete server;
}

void WebServerSetup(WebServer* server, AppConfigs* configs)
{
    if (server == NULL) {
        return;
    }

    server->configs = configs;

    // Serve static files from web root
    if (!server->httpServer.set_mount_point("/", server->webRoot)) {
        printf("WebServer: Failed to mount web root: %s\n", server->webRoot.c_str());
        return;
    }

    // Start HTTP server in background thread
    server->running = true;
    server->httpThread = std::thread([server]() {
        printf("WebServer: HTTP starting on port %d, serving from %s\n",
               server->httpPort, server->webRoot.c_str());
        if (!server->httpServer.listen("0.0.0.0", server->httpPort)) {
            printf("WebServer: HTTP failed to start on port %d\n", server->httpPort);
        }
    });

    // Create WebSocket server with port in constructor
    server->wsServer = new ix::WebSocketServer(server->wsPort, "0.0.0.0");

    // Setup WebSocket callbacks
    server->wsServer->setOnClientMessageCallback(
        [server](std::shared_ptr<ix::ConnectionState> connectionState,
                 ix::WebSocket& webSocket,
                 const ix::WebSocketMessagePtr& msg) {
            if (msg->type == ix::WebSocketMessageType::Open) {
                printf("WebServer: WebSocket client connected from %s\n",
                       connectionState->getRemoteIp().c_str());

                // Send current config to new client
                if (server->configs != NULL) {
                    char configJson[8192];
                    WebBridgeSerializeConfig(server->configs, configJson, sizeof(configJson));
                    webSocket.send(configJson);
                }

                // Send initial preset list to new client
                char files[MAX_PRESET_FILES][PRESET_PATH_MAX];
                int count = PresetListFiles("presets", files, MAX_PRESET_FILES);
                char presetJson[4096];
                WebBridgeSerializePresetStatus(true, NULL, files, count, presetJson, sizeof(presetJson));
                webSocket.send(presetJson);
            }
            else if (msg->type == ix::WebSocketMessageType::Close) {
                printf("WebServer: WebSocket client disconnected\n");
            }
            else if (msg->type == ix::WebSocketMessageType::Message) {
                // Queue command for main thread processing
                std::lock_guard<std::mutex> lock(server->commandMutex);
                server->commandQueue.push_back(msg->str);
            }
            else if (msg->type == ix::WebSocketMessageType::Error) {
                printf("WebServer: WebSocket error: %s\n", msg->errorInfo.reason.c_str());
            }
        }
    );

    // Start WebSocket server
    auto res = server->wsServer->listen();
    if (!res.first) {
        printf("WebServer: WebSocket failed to start on port %d: %s\n",
               server->wsPort, res.second.c_str());
        return;
    }

    server->wsServer->start();
    printf("WebServer: WebSocket starting on port %d\n", server->wsPort);
}

void WebServerProcessCommands(WebServer* server)
{
    if (server == NULL || server->configs == NULL) {
        return;
    }

    std::vector<std::string> commands;
    {
        std::lock_guard<std::mutex> lock(server->commandMutex);
        commands.swap(server->commandQueue);
    }

    for (const std::string& cmdStr : commands) {
        try {
            json msg = json::parse(cmdStr);
            if (!msg.contains("cmd")) {
                continue;
            }

            std::string cmd = msg["cmd"].get<std::string>();
            bool success = WebBridgeApplyCommand(server->configs, cmdStr.c_str());

            // Broadcast status for preset commands
            if (cmd == "presetList" || cmd == "presetLoad" ||
                cmd == "presetSave" || cmd == "presetDelete") {
                const char* message = NULL;
                char msgBuf[128];

                if (cmd == "presetLoad" && success) {
                    snprintf(msgBuf, sizeof(msgBuf), "Loaded %s",
                             msg["filename"].get<std::string>().c_str());
                    message = msgBuf;

                    // Broadcast full config so web UI controls update
                    char configJson[8192];
                    WebBridgeSerializeConfig(server->configs, configJson, sizeof(configJson));
                    BroadcastToClients(server->wsServer, configJson);
                } else if (cmd == "presetSave" && success) {
                    snprintf(msgBuf, sizeof(msgBuf), "Saved %s.json",
                             msg["name"].get<std::string>().c_str());
                    message = msgBuf;
                } else if (cmd == "presetDelete" && success) {
                    snprintf(msgBuf, sizeof(msgBuf), "Deleted %s",
                             msg["filename"].get<std::string>().c_str());
                    message = msgBuf;
                } else if (!success) {
                    message = "Operation failed";
                }

                WebServerBroadcastPresetStatus(server, success, message);
            }
        } catch (...) {
            continue;
        }
    }
}

void WebServerBroadcastAnalysis(WebServer* server,
                                 const BeatDetector* beat,
                                 const BandEnergies* bands)
{
    if (server == NULL || !server->running || server->wsServer == NULL) {
        return;
    }

    char jsonBuf[256];
    WebBridgeSerializeAnalysis(beat, bands, jsonBuf, sizeof(jsonBuf));
    BroadcastToClients(server->wsServer, jsonBuf);
}

void WebServerBroadcastPresetStatus(WebServer* server,
                                     bool success,
                                     const char* message)
{
    if (server == NULL || !server->running || server->wsServer == NULL) {
        return;
    }

    char files[MAX_PRESET_FILES][PRESET_PATH_MAX];
    int count = PresetListFiles("presets", files, MAX_PRESET_FILES);

    char jsonBuf[4096];
    WebBridgeSerializePresetStatus(success, message, files, count, jsonBuf, sizeof(jsonBuf));
    BroadcastToClients(server->wsServer, jsonBuf);
}
