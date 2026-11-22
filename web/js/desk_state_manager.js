/**
 * Desktop State Machine & Dynamics Controller (ays2z-233)
 * Orchestrates:
 * 1. State A: Overview Mode (Startup default, 6s auto-transition to State B)
 * 2. State B: Wallpaper Immersion + Peripheral Carousel (Clean wallpaper, 8s edge rotation)
 * 3. Stealth Top Toolbar (Auto-reveal on hover)
 * 4. High-contrast typography & unified data dispatch
 */

const UI_CONFIG = {
    OVERVIEW_STAY_DURATION: 6000,    // 状态 A 停留 6 秒后自动切入状态 B (毫秒)
    CAROUSEL_INTERVAL: 8000,         // 状态 B 边缘模块轮播间隔 8 秒 (毫秒)
    TRANSITION_DURATION: 400         // 动画平滑过渡时长 (毫秒)
};

class DeskStateManager {
    constructor() {
        this.currentState = 'overview'; // 'overview' (State A) | 'immersion' (State B)
        this.overviewTimer = null;
        this.carouselTimer = null;
        this.currentCarouselIndex = 0;
        this.carouselTotal = 3;
        this.isCarouselPaused = false;
        this.isHoveringOverview = false;

        // Cached Data
        this.weatherData = null;
        this.characterTip = null;
        this.overviewChart = null;
        this.currentDeckSubTab = 'hourly'; // 'hourly' | 'grid12' | 'daily'

        this.init();
    }

    init() {
        this.initElements();
        this.initEventListeners();
        this.initClock();
        
        // Default startup: State A (Overview Mode)
        this.setState('overview', false);

        // Start overview auto-transition countdown (6s)
        this.startOverviewCountdown();

        // Subscribe to companion character tips
        if (window.characterTipsManager) {
            window.characterTipsManager.subscribe((tip) => {
                this.syncCharacterTip(tip);
            });
        }

        // If weather controller already loaded data, sync it immediately
        if (window.weatherCtrl && window.weatherCtrl.msnHourlyData) {
            this.syncWeatherData(window.weatherCtrl.msnHourlyData);
        }
    }

    initElements() {
        // Toolbar
        this.toolbar = document.getElementById('stealth-top-toolbar');
        this.btnStateOverview = document.getElementById('btn-state-overview');
        this.btnStateImmersion = document.getElementById('btn-state-immersion');

        // Views
        this.viewOverview = document.getElementById('view-overview');
        this.viewImmersion = document.getElementById('view-immersion');

        // Clocks
        this.overviewClock = document.getElementById('overview-clock');
        this.overviewDate = document.getElementById('overview-date');
        this.immersionTime = document.getElementById('immersion-time');
        this.immersionDate = document.getElementById('immersion-date');
        this.immersionTopClock = document.getElementById('immersion-top-clock');

        // Character Quote Banner (State A)
        this.overviewCharBadge = document.getElementById('overview-char-badge');
        this.overviewCharQuote = document.getElementById('overview-char-quote');
        this.btnCharNext = document.getElementById('overview-char-next-btn');
        this.btnCharQuote = document.getElementById('overview-char-quote-btn');

        // Overview Weather Summary
        this.ovWeatherIcon = document.getElementById('ov-weather-icon');
        this.ovWeatherTemp = document.getElementById('ov-weather-temp');
        this.ovWeatherCond = document.getElementById('ov-weather-cond');
        this.ovWeatherRange = document.getElementById('ov-weather-range');
        this.ovWeatherFeels = document.getElementById('ov-weather-feels');
        this.ovWeatherAqi = document.getElementById('ov-weather-aqi');
        this.ovWeatherHum = document.getElementById('ov-weather-hum');
        this.ovWeatherWind = document.getElementById('ov-weather-wind');

        // Peripheral Carousel
        this.carouselContainer = document.getElementById('peripheral-carousel');
        this.carouselCards = document.querySelectorAll('.carousel-card');
        this.carouselDots = document.querySelectorAll('.c-dot');
        this.btnCarouselExpand = document.getElementById('btn-carousel-expand');
        this.btnQuoteRefresh = document.getElementById('c-quote-refresh-btn');

        // Carousel Card 1 (Weather)
        this.cCardCity = document.getElementById('c-card-city');
        this.cWeatherIcon = document.getElementById('c-weather-icon');
        this.cWeatherTemp = document.getElementById('c-weather-temp');
        this.cWeatherCond = document.getElementById('c-weather-cond');
        this.cWeatherRange = document.getElementById('c-weather-range');
        this.cWeatherFeels = document.getElementById('c-weather-feels');
        this.cWeatherWind = document.getElementById('c-weather-wind');
        this.cWeatherAqi = document.getElementById('c-weather-aqi');

        // Carousel Card 2 (Quote)
        this.cQuoteBadge = document.getElementById('c-quote-badge');
        this.cQuoteText = document.getElementById('c-quote-text');

        // Carousel Card 3 (Trend)
        this.cMiniHourlyRow = document.getElementById('c-mini-hourly-row');
        this.cTrendTip = document.getElementById('c-trend-tip');

        // Simultaneous Sections & Controls (24h & 12-Grid)
        this.deckSectionHourly = document.getElementById('deck-section-hourly');
        this.deckSectionGrid12 = document.getElementById('deck-section-grid12');
        this.deckPaneDaily = document.getElementById('deck-pane-daily');
        this.btnToggle7Day = document.getElementById('btn-toggle-7day');
        this.btnDockImmersion = document.getElementById('btn-dock-immersion');
        this.cBtnOpenGrid12 = document.getElementById('c-btn-open-grid12');
    }

    initEventListeners() {
        // 1. Manual switch via toolbar buttons
        this.btnStateOverview?.addEventListener('click', () => {
            this.setState('overview');
            this.clearOverviewCountdown(); // Pinned mode when user clicks
            window.showToast?.("已展开「24h逐时走势 + MSN 12宫格」全景大屏", "info");
        });

        this.btnStateImmersion?.addEventListener('click', () => {
            this.setState('immersion');
        });

        // 7-day forecast toggle & immersion button in weather deck
        document.getElementById('btn-toggle-7day')?.addEventListener('click', () => {
            const paneDaily = document.getElementById('deck-pane-daily');
            const btn = document.getElementById('btn-toggle-7day');
            if (paneDaily) {
                const isHidden = paneDaily.classList.toggle('hidden');
                if (btn) btn.textContent = isHidden ? "7天多日预报 ▼" : "收起 7 天预报 ▲";
                if (!isHidden && this.weatherData) {
                    this.renderDailyGrid(this.weatherData.daily || []);
                }
            }
        });

        document.getElementById('btn-dock-immersion')?.addEventListener('click', () => {
            this.setState('immersion');
        });

        // Direct entrance to 12-grid from State B Peripheral Carousel
        this.cBtnOpenGrid12?.addEventListener('click', (e) => {
            e.stopPropagation();
            this.setState('overview');
            this.clearOverviewCountdown();
            const gridSec = document.getElementById('deck-section-grid12');
            gridSec?.scrollIntoView({ behavior: 'smooth' });
        });

        // 2. Click immersion top clock to expand State A (Overview)
        this.immersionTopClock?.addEventListener('click', () => {
            this.setState('overview');
        });

        // 3. Click carousel bottom expand button to expand State A
        this.btnCarouselExpand?.addEventListener('click', () => {
            this.setState('overview');
        });

        // 4. Hover pause/resume on Overview Area (in State A)
        this.viewOverview?.addEventListener('mouseenter', () => {
            this.isHoveringOverview = true;
            this.clearOverviewCountdown();
        });

        this.viewOverview?.addEventListener('mouseleave', () => {
            this.isHoveringOverview = false;
            if (this.currentState === 'overview') {
                this.startOverviewCountdown();
            }
        });

        // 5. Hover pause/resume on Peripheral Carousel (in State B)
        this.carouselContainer?.addEventListener('mouseenter', () => {
            this.isCarouselPaused = true;
            this.clearCarouselTimer();
        });

        this.carouselContainer?.addEventListener('mouseleave', () => {
            this.isCarouselPaused = false;
            if (this.currentState === 'immersion') {
                this.startCarouselLoop();
            }
        });

        // 6. Carousel Dots manual click
        this.carouselDots.forEach(dot => {
            dot.addEventListener('click', (e) => {
                e.stopPropagation();
                const idx = parseInt(dot.getAttribute('data-index'), 10) || 0;
                this.showCarouselSlide(idx);
                // Restart timer after manual switch
                this.startCarouselLoop();
            });
        });

        // 7. Character Voice Actions
        this.btnCharNext?.addEventListener('click', (e) => {
            e.stopPropagation();
            window.characterTipsManager?.switchNextCharacter();
        });

        this.btnCharQuote?.addEventListener('click', (e) => {
            e.stopPropagation();
            window.characterTipsManager?.notify();
        });

        this.btnQuoteRefresh?.addEventListener('click', (e) => {
            e.stopPropagation();
            window.characterTipsManager?.notify();
        });

        // 8. Global Keyboard ESC to toggle state
        window.addEventListener('keydown', (e) => {
            if (e.key === 'Escape') {
                if (this.currentState === 'overview') {
                    this.setState('immersion');
                } else {
                    this.setState('overview');
                }
            }
        });
    }

    initClock() {
        const tick = () => {
            const now = new Date();
            const timeFull = now.toLocaleTimeString('zh-CN', { hour12: false });
            const timeShort = `${String(now.getHours()).padStart(2, '0')}:${String(now.getMinutes()).padStart(2, '0')}`;
            const dateStr = now.toLocaleDateString('zh-CN', {
                year: 'numeric',
                month: 'long',
                day: 'numeric',
                weekday: 'long'
            });
            const weekDays = ['周日', '周一', '周二', '周三', '周四', '周五', '周六'];
            const dateShort = `${now.getMonth() + 1}月${now.getDate()}日 ${weekDays[now.getDay()]}`;

            // State A clock
            if (this.overviewClock) this.overviewClock.textContent = timeFull;
            if (this.overviewDate) this.overviewDate.textContent = dateStr;

            // State B clock
            if (this.immersionTime) this.immersionTime.textContent = timeShort;
            if (this.immersionDate) this.immersionDate.textContent = dateShort;
        };

        setInterval(tick, 1000);
        tick();
    }

    // =========================================================================
    // State Machine Transitions
    // =========================================================================

    setState(state, animated = true) {
        if (state === this.currentState && animated) return;
        this.currentState = state;

        // Update toolbar indicators
        if (this.btnStateOverview) this.btnStateOverview.classList.toggle('active', state === 'overview');
        if (this.btnStateImmersion) this.btnStateImmersion.classList.toggle('active', state === 'immersion');

        if (state === 'overview') {
            // Enter State A: Overview Mode
            this.clearCarouselTimer();
            if (this.viewImmersion) {
                this.viewImmersion.classList.add('hidden');
                this.viewImmersion.classList.remove('state-enter');
            }

            if (this.viewOverview) {
                this.viewOverview.classList.remove('hidden');
                if (animated) {
                    this.viewOverview.classList.add('state-enter');
                    setTimeout(() => this.viewOverview.classList.remove('state-enter'), UI_CONFIG.TRANSITION_DURATION);
                }
            }

            // Resize Chart and Render 12-Grid simultaneously
            setTimeout(() => {
                if (!this.overviewChart && window.HourlyWeatherChart && this.weatherData) {
                    this.overviewChart = new HourlyWeatherChart('overview-hourly-canvas');
                    this.overviewChart.setData(this.weatherData.hourly || [], {
                        sunrise: this.weatherData.current?.sunrise || "07:05",
                        sunset: this.weatherData.current?.sunset || "16:55"
                    });
                }
                if (this.overviewChart) {
                    this.overviewChart.resize();
                    this.overviewChart.render();
                }
                if (this.weatherData) {
                    this.render12Grid(this.weatherData);
                }
            }, 80);

            // Start 6s auto-transition timer
            if (!this.isHoveringOverview) {
                this.startOverviewCountdown();
            }

            if (animated) {
                window.showToast?.("已切换至「全景概览态」，6秒无操作将自动进入壁纸沉浸", "info");
            }

        } else {
            // Enter State B: Wallpaper Immersion + Peripheral Carousel
            this.clearOverviewCountdown();

            if (this.viewOverview) {
                this.viewOverview.classList.add('hidden');
                this.viewOverview.classList.remove('state-enter');
            }

            if (this.viewImmersion) {
                this.viewImmersion.classList.remove('hidden');
                if (animated) {
                    this.viewImmersion.classList.add('state-enter');
                    setTimeout(() => this.viewImmersion.classList.remove('state-enter'), UI_CONFIG.TRANSITION_DURATION);
                }
            }

            // Start 8s peripheral carousel loop
            this.showCarouselSlide(this.currentCarouselIndex, false);
            this.startCarouselLoop();

            if (animated) {
                window.showToast?.("已进入「壁纸沉浸模式」，右下角边缘轮播实时气象", "info");
            }
        }
    }

    startOverviewCountdown() {
        this.clearOverviewCountdown();
        this.overviewTimer = setTimeout(() => {
            if (this.currentState === 'overview' && !this.isHoveringOverview) {
                this.setState('immersion');
            }
        }, UI_CONFIG.OVERVIEW_STAY_DURATION);
    }

    clearOverviewCountdown() {
        if (this.overviewTimer) {
            clearTimeout(this.overviewTimer);
            this.overviewTimer = null;
        }
    }

    // =========================================================================
    // State B: Peripheral Carousel (8s Loop)
    // =========================================================================

    startCarouselLoop() {
        this.clearCarouselTimer();
        this.carouselTimer = setInterval(() => {
            if (!this.isCarouselPaused && this.currentState === 'immersion') {
                this.nextCarouselSlide();
            }
        }, UI_CONFIG.CAROUSEL_INTERVAL);
    }

    clearCarouselTimer() {
        if (this.carouselTimer) {
            clearInterval(this.carouselTimer);
            this.carouselTimer = null;
        }
    }

    nextCarouselSlide() {
        const nextIdx = (this.currentCarouselIndex + 1) % this.carouselTotal;
        this.showCarouselSlide(nextIdx);
    }

    showCarouselSlide(targetIndex, animated = true) {
        this.currentCarouselIndex = targetIndex;

        this.carouselCards.forEach((card, idx) => {
            if (idx === targetIndex) {
                card.classList.remove('exit-down', 'exit-up');
                card.classList.add('active');
                if (animated) {
                    card.classList.add('enter-slide');
                    setTimeout(() => card.classList.remove('enter-slide'), UI_CONFIG.TRANSITION_DURATION);
                }
            } else {
                card.classList.remove('active', 'enter-slide');
            }
        });

        // Update indicator dots
        this.carouselDots.forEach((dot, idx) => {
            dot.classList.toggle('active', idx === targetIndex);
        });
    }




    render12Grid(msnData) {
        if (!msnData) return;
        const cur = msnData.current || {};
        const daily = msnData.daily || [];
        const hourly = msnData.hourly || [];
        const cityName = msnData.cityName || "安阳市";

        const condStr = cur.condition || "小雨";
        const condLower = condStr.toLowerCase();
        const hasRain = condLower.includes("雨") || condLower.includes("雪");

        // 1. 温度 (Temperature)
        const indTempVal = document.getElementById('ind-temp-val');
        const indTempTag = document.getElementById('ind-temp-tag');
        const indTempText = document.getElementById('ind-temp-text');
        if (indTempVal) indTempVal.textContent = `${cur.temperature || 15}°C`;
        if (indTempTag) indTempTag.textContent = "稳定";
        if (indTempText) indTempText.textContent = `气温稳定，早晚温差适中，建议按时增添衣物。`;

        // 2. 体感温度 (Feels Like)
        const indFeelsVal = document.getElementById('ind-feels-val');
        const indFeelsTag = document.getElementById('ind-feels-tag');
        const indFeelsDesc = document.getElementById('ind-feels-desc');
        if (indFeelsVal) indFeelsVal.textContent = `体感 ${cur.feels || (cur.temperature - 1)}°C`;
        if (indFeelsTag) indFeelsTag.textContent = "主要因素: 湿度";
        if (indFeelsDesc) indFeelsDesc.textContent = `体感与实际气温相近，舒适度良好。`;

        // 3. 云量 (Cloud Cover)
        const indCloudVal = document.getElementById('ind-cloud-val');
        const indCloudTag = document.getElementById('ind-cloud-tag');
        const indCloudText = document.getElementById('ind-cloud-text');
        let cloudPct = 5;
        if (condLower.includes("晴")) cloudPct = 5;
        else if (condLower.includes("多云")) cloudPct = 40;
        else if (condLower.includes("阴")) cloudPct = 80;
        else if (hasRain) cloudPct = 95;
        if (indCloudVal) indCloudVal.textContent = `${condStr} ${cloudPct}%`;
        if (indCloudTag) indCloudTag.textContent = `${condStr} ${cloudPct}%`;
        if (indCloudText) indCloudText.textContent = cloudPct < 30 ? "少云覆盖，天高气爽，采光极佳。" : "云层较厚，天空透光度有所减弱。";

        // 4. 降水 (Precipitation)
        const indPrecipVal = document.getElementById('ind-precip-val');
        const indPrecipTag = document.getElementById('ind-precip-tag');
        const indPrecipFill = document.getElementById('ind-precip-fill');
        const indPrecipText = document.getElementById('ind-precip-text');
        if (indPrecipVal) indPrecipVal.textContent = hasRain ? "2.5 毫米" : "0 毫米";
        if (indPrecipTag) indPrecipTag.textContent = hasRain ? "小雨" : "无降水";
        if (indPrecipFill) indPrecipFill.style.width = hasRain ? "45%" : "0%";
        if (indPrecipText) indPrecipText.textContent = hasRain ? "伴有间歇性降水，地面湿滑，出行请备好雨具。" : "未来 24 小时无明显降水过程。";

        // 5. 风速 (Wind)
        const indWindVal = document.getElementById('ind-wind-val');
        const indWindDeg = document.getElementById('ind-wind-deg');
        const indWindTag = document.getElementById('ind-wind-tag');
        const indCompassNeedle = document.getElementById('ind-compass-needle');
        let windAngle = 201;
        if (cur.wind) {
            const numMatch = cur.wind.match(/(\d+)/);
            if (numMatch && parseInt(numMatch[1], 10) > 30) {
                windAngle = parseInt(numMatch[1], 10);
            }
        }
        if (indWindVal) indWindVal.textContent = cur.wind || "西南风 2级 (6 km/h)";
        if (indWindDeg) indWindDeg.textContent = `方位角 ${windAngle}° · 阵风 15 km/h`;
        if (indWindTag) indWindTag.textContent = "风力: 2 (轻风)";
        if (indCompassNeedle) indCompassNeedle.style.transform = `rotate(${windAngle}deg)`;

        // 6. 湿度与露点 (Humidity)
        const humVal = parseInt(cur.humidity, 10) || 45;
        const indHumVal = document.getElementById('ind-hum-val');
        const indDewVal = document.getElementById('ind-dew-val');
        const indHumTag = document.getElementById('ind-hum-tag');
        const indHumDesc = document.getElementById('ind-hum-desc');
        if (indHumVal) indHumVal.textContent = `${humVal}% 相对湿度`;
        if (indDewVal) indDewVal.textContent = `${cur.dewPt || '-8°'} 露点`;
        if (indHumTag) indHumTag.textContent = humVal < 40 ? "干燥" : (humVal < 70 ? "适宜" : "潮湿");
        if (indHumDesc) indHumDesc.textContent = "湿度适中，体感较为舒适。";
        const segments = document.querySelectorAll('.humidity-pillar-seg');
        const activeSegCount = Math.min(7, Math.max(1, Math.round((humVal / 100) * 7)));
        segments.forEach((seg, idx) => {
            seg.classList.toggle('active', idx < activeSegCount);
        });

        // 7. 紫外线 (UV Index)
        const indUvVal = document.getElementById('ind-uv-val');
        const indUvNeedle = document.getElementById('ind-uv-needle');
        const indUvText = document.getElementById('ind-uv-text');
        const indUvTag = document.getElementById('ind-uv-tag');
        const uvIndex = 1;
        if (indUvVal) indUvVal.textContent = `低 (${uvIndex})`;
        if (indUvTag) indUvTag.textContent = "低";
        if (indUvNeedle) indUvNeedle.style.left = `${(uvIndex / 12) * 100}%`;
        if (indUvText) indUvText.textContent = "紫外线指数低，在户外活动安全无须过多防护。";

        // 8. 空气质量 (AQI)
        const aqiNum = parseInt(cur.aqi, 10) || 50;
        const indAqiVal = document.getElementById('ind-aqi-val');
        const indAqiTag = document.getElementById('ind-aqi-tag');
        const indAqiRing = document.getElementById('ind-aqi-ring');
        const indAqiGrade = document.getElementById('ind-aqi-grade');
        const indAqiDesc = document.getElementById('ind-aqi-desc');
        const aqiText = aqiNum <= 50 ? "优" : (aqiNum <= 100 ? "良" : "轻度污染");
        const aqiColor = aqiNum <= 50 ? "#10b981" : (aqiNum <= 100 ? "#f59e0b" : "#ef4444");
        if (indAqiVal) indAqiVal.textContent = aqiNum;
        if (indAqiTag) indAqiTag.textContent = aqiText;
        if (indAqiGrade) indAqiGrade.textContent = `${aqiNum} ${aqiText}`;
        if (indAqiDesc) indAqiDesc.textContent = `${aqiText} (主要污染物: PM2.5 18 µg/m³)`;
        if (indAqiRing) indAqiRing.style.borderColor = aqiColor;

        // 9. 能见度 (Visibility)
        const visVal = cur.visibility || 17.6;
        const indVisVal = document.getElementById('ind-vis-val');
        const indVisTag = document.getElementById('ind-vis-tag');
        const indVisDesc = document.getElementById('ind-vis-desc');
        if (indVisVal) indVisVal.textContent = `${visVal} 公里`;
        if (indVisTag) indVisTag.textContent = visVal >= 15 ? "极好" : "良好";
        if (indVisDesc) indVisDesc.textContent = visVal >= 15 ? "大气通透度极好，视线辽阔。" : "轻微低能见度，视线平稳。";

        // 10. 气压 (Barometer)
        const pressVal = parseInt(cur.airPressure, 10) || 1027;
        const indPressVal = document.getElementById('ind-press-val');
        const indPressTag = document.getElementById('ind-press-tag');
        const indPressIndicator = document.getElementById('ind-press-indicator');
        const indPressDesc = document.getElementById('ind-press-desc');
        if (indPressVal) indPressVal.textContent = `${pressVal} hPa`;
        if (indPressTag) indPressTag.textContent = "缓慢上升";
        if (indPressDesc) indPressDesc.textContent = "在过去 3 小时内缓慢上升。预计在接下来的 3 小时内将下降。";
        if (indPressIndicator) {
            const minP = 980, maxP = 1050;
            const pct = Math.min(95, Math.max(5, ((pressVal - minP) / (maxP - minP)) * 100));
            indPressIndicator.style.left = `${pct}%`;
        }

        // 11. 太阳轨迹 (Sun Arc)
        const sunriseStr = (cur.sunrise || "07:05").slice(0, 5);
        const sunsetStr = (cur.sunset || "16:55").slice(0, 5);
        const indSunTimes = document.getElementById('ind-sun-times');
        const indSunDaylight = document.getElementById('ind-sun-daylight');
        const indSunOrb = document.getElementById('ind-sun-orb');
        if (indSunTimes) indSunTimes.textContent = `日出 ${sunriseStr} · 日落 ${sunsetStr}`;
        if (indSunDaylight) indSunDaylight.textContent = "白昼 9小时50分钟";
        if (indSunOrb) {
            const now = new Date();
            const nowM = now.getHours() * 60 + now.getMinutes();
            const parseM = (s) => {
                const p = s.split(':');
                return parseInt(p[0], 10) * 60 + parseInt(p[1], 10);
            };
            const srM = parseM(sunriseStr);
            const ssM = parseM(sunsetStr);
            let progress = 0.5;
            if (nowM >= srM && nowM <= ssM) {
                progress = (nowM - srM) / (ssM - srM);
            } else if (nowM < srM) {
                progress = 0.05;
            } else {
                progress = 0.95;
            }
            indSunOrb.style.left = `${Math.min(95, Math.max(5, progress * 100))}%`;
        }

        // 12. 月相 (Moon Phase)
        const moonPhaseName = cur.moonPhase || "盈凸月";
        const indMoonVal = document.getElementById('ind-moon-val');
        const indMoonTag = document.getElementById('ind-moon-tag');
        const indMoonDesc = document.getElementById('ind-moon-desc');
        if (indMoonVal) indMoonVal.textContent = moonPhaseName;
        if (indMoonTag) indMoonTag.textContent = moonPhaseName;
        if (indMoonDesc) indMoonDesc.textContent = "照明度 82% · 距满月还有 3 天";
    }

    renderDailyGrid(dailyList) {
        const container = document.getElementById('overview-daily-grid');
        if (!container || !dailyList) return;
        container.innerHTML = "";
        dailyList.forEach(d => {
            const card = document.createElement('div');
            card.className = "ov-daily-card";
            card.innerHTML = `
                <div class="ov-daily-title">${d.title}</div>
                <div class="ov-daily-icon"><img src="/assets/weather/icons/${d.iconCode || '100'}.svg" alt="icon"></div>
                <div class="ov-daily-temps">
                    <span class="high">${d.high}°</span>
                    <span class="low">${d.low}°</span>
                </div>
                <div class="ov-daily-cond">${d.condition}</div>
            `;
            container.appendChild(card);
        });
    }

    // =========================================================================
    // Unified Data Sync & Binding (Weather & Companion Character)
    // =========================================================================

    syncWeatherData(msnData) {
        if (!msnData) return;
        this.weatherData = msnData;

        const cur = msnData.current || {};
        const daily = msnData.daily || [];
        const hourly = msnData.hourly || [];
        const cityName = msnData.cityName || "安阳市";

        const tempStr = `${cur.temperature || 15}°C`;
        const condStr = cur.condition || "小雨";
        const iconSrc = `/assets/weather/icons/${cur.iconCode || '305'}.svg`;
        const feelsStr = `体感 ${cur.feels || (cur.temperature - 1)}°C`;
        const aqiStr = `空气 ${cur.aqi || 50} 优`;
        const humStr = `相对湿度 ${cur.humidity || 52}%`;
        const windStr = cur.wind || "西南风 2级 (6 km/h)";

        const todayDaily = daily.length > 0 ? daily[0] : null;
        const lowT = todayDaily ? todayDaily.low : (cur.temperature - 2);
        const highT = todayDaily ? todayDaily.high : (cur.temperature + 4);
        const rangeStr = `今日 ${lowT}° ~ ${highT}°C`;

        // --- 1. State A Overview Deck ---
        if (this.ovWeatherIcon) this.ovWeatherIcon.src = iconSrc;
        if (this.ovWeatherTemp) this.ovWeatherTemp.textContent = tempStr;
        if (this.ovWeatherCond) this.ovWeatherCond.textContent = `${condStr} · ${cityName}`;
        if (this.ovWeatherRange) this.ovWeatherRange.textContent = rangeStr;
        if (this.ovWeatherFeels) this.ovWeatherFeels.textContent = `${cur.feels !== undefined ? cur.feels : (cur.temperature - 1)}°C`;
        if (this.ovWeatherAqi) this.ovWeatherAqi.textContent = `${cur.aqi || 50} 优`;
        if (this.ovWeatherHum) this.ovWeatherHum.textContent = `${cur.humidity || 52}%`;
        if (this.ovWeatherWind) this.ovWeatherWind.textContent = windStr;

        // --- 2. State B Carousel Card 1 (Weather) ---
        if (this.cCardCity) this.cCardCity.textContent = `${cityName} · 今日气象`;
        if (this.cWeatherIcon) this.cWeatherIcon.src = iconSrc;
        if (this.cWeatherTemp) this.cWeatherTemp.textContent = tempStr;
        if (this.cWeatherCond) this.cWeatherCond.textContent = condStr;
        if (this.cWeatherRange) this.cWeatherRange.textContent = `${lowT}° ~ ${highT}°C`;
        if (this.cWeatherFeels) this.cWeatherFeels.textContent = feelsStr;
        if (this.cWeatherWind) this.cWeatherWind.textContent = windStr;
        if (this.cWeatherAqi) this.cWeatherAqi.textContent = `${cur.aqi || 50} 优`;

        // --- 3. State B Carousel Card 3 (Trend Pill Row) ---
        if (this.cMiniHourlyRow && hourly.length > 0) {
            this.cMiniHourlyRow.innerHTML = "";
            hourly.slice(0, 6).forEach(h => {
                const pill = document.createElement('div');
                pill.className = "c-hourly-pill";
                pill.innerHTML = `
                    <span class="p-hour">${h.hour}</span>
                    <img src="/assets/weather/icons/${h.iconCode || '100'}.svg" class="p-icon" alt="icon">
                    <span class="p-temp">${h.temperature}°</span>
                `;
                this.cMiniHourlyRow.appendChild(pill);
            });
        }
        if (this.cTrendTip) {
            this.cTrendTip.textContent = (condStr.includes('雨') || condStr.includes('雪'))
                ? "未来几小时仍有降水，出行请备好雨具并注意防滑。"
                : "未来几小时天气晴好，气温适宜，适合开窗通风与出行。";
        }

        // --- 4. Render Overview Chart ---
        if (!this.overviewChart && window.HourlyWeatherChart) {
            this.overviewChart = new HourlyWeatherChart('overview-hourly-canvas');
        }
        if (this.overviewChart) {
            this.overviewChart.setData(hourly, {
                sunrise: cur.sunrise || "07:05",
                sunset: cur.sunset || "16:55"
            });
            this.overviewChart.render();
        }

        // --- 5. Render MSN 12-Grid Indicators & 7-Day Forecast ---
        this.render12Grid(msnData);
        this.renderDailyGrid(daily);
    }

    syncCharacterTip(tip) {
        if (!tip) return;
        this.characterTip = tip;

        const badgeText = `${tip.band || 'MyGO!!!!!'} · ${tip.name || '高松灯'}`;
        const quoteText = `“${tip.quote || ''}”`;

        // 1. State A Overview Character Banner (Unique in State A)
        if (this.overviewCharBadge) this.overviewCharBadge.textContent = badgeText;
        if (this.overviewCharQuote) this.overviewCharQuote.textContent = quoteText;

        // 2. State B Carousel Card 2 (Quote Capsule)
        if (this.cQuoteBadge) this.cQuoteBadge.textContent = badgeText;
        if (this.cQuoteText) this.cQuoteText.textContent = tip.quote || '';
    }
}

window.DeskStateManager = DeskStateManager;
