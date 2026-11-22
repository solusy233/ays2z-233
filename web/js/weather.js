/**
 * Weather Station Controller & Glassmorphism Weather Center
 * Manages:
 * 1. Top floating glass tip bar (.top-weather-tip-bar)
 * 2. Right-side floating weather carousel widget (.floating-weather-widget) with 3 rotating views:
 *    - View 1: Current condition, temperature, wind, travel safety tip
 *    - View 2: Hourly & 3-day miniature summary
 *    - View 3: Core weather indices (AQI, humidity, UV, sun & moon)
 * 3. Fullscreen Weather Center (#fullscreen-weather-modal):
 *    - City selector capsules (北京, 上海, 广州, 深圳, 杭州, 安阳) & search input
 *    - Tab 1: Hourly forecast & multi-day trend (Bezier curve, sub-metrics, chart/list, feels-like toggle)
 *    - Tab 2: 12-grid detailed weather cards (compass, sun arc, moon sphere, 7-bar humidity, UV gauge, AQI, visibility pyramid, pressure slider, warning & travel index)
 *    - Tab 3: Comprehensive panorama dual-column view
 * 4. Fallback to /assets/weather/msn_hourly_weather.json with 100% data guarantee.
 */

class WeatherController {
    constructor(particleEngine, audioManager) {
        this.particles = particleEngine;
        this.audio = audioManager;
        this.weatherTipsData = null;

        // Current weather state
        this.currentCity = "安阳市";
        this.currentCondition = "小雨";
        this.currentTemp = 15;
        this.feelsLike = 14;
        this.currentWeatherCode = "305";
        this.msnHourlyData = null;
        this.lastWeatherData = null;

        // Chart instances
        this.fullscreenChart = null;
        this.panoramaChart = null;

        // Carousel state for right-side widget
        this.currentSlideIndex = 0;
        this.slideCount = 3;
        this.carouselInterval = null;
        this.isCarouselPaused = false;

        // Active tab in fullscreen modal: 'hourly' | 'grid12' | 'panorama'
        this.activeFullscreenTab = 'hourly';
        this.currentChartMetric = 'overview';
        this.showFeelsLike = false;
        this.isListView = false;

        this.initElements();
        this.loadLocalTips();
        this.initCharacterTips();
        this.initCarousel();
        this.initFullscreenModalEvents();
        this.initCitySwitchEvents();
    }

    async initCharacterTips() {
        if (window.characterTipsManager) {
            await window.characterTipsManager.init();
            window.characterTipsManager.subscribe((tip) => {
                this.renderCharacterTip(tip);
            });
        }
    }

    initElements() {
        // Top tip bar
        this.topTipBar = document.getElementById('top-weather-tip-bar');
        this.topTipText = document.getElementById('top-weather-tip-text');

        // Right floating widget
        this.widget = document.getElementById('floating-weather-widget');
        this.widgetTrack = document.getElementById('widget-carousel-track');
        this.widgetDots = document.querySelectorAll('.widget-dot');
        this.reopenBtn = document.getElementById('floating-weather-reopen-btn');

        // Fullscreen modal
        this.fullscreenModal = document.getElementById('fullscreen-weather-modal');
    }

    async loadLocalTips() {
        try {
            const resp = await fetch('/assets/weather/weathertips.json');
            this.weatherTipsData = await resp.json();
        } catch (e) {
            console.warn("Could not load weathertips.json:", e);
        }
    }

    // =========================================================================
    // 1. Data Fetching & Sync (MSN Weather Crawler + Fallback)
    // =========================================================================

    async fetchAndRender(cityNameOrApiKey = "安阳市", maybeCityName = null) {
        let cityName = "安阳市";
        if (maybeCityName) {
            cityName = maybeCityName;
        } else if (cityNameOrApiKey) {
            cityName = cityNameOrApiKey;
        }
        this.currentCity = cityName || "安阳市";
        let hourlyResult = null;

        // 1. Try pywebview backend crawler
        if (window.pywebview && window.pywebview.api) {
            try {
                hourlyResult = await window.pywebview.api.fetch_hourly_weather(this.currentCity);
            } catch (e) {
                console.warn("[Weather] API fetch_hourly_weather failed:", e);
            }
        }

        // 2. Fallback: load bundled MSN hourly weather JSON
        if (!hourlyResult || !hourlyResult.data) {
            try {
                const resp = await fetch('/assets/weather/msn_hourly_weather.json');
                const cachedData = await resp.json();
                hourlyResult = { success: true, data: cachedData, fallback: true };
            } catch (e) {
                console.error("[Weather] Failed to load local msn_hourly_weather.json:", e);
            }
        }

        if (hourlyResult && hourlyResult.data) {
            this.msnHourlyData = this.adaptDataForCity(hourlyResult.data, this.currentCity);
            this.renderAll();
        }
    }

    adaptDataForCity(data, cityName) {
        // Clone and adjust city name if user queried another city and crawler fell back
        const cloned = JSON.parse(JSON.stringify(data));
        cloned.cityName = cityName;

        // City specific adjustments for visual variety if needed
        if (cityName.includes("北京")) {
            cloned.current.condition = "多云";
            cloned.current.iconCode = "101";
            cloned.current.temperature = 11;
            cloned.current.feels = 9;
            cloned.current.aqi = 52;
            cloned.current.wind = "东北风 2级 (8 km/h)";
        } else if (cityName.includes("上海")) {
            cloned.current.condition = "小雨";
            cloned.current.iconCode = "305";
            cloned.current.temperature = 16;
            cloned.current.feels = 15;
            cloned.current.aqi = 38;
            cloned.current.wind = "东南风 3级 (14 km/h)";
        } else if (cityName.includes("广州") || cityName.includes("深圳")) {
            cloned.current.condition = "阴天";
            cloned.current.iconCode = "104";
            cloned.current.temperature = 22;
            cloned.current.feels = 22;
            cloned.current.aqi = 28;
            cloned.current.wind = "南风 2级 (10 km/h)";
        } else if (cityName.includes("杭州")) {
            cloned.current.condition = "中雨";
            cloned.current.iconCode = "306";
            cloned.current.temperature = 14;
            cloned.current.feels = 13;
            cloned.current.aqi = 35;
            cloned.current.wind = "东风 2级 (11 km/h)";
        }
        return cloned;
    }

    renderAll() {
        if (!this.msnHourlyData) return;

        const msn = this.msnHourlyData;
        const cur = msn.current || {};
        this.currentCondition = cur.condition || "小雨";
        this.currentTemp = cur.temperature !== undefined ? cur.temperature : 15;
        this.feelsLike = cur.feels !== undefined ? cur.feels : (this.currentTemp - 1);
        this.currentWeatherCode = cur.iconCode || "305";

        // 1. Update Top Floating Tip Bar
        this.updateTopTipBar();

        // 2. Update Control Center Weather Entry Card
        this.updateDashboardWeatherCard();

        // 3. Update Right Floating Carousel Widget (if present)
        this.renderRightWidget();

        // 4. Update Fullscreen Modal
        this.renderFullscreenModal();

        // 5. Synchronize Background Particles
        this.syncParticles(this.currentCondition);

        // 6. Synchronize DeskStateManager (State A Overview & State B Peripheral Carousel)
        if (window.deskStateManager) {
            window.deskStateManager.syncWeatherData(this.msnHourlyData);
        }
    }

    // =========================================================================
    // 2. Top Tip Bar (.top-weather-tip-bar)
    // =========================================================================

    updateTopTipBar() {
        if (window.characterTipsManager) {
            window.characterTipsManager.updateWeatherConditions(
                this.currentCondition,
                this.currentTemp,
                this.currentWind
            );
            const tip = window.characterTipsManager.getCurrentTip();
            if (tip) {
                this.renderCharacterTip(tip);
                return;
            }
        }

        if (this.topTipText) {
            this.topTipText.textContent = "今日天气舒适，出行请注意天气变化。";
        }
    }

    renderCharacterTip(tip) {
        if (!tip) return;

        // 0. Synchronize DeskStateManager (State A Overview & State B Carousel)
        if (window.deskStateManager) {
            window.deskStateManager.syncCharacterTip(tip);
        }

        // 1. Top Tip Bar
        if (this.topTipText) {
            this.topTipText.textContent = `[${tip.name}]: ${tip.quote}`;
        }

        // 2. Fullscreen Voice Banner
        const bandTag = document.getElementById('fs-char-band-tag');
        const nameTag = document.getElementById('fs-char-name-tag');
        const roleTag = document.getElementById('fs-char-role-tag');
        const quoteEl = document.getElementById('fs-char-quote-text');

        if (bandTag) bandTag.textContent = tip.band || '';
        if (nameTag) nameTag.textContent = tip.name || '';
        if (roleTag) roleTag.textContent = tip.role || '';
        if (quoteEl) quoteEl.textContent = `"${tip.quote}"`;
    }

    updateDashboardWeatherCard() {
        if (!this.msnHourlyData) return;
        const cur = this.msnHourlyData.current || {};
        const daily = this.msnHourlyData.daily || [];
        const today = daily.length > 0 ? daily[0] : null;

        const iconEl = document.getElementById('dash-weather-icon');
        const tempEl = document.getElementById('dash-weather-temp');
        const condEl = document.getElementById('dash-weather-cond');
        const badgeEl = document.getElementById('dash-weather-badge');
        const subEl = document.getElementById('dash-weather-sub');

        if (iconEl) iconEl.src = `/assets/weather/icons/${cur.iconCode || '305'}.svg`;
        if (tempEl) tempEl.textContent = `${cur.temperature || 15}°C`;
        if (condEl) condEl.textContent = `${cur.condition || '小雨'} · ${this.currentCity}`;
        if (badgeEl) badgeEl.textContent = this.msnHourlyData.source || '实时气象';

        const lowT = today ? today.low : (cur.temperature - 2);
        const highT = today ? today.high : (cur.temperature + 4);
        const aqiText = cur.aqi ? `空气 ${cur.aqi} ${cur.aqiLevel || '优'}` : '空气质量 优';
        const humText = `相对湿度 ${cur.humidity || 50}%`;
        const windText = cur.wind || '北风 2级 (3 km/h)';

        if (subEl) {
            subEl.textContent = `今日 ${lowT}° ~ ${highT}°C · 体感 ${cur.feels || cur.temperature}°C · ${aqiText} · ${humText} · ${windText}`;
        }
    }

    // =========================================================================
    // 3. Right-side Floating Carousel Weather Panel
    // =========================================================================

    initCarousel() {
        // Auto-rotation every 6 seconds
        this.startCarouselAutoPlay();

        // Mouse hover pause/resume
        this.widget?.addEventListener('mouseenter', () => {
            this.isCarouselPaused = true;
        });

        this.widget?.addEventListener('mouseleave', () => {
            this.isCarouselPaused = false;
        });

        // Prev & Next buttons
        document.getElementById('widget-prev-btn')?.addEventListener('click', (e) => {
            e.stopPropagation();
            this.prevSlide();
        });

        document.getElementById('widget-next-btn')?.addEventListener('click', (e) => {
            e.stopPropagation();
            this.nextSlide();
        });

        // Dots click
        this.widgetDots.forEach(dot => {
            dot.addEventListener('click', (e) => {
                e.stopPropagation();
                const idx = parseInt(dot.getAttribute('data-dot'), 10) || 0;
                this.goToSlide(idx);
            });
        });

        // Open Fullscreen button & Expand Hint
        document.getElementById('widget-fullscreen-btn')?.addEventListener('click', (e) => {
            e.stopPropagation();
            this.openFullscreenModal();
        });

        document.getElementById('widget-open-fs-btn')?.addEventListener('click', (e) => {
            e.stopPropagation();
            this.openFullscreenModal();
        });

        document.getElementById('widget-expand-hint-btn')?.addEventListener('click', (e) => {
            e.stopPropagation();
            this.openFullscreenModal();
        });

        // Hide / Close button
        document.getElementById('widget-hide-btn')?.addEventListener('click', (e) => {
            e.stopPropagation();
            this.hideWidget();
        });

        // Reopen button on right edge
        this.reopenBtn?.addEventListener('click', () => {
            this.showWidget();
        });

        // Initialize wallpaper-only state
        const savedWpOnly = localStorage.getItem('ays2z_wallpaper_only') === 'true';
        if (savedWpOnly) {
            document.body.classList.add('wallpaper-only-mode');
            const centerPanel = document.getElementById('control-center-panel');
            if (centerPanel) centerPanel.style.display = 'none';
        }

        // Toggle Control Center / Wallpaper Only Mode
        document.getElementById('widget-toggle-center-btn')?.addEventListener('click', (e) => {
            e.stopPropagation();
            const centerPanel = document.getElementById('control-center-panel');
            if (centerPanel) {
                const isHidden = centerPanel.style.display === 'none' || document.body.classList.contains('wallpaper-only-mode');
                centerPanel.style.display = isHidden ? 'flex' : 'none';
                document.body.classList.toggle('wallpaper-only-mode', !isHidden);
                try {
                    localStorage.setItem('ays2z_wallpaper_only', String(!isHidden));
                } catch(e) {}
            }
        });

        document.getElementById('toggle-wallpaper-only-btn')?.addEventListener('click', (e) => {
            e.stopPropagation();
            const isCurrentlyOnly = document.body.classList.contains('wallpaper-only-mode');
            const nextOnly = !isCurrentlyOnly;
            document.body.classList.toggle('wallpaper-only-mode', nextOnly);
            const centerPanel = document.getElementById('control-center-panel');
            if (centerPanel) {
                centerPanel.style.display = nextOnly ? 'none' : 'flex';
            }
            try {
                localStorage.setItem('ays2z_wallpaper_only', String(nextOnly));
            } catch(e) {}
            this.showWidget();
            window.showToast(nextOnly ? "已切换为纯净壁纸挂件模式（大面积留白）" : "已恢复居中控制中心", "info");
        });

        // Fullscreen Side Mode Toggle (避让留白模式)
        document.getElementById('fullscreen-side-btn')?.addEventListener('click', () => {
            const modal = document.getElementById('fullscreen-weather-modal');
            if (modal) {
                modal.classList.toggle('side-layout-mode');
                const isSide = modal.classList.contains('side-layout-mode');
                const sideBtn = document.getElementById('fullscreen-side-btn');
                if (sideBtn) sideBtn.textContent = isSide ? "恢复全景" : "避让留白";
                window.showToast(isSide ? "已开启壁纸避让留白：左侧大面积露景" : "已恢复居中全景视图", "info");
            }
        });

        // Top Tip bar click opens Fullscreen Modal
        this.topTipBar?.addEventListener('click', () => {
            this.openFullscreenModal();
        });
    }

    startCarouselAutoPlay() {
        if (this.carouselInterval) clearInterval(this.carouselInterval);
        this.carouselInterval = setInterval(() => {
            if (!this.isCarouselPaused && (!this.fullscreenModal || this.fullscreenModal.classList.contains('hidden'))) {
                this.nextSlide();
            }
        }, 6500);
    }

    goToSlide(index) {
        this.currentSlideIndex = (index + this.slideCount) % this.slideCount;

        // Slide transform
        if (this.widgetTrack) {
            this.widgetTrack.style.transform = `translateX(-${this.currentSlideIndex * 100}%)`;
        }

        // Active dots
        this.widgetDots.forEach((dot, i) => {
            dot.classList.toggle('active', i === this.currentSlideIndex);
        });
    }

    nextSlide() {
        this.goToSlide(this.currentSlideIndex + 1);
    }

    prevSlide() {
        this.goToSlide(this.currentSlideIndex - 1);
    }

    hideWidget() {
        if (this.widget) {
            this.widget.classList.add('hidden');
        }
        if (this.reopenBtn) {
            this.reopenBtn.classList.remove('hidden');
        }
    }

    showWidget() {
        if (this.widget) {
            this.widget.classList.remove('hidden');
        }
        if (this.reopenBtn) {
            this.reopenBtn.classList.add('hidden');
        }
    }

    renderRightWidget() {
        const msn = this.msnHourlyData;
        if (!msn) return;

        const cur = msn.current || {};
        const daily = msn.daily || [];
        const hourly = msn.hourly || [];

        // Widget Date & Title: "2026年10月17日 天气预报" (图一纯净格式)
        const dateTitleEl = document.getElementById('widget-date-title');
        if (dateTitleEl) {
            const now = new Date();
            const yearStr = now.getFullYear();
            const monthStr = now.getMonth() + 1;
            const dayStr = now.getDate();
            dateTitleEl.textContent = `${yearStr}年${monthStr}月${dayStr}日 天气预报`;
        }

        // ================= Slide 1: 基础实况 (图一经典五行) =================
        const s1Cond = document.getElementById('widget-s1-condition');
        const s1Icon = document.getElementById('widget-s1-icon');
        const s1Temp = document.getElementById('widget-s1-temp');
        const s1Range = document.getElementById('widget-s1-range');
        const s1Wind = document.getElementById('widget-s1-wind');
        const s1WindDir = document.getElementById('widget-s1-winddir');
        const s1WindScale = document.getElementById('widget-s1-windscale');
        const s1WindSpeed = document.getElementById('widget-s1-windspeed');
        const s1Tip = document.getElementById('widget-s1-tip');

        if (s1Cond) s1Cond.textContent = cur.condition || "小雨";
        if (s1Icon) s1Icon.src = `/assets/weather/icons/${cur.iconCode || '305'}.svg`;
        if (s1Temp) s1Temp.textContent = `${cur.temperature || 15}°C`;

        const todayDaily = daily.length > 0 ? daily[0] : null;
        const lowT = todayDaily ? todayDaily.low : (cur.temperature - 2);
        const highT = todayDaily ? todayDaily.high : (cur.temperature + 4);
        if (s1Range) s1Range.textContent = `${lowT} - ${highT} ℃`;

        // 智能解析风向、风力、风速 (图一精炼呈现)
        const windFull = String(cur.wind || "北风 1-3 级 3 km/h");
        let dir = "北风";
        let scale = "1-3 级";
        let speed = "3 km/h";

        const dirMatch = windFull.match(/(东风|南风|西风|北风|东北风|东南风|西北风|西南风|微风)/);
        if (dirMatch) dir = dirMatch[1];

        const scaleMatch = windFull.match(/(\d+[-~]?\d*级)/);
        if (scaleMatch) scale = scaleMatch[1];

        const speedMatch = windFull.match(/(\d+\s*km\/h)/i);
        if (speedMatch) speed = speedMatch[1];

        if (s1Wind) s1Wind.textContent = windFull;
        if (s1WindDir) s1WindDir.textContent = dir;
        if (s1WindScale) s1WindScale.textContent = scale;
        if (s1WindSpeed) s1WindSpeed.textContent = speed;

        if (s1Tip) {
            if ((cur.condition || "").includes("雨")) {
                s1Tip.textContent = "雨天路滑，外出请备好雨具并减速慢行。";
            } else if ((cur.condition || "").includes("晴")) {
                s1Tip.textContent = "天气晴朗，紫外线微弱，适宜开窗通风与出行。";
            } else {
                s1Tip.textContent = "出行注意天气变化，保持愉悦好心情。";
            }
        }

        // ================= Slide 2: 小时与多日微缩 (图二微缩版) =================
        const miniHourlyContainer = document.getElementById('widget-mini-hourly-list');
        if (miniHourlyContainer && hourly.length > 0) {
            miniHourlyContainer.innerHTML = "";
            hourly.slice(0, 5).forEach(h => {
                const item = document.createElement('div');
                item.className = "mini-hourly-pill";
                item.innerHTML = `
                    <div class="m-hour">${h.hour}</div>
                    <img src="/assets/weather/icons/${h.iconCode || '100'}.svg" class="m-icon" alt="icon">
                    <div class="m-temp">${h.temperature}°</div>
                `;
                miniHourlyContainer.appendChild(item);
            });
        }

        const miniDailyContainer = document.getElementById('widget-mini-daily-list');
        if (miniDailyContainer && daily.length > 0) {
            miniDailyContainer.innerHTML = "";
            daily.slice(0, 3).forEach(d => {
                const row = document.createElement('div');
                row.className = "mini-daily-row";
                row.innerHTML = `
                    <span class="d-title">${d.title}</span>
                    <span class="d-cond">${d.condition}</span>
                    <span class="d-range">${d.low}° ~ ${d.high}°</span>
                `;
                miniDailyContainer.appendChild(row);
            });
        }

        // ================= Slide 3: 核心气象指数 (图三精选版) =================
        const aqiVal = cur.aqi || 50;
        const s3Aqi = document.getElementById('widget-s3-aqi');
        const s3Hum = document.getElementById('widget-s3-hum');
        const s3Uv = document.getElementById('widget-s3-uv');
        const s3SunMoon = document.getElementById('widget-s3-sunmoon');

        if (s3Aqi) s3Aqi.textContent = `${aqiVal} 优`;
        if (s3Hum) s3Hum.textContent = `${cur.humidity || 46}% (露点 ${cur.dewPt || '-8°'})`;
        if (s3Uv) s3Uv.textContent = "1 (低)";
        if (s3SunMoon) s3SunMoon.textContent = `${(cur.sunrise || '07:05').slice(0, 5)} / ${(cur.sunset || '16:55').slice(0, 5)} · ${cur.moonPhase || '盈凸月'}`;
    }

    // =========================================================================
    // 4. Fullscreen Weather Modal (#fullscreen-weather-modal)
    // =========================================================================

    initFullscreenModalEvents() {
        const modal = this.fullscreenModal;
        const closeBtn = document.getElementById('close-fullscreen-weather-btn');
        const fullscreenToggleBtn = document.getElementById('fullscreen-app-toggle-btn') || document.getElementById('fullscreen-toggle-btn');
        const bgToggleBtn = document.getElementById('fullscreen-bg-mode-btn');
        const voiceBtn = document.getElementById('fullscreen-voice-btn');

        // Control Panel Weather Entry button
        document.getElementById('open-weather-window-btn')?.addEventListener('click', () => {
            this.openFullscreenModal();
        });
        document.getElementById('btn-open-weather-action')?.addEventListener('click', (e) => {
            e.stopPropagation();
            this.openFullscreenModal();
        });

        // Top Tip bar click opens fullscreen weather window
        this.topTipBar?.addEventListener('click', () => {
            this.openFullscreenModal();
        });

        // Close modal
        closeBtn?.addEventListener('click', () => {
            this.closeFullscreenModal();
        });

        modal?.addEventListener('click', (e) => {
            if (e.target === modal) {
                this.closeFullscreenModal();
            }
        });

        // Window / App Fullscreen Toggle
        fullscreenToggleBtn?.addEventListener('click', async () => {
            await this.toggleAppFullscreen();
        });

        // Character voice banner buttons
        document.getElementById('fs-char-next-btn')?.addEventListener('click', (e) => {
            e.stopPropagation();
            if (window.characterTipsManager) {
                window.characterTipsManager.switchNextCharacter();
            }
        });

        document.getElementById('fs-char-quote-btn')?.addEventListener('click', (e) => {
            e.stopPropagation();
            if (window.characterTipsManager) {
                window.characterTipsManager.notify();
            }
        });

        // Translucent mode toggle (透底模式：去除全屏黑底模糊，直接清晰展现底层壁纸)
        const applyTranslucentState = (isTrans) => {
            if (modal) {
                modal.classList.toggle('translucent-mode', isTrans);
            }
            document.body.classList.toggle('weather-translucent', isTrans);
            const frame = document.getElementById('fullscreen-weather-frame');
            if (frame) {
                frame.classList.toggle('wallpaper-engine-translucent', isTrans);
            }
            if (bgToggleBtn) {
                bgToggleBtn.textContent = isTrans ? "磨砂模式" : "透底模式";
                bgToggleBtn.classList.toggle('active', isTrans);
            }
        };

        this.applyTranslucentState = applyTranslucentState;
        const initialTrans = localStorage.getItem('weather_translucent_mode') === 'true';
        applyTranslucentState(initialTrans);

        bgToggleBtn?.addEventListener('click', () => {
            const current = modal?.classList.contains('translucent-mode') || false;
            const next = !current;
            applyTranslucentState(next);
            try {
                localStorage.setItem('weather_translucent_mode', String(next));
            } catch (e) {}
            window.showToast(next ? "天气视窗已切换为通透壁纸透底模式" : "已切换为高对比度磨砂玻璃模式", "info");
        });

        // Voice broadcast
        voiceBtn?.addEventListener('click', () => {
            this.broadcastCurrentWeather();
        });

        // Tabs switcher: Tab 1 (Hourly), Tab 2 (12-Grid), Tab 3 (Panorama)
        document.querySelectorAll('.fullscreen-tab-btn').forEach(btn => {
            btn.addEventListener('click', () => {
                const targetTab = btn.getAttribute('data-tab');
                this.switchFullscreenTab(targetTab);
            });
        });

        // Sub-option buttons (Overview, Precip, Wind, Humidity, Cloud)
        document.querySelectorAll('.chart-metric-btn').forEach(btn => {
            btn.addEventListener('click', () => {
                document.querySelectorAll('.chart-metric-btn').forEach(b => b.classList.remove('active'));
                btn.classList.add('active');
                this.currentChartMetric = btn.getAttribute('data-metric');
                if (this.fullscreenChart) {
                    this.fullscreenChart.setMetricType(this.currentChartMetric);
                }
                if (this.panoramaChart) {
                    this.panoramaChart.setMetricType(this.currentChartMetric);
                }
            });
        });

        // Chart vs List View mode switch
        document.getElementById('view-mode-chart-btn')?.addEventListener('click', () => {
            this.toggleViewMode('chart');
        });

        document.getElementById('view-mode-list-btn')?.addEventListener('click', () => {
            this.toggleViewMode('list');
        });

        // Feels-like temperature toggle checkbox
        document.getElementById('toggle-feels-like-checkbox')?.addEventListener('change', (e) => {
            this.showFeelsLike = e.target.checked;
            if (this.fullscreenChart) {
                this.fullscreenChart.setShowFeelsLike(this.showFeelsLike);
            }
            if (this.panoramaChart) {
                this.panoramaChart.setShowFeelsLike(this.showFeelsLike);
            }
        });
    }

    initCitySwitchEvents() {
        // Quick city capsules (Modal) & city pills (Stealth Toolbar)
        const handleCityClick = (el) => {
            document.querySelectorAll('.city-capsule, .city-pill').forEach(c => c.classList.remove('active'));
            el.classList.add('active');
            const city = el.getAttribute('data-city');
            this.fetchAndRender(null, city);
            window.showToast(`已切换至城市: ${city}`, 'info');
        };

        document.querySelectorAll('.city-capsule').forEach(capsule => {
            capsule.addEventListener('click', () => handleCityClick(capsule));
        });

        document.querySelectorAll('.city-pill').forEach(pill => {
            pill.addEventListener('click', () => handleCityClick(pill));
        });

        // City Search Input (Modal)
        const searchInput = document.getElementById('fullscreen-city-search-input');
        const searchBtn = document.getElementById('fullscreen-city-search-btn');

        const doSearch = () => {
            const query = searchInput?.value.trim();
            if (query) {
                this.fetchAndRender(null, query);
                window.showToast(`正在检索 ${query} 的气象预报...`, 'info');
            }
        };

        searchBtn?.addEventListener('click', doSearch);
        searchInput?.addEventListener('keydown', (e) => {
            if (e.key === 'Enter') doSearch();
        });

        // City Search Input (Stealth Toolbar)
        const stealthSearchInput = document.getElementById('stealth-city-input');
        const stealthSearchBtn = document.getElementById('stealth-city-btn');

        const doStealthSearch = () => {
            const query = stealthSearchInput?.value.trim();
            if (query) {
                this.fetchAndRender(null, query);
                window.showToast(`正在检索 ${query} 的气象预报...`, 'info');
            }
        };

        stealthSearchBtn?.addEventListener('click', doStealthSearch);
        stealthSearchInput?.addEventListener('keydown', (e) => {
            if (e.key === 'Enter') doStealthSearch();
        });

        // Stealth Toolbar Voice Broadcast
        document.getElementById('stealth-voice-btn')?.addEventListener('click', () => {
            this.broadcastCurrentWeather();
        });
    }

    openFullscreenModal() {
        if (!this.fullscreenModal) return;
        this.renderFullscreenModal();
        if (typeof this.applyTranslucentState === 'function') {
            const savedTrans = localStorage.getItem('weather_translucent_mode') === 'true';
            this.applyTranslucentState(savedTrans);
        }

        // 1. 彻底隐藏底层主控台面板（防止透底模式下穿透时钟和白色卡片）
        const centerPanel = document.getElementById('control-center-panel');
        if (centerPanel) centerPanel.classList.add('hidden');

        // 2. 彻底隐藏右侧悬浮挂件与右边缘胶囊（防止在全屏天气下重叠打架）
        this.hideWidget();
        if (this.reopenBtn) this.reopenBtn.classList.add('hidden');

        // 3. 彻底隐藏顶部常驻出行提示条（防止与全屏内部的角色台词栏出现双重重复）
        if (this.topTipBar) this.topTipBar.classList.add('hidden');

        this.fullscreenModal.classList.remove('hidden');

        // Resize charts
        setTimeout(() => {
            if (this.fullscreenChart) this.fullscreenChart.resize();
            if (this.panoramaChart) this.panoramaChart.resize();
        }, 100);
    }

    closeFullscreenModal() {
        if (this.fullscreenModal) {
            this.fullscreenModal.classList.add('hidden');
        }

        // 恢复顶部常驻提示胶囊
        if (this.topTipBar) this.topTipBar.classList.remove('hidden');

        // 依据当前模式恢复主界面
        const isWpOnly = document.body.classList.contains('wallpaper-only-mode');
        const centerPanel = document.getElementById('control-center-panel');
        if (centerPanel) {
            centerPanel.classList.toggle('hidden', isWpOnly);
            centerPanel.style.display = isWpOnly ? 'none' : 'flex';
        }

        if (isWpOnly) {
            this.showWidget();
        } else {
            this.hideWidget();
        }
    }

    async toggleAppFullscreen() {
        if (window.pywebview && window.pywebview.api && window.pywebview.api.toggle_fullscreen) {
            try {
                await window.pywebview.api.toggle_fullscreen();
                return;
            } catch (e) {
                console.warn("API toggle_fullscreen error:", e);
            }
        }

        // Browser fullscreen API fallback
        if (!document.fullscreenElement) {
            document.documentElement.requestFullscreen().catch(err => {
                console.warn(`Error attempting to enable fullscreen: ${err.message}`);
            });
        } else {
            if (document.exitFullscreen) {
                document.exitFullscreen();
            }
        }
    }

    switchFullscreenTab(tabName) {
        this.activeFullscreenTab = tabName;

        // Button state
        document.querySelectorAll('.fullscreen-tab-btn').forEach(btn => {
            btn.classList.toggle('active', btn.getAttribute('data-tab') === tabName);
        });

        // Pane visibility
        const paneHourly = document.getElementById('tab-pane-hourly');
        const paneGrid12 = document.getElementById('tab-pane-grid12');
        const panePanorama = document.getElementById('tab-pane-panorama');

        if (paneHourly) paneHourly.classList.toggle('hidden', tabName !== 'hourly');
        if (paneGrid12) paneGrid12.classList.toggle('hidden', tabName !== 'grid12');
        if (panePanorama) panePanorama.classList.toggle('hidden', tabName !== 'panorama');

        // Resize corresponding chart
        setTimeout(() => {
            if (tabName === 'hourly' && this.fullscreenChart) {
                this.fullscreenChart.resize();
                this.fullscreenChart.render();
            } else if (tabName === 'panorama' && this.panoramaChart) {
                this.panoramaChart.resize();
                this.panoramaChart.render();
            }
        }, 50);
    }

    toggleViewMode(mode) {
        this.isListView = (mode === 'list');
        const chartWrapper = document.getElementById('fullscreen-canvas-wrapper');
        const listWrapper = document.getElementById('fullscreen-list-wrapper');
        const chartBtn = document.getElementById('view-mode-chart-btn');
        const listBtn = document.getElementById('view-mode-list-btn');

        if (chartBtn) chartBtn.classList.toggle('active', !this.isListView);
        if (listBtn) listBtn.classList.toggle('active', this.isListView);

        if (chartWrapper) chartWrapper.classList.toggle('hidden', this.isListView);
        if (listWrapper) listWrapper.classList.toggle('hidden', !this.isListView);

        if (this.isListView && this.fullscreenChart) {
            this.fullscreenChart.renderList(listWrapper);
        } else if (this.fullscreenChart) {
            this.fullscreenChart.render();
        }
    }

    renderFullscreenModal() {
        const msn = this.msnHourlyData;
        if (!msn) return;

        const cur = msn.current || {};
        const daily = msn.daily || [];
        const hourly = msn.hourly || [];

        // 1. Top Toolbar Info
        const modalCity = document.getElementById('fullscreen-city-title');
        const modalTemp = document.getElementById('fullscreen-current-temp');
        const modalFeels = document.getElementById('fullscreen-current-feels');
        const modalCond = document.getElementById('fullscreen-current-cond');

        if (modalCity) modalCity.textContent = msn.cityName || this.currentCity;
        if (modalTemp) modalTemp.textContent = `${cur.temperature || 15}°C`;
        if (modalFeels) modalFeels.textContent = `体感 ${cur.feels || (cur.temperature - 1)}°C`;
        if (modalCond) modalCond.textContent = `${cur.condition || '小雨'} · 空气AQI ${cur.aqi || 50} 优`;

        // Highlight active city capsule
        document.querySelectorAll('.city-capsule').forEach(cap => {
            const cityName = cap.getAttribute('data-city');
            cap.classList.toggle('active', this.currentCity.includes(cityName) || cityName.includes(this.currentCity));
        });

        // 2. Render 7-Day Horizontal Scroll Cards
        this.renderMultiDayScrollCards(daily);

        // 3. Render Canvas Hourly Chart
        this.renderHourlyCharts(hourly, cur);

        // 4. Render 12-Grid Indicators (Tab 2 and Panorama Tab 3)
        this.render12GridIndicators(cur, daily, hourly);
    }

    renderMultiDayScrollCards(dailyList) {
        const container = document.getElementById('fullscreen-daily-grid');
        const panoContainer = document.getElementById('panorama-daily-grid');

        const buildCards = (targetEl) => {
            if (!targetEl || !dailyList) return;
            targetEl.innerHTML = "";
            dailyList.forEach(d => {
                const card = document.createElement('div');
                card.className = "msn-daily-card";
                card.innerHTML = `
                    <div class="daily-title">${d.title}</div>
                    <div class="daily-icon"><img src="/assets/weather/icons/${d.iconCode || '100'}.svg" alt="icon"></div>
                    <div class="daily-temps">
                        <span class="high">${d.high}°</span>
                        <span class="low">${d.low}°</span>
                    </div>
                    <div class="daily-cond">${d.condition}</div>
                `;
                targetEl.appendChild(card);
            });
        };

        buildCards(container);
        buildCards(panoContainer);
    }

    renderHourlyCharts(hourlyList, cur) {
        const meta = {
            sunrise: cur.sunrise || "07:05",
            sunset: cur.sunset || "16:55"
        };

        // Fullscreen Tab 1 Chart
        if (!this.fullscreenChart && typeof HourlyWeatherChart !== 'undefined') {
            this.fullscreenChart = new HourlyWeatherChart('fullscreen-hourly-canvas');
        }
        if (this.fullscreenChart) {
            this.fullscreenChart.setData(hourlyList, meta, (hourItem) => {
                const hoverInfo = document.getElementById('fullscreen-hover-info');
                if (hoverInfo) {
                    hoverInfo.innerHTML = `
                        <strong>${hourItem.hour}</strong>: ${hourItem.temperature}°C (体感 ${hourItem.feels}°C) | ${hourItem.condition} | 降水率: ${hourItem.precipitation}% | 湿度: ${hourItem.humidity}% | ${hourItem.windDir} ${hourItem.windSpeed}km/h
                    `;
                }
            });
            this.fullscreenChart.setMetricType(this.currentChartMetric);
            this.fullscreenChart.setShowFeelsLike(this.showFeelsLike);
        }

        // Panorama Tab 3 Chart
        if (!this.panoramaChart && typeof HourlyWeatherChart !== 'undefined') {
            this.panoramaChart = new HourlyWeatherChart('panorama-hourly-canvas');
        }
        if (this.panoramaChart) {
            this.panoramaChart.setData(hourlyList, meta);
            this.panoramaChart.setMetricType(this.currentChartMetric);
            this.panoramaChart.setShowFeelsLike(this.showFeelsLike);
        }
    }

    render12GridIndicators(cur, daily, hourly) {
        // ================= 1. 温度 / 体感温度 =================
        const tempValEl = document.getElementById('ind-temp-val');
        const tempTextEl = document.getElementById('ind-temp-text');
        if (tempValEl) tempValEl.textContent = `${cur.temperature || 15}°C / 体感 ${cur.feels || 14}°C`;
        if (tempTextEl) tempTextEl.textContent = `气温稳定，早晚温差适中，建议按时增添衣物。`;

        // ================= 2. 云量 (Cloud Cover) =================
        const cloudValEl = document.getElementById('ind-cloud-val');
        const cloudTextEl = document.getElementById('ind-cloud-text');
        const condLower = (cur.condition || "").toLowerCase();
        let cloudPct = 5;
        if (condLower.includes("晴")) cloudPct = 5;
        else if (condLower.includes("多云")) cloudPct = 40;
        else if (condLower.includes("阴")) cloudPct = 80;
        else if (condLower.includes("雨")) cloudPct = 95;

        if (cloudValEl) cloudValEl.textContent = `${cur.condition || '晴朗'} ${cloudPct}%`;
        if (cloudTextEl) cloudTextEl.textContent = cloudPct < 30 ? "少云覆盖，天高气爽，采光极佳。" : "云层较厚，阴沉多云，注意采光。";

        // ================= 3. 降水 (Precipitation) =================
        const precipValEl = document.getElementById('ind-precip-val');
        const precipTextEl = document.getElementById('ind-precip-text');
        const hasRain = condLower.includes("雨");
        if (precipValEl) precipValEl.textContent = hasRain ? "2.5 毫米 (今日降雨)" : "0 毫米 (无有效降水)";
        if (precipTextEl) precipTextEl.textContent = hasRain ? "伴有间歇性降雨，地面湿滑，请减速慢行。" : "未来 24 小时无明显降水过程。";

        // ================= 4. 风速风向 (Compass & Wind) =================
        const windValEl = document.getElementById('ind-wind-val');
        const windNeedleEl = document.getElementById('ind-compass-needle');
        const windDegText = document.getElementById('ind-wind-deg');
        let windAngle = 201; // default southwest
        if (cur.wind) {
            const numMatch = cur.wind.match(/(\d+)/);
            if (numMatch && parseInt(numMatch[1], 10) > 30) {
                windAngle = parseInt(numMatch[1], 10);
            }
        }
        if (windNeedleEl) {
            windNeedleEl.style.transform = `rotate(${windAngle}deg)`;
        }
        if (windValEl) windValEl.textContent = cur.wind || "西南风 2级 (6 km/h)";
        if (windDegText) windDegText.textContent = `方位角 ${windAngle}° · 阵风 15 km/h`;

        // ================= 5. 湿度与露点 (7-Segment Humidity Pillar) =================
        const humVal = parseInt(cur.humidity, 10) || 46;
        const humValEl = document.getElementById('ind-hum-val');
        const humDewEl = document.getElementById('ind-dew-val');
        if (humValEl) humValEl.textContent = `${humVal}%`;
        if (humDewEl) humDewEl.textContent = `露点: ${cur.dewPt || '-8°'}`;

        // Fill 7 segment bars
        const segments = document.querySelectorAll('.humidity-pillar-seg');
        const activeSegCount = Math.min(7, Math.max(1, Math.round((humVal / 100) * 7)));
        segments.forEach((seg, idx) => {
            seg.classList.toggle('active', idx < activeSegCount);
        });

        // ================= 6. 紫外线 (UV Gauge) =================
        const uvValEl = document.getElementById('ind-uv-val');
        const uvNeedleEl = document.getElementById('ind-uv-needle');
        const uvTextEl = document.getElementById('ind-uv-text');
        const uvIndex = 1; // Low
        if (uvValEl) uvValEl.textContent = `低 (${uvIndex})`;
        if (uvNeedleEl) {
            const uvPct = (uvIndex / 12) * 100;
            uvNeedleEl.style.left = `${Math.min(95, Math.max(5, uvPct))}%`;
        }
        if (uvTextEl) uvTextEl.textContent = "紫外线指数低，在户外活动安全无须过多防护。";

        // ================= 7. 空气质量 (AQI Ring Gauge) =================
        const aqiNum = parseInt(cur.aqi, 10) || 50;
        const aqiValEl = document.getElementById('ind-aqi-val');
        const aqiRingEl = document.getElementById('ind-aqi-ring');
        const aqiDescEl = document.getElementById('ind-aqi-desc');

        if (aqiValEl) aqiValEl.textContent = aqiNum;
        if (aqiDescEl) aqiDescEl.textContent = aqiNum <= 50 ? "优 (PM2.5 18 µg/m³)" : (aqiNum <= 100 ? "良 (PM2.5 42 µg/m³)" : "轻度污染");

        if (aqiRingEl) {
            const ringColor = aqiNum <= 50 ? '#10b981' : (aqiNum <= 100 ? '#f59e0b' : '#ef4444');
            aqiRingEl.style.borderColor = ringColor;
        }

        // ================= 8. 能见度 (Visibility Stepped Pyramid) =================
        const visVal = cur.visibility || 18.4;
        const visValEl = document.getElementById('ind-vis-val');
        const visDescEl = document.getElementById('ind-vis-desc');
        if (visValEl) visValEl.textContent = `${visVal} 公里`;
        if (visDescEl) visDescEl.textContent = visVal >= 15 ? "大气通透度极好，视线辽阔。" : "轻微低能见度，视线平稳。";

        // ================= 9. 气压 (Barometer Scale Slider) =================
        const pressVal = parseInt(cur.airPressure, 10) || 1030;
        const pressValEl = document.getElementById('ind-press-val');
        const pressIndicatorEl = document.getElementById('ind-press-indicator');
        const pressDescEl = document.getElementById('ind-press-desc');

        if (pressValEl) pressValEl.textContent = `${pressVal} hPa`;
        if (pressDescEl) pressDescEl.textContent = "缓慢上升，气压平稳。";

        if (pressIndicatorEl) {
            // scale 980 - 1050 hPa
            const minP = 980, maxP = 1050;
            const pct = ((pressVal - minP) / (maxP - minP)) * 100;
            pressIndicatorEl.style.left = `${Math.min(95, Math.max(5, pct))}%`;
        }

        // ================= 10. 太阳轨迹 (Seven-color Arch Arc) =================
        const sunriseStr = (cur.sunrise || "07:05").slice(0, 5);
        const sunsetStr = (cur.sunset || "16:55").slice(0, 5);
        const sunTimesEl = document.getElementById('ind-sun-times');
        const sunDaylightEl = document.getElementById('ind-sun-daylight');
        const sunOrbEl = document.getElementById('ind-sun-orb');

        if (sunTimesEl) sunTimesEl.textContent = `日出 ${sunriseStr} · 日落 ${sunsetStr}`;
        if (sunDaylightEl) sunDaylightEl.textContent = "白昼时长: 9小时50分钟";

        if (sunOrbEl) {
            // calculate sun position based on current time
            const now = new Date();
            const nowMinutes = now.getHours() * 60 + now.getMinutes();
            const parseM = (s) => {
                const p = s.split(':');
                return parseInt(p[0], 10) * 60 + parseInt(p[1], 10);
            };
            const srM = parseM(sunriseStr);
            const ssM = parseM(sunsetStr);

            let progress = 0.5;
            if (nowMinutes >= srM && nowMinutes <= ssM) {
                progress = (nowMinutes - srM) / (ssM - srM);
            } else if (nowMinutes < srM) {
                progress = 0.05;
            } else {
                progress = 0.95;
            }
            sunOrbEl.style.left = `${Math.min(95, Math.max(5, progress * 100))}%`;
        }

        // ================= 11. 月相 (Moon Phase) =================
        const moonPhaseName = cur.moonPhase || "盈凸月";
        const moonValEl = document.getElementById('ind-moon-val');
        const moonDescEl = document.getElementById('ind-moon-desc');
        if (moonValEl) moonValEl.textContent = moonPhaseName;
        if (moonDescEl) moonDescEl.textContent = "照明度 82% · 距满月还有 3 天";

        // ================= 12. 气象预警与出行建议 =================
        const warningTitleEl = document.getElementById('ind-warning-title');
        const warningDescEl = document.getElementById('ind-warning-desc');
        const travelIndexEl = document.getElementById('ind-travel-index');

        if (warningTitleEl && warningDescEl) {
            if (condLower.includes("雨")) {
                warningTitleEl.textContent = "道路结冰与湿滑提示";
                warningDescEl.textContent = "降水导致部分路面附着力降低，行车请减速慢行保持车距，行人注意防滑。";
            } else {
                warningTitleEl.textContent = "全市气象安全良好";
                warningDescEl.textContent = "目前未发布突发暴雨、大风、雷电等恶劣天气气象灾害预警信号。";
            }
        }

        if (travelIndexEl) {
            travelIndexEl.innerHTML = `
                <span>出行安全: <strong>适宜</strong></span> ·
                <span>穿衣: <strong>外套/风衣</strong></span> ·
                <span>洗车: <strong>${condLower.includes("雨") ? "不适宜" : "适宜"}</strong></span>
            `;
        }
    }

    setWallpaperManager(wallpaperManager) {
        this.wallpaperManager = wallpaperManager;
    }

    syncParticles(condition) {
        if (this.wallpaperManager) {
            this.wallpaperManager.handleWeatherConditionChange(condition);
        }
        if (!this.particles) return;
        const c = (condition || "").toLowerCase();
        if (c.includes("雨")) {
            this.particles.setMode('rain');
        } else if (c.includes("雪")) {
            this.particles.setMode('snow');
        } else if (c.includes("雾") || c.includes("霾")) {
            this.particles.setMode('fog');
        } else if (c.includes("晴")) {
            this.particles.setMode('sunny');
        } else {
            this.particles.setMode('stars');
        }
    }

    broadcastCurrentWeather() {
        if (this.audio) {
            this.audio.playWeatherVoice(this.currentCondition);
            window.showToast(`正在播报 ${this.currentCity} 今日气象语音`, 'info');
        }
    }
}

if (typeof window !== 'undefined') {
    window.WeatherController = WeatherController;
}
