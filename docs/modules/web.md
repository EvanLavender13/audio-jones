# Web Module

> Part of [AudioJones](../architecture.md)

## Purpose

HTTP server with WebSocket streaming for browser-based remote control. Serves static files from the web/ directory and provides bidirectional communication for config updates and real-time analysis data.

## Files

- `src/web/web_server.h` - Server API
- `src/web/web_server.cpp` - cpp-httplib and IXWebSocket implementation
- `src/web/web_bridge.h` - JSON serialization API
- `src/web/web_bridge.cpp` - Message parsing and config application

## Function Reference

### WebServer

| Function | Purpose |
|----------|---------|
| `WebServerInit` | Creates HTTP server on specified port, WebSocket on port+1 |
| `WebServerUninit` | Stops server threads and frees resources |
| `WebServerSetup` | Configures routes and WebSocket handlers, starts server threads |
| `WebServerProcessCommands` | Processes queued commands from WebSocket clients (call at frame start) |
| `WebServerBroadcastAnalysis` | Sends JSON analysis data (beat, bands) to all connected clients |

### WebBridge

| Function | Purpose |
|----------|---------|
| `WebBridgeSerializeAnalysis` | Encodes beat intensity and band energies as JSON |
| `WebBridgeApplyCommand` | Parses JSON command and applies to config structs |
| `WebBridgeSerializeConfig` | Encodes current config state as JSON for client sync |

## Types

### WebServer (opaque)

Internal structure containing cpp-httplib server, IXWebSocket server, command queue, and client list.

## Constants

| Constant | Value | Purpose |
|----------|-------|---------|
| Default HTTP port | 8080 | Static file serving |
| WebSocket port | 8081 | Real-time communication |
| Broadcast rate | 20Hz | Analysis data streaming frequency |

## Data Flow

1. **Entry:** WebSocket commands from browser clients (JSON)
2. **Queue:** Commands stored in thread-safe queue
3. **Process:** Main thread polls queue and applies to AppConfigs
4. **Broadcast:** Analysis data (beat/bands) sent to all clients at 20Hz
5. **Exit:** HTTP responses, WebSocket messages
