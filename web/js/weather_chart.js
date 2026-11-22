/**
 * Hourly Weather Temperature Curve Chart (Canvas-based)
 * Renders interactive smooth Bezier curve of 24-48 hours weather data
 * with temperature nodes, weather icons, precipitation bars, sunrise/sunset golden rays,
 * and multi-metric switching (概览/降水/风速/湿度/云量 + 实际/体感温度切换 + 图表/列表模式).
 * Styled after MSN Weather hourly forecast view.
 */

class HourlyWeatherChart {
    constructor(canvasId) {
        this.canvasId = canvasId;
        this.canvas = document.getElementById(canvasId);
        this.ctx = this.canvas ? this.canvas.getContext('2d') : null;
        this.hourlyData = [];
        this.meta = {
            sunrise: "07:05",
            sunset: "16:55"
        };
        this.metricType = 'overview'; // 'overview', 'precipitation', 'wind', 'humidity', 'cloud'
        this.showFeelsLike = false;
        this.hoverIndex = -1;
        this.nodeWidth = 68; // pixels per hour column
        this.height = 200;
        this.width = 0;
        this.dpr = window.devicePixelRatio || 1;
        this.iconCache = {};

        if (this.canvas) {
            this.initEvents();
        }
    }

    initEvents() {
        if (!this.canvas) return;

        this.canvas.addEventListener('mousemove', (e) => {
            const rect = this.canvas.getBoundingClientRect();
            const mouseX = e.clientX - rect.left;
            const idx = Math.floor(mouseX / this.nodeWidth);
            if (idx >= 0 && idx < this.hourlyData.length) {
                if (this.hoverIndex !== idx) {
                    this.hoverIndex = idx;
                    this.render();
                    if (this.onHoverCallback) {
                        this.onHoverCallback(this.hourlyData[idx], idx);
                    }
                }
            } else {
                if (this.hoverIndex !== -1) {
                    this.hoverIndex = -1;
                    this.render();
                }
            }
        });

        this.canvas.addEventListener('mouseleave', () => {
            this.hoverIndex = -1;
            this.render();
        });

        window.addEventListener('resize', () => {
            this.resize();
            this.render();
        });
    }

    setData(hourlyList, meta = {}, onHover = null) {
        this.canvas = document.getElementById(this.canvasId);
        if (this.canvas && !this.ctx) {
            this.ctx = this.canvas.getContext('2d');
            this.initEvents();
        }

        this.hourlyData = (hourlyList && hourlyList.length > 0) ? hourlyList : this.generateDefaultHourly();
        this.meta = Object.assign({ sunrise: "07:05", sunset: "16:55" }, meta);
        this.onHoverCallback = onHover;

        this.preloadIcons();
        this.resize();
        this.render();
    }

    preloadIcons() {
        this.hourlyData.forEach(d => {
            const code = d.iconCode || "100";
            if (!this.iconCache[code]) {
                const img = new Image();
                img.src = `/assets/weather/icons/${code}.svg`;
                img.onload = () => {
                    this.iconCache[code] = img;
                    this.render();
                };
            }
        });
    }

    setMetricType(type) {
        this.metricType = type || 'overview';
        this.render();
    }

    setShowFeelsLike(enabled) {
        this.showFeelsLike = Boolean(enabled);
        this.render();
    }

    generateDefaultHourly() {
        const list = [];
        const baseTemp = 14;
        const now = new Date();
        const startHour = now.getHours();

        for (let i = 0; i < 24; i++) {
            const h = (startHour + i) % 24;
            const hourStr = `${h.toString().padStart(2, '0')}:00`;
            const tempOffset = Math.round(Math.sin((i - 6) / 24 * Math.PI * 2) * 4);
            const t = baseTemp + tempOffset;
            list.push({
                hour: hourStr,
                timeStr: hourStr,
                date: "今日",
                temperature: t,
                feels: t - 1,
                condition: i < 6 ? "小雨" : (i < 14 ? "阴" : "多云"),
                iconCode: i < 6 ? "305" : (i < 14 ? "104" : "101"),
                precipitation: i < 6 ? 65 : (i < 10 ? 30 : 0),
                humidity: 70 - tempOffset * 4,
                windDir: "北风",
                windSpeed: "4",
                airPressure: "1025",
                visibility: 15,
                uv: (h >= 10 && h <= 16) ? 3 : 0,
                aqi: 45
            });
        }
        return list;
    }

    resize() {
        if (!this.canvas) return;
        const parentW = this.canvas.parentElement ? this.canvas.parentElement.clientWidth : 0;
        const count = Math.max(1, this.hourlyData.length);
        const minW = Math.max(900, count * 68);
        const totalW = Math.max(parentW, minW);
        this.nodeWidth = totalW / count;
        this.dpr = window.devicePixelRatio || 1;
        this.width = totalW;
        this.height = 165;

        this.canvas.width = totalW * this.dpr;
        this.canvas.height = this.height * this.dpr;
        this.canvas.style.width = `${totalW}px`;
        this.canvas.style.height = `${this.height}px`;

        if (this.ctx) {
            this.ctx.setTransform(this.dpr, 0, 0, this.dpr, 0, 0);
        }
    }

    render() {
        if (!this.ctx || this.hourlyData.length === 0) return;

        const ctx = this.ctx;
        const w = this.width;
        const h = this.height;
        ctx.clearRect(0, 0, w, h);

        const data = this.hourlyData;

        // Determine values based on metricType
        let values = [];
        let unit = "°";
        let mainColor = '#00e5ff';
        let areaGradientStart = 'rgba(0, 229, 255, 0.35)';
        let areaGradientEnd = 'rgba(0, 229, 255, 0)';

        if (this.metricType === 'precipitation') {
            values = data.map(d => parseFloat(d.precipitation) || 0);
            unit = "%";
            mainColor = '#3b82f6';
            areaGradientStart = 'rgba(59, 130, 246, 0.4)';
            areaGradientEnd = 'rgba(59, 130, 246, 0.02)';
        } else if (this.metricType === 'wind') {
            values = data.map(d => parseFloat(d.windSpeed) || 5);
            unit = " km/h";
            mainColor = '#10b981';
            areaGradientStart = 'rgba(16, 185, 129, 0.35)';
            areaGradientEnd = 'rgba(16, 185, 129, 0.02)';
        } else if (this.metricType === 'humidity') {
            values = data.map(d => parseFloat(d.humidity) || 50);
            unit = "%";
            mainColor = '#06b6d4';
            areaGradientStart = 'rgba(6, 182, 212, 0.35)';
            areaGradientEnd = 'rgba(6, 182, 212, 0.02)';
        } else if (this.metricType === 'cloud') {
            values = data.map(d => {
                const c = (d.condition || "").toLowerCase();
                if (c.includes("晴")) return 10;
                if (c.includes("多云")) return 45;
                if (c.includes("阴")) return 85;
                if (c.includes("雨")) return 95;
                return 30;
            });
            unit = "%";
            mainColor = '#8b5cf6';
            areaGradientStart = 'rgba(139, 92, 246, 0.35)';
            areaGradientEnd = 'rgba(139, 92, 246, 0.02)';
        } else {
            // Overview: Temperature (actual or feels-like)
            values = data.map(d => {
                const val = this.showFeelsLike ? (d.feels !== undefined ? d.feels : d.temperature) : d.temperature;
                return parseFloat(val) || 0;
            });
            unit = "°";
            mainColor = this.showFeelsLike ? '#f59e0b' : '#00e5ff';
            areaGradientStart = this.showFeelsLike ? 'rgba(245, 158, 11, 0.35)' : 'rgba(0, 229, 255, 0.35)';
            areaGradientEnd = this.showFeelsLike ? 'rgba(245, 158, 11, 0.02)' : 'rgba(0, 229, 255, 0)';
        }

        const minVal = Math.min(...values);
        const maxVal = Math.max(...values);
        const valPadding = (maxVal === minVal) ? 4 : Math.max(2, (maxVal - minVal) * 0.25);
        const yMin = minVal - valPadding;
        const yMax = maxVal + valPadding;
        const valRange = Math.max(1, yMax - yMin);

        const chartTop = 40;
        const chartHeight = 70;
        const bottomLineY = h - 35;

        const getX = (i) => i * this.nodeWidth + this.nodeWidth / 2;
        const getY = (val) => chartTop + chartHeight - ((val - yMin) / valRange) * chartHeight;

        // 1. Draw Subtle Glass Horizontal Grid Lines
        ctx.strokeStyle = 'rgba(255, 255, 255, 0.12)';
        ctx.lineWidth = 1;
        ctx.setLineDash([4, 6]);
        [chartTop, chartTop + chartHeight / 2, chartTop + chartHeight].forEach(yPos => {
            ctx.beginPath();
            ctx.moveTo(0, yPos);
            ctx.lineTo(w, yPos);
            ctx.stroke();
        });
        ctx.setLineDash([]);

        // 2. Draw Sunrise & Sunset Golden Rays
        this.renderSunMarkers(ctx, data, getX, chartTop, chartHeight);

        // 3. Draw Bezier Spline Fill Area
        ctx.beginPath();
        ctx.moveTo(getX(0), getY(values[0]));
        for (let i = 0; i < data.length - 1; i++) {
            const x0 = getX(i);
            const y0 = getY(values[i]);
            const x1 = getX(i + 1);
            const y1 = getY(values[i + 1]);
            const cx = (x0 + x1) / 2;
            ctx.bezierCurveTo(cx, y0, cx, y1, x1, y1);
        }
        ctx.lineTo(getX(data.length - 1), bottomLineY);
        ctx.lineTo(getX(0), bottomLineY);
        ctx.closePath();

        const grad = ctx.createLinearGradient(0, chartTop, 0, bottomLineY);
        grad.addColorStop(0, areaGradientStart);
        grad.addColorStop(1, areaGradientEnd);
        ctx.fillStyle = grad;
        ctx.fill();

        // 4. Draw Bezier Spline Stroke Line
        ctx.beginPath();
        ctx.moveTo(getX(0), getY(values[0]));
        for (let i = 0; i < data.length - 1; i++) {
            const x0 = getX(i);
            const y0 = getY(values[i]);
            const x1 = getX(i + 1);
            const y1 = getY(values[i + 1]);
            const cx = (x0 + x1) / 2;
            ctx.bezierCurveTo(cx, y0, cx, y1, x1, y1);
        }
        ctx.strokeStyle = mainColor;
        ctx.lineWidth = 2.5;
        ctx.shadowColor = mainColor;
        ctx.shadowBlur = 10;
        ctx.stroke();
        ctx.shadowBlur = 0;

        // 5. Draw Optional Secondary Curve for Feels-like Comparison in Overview
        if (this.metricType === 'overview' && !this.showFeelsLike) {
            const feelsValues = data.map(d => parseFloat(d.feels !== undefined ? d.feels : d.temperature) || 0);
            ctx.beginPath();
            ctx.setLineDash([3, 4]);
            ctx.moveTo(getX(0), getY(feelsValues[0]));
            for (let i = 0; i < data.length - 1; i++) {
                const x0 = getX(i);
                const y0 = getY(feelsValues[i]);
                const x1 = getX(i + 1);
                const y1 = getY(feelsValues[i + 1]);
                const cx = (x0 + x1) / 2;
                ctx.bezierCurveTo(cx, y0, cx, y1, x1, y1);
            }
            ctx.strokeStyle = 'rgba(245, 158, 11, 0.65)';
            ctx.lineWidth = 1.6;
            ctx.stroke();
            ctx.setLineDash([]);
        }

        // 6. Draw Nodes, Labels, Weather Icons, and Precipitation Probability Columns
        ctx.textAlign = 'center';
        ctx.textBaseline = 'middle';

        for (let i = 0; i < data.length; i++) {
            const x = getX(i);
            const y = getY(values[i]);
            const isHover = (i === this.hoverIndex);
            const item = data[i];

            // Weather Icon above the line (改为纯白高亮 + 柔和深色阴影)
            const code = item.iconCode || "100";
            const iconImg = this.iconCache[code];
            const iconY = chartTop - 24;
            ctx.save();
            ctx.filter = 'brightness(0) invert(1) drop-shadow(0 2px 4px rgba(0, 0, 0, 0.75))';
            if (iconImg && iconImg.complete && iconImg.naturalWidth > 0) {
                try {
                    ctx.drawImage(iconImg, x - 12, iconY - 12, 24, 24);
                } catch (e) {
                    this.renderIconFallback(ctx, item.condition, x, iconY);
                }
            } else {
                this.renderIconFallback(ctx, item.condition, x, iconY);
            }
            ctx.restore();

            // Value label above node (纯白高透字色 + 深色文字阴影)
            ctx.save();
            ctx.shadowColor = 'rgba(0, 0, 0, 0.9)';
            ctx.shadowBlur = 5;
            ctx.shadowOffsetY = 1;
            ctx.fillStyle = isHover ? '#38bdf8' : (this.showFeelsLike ? '#fbbf24' : '#F0F9FF');
            ctx.font = isHover ? 'bold 13px "HarmonyOS Sans"' : 'bold 12px "HarmonyOS Sans"';
            ctx.fillText(`${values[i]}${unit}`, x, y - 12);
            ctx.restore();

            // Node Dot
            ctx.beginPath();
            ctx.arc(x, y, isHover ? 5.5 : 3.5, 0, Math.PI * 2);
            ctx.fillStyle = isHover ? '#38bdf8' : '#FFFFFF';
            ctx.shadowColor = 'rgba(0, 0, 0, 0.8)';
            ctx.shadowBlur = 4;
            ctx.fill();
            ctx.strokeStyle = '#38bdf8';
            ctx.lineWidth = 1.5;
            ctx.stroke();

            // Precipitation probability column bar at bottom
            const precip = parseInt(item.precipitation) || 0;
            if (precip > 0 && this.metricType === 'overview') {
                const maxBarH = 22;
                const barH = Math.max(4, (precip / 100) * maxBarH);
                const barY = bottomLineY - barH;

                ctx.fillStyle = 'rgba(56, 189, 248, 0.55)';
                ctx.beginPath();
                ctx.roundRect ? ctx.roundRect(x - 8, barY, 16, barH, [3, 3, 0, 0]) : ctx.rect(x - 8, barY, 16, barH);
                ctx.fill();

                ctx.save();
                ctx.shadowColor = 'rgba(0,0,0,0.85)';
                ctx.shadowBlur = 3;
                ctx.fillStyle = '#7dd3fc';
                ctx.font = '9px "HarmonyOS Sans"';
                ctx.fillText(`${precip}%`, x, barY - 6);
                ctx.restore();
            }

            // Time label at bottom (浅冰蓝 + 阴影)
            ctx.save();
            ctx.shadowColor = 'rgba(0, 0, 0, 0.85)';
            ctx.shadowBlur = 4;
            ctx.fillStyle = isHover ? '#38bdf8' : '#BAE6FD';
            ctx.font = isHover ? 'bold 12px "HarmonyOS Sans"' : '11px "HarmonyOS Sans"';
            ctx.fillText(item.hour || `${i}:00`, x, h - 16);
            ctx.restore();
        }

        // 7. Hover Guide Line & Info Box
        if (this.hoverIndex >= 0 && this.hoverIndex < data.length) {
            const hx = getX(this.hoverIndex);
            ctx.beginPath();
            ctx.setLineDash([4, 4]);
            ctx.moveTo(hx, 15);
            ctx.lineTo(hx, bottomLineY);
            ctx.strokeStyle = 'rgba(37, 99, 235, 0.6)';
            ctx.lineWidth = 1.5;
            ctx.stroke();
            ctx.setLineDash([]);
        }
    }

    renderIconFallback(ctx, condition, x, y) {
        ctx.save();
        ctx.shadowColor = 'rgba(0, 0, 0, 0.85)';
        ctx.shadowBlur = 4;
        ctx.fillStyle = '#F0F9FF';
        ctx.font = '11px "HarmonyOS Sans"';
        const cond = (condition || "").toLowerCase();
        let sym = "晴";
        if (cond.includes("雨")) sym = "雨";
        else if (cond.includes("雪")) sym = "雪";
        else if (cond.includes("阴")) sym = "阴";
        else if (cond.includes("多云")) sym = "云";
        ctx.fillText(sym, x, y);
        ctx.restore();
    }

    renderSunMarkers(ctx, data, getX, chartTop, chartHeight) {
        const sunriseStr = this.meta.sunrise || "07:05";
        const sunsetStr = this.meta.sunset || "16:55";

        const parseMinutes = (timeStr) => {
            if (!timeStr) return null;
            const parts = timeStr.includes('T') ? timeStr.split('T')[1].split(':') : timeStr.split(':');
            return parseInt(parts[0], 10) * 60 + parseInt(parts[1], 10);
        };

        const sunriseMins = parseMinutes(sunriseStr);
        const sunsetMins = parseMinutes(sunsetStr);

        data.forEach((d, idx) => {
            const hourMins = parseMinutes(d.hour || d.timeStr);
            if (hourMins === null) return;

            // Check if sunrise occurs in this hour segment
            if (sunriseMins !== null && Math.abs(hourMins - sunriseMins) < 30) {
                const sx = getX(idx) + ((sunriseMins - hourMins) / 60) * this.nodeWidth;
                this.drawSunMarker(ctx, sx, chartTop, chartHeight, "日出", sunriseStr, "#f59e0b");
            }

            // Check if sunset occurs in this hour segment
            if (sunsetMins !== null && Math.abs(hourMins - sunsetMins) < 30) {
                const sx = getX(idx) + ((sunsetMins - hourMins) / 60) * this.nodeWidth;
                this.drawSunMarker(ctx, sx, chartTop, chartHeight, "日落", sunsetStr, "#e11d48");
            }
        });
    }

    drawSunMarker(ctx, x, top, height, label, timeStr, color) {
        // Vertical radiant line
        ctx.save();
        ctx.beginPath();
        ctx.setLineDash([2, 4]);
        ctx.moveTo(x, top - 10);
        ctx.lineTo(x, top + height + 25);
        ctx.strokeStyle = color;
        ctx.lineWidth = 1.5;
        ctx.stroke();

        // Glowing sun disc badge
        ctx.beginPath();
        ctx.arc(x, top - 12, 6, 0, Math.PI * 2);
        ctx.fillStyle = color;
        ctx.shadowColor = color;
        ctx.shadowBlur = 8;
        ctx.fill();
        ctx.shadowBlur = 0;

        ctx.font = '10px "HarmonyOS Sans"';
        ctx.fillStyle = color;
        ctx.textAlign = 'center';
        ctx.fillText(`${label} ${timeStr.slice(0, 5)}`, x, top - 26);
        ctx.restore();
    }

    renderList(containerEl) {
        if (!containerEl || !this.hourlyData) return;

        let html = `
            <table class="hourly-list-table">
                <thead>
                    <tr>
                        <th>时间</th>
                        <th>天气</th>
                        <th>实际气温</th>
                        <th>体感气温</th>
                        <th>降水概率</th>
                        <th>相对湿度</th>
                        <th>风向风速</th>
                        <th>紫外线</th>
                        <th>空气质量</th>
                    </tr>
                </thead>
                <tbody>
        `;

        this.hourlyData.forEach(item => {
            const code = item.iconCode || "100";
            const precip = parseInt(item.precipitation) || 0;
            const aqi = item.aqi || 30;
            const aqiClass = aqi <= 50 ? "aqi-good" : (aqi <= 100 ? "aqi-moderate" : "aqi-bad");

            html += `
                <tr>
                    <td class="col-time"><strong>${item.hour}</strong></td>
                    <td class="col-cond">
                        <img src="/assets/weather/icons/${code}.svg" class="list-weather-icon" alt="icon">
                        <span>${item.condition}</span>
                    </td>
                    <td class="col-temp">${item.temperature}°C</td>
                    <td class="col-feels">${item.feels !== undefined ? item.feels : item.temperature}°C</td>
                    <td class="col-precip">${precip}%</td>
                    <td class="col-hum">${item.humidity}%</td>
                    <td class="col-wind">${item.windDir} ${item.windSpeed} km/h</td>
                    <td class="col-uv">${item.uv || 0}</td>
                    <td class="col-aqi"><span class="badge ${aqiClass}">${aqi}</span></td>
                </tr>
            `;
        });

        html += `
                </tbody>
            </table>
        `;

        containerEl.innerHTML = html;
    }
}

if (typeof window !== 'undefined') {
    window.HourlyWeatherChart = HourlyWeatherChart;
}
