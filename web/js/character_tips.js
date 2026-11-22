/**
 * Anime Girls Band Weather Companion Voice & Character Tips Manager
 * Manages 24 characters across 5 Anime Bands:
 *   - MyGO!!!!!: 高松灯, 千早爱音, 要乐奈, 长崎爽世, 椎名立希
 *   - Ave Mujica: 丰川祥子, 若叶睦, 八幡海铃, 祐天寺若麦, 三角初华
 *   - 结束乐队 (孤独摇滚): 后藤一里, 伊地知虹夏, 山田凉, 喜多郁代
 *   - 放学后TEA TIME (轻音少女): 平泽唯, 秋山澪, 田井中律, 琴吹紬, 中野梓
 *   - Togenashi Togeari (少女乐队的呐喊): 井芹仁菜, 河原木桃香, 安和昴, 海老冢智, 鲁帕
 * 
 * Clean Architecture, Zero Emojis, Complete Dynamic Weather Binding.
 */

class CharacterTipsManager {
    constructor() {
        this.meta = null;
        this.cache = new Map(); // path -> character data
        this.currentBandId = 'mygo';
        this.currentCharacterId = 'tomori';
        this.currentCondition = '晴';
        this.currentTemp = 20;
        this.currentWind = '微风';
        this.currentQuote = '';
        this.initialized = false;
        this.subscribers = [];

        window.addEventListener('themeChanged', (e) => {
            const themeId = e.detail?.themeId || 'default';
            this.handleThemeChange(themeId);
        });
    }

    async init() {
        if (this.initialized) return;
        try {
            const resp = await fetch('/assets/weather/character_tips/characters_meta.json');
            this.meta = await resp.json();
            
            // Read saved preference or default
            const savedChar = localStorage.getItem('ays2z_companion_char');
            if (savedChar) {
                this.currentCharacterId = savedChar;
            } else {
                this.currentCharacterId = 'tomori';
            }

            // Sync with current body data-theme if present
            const activeTheme = document.body.getAttribute('data-theme') || 'default';
            this.handleThemeChange(activeTheme, false);

            await this.loadCharacter(this.currentCharacterId);
            this.initialized = true;
            this.notify();
        } catch (e) {
            console.warn('[CharacterTipsManager] Failed to load characters meta:', e);
        }
    }

    handleThemeChange(themeId, autoNotify = true) {
        if (!this.meta) return;

        let targetBand = 'mygo';
        if (themeId === 'mujica' || themeId === 'ave-mujica') {
            targetBand = 'mujica';
        } else if (themeId === 'bocchi') {
            targetBand = 'bocchi';
        } else if (themeId === 'kon' || themeId === 'k-on') {
            targetBand = 'kon';
        } else if (themeId === 'gbc' || themeId === 'togenashi') {
            targetBand = 'gbc';
        } else {
            // default theme: keep current or pick mygo
            targetBand = this.currentBandId || 'mygo';
        }

        this.currentBandId = targetBand;
        const bandMeta = this.meta.bands.find(b => b.id === targetBand);
        if (bandMeta) {
            // Check if current character belongs to this band
            const hasChar = bandMeta.characters.some(c => c.id === this.currentCharacterId);
            if (!hasChar) {
                this.currentCharacterId = bandMeta.defaultCharacterId || bandMeta.characters[0].id;
            }
        }

        this.loadCharacter(this.currentCharacterId).then(() => {
            if (autoNotify) this.notify();
        });
    }

    async loadCharacter(charId) {
        if (!this.meta) return null;

        let charMeta = null;
        let bandMeta = null;

        for (const b of this.meta.bands) {
            const found = b.characters.find(c => c.id === charId);
            if (found) {
                charMeta = found;
                bandMeta = b;
                break;
            }
        }

        if (!charMeta) return null;

        const filePath = `/assets/weather/character_tips/${charMeta.file}`;
        if (this.cache.has(filePath)) {
            return this.cache.get(filePath);
        }

        try {
            const resp = await fetch(filePath);
            const data = await resp.json();
            this.cache.set(filePath, data);
            return data;
        } catch (e) {
            console.warn('[CharacterTipsManager] Failed to fetch character file:', filePath, e);
            return null;
        }
    }

    getAvailableCharactersInBand(bandId = null) {
        if (!this.meta) return [];
        const target = bandId || this.currentBandId;
        const b = this.meta.bands.find(band => band.id === target);
        return b ? b.characters : [];
    }

    async switchNextCharacter() {
        if (!this.meta) return;
        const chars = this.getAvailableCharactersInBand();
        if (chars.length === 0) return;

        const idx = chars.findIndex(c => c.id === this.currentCharacterId);
        const nextIdx = (idx + 1) % chars.length;
        const nextChar = chars[nextIdx];

        this.currentCharacterId = nextChar.id;
        localStorage.setItem('ays2z_companion_char', this.currentCharacterId);

        await this.loadCharacter(this.currentCharacterId);
        this.notify();
    }

    async setCharacter(charId) {
        this.currentCharacterId = charId;
        localStorage.setItem('ays2z_companion_char', this.currentCharacterId);
        await this.loadCharacter(this.currentCharacterId);
        this.notify();
    }

    updateWeatherConditions(condition, temp, wind = '') {
        this.currentCondition = condition || '晴';
        this.currentTemp = typeof temp === 'number' ? temp : 20;
        this.currentWind = wind || '';
        this.notify();
    }

    mapWeatherCategory(cond, temp) {
        const text = String(cond || '');
        const t = typeof temp === 'number' ? temp : 20;

        if (text.includes('雷') || text.includes('暴风雨')) return 'thunder';
        if (text.includes('雨') || text.includes('阵雨')) return 'rain';
        if (text.includes('雪') || text.includes('冰雹')) return 'snow';
        if (text.includes('雾') || text.includes('霾') || text.includes('沙尘')) return 'fog';
        if (text.includes('大风') || text.includes('狂风') || text.includes('飑')) return 'wind';
        if (t >= 28) return 'hot';
        if (t <= 5) return 'cold';

        const hour = new Date().getHours();
        if (hour >= 21 || hour < 5) return 'night';

        if (text.includes('云') || text.includes('阴')) return 'cloudy';
        return 'sunny';
    }

    getCurrentTip() {
        if (!this.meta) {
            return {
                name: '天气伴侣',
                role: '气象提醒',
                band: '桌面助手',
                quote: '今日天气舒适，出行请注意天气变化。'
            };
        }

        let charMeta = null;
        let bandMeta = null;
        for (const b of this.meta.bands) {
            const found = b.characters.find(c => c.id === this.currentCharacterId);
            if (found) {
                charMeta = found;
                bandMeta = b;
                break;
            }
        }

        if (!charMeta) return null;

        const filePath = `/assets/weather/character_tips/${charMeta.file}`;
        const charData = this.cache.get(filePath);

        if (!charData || !charData.weather_tips) {
            return {
                name: charMeta.name,
                role: charMeta.role,
                band: bandMeta ? bandMeta.name : '',
                quote: '气象数据加载中，请稍候。'
            };
        }

        const category = this.mapWeatherCategory(this.currentCondition, this.currentTemp);
        const quotes = charData.weather_tips[category] || charData.weather_tips['sunny'] || [];
        
        let quote = quotes[0] || '出行请注意天气变化，保持好心情。';
        if (quotes.length > 1) {
            // Pick based on minute or stable pseudo-random
            const seed = Math.floor(Date.now() / 60000);
            quote = quotes[seed % quotes.length];
        }

        this.currentQuote = quote;

        return {
            id: charData.id,
            name: charData.name,
            role: charData.role,
            band: charData.band,
            personality: charData.personality,
            category: category,
            quote: quote
        };
    }

    subscribe(fn) {
        if (typeof fn === 'function') {
            this.subscribers.push(fn);
            // Call immediately if ready
            if (this.initialized) {
                const tip = this.getCurrentTip();
                if (tip) fn(tip);
            }
        }
    }

    notify() {
        const tip = this.getCurrentTip();
        if (!tip) return;
        for (const fn of this.subscribers) {
            try {
                fn(tip);
            } catch (e) {
                console.error('[CharacterTipsManager] Subscriber error:', e);
            }
        }
    }
}

// Global instance
window.CharacterTipsManager = CharacterTipsManager;
window.characterTipsManager = new CharacterTipsManager();
