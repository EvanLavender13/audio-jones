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

        // Preset state
        presets: [],
        selectedPreset: null,
        presetName: '',
        presetStatus: '',
        pendingLoadPreset: null,

        // Config state (populated on preset load)
        effects: {},
        waveformCount: 1,
        waveforms: [],
        spectrum: {},
        bands: {},

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
            // Full preset data comes nested under 'preset' key
            const p = config.preset;
            if (p) {
                // Audio
                if (p.audio) {
                    this.channelMode = p.audio.channelMode;
                }

                // Effects
                if (p.effects) {
                    this.effects = p.effects;
                }

                // Waveforms
                if (p.waveforms && p.waveformCount != null) {
                    this.waveformCount = p.waveformCount;
                    this.waveforms = p.waveforms.slice(0, p.waveformCount);
                }

                // Spectrum
                if (p.spectrum) {
                    this.spectrum = p.spectrum;
                }

                // Bands
                if (p.bands) {
                    this.bands = p.bands;
                }
            }
            // Legacy: direct audio config (initial connect)
            else if (config.audio?.channelMode != null) {
                this.channelMode = config.audio.channelMode;
            }
        },

        updatePresets(data) {
            if (data.presets) {
                this.presets = data.presets;
            }
            if (data.message) {
                this.presetStatus = data.message;
                setTimeout(() => { this.presetStatus = ''; }, 3000);
            }
            if (data.success && this.pendingLoadPreset) {
                this.selectedPreset = this.pendingLoadPreset;
                this.pendingLoadPreset = null;
            }
        },

        loadPreset(filename) {
            if (!this.connected) return;
            this.pendingLoadPreset = filename;
            window.sendCommand('presetLoad', filename);
        },

        savePreset() {
            if (!this.connected) return;
            const name = this.presetName.trim();
            if (!name) return;
            window.sendCommand('presetSave', name);
            this.presetName = '';
        },

        deletePreset(filename) {
            if (!this.connected) return;
            if (!confirm('Delete ' + filename + '?')) return;
            window.sendCommand('presetDelete', filename);
        },

        refreshPresets() {
            if (!this.connected) return;
            window.sendCommand('presetList', null);
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
            else if (msg.type === 'presetStatus') {
                Alpine.store('app').updatePresets(msg);
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
            // Request initial preset list
            window.sendCommand('presetList', null);
        };

        ws.onclose = function() {
            console.log('WebSocket disconnected');
            Alpine.store('app').setStatus(false, 'Connecting...');
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
            let payload = { cmd: cmd };
            if (cmd === 'presetLoad' || cmd === 'presetDelete') {
                payload.filename = value;
            } else if (cmd === 'presetSave') {
                payload.name = value;
            } else if (value !== null) {
                payload.value = value;
            }
            ws.send(JSON.stringify(payload));
        }
    };

    // Start connection
    connect();
}
