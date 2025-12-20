// AudioJones Web Control
// WebSocket client for real-time analysis streaming

(function() {
    'use strict';

    // WebSocket runs on HTTP port + 1
    const WS_PORT = 8081;
    const RECONNECT_DELAY = 2000;

    let ws = null;
    let reconnectTimeout = null;

    // DOM elements
    const statusEl = document.getElementById('status');
    const beatMeter = document.getElementById('beat-meter');
    const bassMeter = document.getElementById('bass-meter');
    const midMeter = document.getElementById('mid-meter');
    const trebMeter = document.getElementById('treb-meter');
    const beatValue = document.getElementById('beat-value');
    const bassValue = document.getElementById('bass-value');
    const midValue = document.getElementById('mid-value');
    const trebValue = document.getElementById('treb-value');
    const channelModeSelect = document.getElementById('channel-mode');

    function setStatus(connected, text) {
        statusEl.textContent = text;
        statusEl.className = 'status ' + (connected ? 'connected' : 'disconnected');
        channelModeSelect.disabled = !connected;
    }

    function updateMeter(meterEl, valueEl, value, maxValue) {
        const clamped = Math.min(Math.max(value, 0), maxValue);
        const percent = (clamped / maxValue) * 100;
        meterEl.style.width = percent + '%';
        valueEl.textContent = value.toFixed(2);
    }

    function handleMessage(event) {
        try {
            const msg = JSON.parse(event.data);

            if (msg.type === 'analysis') {
                // Beat is 0-1, bands are normalized (typically 0-3 range)
                updateMeter(beatMeter, beatValue, msg.beat, 1.0);
                updateMeter(bassMeter, bassValue, msg.bass, 3.0);
                updateMeter(midMeter, midValue, msg.mid, 3.0);
                updateMeter(trebMeter, trebValue, msg.treb, 3.0);
            }
            else if (msg.type === 'config') {
                // Apply config to UI controls
                if (msg.audio && typeof msg.audio.channelMode === 'number') {
                    channelModeSelect.value = msg.audio.channelMode;
                }
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
        setStatus(false, 'Connecting...');

        ws = new WebSocket(wsUrl);

        ws.onopen = function() {
            console.log('WebSocket connected');
            setStatus(true, 'Connected');
        };

        ws.onclose = function() {
            console.log('WebSocket disconnected');
            setStatus(false, 'Disconnected - reconnecting...');
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

    function sendCommand(cmd, value) {
        if (ws && ws.readyState === WebSocket.OPEN) {
            ws.send(JSON.stringify({ cmd: cmd, value: value }));
        }
    }

    // Wire up controls
    channelModeSelect.addEventListener('change', function() {
        sendCommand('setAudioChannel', parseInt(this.value, 10));
    });

    // Start connection
    connect();
})();
