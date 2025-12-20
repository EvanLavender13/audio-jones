// AudioJones Web Control
// WebSocket client for real-time analysis streaming

// Alpine store initialization
document.addEventListener('alpine:init', () => {
    Alpine.store('app', {
        // Connection state
        connected: false,
        statusText: 'Connecting...',

        // Analysis values (updated at 20Hz)
        beat: 0,
        bass: 0,
        mid: 0,
        treb: 0,

        // Config state
        channelMode: 2,

        // Computed: meter percentages (clamped)
        get beatPercent() { return Math.min((this.beat / 1.0) * 100, 100); },
        get bassPercent() { return Math.min((this.bass / 3.0) * 100, 100); },
        get midPercent() { return Math.min((this.mid / 3.0) * 100, 100); },
        get trebPercent() { return Math.min((this.treb / 3.0) * 100, 100); },

        setStatus(connected, text) {
            this.connected = connected;
            this.statusText = text;
        },

        updateAnalysis(data) {
            this.beat = data.beat;
            this.bass = data.bass;
            this.mid = data.mid;
            this.treb = data.treb;
        },

        updateConfig(config) {
            if (config.audio?.channelMode != null) {
                this.channelMode = config.audio.channelMode;
            }
        },

        onChannelChange() {
            window.sendCommand('setAudioChannel', this.channelMode);
        }
    });

    // Initialize WebSocket after store is ready
    initWebSocket();
});

function initWebSocket() {
    'use strict';

    // WebSocket runs on HTTP port + 1
    const WS_PORT = 8081;
    const RECONNECT_DELAY = 2000;

    let ws = null;
    let reconnectTimeout = null;

    function handleMessage(event) {
        try {
            const msg = JSON.parse(event.data);

            if (msg.type === 'analysis') {
                Alpine.store('app').updateAnalysis(msg);
            }
            else if (msg.type === 'config') {
                Alpine.store('app').updateConfig(msg);
            }
        } catch (e) {
            console.error('Failed to parse message:', e);
        }
    }

    function connect() {
        // Clear any pending reconnect
        if (reconnectTimeout) {
            clearTimeout(reconnectTimeout);
            reconnectTimeout = null;
        }

        const wsUrl = 'ws://' + window.location.hostname + ':' + WS_PORT;
        console.log('Connecting to', wsUrl);
        Alpine.store('app').setStatus(false, 'Connecting...');

        ws = new WebSocket(wsUrl);

        ws.onopen = function() {
            console.log('WebSocket connected');
            Alpine.store('app').setStatus(true, 'Connected');
        };

        ws.onclose = function() {
            console.log('WebSocket disconnected');
            Alpine.store('app').setStatus(false, 'Disconnected - reconnecting...');
            scheduleReconnect();
        };

        ws.onerror = function(error) {
            console.error('WebSocket error:', error);
        };

        ws.onmessage = handleMessage;
    }

    function scheduleReconnect() {
        if (!reconnectTimeout) {
            reconnectTimeout = setTimeout(function() {
                reconnectTimeout = null;
                connect();
            }, RECONNECT_DELAY);
        }
    }

    window.sendCommand = function(cmd, value) {
        if (ws && ws.readyState === WebSocket.OPEN) {
            ws.send(JSON.stringify({ cmd: cmd, value: value }));
        }
    };

    // Start connection
    connect();
}
