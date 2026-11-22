/**
 * Girls Band Theme Interactive Controller & Persistence Manager
 * Manages 6 Girls Band Themes:
 *   - default: 默认纯白玻璃拟态
 *   - mygo:    MyGO 迷子青蓝雨夜
 *   - mujica:  Ave Mujica 暗夜黑曜蔷薇
 *   - bocchi:  孤独摇滚 结束乐队粉彩
 *   - kon:     轻音少女 放学后下午茶暖调
 *   - gbc:     少女乐队的呐喊 刺无赤黑霓虹
 * Clean Architecture, Zero Emojis, Complete State Persistence.
 */

const GIRLS_BAND_THEMES = [
    {
        id: 'default',
        name: '默认原版',
        subname: '纯白玻璃拟态',
        desc: '通透清澈的白昼磨砂玻璃，高对比度文字与极简微光。',
        wallpaper: '/assets/themes/default/background.jpg',
        fallbackWallpaper: '/assets/images/anime_rainy_night.jpg',
        particleMode: 'rain',
        particleName: '雨滴粒子',
        colors: ['#FFFFFF', '#E2E8F0', '#0284C7', '#30343B'],
        previewClass: 'preview-default',
        badgeText: 'DEFAULT'
    },
    {
        id: 'mygo',
        name: 'MyGO!!!!!',
        subname: '迷子青蓝雨夜',
        desc: '迷茫中奔向微光的春日影与青苍雨夜，冷调冰蓝与深海苍青。',
        wallpaper: '/assets/themes/mygo/background.jpg',
        fallbackWallpaper: '/assets/images/anime_rainy_night.jpg',
        particleMode: 'rain',
        particleName: '雨滴粒子',
        colors: ['#0C2138', '#17385E', '#38BDF8', '#E0F2FE'],
        previewClass: 'preview-mygo',
        badgeText: 'MyGO!!!!!'
    },
    {
        id: 'mujica',
        name: 'Ave Mujica',
        subname: '暗夜黑曜蔷薇',
        desc: '沉沦于黑曜剧场的假面玩偶，午夜猩红、银月冷辉与漆黑蔷薇。',
        wallpaper: '/assets/themes/mujica/background.jpg',
        fallbackWallpaper: '/assets/themes/mujica/background.jpg',
        particleMode: 'petals',
        particleName: '暗蔷薇羽毛',
        colors: ['#0F081D', '#3D1030', '#E11D48', '#F7EDF5'],
        previewClass: 'preview-mujica',
        badgeText: 'Ave Mujica'
    },
    {
        id: 'bocchi',
        name: '孤独摇滚',
        subname: '结束乐队粉彩',
        desc: '下北泽壁橱吉他手的内向青春，草莓甜粉、熟芒明黄与蓝紫电声。',
        wallpaper: '/assets/themes/bocchi/background.jpg',
        fallbackWallpaper: '/assets/themes/bocchi/background.jpg',
        particleMode: 'stars',
        particleName: '星光粒子',
        colors: ['#FFF1F5', '#FED7AA', '#F43F5E', '#3C2235'],
        previewClass: 'preview-bocchi',
        badgeText: 'BOCCHI'
    },
    {
        id: 'kon',
        name: '轻音少女',
        subname: '放学后下午茶暖调',
        desc: '樱丘高校活动室的午后红茶与草莓蛋糕，暖木淡金与温柔奶油色。',
        wallpaper: '/assets/themes/kon/background.jpg',
        fallbackWallpaper: '/assets/themes/kon/background.jpg',
        particleMode: 'sunny',
        particleName: '柔光暖茶粒子',
        colors: ['#FFFBEB', '#FDE68A', '#D97706', '#422B1D'],
        previewClass: 'preview-kon',
        badgeText: 'K-ON!'
    },
    {
        id: 'gbc',
        name: '少女乐队的呐喊',
        subname: '刺无赤黑霓虹',
        desc: '川崎街头的无刺之刺与怒吼青春，高对比赤红、赛博青与反叛黑曜。',
        wallpaper: '/assets/themes/gbc/background.jpg',
        fallbackWallpaper: '/assets/themes/gbc/background.jpg',
        particleMode: 'neon',
        particleName: '霓虹粒子',
        colors: ['#090E1A', '#1F2937', '#EF4444', '#00E5FF'],
        previewClass: 'preview-gbc',
        badgeText: 'GBC'
    }
];

class ThemeManager {
    constructor(wallpaperManager = null, particleEngine = null) {
        this.wallpaperManager = wallpaperManager;
        this.particleEngine = particleEngine;
        this.themes = GIRLS_BAND_THEMES;
        this.currentThemeId = 'default';
        this.selectedThemeId = 'default';
        this.initialized = false;
    }

    setDependencies(wallpaperManager, particleEngine) {
        if (wallpaperManager) this.wallpaperManager = wallpaperManager;
        if (particleEngine) this.particleEngine = particleEngine;
    }

    getThemes() {
        return this.themes;
    }

    getTheme(id) {
        return this.themes.find(t => t.id === id) || this.themes[0];
    }

    getCurrentTheme() {
        return this.currentThemeId;
    }

    /**
     * Apply specified theme with complete visual and particle linkages
     * @param {string} themeId
     * @param {object} options { save: boolean, notify: boolean }
     */
    async applyTheme(themeId, options = { save: true, notify: true }) {
        const targetTheme = this.getTheme(themeId);
        const resolvedId = targetTheme.id;
        this.currentThemeId = resolvedId;
        this.selectedThemeId = resolvedId;

        // 1. Set DOM attribute for global theme variables & styling
        document.body.setAttribute('data-theme', resolvedId);

        // 2. Switch wallpaper linkage
        const wpPath = targetTheme.wallpaper || targetTheme.fallbackWallpaper;
        if (this.wallpaperManager && typeof this.wallpaperManager.setImageWallpaper === 'function') {
            try {
                this.wallpaperManager.setImageWallpaper(wpPath);
            } catch (err) {
                console.warn('[ThemeManager] WallpaperManager setImageWallpaper error:', err);
            }
        }
        // Direct fallback update to #bg-image
        const bgImg = document.getElementById('bg-image');
        if (bgImg) {
            bgImg.style.backgroundImage = `url('${wpPath}')`;
        }

        // 3. Switch dynamic particle linkage
        if (this.particleEngine && typeof this.particleEngine.setMode === 'function') {
            try {
                this.particleEngine.setMode(targetTheme.particleMode);
                if (typeof this.particleEngine.start === 'function') {
                    this.particleEngine.start();
                }
            } catch (err) {
                console.warn('[ThemeManager] ParticleEngine setMode error:', err);
            }
        }

        // 4. Update UI card active status
        this.updateModalActiveCard(resolvedId);

        // 5. Persistence: localStorage & pywebview.api.save_config
        if (options.save !== false) {
            try {
                localStorage.setItem('ays2z_current_theme', resolvedId);
                localStorage.setItem('ays2z_theme', resolvedId);
            } catch (e) {
                console.warn('[ThemeManager] localStorage save failed:', e);
            }

            if (window.pywebview && window.pywebview.api && typeof window.pywebview.api.save_config === 'function') {
                try {
                    await window.pywebview.api.save_config({ currentTheme: resolvedId });
                } catch (e) {
                    console.warn('[ThemeManager] Backend save_config failed:', e);
                }
            }
        }

        // 6. Broadcast event
        window.dispatchEvent(new CustomEvent('themeChanged', {
            detail: { themeId: resolvedId, theme: targetTheme }
        }));

        // 7. Update quick tile on simplified dock & menu badge
        const dockThemeVal = document.getElementById('dock-status-theme');
        if (dockThemeVal) {
            dockThemeVal.textContent = targetTheme.name;
        }
        const badgeMenuTheme = document.getElementById('badge-menu-theme');
        if (badgeMenuTheme) {
            badgeMenuTheme.textContent = resolvedId === 'default' ? '原版' : (targetTheme.subname || targetTheme.name);
        }

        // 8. Toast feedback
        if (options.notify && typeof window.showToast === 'function') {
            window.showToast(`已切换主题风格: ${targetTheme.name} (${targetTheme.subname})`, 'success');
        }

        console.log(`[ThemeManager] Theme applied: ${resolvedId} (${targetTheme.name})`);
        return targetTheme;
    }

    /**
     * Update active class and button text inside modal
     */
    updateModalActiveCard(activeId) {
        const cards = document.querySelectorAll('.theme-card');
        cards.forEach(card => {
            const cardThemeId = card.getAttribute('data-theme-id') || card.dataset.theme;
            const btn = card.querySelector('.theme-card-btn, .theme-apply-btn');
            const statusTag = card.querySelector('.theme-status-tag');
            const checkIcon = card.querySelector('.theme-check-icon');

            if (cardThemeId === activeId) {
                card.classList.add('active');
                if (btn) btn.textContent = '使用中';
                if (statusTag) statusTag.textContent = '当前使用';
                if (checkIcon) checkIcon.style.display = 'flex';
            } else {
                card.classList.remove('active');
                if (btn) btn.textContent = '应用';
                if (statusTag) {
                    const themeObj = this.getTheme(cardThemeId);
                    statusTag.textContent = themeObj ? themeObj.particleName : '风格';
                }
                if (checkIcon) checkIcon.style.display = 'none';
            }
        });
    }

    /**
     * Retrieve persisted theme setting from backend or localStorage
     */
    async loadSavedTheme() {
        let saved = null;

        // Try backend config first
        if (window.pywebview && window.pywebview.api && typeof window.pywebview.api.get_config === 'function') {
            try {
                const conf = await window.pywebview.api.get_config();
                if (conf && conf.currentTheme) {
                    saved = conf.currentTheme;
                }
            } catch (e) {
                console.warn('[ThemeManager] Could not read theme from pywebview config:', e);
            }
        }

        // Fallback to localStorage
        if (!saved) {
            try {
                saved = localStorage.getItem('ays2z_current_theme') || localStorage.getItem('ays2z_theme');
            } catch (e) {
                console.warn('[ThemeManager] localStorage read error:', e);
            }
        }

        return (saved && this.themes.some(t => t.id === saved)) ? saved : 'default';
    }

    /**
     * Initialize theme manager, load persisted state, bind UI events
     */
    async init() {
        if (this.initialized) return;
        this.initialized = true;

        const savedTheme = await this.loadSavedTheme();
        await this.applyTheme(savedTheme, { save: false, notify: false });
        this.bindEvents();
    }

    /**
     * Bind click and modal interaction events
     */
    bindEvents() {
        const openBtn = document.getElementById('open-theme-btn');
        const modal = document.getElementById('theme-modal');
        const closeBtn = document.getElementById('close-theme-btn');
        const confirmBtn = document.getElementById('apply-theme-confirm-btn');

        // Open modal
        openBtn?.addEventListener('click', () => {
            if (typeof window.openTwoTierMenu === 'function') {
                window.openTwoTierMenu('theme');
            } else {
                this.selectedThemeId = this.currentThemeId;
                this.updateModalActiveCard(this.currentThemeId);
                modal?.classList.remove('hidden');
            }
        });

        // Close modal
        closeBtn?.addEventListener('click', () => {
            modal?.classList.add('hidden');
        });

        // Click outside modal content to close
        modal?.addEventListener('click', (e) => {
            if (e.target === modal) {
                modal.classList.add('hidden');
            }
        });

        // Theme card clicks
        const cards = document.querySelectorAll('.theme-card');
        cards.forEach(card => {
            const themeId = card.getAttribute('data-theme-id') || card.dataset.theme;

            // Direct click on card applies theme immediately
            card.addEventListener('click', (e) => {
                this.selectedThemeId = themeId;
                this.applyTheme(themeId, { save: true, notify: true });
            });

            // Double click applies theme and closes modal
            card.addEventListener('dblclick', (e) => {
                this.selectedThemeId = themeId;
                this.applyTheme(themeId, { save: true, notify: true });
                modal?.classList.add('hidden');
            });

            // Card button click
            const btn = card.querySelector('.theme-card-btn, .theme-apply-btn');
            btn?.addEventListener('click', (e) => {
                e.stopPropagation();
                this.applyTheme(themeId, { save: true, notify: true });
            });
        });

        // Global modal footer confirm / apply button if present
        confirmBtn?.addEventListener('click', () => {
            if (this.selectedThemeId) {
                this.applyTheme(this.selectedThemeId, { save: true, notify: true });
            }
            modal?.classList.add('hidden');
        });

        const cancelBtn = document.getElementById('cancel-theme-btn');
        cancelBtn?.addEventListener('click', () => {
            modal?.classList.add('hidden');
        });

        const footerApplyBtn = document.getElementById('apply-theme-btn');
        footerApplyBtn?.addEventListener('click', () => {
            if (this.selectedThemeId) {
                this.applyTheme(this.selectedThemeId, { save: true, notify: true });
            }
            modal?.classList.add('hidden');
        });

        window.addEventListener('keydown', (e) => {
            if (e.key === 'Escape' && modal && !modal.classList.contains('hidden')) {
                modal.classList.add('hidden');
            }
        });
    }
}

// Export for global availability
window.ThemeManager = ThemeManager;
window.GIRLS_BAND_THEMES = GIRLS_BAND_THEMES;
