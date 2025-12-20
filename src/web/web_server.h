#ifndef WEB_SERVER_H
#define WEB_SERVER_H

// Forward declarations to avoid raylib/Windows header conflicts
struct AppConfigs;
struct BeatDetector;
struct BandEnergies;

typedef struct WebServer WebServer;

// Initialize web server on specified port, serving static files from webRoot
// Returns NULL on failure
WebServer* WebServerInit(const char* webRoot, int port);

// Cleanup and stop server
void WebServerUninit(WebServer* server);

// Setup routes and config pointer (call after configs ready)
void WebServerSetup(WebServer* server, AppConfigs* configs);

// Process queued commands from WebSocket clients (call at start of frame)
void WebServerProcessCommands(WebServer* server);

// Broadcast analysis data to all connected WebSocket clients
void WebServerBroadcastAnalysis(WebServer* server,
                                 const BeatDetector* beat,
                                 const BandEnergies* bands);

#endif // WEB_SERVER_H
