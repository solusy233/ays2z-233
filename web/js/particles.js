/**
 * Dynamic Canvas Particle Engine
 * Highly optimized, hardware-accelerated dynamic weather & ambient backgrounds.
 * Supports: Rain, Snow, Fog, Sunny/Dust, Stars, and Cyberpunk Neon Orbs.
 */

class WeatherParticleEngine {
    constructor(canvasId) {
        this.canvas = document.getElementById(canvasId);
        this.ctx = this.canvas.getContext('2d');
        this.mode = 'rain'; // 'rain', 'snow', 'fog', 'sunny', 'stars', 'neon'
        this.particles = [];
        this.splashes = [];
        this.animId = null;
        this.width = 0;
        this.height = 0;
        this.mouseX = 0;
        this.mouseY = 0;

        this.initResize();
        this.initMouse();
    }

    initResize() {
        const resize = () => {
            this.width = this.canvas.width = window.innerWidth;
            this.height = this.canvas.height = window.innerHeight;
            this.resetParticles();
        };
        window.addEventListener('resize', resize);
        resize();
    }

    initMouse() {
        window.addEventListener('mousemove', (e) => {
            this.mouseX = e.clientX;
            this.mouseY = e.clientY;
        });
    }

    setMode(mode) {
        if (this.mode === mode) return;
        this.mode = mode;
        this.resetParticles();
    }

    resetParticles() {
        this.particles = [];
        this.splashes = [];

        if (this.mode === 'rain') {
            const count = Math.floor(this.width / 8);
            for (let i = 0; i < count; i++) {
                this.particles.push({
                    x: Math.random() * this.width,
                    y: Math.random() * this.height,
                    speed: 14 + Math.random() * 12,
                    length: 18 + Math.random() * 22,
                    opacity: 0.2 + Math.random() * 0.4,
                    width: 1 + Math.random() * 1.2
                });
            }
        } else if (this.mode === 'snow') {
            const count = Math.floor(this.width / 12);
            for (let i = 0; i < count; i++) {
                this.particles.push({
                    x: Math.random() * this.width,
                    y: Math.random() * this.height,
                    radius: 1.5 + Math.random() * 3.5,
                    speed: 0.8 + Math.random() * 1.8,
                    swing: Math.random() * 2,
                    swingSpeed: 0.01 + Math.random() * 0.02,
                    opacity: 0.3 + Math.random() * 0.6
                });
            }
        } else if (this.mode === 'fog') {
            const count = 18;
            for (let i = 0; i < count; i++) {
                this.particles.push({
                    x: Math.random() * this.width,
                    y: Math.random() * this.height,
                    radius: 140 + Math.random() * 220,
                    vx: 0.2 + Math.random() * 0.4,
                    vy: (Math.random() - 0.5) * 0.1,
                    opacity: 0.03 + Math.random() * 0.07
                });
            }
        } else if (this.mode === 'sunny') {
            const count = 60;
            for (let i = 0; i < count; i++) {
                this.particles.push({
                    x: Math.random() * this.width,
                    y: Math.random() * this.height,
                    radius: 1 + Math.random() * 3,
                    vx: (Math.random() - 0.5) * 0.4,
                    vy: -0.3 - Math.random() * 0.6,
                    opacity: 0.2 + Math.random() * 0.5
                });
            }
        } else if (this.mode === 'stars') {
            const count = 120;
            for (let i = 0; i < count; i++) {
                this.particles.push({
                    x: Math.random() * this.width,
                    y: Math.random() * this.height,
                    radius: 0.8 + Math.random() * 2,
                    baseAlpha: 0.2 + Math.random() * 0.7,
                    twinkleSpeed: 0.02 + Math.random() * 0.04,
                    phase: Math.random() * Math.PI * 2
                });
            }
        } else if (this.mode === 'neon') {
            const count = 25;
            for (let i = 0; i < count; i++) {
                this.particles.push({
                    x: Math.random() * this.width,
                    y: Math.random() * this.height,
                    radius: 40 + Math.random() * 90,
                    vx: (Math.random() - 0.5) * 0.8,
                    vy: (Math.random() - 0.5) * 0.8,
                    color: Math.random() > 0.5 ? 'rgba(242, 210, 140, ' : 'rgba(0, 195, 255, ',
                    opacity: 0.04 + Math.random() * 0.08
                });
            }
        } else if (this.mode === 'petals' || this.mode === 'feathers') {
            const count = Math.max(28, Math.floor(this.width / 32));
            for (let i = 0; i < count; i++) {
                this.particles.push({
                    x: Math.random() * this.width,
                    y: Math.random() * this.height,
                    radius: 3.5 + Math.random() * 5.5,
                    vx: 0.3 + Math.random() * 0.8,
                    vy: 0.9 + Math.random() * 1.5,
                    rotation: Math.random() * Math.PI * 2,
                    rotSpeed: (Math.random() - 0.5) * 0.04,
                    swing: Math.random() * Math.PI * 2,
                    swingSpeed: 0.015 + Math.random() * 0.025,
                    color: Math.random() > 0.45 ? 'rgba(163, 24, 60, ' : 'rgba(75, 18, 72, ',
                    opacity: 0.3 + Math.random() * 0.45
                });
            }
        }
    }

    start() {
        if (this.animId) return;
        const loop = () => {
            this.update();
            this.render();
            this.animId = requestAnimationFrame(loop);
        };
        loop();
    }

    stop() {
        if (this.animId) {
            cancelAnimationFrame(this.animId);
            this.animId = null;
        }
        this.ctx.clearRect(0, 0, this.width, this.height);
    }

    update() {
        if (this.mode === 'rain') {
            for (let p of this.particles) {
                p.y += p.speed;
                p.x += 1.5; // slight wind tilt
                if (p.y > this.height) {
                    p.y = -p.length;
                    p.x = Math.random() * this.width;
                    // Spawn puddle splash
                    if (Math.random() > 0.6) {
                        this.splashes.push({
                            x: p.x,
                            y: this.height - 10 + Math.random() * 10,
                            r: 1,
                            maxR: 8 + Math.random() * 10,
                            alpha: 0.4
                        });
                    }
                }
            }
            // Update splashes
            for (let i = this.splashes.length - 1; i >= 0; i--) {
                const s = this.splashes[i];
                s.r += 0.8;
                s.alpha -= 0.03;
                if (s.alpha <= 0) {
                    this.splashes.splice(i, 1);
                }
            }
        } else if (this.mode === 'snow') {
            for (let p of this.particles) {
                p.y += p.speed;
                p.swing += p.swingSpeed;
                p.x += Math.sin(p.swing) * 0.8;
                if (p.y > this.height) {
                    p.y = -10;
                    p.x = Math.random() * this.width;
                }
            }
        } else if (this.mode === 'fog') {
            for (let p of this.particles) {
                p.x += p.vx;
                p.y += p.vy;
                if (p.x - p.radius > this.width) {
                    p.x = -p.radius;
                    p.y = Math.random() * this.height;
                }
            }
        } else if (this.mode === 'sunny') {
            for (let p of this.particles) {
                p.x += p.vx;
                p.y += p.vy;
                if (p.y < -10) {
                    p.y = this.height + 10;
                    p.x = Math.random() * this.width;
                }
                if (p.x < 0) p.x = this.width;
                if (p.x > this.width) p.x = 0;
            }
        } else if (this.mode === 'stars') {
            for (let p of this.particles) {
                p.phase += p.twinkleSpeed;
            }
        } else if (this.mode === 'neon') {
            for (let p of this.particles) {
                p.x += p.vx;
                p.y += p.vy;
                if (p.x < -p.radius) p.x = this.width + p.radius;
                if (p.x > this.width + p.radius) p.x = -p.radius;
                if (p.y < -p.radius) p.y = this.height + p.radius;
                if (p.y > this.height + p.radius) p.y = -p.radius;
            }
        } else if (this.mode === 'petals' || this.mode === 'feathers') {
            for (let p of this.particles) {
                p.swing += p.swingSpeed;
                p.x += Math.sin(p.swing) * 0.9 + p.vx;
                p.y += p.vy;
                p.rotation += p.rotSpeed;
                if (p.y > this.height + 20) {
                    p.y = -20;
                    p.x = Math.random() * this.width;
                }
                if (p.x > this.width + 20) p.x = -20;
            }
        }
    }

    render() {
        this.ctx.clearRect(0, 0, this.width, this.height);

        if (this.mode === 'rain') {
            this.ctx.strokeStyle = '#a4d2ff';
            this.ctx.lineCap = 'round';
            for (let p of this.particles) {
                this.ctx.lineWidth = p.width;
                this.ctx.globalAlpha = p.opacity;
                this.ctx.beginPath();
                this.ctx.moveTo(p.x, p.y);
                this.ctx.lineTo(p.x + 3, p.y + p.length);
                this.ctx.stroke();
            }
            // Splashes
            this.ctx.lineWidth = 1;
            for (let s of this.splashes) {
                this.ctx.globalAlpha = s.alpha;
                this.ctx.beginPath();
                this.ctx.ellipse(s.x, s.y, s.r * 1.5, s.r * 0.5, 0, 0, Math.PI * 2);
                this.ctx.stroke();
            }
            this.ctx.globalAlpha = 1.0;
        } else if (this.mode === 'snow') {
            this.ctx.fillStyle = '#ffffff';
            for (let p of this.particles) {
                this.ctx.globalAlpha = p.opacity;
                this.ctx.beginPath();
                this.ctx.arc(p.x, p.y, p.radius, 0, Math.PI * 2);
                this.ctx.fill();
            }
            this.ctx.globalAlpha = 1.0;
        } else if (this.mode === 'fog') {
            for (let p of this.particles) {
                const grad = this.ctx.createRadialGradient(p.x, p.y, 0, p.x, p.y, p.radius);
                grad.addColorStop(0, `rgba(180, 205, 230, ${p.opacity})`);
                grad.addColorStop(1, 'rgba(180, 205, 230, 0)');
                this.ctx.fillStyle = grad;
                this.ctx.beginPath();
                this.ctx.arc(p.x, p.y, p.radius, 0, Math.PI * 2);
                this.ctx.fill();
            }
        } else if (this.mode === 'sunny') {
            // Sun ray subtle gradient
            const sunGrad = this.ctx.createLinearGradient(0, 0, this.width, this.height);
            sunGrad.addColorStop(0, 'rgba(242, 210, 140, 0.08)');
            sunGrad.addColorStop(1, 'rgba(0, 0, 0, 0)');
            this.ctx.fillStyle = sunGrad;
            this.ctx.fillRect(0, 0, this.width, this.height);

            // Floating dust motes
            this.ctx.fillStyle = '#fce5b2';
            for (let p of this.particles) {
                this.ctx.globalAlpha = p.opacity;
                this.ctx.beginPath();
                this.ctx.arc(p.x, p.y, p.radius, 0, Math.PI * 2);
                this.ctx.fill();
            }
            this.ctx.globalAlpha = 1.0;
        } else if (this.mode === 'stars') {
            this.ctx.fillStyle = '#ffffff';
            for (let p of this.particles) {
                const alpha = p.baseAlpha + Math.sin(p.phase) * 0.3;
                this.ctx.globalAlpha = Math.max(0.05, Math.min(1.0, alpha));
                this.ctx.beginPath();
                this.ctx.arc(p.x, p.y, p.radius, 0, Math.PI * 2);
                this.ctx.fill();
            }
            this.ctx.globalAlpha = 1.0;
        } else if (this.mode === 'neon') {
            for (let p of this.particles) {
                const grad = this.ctx.createRadialGradient(p.x, p.y, 0, p.x, p.y, p.radius);
                grad.addColorStop(0, `${p.color}${p.opacity})`);
                grad.addColorStop(1, `${p.color}0)`);
                this.ctx.fillStyle = grad;
                this.ctx.beginPath();
                this.ctx.arc(p.x, p.y, p.radius, 0, Math.PI * 2);
                this.ctx.fill();
            }
        } else if (this.mode === 'petals' || this.mode === 'feathers') {
            for (let p of this.particles) {
                this.ctx.save();
                this.ctx.translate(p.x, p.y);
                this.ctx.rotate(p.rotation);
                this.ctx.fillStyle = `${p.color}${p.opacity})`;
                this.ctx.beginPath();
                this.ctx.ellipse(0, 0, p.radius * 2.2, p.radius * 0.85, 0, 0, Math.PI * 2);
                this.ctx.fill();
                this.ctx.restore();
            }
            this.ctx.globalAlpha = 1.0;
        }
    }
}
