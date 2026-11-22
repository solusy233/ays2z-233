/**
 * Main Application Controller (ays2z-233 Web Edition)
 * Orchestrates modules, user interaction, weather-categorized wallpapers, and system bridges.
 * Clean, modular architecture with zero emojis.
 */

document.addEventListener("DOMContentLoaded", () => {
    // 1. Initialize Subsystems
    const particleEngine = new WeatherParticleEngine('bg-canvas');
    const audioManager = new AudioManager();
    const wallpaperMgr = new WallpaperManager(particleEngine, audioManager);
    const weatherCtrl = new WeatherController(particleEngine, audioManager);
    const themeManager = new ThemeManager(wallpaperMgr, particleEngine);
    window.themeManager = themeManager;
    window.weatherCtrl = weatherCtrl;
    window.audioManager = audioManager;
    window.wallpaperMgr = wallpaperMgr;

    // Link weather controller with wallpaper manager for auto-weather switching
    weatherCtrl.setWallpaperManager(wallpaperMgr);

    // 2. Initialize Desk State Manager (State A Overview & State B Wallpaper Immersion)
    const deskStateManager = new DeskStateManager();
    window.deskStateManager = deskStateManager;

    // Global state
    let appConfig = {
        password: "1",
        startTime: "23:00",
        locationId: "101180201",
        cityName: "安阳",
        wallpaperType: "classic",
        wallpaperPath: "/assets/images/anime_rainy_night.jpg",
        wallpaperFile: "",
        wallpaperAutoSwitch: true,
        autoStart: false,
        volume: 0.8
    };

    let pendingShutdown = false;
    let shutdownCountdownSeconds = 300;

    // 2. Clock & Date Display
    function updateClock() {
        const now = new Date();
        const timeStr = now.toLocaleTimeString('zh-CN', { hour12: false });
        const dateStr = now.toLocaleDateString('zh-CN', {
            year: 'numeric',
            month: 'long',
            day: 'numeric',
            weekday: 'long'
        });

        const clockEl = document.getElementById('digital-clock');
        const dateEl = document.getElementById('digital-date');
        if (clockEl) clockEl.textContent = timeStr;
        if (dateEl) dateEl.textContent = dateStr;

        // Check if shutdown countdown is active
        if (pendingShutdown && shutdownCountdownSeconds > 0) {
            shutdownCountdownSeconds--;
            const mins = Math.floor(shutdownCountdownSeconds / 60);
            const secs = shutdownCountdownSeconds % 60;
            const cdText = `${mins.toString().padStart(2, '0')}:${secs.toString().padStart(2, '0')}`;
            const cdEl = document.getElementById('shutdown-countdown-display');
            if (cdEl) cdEl.textContent = `系统将在 ${cdText} 后关机`;
            const dockCd = document.getElementById('dock-status-shutdown');
            if (dockCd) dockCd.textContent = `关机倒计时 ${cdText}`;
            const badgeCd = document.getElementById('badge-menu-shutdown');
            if (badgeCd) badgeCd.textContent = cdText;
        }
    }
    setInterval(updateClock, 1000);
    updateClock();

    // 3. Toast Notifications (Zero Emojis)
    window.showToast = function (message, type = "info") {
        const container = document.getElementById('toast-container');
        if (!container) return;
        const toast = document.createElement('div');
        toast.className = `toast toast-${type} animate-slide-in`;
        toast.innerHTML = `<span class="toast-dot"></span><span>${message}</span>`;
        container.appendChild(toast);
        setTimeout(() => {
            toast.classList.add('animate-slide-out');
            setTimeout(() => toast.remove(), 400);
        }, 3200);
    };

    // 4. Music Playback State
    window.onMusicStateChanged = function (isPlaying) {
        const textSpan = document.getElementById('music-btn-text');
        const anim = document.getElementById('music-eq-anim');
        if (textSpan) {
            textSpan.textContent = isPlaying ? "暂停" : "播放";
        }
        if (anim) {
            anim.style.display = isPlaying ? "flex" : "none";
        }
        const quickBtn = document.getElementById('quick-music-play-btn');
        if (quickBtn) quickBtn.textContent = isPlaying ? "暂停" : "播放";
        const dockMusic = document.getElementById('dock-status-music');
        if (dockMusic) dockMusic.textContent = isPlaying ? "正在播放" : "点击播放";
    };

    // 5. Daily Schedule Trigger Callback
    window.onDailyScheduleTriggered = function (weekday) {
        showToast(`定时时间到达，正在播放音乐并准备关机`, "info");
        audioManager.playDailyMusic(weekday);
        weatherCtrl.broadcastCurrentWeather();
        startShutdownCountdown(300);
    };

    function startShutdownCountdown(seconds = 300) {
        pendingShutdown = true;
        shutdownCountdownSeconds = seconds;
        const cancelBtn = document.getElementById('cancel-shutdown-btn');
        if (cancelBtn) cancelBtn.classList.remove('hidden');
    }

    function cancelShutdownSchedule() {
        if (window.pywebview && window.pywebview.api) {
            window.pywebview.api.cancel_shutdown();
        }
        pendingShutdown = false;
        shutdownCountdownSeconds = 0;
        const cdEl = document.getElementById('shutdown-countdown-display');
        if (cdEl) cdEl.textContent = "待执行关机已取消";
        const cancelBtn = document.getElementById('cancel-shutdown-btn');
        if (cancelBtn) cancelBtn.classList.add('hidden');
        showToast("已取消待执行关机", "info");
    }

    // 6. Load Settings & Sync with Backend
    async function loadConfig() {
        if (window.pywebview && window.pywebview.api) {
            try {
                const conf = await window.pywebview.api.get_config();
                if (conf) {
                    appConfig = Object.assign(appConfig, conf);
                }
            } catch (e) {
                console.warn("[App] Failed to get config from API:", e);
            }
        }

        // Apply config to UI
        const timeInput = document.getElementById('shutdown-time-input');
        if (timeInput) timeInput.value = appConfig.startTime || "23:00";

        const autoStartCb = document.getElementById('autostart-checkbox');
        if (autoStartCb) autoStartCb.checked = Boolean(appConfig.autoStart);

        const volumeSlider = document.getElementById('volume-slider');
        if (volumeSlider) {
            volumeSlider.value = (appConfig.volume || 0.8) * 100;
            audioManager.setVolume(appConfig.volume || 0.8);
        }

        const autoSwitchToggle = document.getElementById('wp-auto-switch-toggle');
        if (autoSwitchToggle) {
            autoSwitchToggle.checked = appConfig.wallpaperAutoSwitch !== false;
            wallpaperMgr.autoWeatherSwitch = autoSwitchToggle.checked;
        }

        const dockTime = document.getElementById('dock-status-shutdown');
        if (dockTime) dockTime.textContent = `${appConfig.startTime || "23:00"} 待命中`;
        const badgeTime = document.getElementById('badge-menu-shutdown');
        if (badgeTime) badgeTime.textContent = appConfig.startTime || "23:00";
        const dockWp = document.getElementById('dock-status-wallpaper');
        if (dockWp) dockWp.textContent = appConfig.wallpaperAutoSwitch !== false ? "天气联动中" : "固定壁纸";

        // Apply Anime Band Theme via ThemeManager
        const savedTheme = appConfig.currentTheme || appConfig.theme || localStorage.getItem('ays2z_current_theme') || localStorage.getItem('ays2z_theme') || 'default';
        await themeManager.applyTheme(savedTheme, { save: false, notify: false });

        // Load Weather
        const initialCity = appConfig.cityName || (appConfig.locationId && !appConfig.locationId.match(/^\d+$/) ? appConfig.locationId : "安阳市");
        weatherCtrl.fetchAndRender(initialCity);
    }

    // 7. Event Listeners
    // Music controls
    document.getElementById('music-toggle-btn')?.addEventListener('click', () => {
        audioManager.toggleMusic();
    });

    // Volume Slider
    document.getElementById('volume-slider')?.addEventListener('input', (e) => {
        const val = e.target.value / 100;
        audioManager.setVolume(val);
        appConfig.volume = val;
        if (window.pywebview && window.pywebview.api) {
            window.pywebview.api.save_config({ volume: val });
        }
    });

    // Save Shutdown Time
    document.getElementById('save-time-btn')?.addEventListener('click', async () => {
        const timeInput = document.getElementById('shutdown-time-input');
        if (timeInput) {
            const newTime = timeInput.value;
            appConfig.startTime = newTime;
            const dockTime = document.getElementById('dock-status-shutdown');
            if (dockTime) dockTime.textContent = `${newTime} 待命中`;
            const badgeTime = document.getElementById('badge-menu-shutdown');
            if (badgeTime) badgeTime.textContent = newTime;

            if (window.pywebview && window.pywebview.api) {
                await window.pywebview.api.save_config({ startTime: newTime });
            }
            showToast(`已保存每日关机时间: ${newTime}`, 'success');
        }
    });

    // Cancel Shutdown
    document.getElementById('cancel-shutdown-btn')?.addEventListener('click', () => {
        cancelShutdownSchedule();
    });

    // Test Shutdown (5 min)
    document.getElementById('test-shutdown-btn')?.addEventListener('click', async () => {
        if (window.pywebview && window.pywebview.api) {
            await window.pywebview.api.schedule_shutdown(300);
        }
        startShutdownCountdown(300);
        showToast("已启动 5 分钟测试关机倒计时", "warning");
    });

    // Autostart toggle
    document.getElementById('autostart-checkbox')?.addEventListener('change', async (e) => {
        const enabled = e.target.checked;
        appConfig.autoStart = enabled;
        if (window.pywebview && window.pywebview.api) {
            await window.pywebview.api.set_autostart_status(enabled);
        }
        showToast(enabled ? "已开启开机自启" : "已关闭开机自启", "info");
    });

    // 8. Two-Tier Options Center Controller (二级选项菜单中枢)
    const twoTierModal = document.getElementById('two-tier-menu-modal');
    const closeTwoTierBtn = document.getElementById('close-two-tier-menu-btn');
    const openMenuCenterBtn = document.getElementById('open-menu-center-btn');

    function openTwoTierMenu(tabName = 'shutdown') {
        if (!twoTierModal) return;
        switchTwoTierTab(tabName);
        twoTierModal.classList.remove('hidden');
    }

    function closeTwoTierMenu() {
        if (twoTierModal) twoTierModal.classList.add('hidden');
    }

    function switchTwoTierTab(tabName) {
        // Update sidebar items
        document.querySelectorAll('.tier1-nav-item').forEach(btn => {
            btn.classList.toggle('active', btn.dataset.tab === tabName);
        });

        // Update content panes
        document.querySelectorAll('.tier2-tab-pane').forEach(pane => {
            pane.classList.toggle('active', pane.id === `pane-${tabName}`);
        });

        // Refresh tab-specific views
        if (tabName === 'wallpaper') {
            wallpaperMgr.renderCategoryGrid(wallpaperMgr.currentCategory || 'all', 'wallpaper-grid');
        } else if (tabName === 'theme') {
            themeManager.updateModalActiveCard(themeManager.currentThemeId);
        } else if (tabName === 'settings') {
            const locEl = document.getElementById('cfg-location');
            const themeEl = document.getElementById('cfg-theme');
            const pwdEl = document.getElementById('cfg-newpwd');
            if (locEl) locEl.value = appConfig.locationId || "101180201";
            if (themeEl) themeEl.value = appConfig.theme || "default";
            if (pwdEl) pwdEl.value = "";
        }
    }

    window.openTwoTierMenu = openTwoTierMenu;
    window.closeTwoTierMenu = closeTwoTierMenu;

    // Sidebar tab clicks
    document.querySelectorAll('.tier1-nav-item').forEach(btn => {
        btn.addEventListener('click', () => {
            const tab = btn.dataset.tab;
            if (tab === 'settings' && appConfig.password && appConfig.password !== "1") {
                // If password protected, prompt with password modal
                pwdInput.value = "";
                passwordModal.classList.remove('hidden');
                pwdInput.focus();
                window._pendingTierTab = tab;
            } else {
                switchTwoTierTab(tab);
            }
        });
    });

    // Header Menu Button
    openMenuCenterBtn?.addEventListener('click', () => openTwoTierMenu('shutdown'));
    closeTwoTierBtn?.addEventListener('click', closeTwoTierMenu);

    // Quick Status Tiles Clicks on Main Dock
    document.getElementById('quick-tile-shutdown')?.addEventListener('click', () => openTwoTierMenu('shutdown'));
    document.getElementById('quick-tile-wallpaper')?.addEventListener('click', () => openTwoTierMenu('wallpaper'));
    document.getElementById('quick-tile-theme')?.addEventListener('click', () => openTwoTierMenu('theme'));
    document.getElementById('quick-tile-music')?.addEventListener('click', (e) => {
        if (e.target.id === 'quick-music-play-btn') {
            audioManager.toggleMusic();
        } else {
            openTwoTierMenu('shutdown');
        }
    });
    document.getElementById('quick-music-play-btn')?.addEventListener('click', (e) => {
        e.stopPropagation();
        audioManager.toggleMusic();
    });

    // Close when clicking modal backdrop
    twoTierModal?.addEventListener('click', (e) => {
        if (e.target === twoTierModal) closeTwoTierMenu();
    });

    // Easter egg trigger from menu tab
    const easterEggModal = document.getElementById('easter-egg-modal');
    document.getElementById('trigger-fullscreen-egg-btn')?.addEventListener('click', () => {
        closeTwoTierMenu();
        if (easterEggModal) {
            easterEggModal.classList.remove('hidden');
            easterEggModal.focus();
        }
    });

    easterEggModal?.addEventListener('click', () => {
        easterEggModal.classList.add('hidden');
    });

    // Global ESC key listener
    window.addEventListener('keydown', (e) => {
        if (e.key === 'Escape') {
            closeTwoTierMenu();
            easterEggModal?.classList.add('hidden');
            document.getElementById('password-modal')?.classList.add('hidden');
            document.getElementById('fullscreen-weather-modal')?.classList.add('hidden');
        }
    });

    // 9. Password Verification Modal
    const passwordModal = document.getElementById('password-modal');
    const pwdInput = document.getElementById('pwd-input');
    const pwdSubmit = document.getElementById('pwd-submit-btn');
    const pwdCancel = document.getElementById('pwd-cancel-btn');

    pwdSubmit?.addEventListener('click', verifyPassword);
    pwdInput?.addEventListener('keydown', (e) => {
        if (e.key === 'Enter') verifyPassword();
    });

    pwdCancel?.addEventListener('click', () => {
        passwordModal.classList.add('hidden');
        window._pendingTierTab = null;
    });

    function verifyPassword() {
        const entered = pwdInput.value.trim();
        const correct = appConfig.password || "1";
        if (entered === correct) {
            passwordModal.classList.add('hidden');
            if (window._pendingTierTab) {
                switchTwoTierTab(window._pendingTierTab);
                window._pendingTierTab = null;
            } else {
                openTwoTierMenu('settings');
            }
        } else {
            showToast("密码错误", "error");
            pwdInput.select();
        }
    }

    // 10. Settings Form Logic
    function applyTheme(themeName, showToastNotify = false) {
        appConfig.currentTheme = themeName || 'default';
        appConfig.theme = appConfig.currentTheme;
        themeManager.applyTheme(appConfig.currentTheme, { save: true, notify: showToastNotify });

        const themeSelect = document.getElementById('cfg-theme');
        if (themeSelect) themeSelect.value = appConfig.currentTheme;
    }

    document.getElementById('cfg-theme')?.addEventListener('change', (e) => {
        applyTheme(e.target.value);
    });

    document.getElementById('save-settings-btn')?.addEventListener('click', async () => {
        const loc = document.getElementById('cfg-location')?.value.trim() || "101180201";
        const newPwd = document.getElementById('cfg-newpwd')?.value.trim() || "";
        const selectedTheme = document.getElementById('cfg-theme')?.value || "default";

        appConfig.locationId = loc;
        appConfig.theme = selectedTheme;
        applyTheme(selectedTheme);

        if (newPwd) {
            appConfig.password = newPwd;
        }

        if (window.pywebview && window.pywebview.api) {
            await window.pywebview.api.save_config({
                locationId: loc,
                password: appConfig.password,
                theme: selectedTheme
            });
        }

        closeTwoTierMenu();
        showToast("设置已保存", "success");
        weatherCtrl.fetchAndRender(loc);
    });

    // 11. Theme Manager Events Binding
    themeManager.bindEvents();

    // 12. Window Controls (Minimize, Close to Tray)
    document.getElementById('win-min-btn')?.addEventListener('click', () => {
        if (window.pywebview && window.pywebview.api) {
            window.pywebview.api.minimize_window();
        }
    });

    document.getElementById('win-close-btn')?.addEventListener('click', () => {
        if (window.pywebview && window.pywebview.api) {
            window.pywebview.api.hide_to_tray();
            showToast("程序已最小化到托盘", "info");
        }
    });

    // Initialize configuration
    loadConfig();
});
