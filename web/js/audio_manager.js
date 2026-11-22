/**
 * Audio Manager & Web Audio Analyzer
 * Manages daily scheduled music, alert sounds, weather voice broadcasts,
 * and feeds real-time audio spectrum data into Wallpaper Engine web wallpapers!
 */

class AudioManager {
    constructor() {
        this.audioCtx = null;
        this.analyser = null;
        this.musicAudio = new Audio();
        this.sfxAudio = new Audio();
        this.voiceAudio = new Audio();
        this.volume = 0.8;
        this.isPlaying = false;
        this.audioDataCallback = null;
        this.animFrame = null;

        this.initMusicEvents();
    }

    initAudioContext() {
        if (!this.audioCtx) {
            const AudioContext = window.AudioContext || window.webkitAudioContext;
            this.audioCtx = new AudioContext();
            this.analyser = this.audioCtx.createAnalyser();
            this.analyser.fftSize = 128; // 64 frequency bins

            const source = this.audioCtx.createMediaElementSource(this.musicAudio);
            source.connect(this.analyser);
            this.analyser.connect(this.audioCtx.destination);

            this.startAudioLoop();
        }
    }

    startAudioLoop() {
        const bufferLength = this.analyser.frequencyBinCount;
        const dataArray = new Uint8Array(bufferLength);

        const loop = () => {
            if (this.isPlaying && this.audioDataCallback) {
                this.analyser.getByteFrequencyData(dataArray);
                // Normalize 0.0 - 1.0 for Wallpaper Engine audio visualizers
                const normalized = [];
                for (let i = 0; i < bufferLength; i++) {
                    normalized.push(dataArray[i] / 255.0);
                }
                this.audioDataCallback(normalized);
            }
            this.animFrame = requestAnimationFrame(loop);
        };
        loop();
    }

    onAudioData(callback) {
        this.audioDataCallback = callback;
    }

    setVolume(val) {
        this.volume = Math.max(0, Math.min(1, val));
        this.musicAudio.volume = this.volume;
        this.sfxAudio.volume = this.volume;
        this.voiceAudio.volume = this.volume;
    }

    initMusicEvents() {
        this.musicAudio.addEventListener('play', () => {
            this.isPlaying = true;
            if (window.onMusicStateChanged) window.onMusicStateChanged(true);
        });
        this.musicAudio.addEventListener('pause', () => {
            this.isPlaying = false;
            if (window.onMusicStateChanged) window.onMusicStateChanged(false);
        });
        this.musicAudio.addEventListener('ended', () => {
            this.isPlaying = false;
            if (window.onMusicStateChanged) window.onMusicStateChanged(false);
        });
    }

    playDailyMusic(weekday = null) {
        if (!weekday) {
            const day = new Date().getDay();
            weekday = day === 0 ? 7 : day; // 1 = Mon ... 7 = Sun
        }
        const trackName = `00${weekday}.mp3`;
        const trackUrl = `/assets/music/${trackName}`;
        this.playMusic(trackUrl, `星期 ${weekday} 定时音乐 (${trackName})`);
    }

    playMusic(url, title = "") {
        this.initAudioContext();
        if (this.audioCtx && this.audioCtx.state === 'suspended') {
            this.audioCtx.resume();
        }

        this.musicAudio.src = url;
        this.musicAudio.volume = this.volume;
        this.musicAudio.play().then(() => {
            console.log(`[Audio] Playing music: ${title}`);
        }).catch(err => {
            console.warn("[Audio] Music play blocked/failed:", err);
        });
    }

    pauseMusic() {
        this.musicAudio.pause();
    }

    resumeMusic() {
        if (this.musicAudio.src) {
            this.musicAudio.play().catch(e => console.warn(e));
        } else {
            this.playDailyMusic();
        }
    }

    toggleMusic() {
        if (this.isPlaying) {
            this.pauseMusic();
        } else {
            this.resumeMusic();
        }
    }

    playWeatherVoice(condition = "others") {
        let src = "";
        const c = condition.toLowerCase();
        if (c.includes("雨") || c.includes("rain")) {
            src = "/assets/weather/sounds/rainy/rain-1.wav";
        } else if (c.includes("雪") || c.includes("snow")) {
            src = "/assets/weather/sounds/snow/snow.wav";
        } else if (c.includes("雾") || c.includes("fog")) {
            src = "/assets/weather/sounds/fog/fog.wav";
        } else {
            // Pick a random tip voice 01 - 10
            const rand = Math.floor(Math.random() * 10) + 1;
            const num = rand < 10 ? `0${rand}` : `${rand}`;
            src = `/assets/weather/sounds/others/volume-150pct-${num}.wav`;
        }

        this.voiceAudio.src = src;
        this.voiceAudio.volume = this.volume;
        this.voiceAudio.play().catch(e => console.warn("[Audio] Weather voice play failed:", e));
    }
}
