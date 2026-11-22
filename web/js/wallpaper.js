/**
 * High-Performance Weather-Categorized Media Wallpaper Manager
 * Supports ONLY Video (.mp4, .webm, .mkv, .avi) and Image (.jpg, .jpeg, .png, .webp, .bmp, .gif)
 * Direct hardware-accelerated playback with C-level backend streaming.
 * Zero emojis, clean modular architecture.
 */

class WallpaperManager {
    constructor(particleEngine, audioManager) {
        this.particles = particleEngine;
        this.audio = audioManager;
        this.currentMode = 'classic'; // 'classic' (image) | 'video'
        this.currentWallpaperPath = '/assets/images/anime_rainy_night.jpg';
        this.currentWallpaperFile = '';
        this.currentCategory = 'all';
        this.autoWeatherSwitch = true;
        this.categorizedWallpapers = {
            all: [],
            sunny: [],
            rainy: [],
            snow: [],
            fog: [],
            video: [],
            custom: []
        };

        this.videoEl = document.getElementById('bg-video');
        this.canvasEl = document.getElementById('bg-canvas');
        this.imageEl = document.getElementById('bg-image');

        if (this.videoEl) {
            this.videoEl.addEventListener('error', () => {
                console.warn("[Wallpaper] Video playback error, fallback to static image");
                this.videoEl.style.display = 'none';
                if (this.imageEl) this.imageEl.style.display = 'block';
            });
        }

        this.initDragDrop();
        this.initEventListeners();
        this.loadCategorizedWallpapers();
    }

    initDragDrop() {
        window.addEventListener('dragover', (e) => e.preventDefault());
        window.addEventListener('drop', async (e) => {
            e.preventDefault();
            if (e.dataTransfer && e.dataTransfer.files.length > 0) {
                const file = e.dataTransfer.files[0];
                const fileName = file.name.toLowerCase();

                if (fileName.endsWith('.mp4') || fileName.endsWith('.webm') || fileName.endsWith('.mkv') || fileName.endsWith('.avi')) {
                    const objectUrl = URL.createObjectURL(file);
                    this.setVideoWallpaper(objectUrl);
                    window.showToast(`已加载视频壁纸: ${file.name}`, 'info');
                } else if (fileName.endsWith('.png') || fileName.endsWith('.jpg') || fileName.endsWith('.jpeg') || fileName.endsWith('.webp') || fileName.endsWith('.bmp') || fileName.endsWith('.gif')) {
                    const objectUrl = URL.createObjectURL(file);
                    this.setImageWallpaper(objectUrl);
                    window.showToast(`已设置背景图片: ${file.name}`, 'info');
                } else {
                    window.showToast('仅支持拖入图片或视频格式壁纸', 'warning');
                }
            }
        });
    }

    initEventListeners() {
        // Modal Category Tabs
        document.querySelectorAll('.wp-cat-tab').forEach(tab => {
            tab.addEventListener('click', () => {
                document.querySelectorAll('.wp-cat-tab').forEach(t => t.classList.remove('active'));
                tab.classList.add('active');
                const cat = tab.dataset.category || 'all';
                this.renderCategoryGrid(cat, 'wallpaper-grid');
            });
        });

        // Dashboard Card Category Tabs
        document.querySelectorAll('.dash-wp-cat-tab').forEach(tab => {
            tab.addEventListener('click', () => {
                document.querySelectorAll('.dash-wp-cat-tab').forEach(t => t.classList.remove('active'));
                tab.classList.add('active');
                const cat = tab.dataset.category || 'all';
                this.renderCategoryGrid(cat, 'dash-wallpaper-mini-grid');
            });
        });

        // Scan Media Button
        document.getElementById('scan-steam-btn')?.addEventListener('click', async () => {
            window.showToast('正在极速扫描壁纸库...', 'info');
            await this.loadCategorizedWallpapers();
            window.showToast('壁纸扫描完成', 'success');
        });

        // Pick Custom File Button
        document.getElementById('select-wp-file-btn')?.addEventListener('click', async () => {
            if (window.pywebview && window.pywebview.api) {
                try {
                    const chosen = await window.pywebview.api.select_wallpaper_file();
                    if (chosen) {
                        this.applyCustomFile(chosen);
                    }
                } catch (e) {
                    console.warn(e);
                }
            }
        });

        // Pick Custom Folder Button
        document.getElementById('browse-folder-btn')?.addEventListener('click', async () => {
            if (window.pywebview && window.pywebview.api) {
                try {
                    const chosen = await window.pywebview.api.select_wallpaper_folder();
                    if (chosen) {
                        window.showToast(`已选择目录: ${chosen}`, 'info');
                        await this.loadCategorizedWallpapers(chosen);
                    }
                } catch (e) {
                    console.warn(e);
                }
            }
        });

        // Auto Switch Toggle
        const autoToggle = document.getElementById('wp-auto-switch-toggle');
        if (autoToggle) {
            autoToggle.addEventListener('change', (e) => {
                this.autoWeatherSwitch = e.target.checked;
                const dockWp = document.getElementById('dock-status-wallpaper');
                if (dockWp) dockWp.textContent = this.autoWeatherSwitch ? '天气联动中' : '固定壁纸';
                window.showToast(this.autoWeatherSwitch ? '已开启天气联动壁纸' : '已关闭天气联动壁纸', 'info');
                if (window.pywebview && window.pywebview.api) {
                    window.pywebview.api.save_config({ wallpaperAutoSwitch: this.autoWeatherSwitch });
                }
            });
        }
    }

    async loadCategorizedWallpapers(customPath = null) {
        if (window.pywebview && window.pywebview.api) {
            try {
                const res = await window.pywebview.api.get_categorized_wallpapers(customPath);
                if (res) {
                    this.categorizedWallpapers = res;
                    this.renderCategoryGrid(this.currentCategory, 'wallpaper-grid');
                    this.renderCategoryGrid(this.currentCategory, 'dash-wallpaper-mini-grid');
                    return;
                }
            } catch (e) {
                console.warn("[Wallpaper] API load failed:", e);
            }
        }

        // Fallback default stock wallpapers (Image only)
        this.categorizedWallpapers = {
            all: [
                { id: "rainy-anime", name: "雨夜月色动漫", path: "/assets/images/anime_rainy_night.jpg", category: "rainy", categoryName: "雨天", type: "image", thumb: "/assets/images/anime_rainy_night.jpg" },
                { id: "sunny-1", name: "晴朗日光 01", path: "/assets/weather/pictures/sunny/sunny-1.png", category: "sunny", categoryName: "晴天", type: "image", thumb: "/assets/weather/pictures/sunny/sunny-1.png" },
                { id: "sunny-2", name: "碧空万里 02", path: "/assets/weather/pictures/sunny/sunny-2.png", category: "sunny", categoryName: "晴天", type: "image", thumb: "/assets/weather/pictures/sunny/sunny-2.png" },
                { id: "rainy-1", name: "烟雨朦胧 01", path: "/assets/weather/pictures/rainy/rainy-1.png", category: "rainy", categoryName: "雨天", type: "image", thumb: "/assets/weather/pictures/rainy/rainy-1.png" },
                { id: "snow-1", name: "初冬初雪 01", path: "/assets/weather/pictures/snow/snow.png", category: "snow", categoryName: "雪天", type: "image", thumb: "/assets/weather/pictures/snow/snow.png" },
                { id: "fog-1", name: "晨雾微澜 01", path: "/assets/weather/pictures/fog/fog.png", category: "fog", categoryName: "雾天", type: "image", thumb: "/assets/weather/pictures/fog/fog.png" }
            ],
            sunny: [
                { id: "sunny-1", name: "晴朗日光 01", path: "/assets/weather/pictures/sunny/sunny-1.png", category: "sunny", categoryName: "晴天", type: "image", thumb: "/assets/weather/pictures/sunny/sunny-1.png" },
                { id: "sunny-2", name: "碧空万里 02", path: "/assets/weather/pictures/sunny/sunny-2.png", category: "sunny", categoryName: "晴天", type: "image", thumb: "/assets/weather/pictures/sunny/sunny-2.png" }
            ],
            rainy: [
                { id: "rainy-anime", name: "雨夜月色动漫", path: "/assets/images/anime_rainy_night.jpg", category: "rainy", categoryName: "雨天", type: "image", thumb: "/assets/images/anime_rainy_night.jpg" },
                { id: "rainy-1", name: "烟雨朦胧 01", path: "/assets/weather/pictures/rainy/rainy-1.png", category: "rainy", categoryName: "雨天", type: "image", thumb: "/assets/weather/pictures/rainy/rainy-1.png" }
            ],
            snow: [
                { id: "snow-1", name: "初冬初雪 01", path: "/assets/weather/pictures/snow/snow.png", category: "snow", categoryName: "雪天", type: "image", thumb: "/assets/weather/pictures/snow/snow.png" }
            ],
            fog: [
                { id: "fog-1", name: "晨雾微澜 01", path: "/assets/weather/pictures/fog/fog.png", category: "fog", categoryName: "雾天", type: "image", thumb: "/assets/weather/pictures/fog/fog.png" }
            ],
            video: [],
            custom: []
        };

        this.renderCategoryGrid(this.currentCategory, 'wallpaper-grid');
        this.renderCategoryGrid(this.currentCategory, 'dash-wallpaper-mini-grid');
    }

    renderCategoryGrid(category = 'all', containerId = 'wallpaper-grid') {
        this.currentCategory = category;
        const container = document.getElementById(containerId);
        if (!container) return;

        const items = this.categorizedWallpapers[category] || this.categorizedWallpapers['all'] || [];
        if (items.length === 0) {
            container.innerHTML = '<div class="empty-wp-tip">该分类暂无壁纸，可点击下方导入或拖入视频/图片文件。</div>';
            return;
        }

        container.innerHTML = '';
        items.forEach(item => {
            const card = document.createElement('div');
            card.className = 'wallpaper-card';
            if (this.currentWallpaperPath === item.path || this.currentWallpaperFile === item.path) {
                card.classList.add('active');
            }

            const isVideo = item.type === 'video';
            const thumbUrl = item.thumb || item.path || '/assets/images/settings.png';

            card.innerHTML = `
                <div class="wp-card-thumb" style="background-image: url('${thumbUrl}');">
                    <span class="wp-card-badge">${isVideo ? '视频' : (item.categoryName || '图片')}</span>
                </div>
                <div class="wp-card-info">
                    <div class="wp-card-title">${item.name}</div>
                    <div class="wp-card-sub">${isVideo ? '超高清流畅视频背景' : '物理漫反射超清图层'}</div>
                </div>
            `;

            card.addEventListener('click', () => {
                this.applyWallpaperItem(item);
            });

            container.appendChild(card);
        });
    }

    applyWallpaperItem(item) {
        if (!item) return;

        if (item.type === 'video') {
            this.setVideoWallpaper(item.localPath || item.path);
        } else {
            this.setImageWallpaper(item.path);
        }

        this.currentWallpaperPath = item.path;
        const dockWp = document.getElementById('dock-status-wallpaper');
        if (dockWp) dockWp.textContent = `${item.categoryName || '壁纸'} · ${item.type === 'video' ? '视频' : '图层'}`;
        const badgeWp = document.getElementById('badge-menu-wallpaper');
        if (badgeWp) badgeWp.textContent = item.categoryName || '已选';

        window.showToast(`已切换壁纸: ${item.name}`, 'info');

        this.renderCategoryGrid(this.currentCategory, 'wallpaper-grid');
        this.renderCategoryGrid(this.currentCategory, 'dash-wallpaper-mini-grid');
    }

    applyCustomFile(filePath) {
        const ext = filePath.split('.').pop().toLowerCase();
        if (['mp4', 'webm', 'mkv', 'avi'].includes(ext)) {
            this.setVideoWallpaper(filePath);
        } else {
            this.setImageWallpaper(filePath);
        }
        window.showToast('已加载自定义壁纸', 'info');
    }

    setMode(mode, path = '', file = '') {
        this.currentMode = mode;
        this.currentWallpaperPath = path;
        this.currentWallpaperFile = file;

        // Reset elements
        if (this.videoEl) {
            this.videoEl.style.display = 'none';
            this.videoEl.pause();
        }
        if (this.canvasEl) this.canvasEl.style.display = 'none';
        if (this.imageEl) this.imageEl.style.display = 'none';

        if (mode === 'classic' || mode === 'image') {
            if (this.imageEl) {
                this.imageEl.style.display = 'block';
                const bg = path || '/assets/images/anime_rainy_night.jpg';
                const bgUrl = bg.startsWith('http') || bg.startsWith('blob:') || bg.startsWith('/assets') || bg.startsWith('/stream')
                    ? bg
                    : `/stream?path=${encodeURIComponent(bg)}`;
                this.imageEl.style.backgroundImage = `url('${bgUrl}')`;
            }
            if (this.canvasEl) {
                this.canvasEl.style.display = 'block';
                if (this.particles) this.particles.start();
            }
        } else if (mode === 'video') {
            if (this.particles) this.particles.stop();
            if (path && this.videoEl) {
                this.videoEl.style.display = 'block';
                let streamUrl = path;
                if (!path.startsWith('http') && !path.startsWith('blob:') && !path.startsWith('/assets') && !path.startsWith('/stream')) {
                    streamUrl = `/stream?path=${encodeURIComponent(path)}`;
                }
                this.videoEl.src = streamUrl;
                this.videoEl.play().catch(e => console.warn(e));
            }
        }

        // Persist config
        if (window.pywebview && window.pywebview.api) {
            const savePath = (path && path.startsWith('blob:')) ? '' : path;
            window.pywebview.api.save_config({
                wallpaperType: mode,
                wallpaperPath: savePath,
                wallpaperFile: file,
                wallpaperAutoSwitch: this.autoWeatherSwitch
            });
        }
    }

    setImageWallpaper(path) {
        this.setMode('classic', path);
    }

    setVideoWallpaper(videoPath) {
        this.setMode('video', videoPath);
    }

    handleWeatherConditionChange(condition) {
        if (!this.autoWeatherSwitch) return;

        let cat = 'sunny';
        const cond = String(condition || '').toLowerCase();

        if (cond.includes('雨') || cond.includes('rain')) {
            cat = 'rainy';
        } else if (cond.includes('雪') || cond.includes('snow')) {
            cat = 'snow';
        } else if (cond.includes('雾') || cond.includes('霾') || cond.includes('fog')) {
            cat = 'fog';
        } else {
            cat = 'sunny';
        }

        const candidateList = this.categorizedWallpapers[cat] || [];
        if (candidateList.length > 0) {
            const picked = candidateList[Math.floor(Math.random() * candidateList.length)];
            this.applyWallpaperItem(picked);
        }
    }
}

window.WallpaperManager = WallpaperManager;
