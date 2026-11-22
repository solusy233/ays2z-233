"""
Weather Service Module
Fetches real-time weather with zero-configuration fallback.
100% keyless meteorological telemetry powered by:
  1. MSN Weather Real-time & Hourly Telemetry Crawler (Primary)
  2. Open-Meteo Free Real-time Meteorological API (Secondary Fallback)
Provides condition mapping to standard SVG icon codes, temperature history comparison,
and background picture selection.
"""

from datetime import datetime, date
import json
import os
import random
import urllib.parse
import urllib.request

from .msn_weather_crawler import MSNWeatherCrawler

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
WEATHER_DIR = os.path.join(BASE_DIR, "assets", "weather")
WEATHER_JSON_PATH = os.path.join(WEATHER_DIR, "weather.json")
WEATHER_TIPS_PATH = os.path.join(WEATHER_DIR, "weathertips.json")

# Coordinates lookup for common Chinese cities (default: Anyang 安阳)
CITY_COORDINATES = {
    "101180201": (36.10, 114.39, "安阳"),
    "安阳": (36.10, 114.39, "安阳"),
    "101010100": (39.90, 116.40, "北京"),
    "北京": (39.90, 116.40, "北京"),
    "101020100": (31.23, 121.47, "上海"),
    "上海": (31.23, 121.47, "上海"),
    "101180101": (34.75, 113.66, "郑州"),
    "郑州": (34.75, 113.66, "郑州"),
    "101280101": (23.13, 113.26, "广州"),
    "广州": (23.13, 113.26, "广州"),
    "101280601": (22.54, 114.06, "深圳"),
    "深圳": (22.54, 114.06, "深圳"),
    "101270101": (30.57, 104.07, "成都"),
    "成都": (30.57, 104.07, "成都"),
    "101210101": (30.27, 120.15, "杭州"),
    "杭州": (30.27, 120.15, "杭州"),
    "101110101": (34.34, 108.94, "西安"),
    "西安": (34.34, 108.94, "西安"),
    "101200101": (30.59, 114.31, "武汉"),
    "武汉": (30.59, 114.31, "武汉"),
    "101040100": (29.56, 106.55, "重庆"),
    "重庆": (29.56, 106.55, "重庆"),
    "101090101": (38.04, 114.51, "石家庄"),
    "石家庄": (38.04, 114.51, "石家庄"),
    "101180301": (35.30, 113.88, "新乡"),
    "新乡": (35.30, 113.88, "新乡"),
    "101180401": (34.80, 114.35, "开封"),
    "开封": (34.80, 114.35, "开封"),
    "101180901": (34.62, 112.45, "洛阳"),
    "洛阳": (34.62, 112.45, "洛阳"),
}

WMO_CODE_MAP = {
    0: ("100", "晴", "sunny"),
    1: ("101", "多云", "sunny"),
    2: ("102", "少云", "sunny"),
    3: ("104", "阴", "sunny"),
    45: ("501", "雾", "fog"),
    48: ("501", "雾凇", "fog"),
    51: ("305", "毛毛雨", "rainy"),
    53: ("305", "小雨", "rainy"),
    55: ("306", "中雨", "rainy"),
    56: ("305", "冻毛毛雨", "rainy"),
    57: ("306", "重冻雨", "rainy"),
    61: ("305", "小雨", "rainy"),
    63: ("306", "中雨", "rainy"),
    65: ("307", "大雨", "rainy"),
    66: ("305", "冻雨", "rainy"),
    67: ("307", "大暴雨", "rainy"),
    71: ("400", "小雪", "snow"),
    73: ("401", "中雪", "snow"),
    75: ("402", "大雪", "snow"),
    77: ("403", "雪粒", "snow"),
    80: ("300", "阵雨", "rainy"),
    81: ("301", "大阵雨", "rainy"),
    82: ("302", "暴阵雨", "rainy"),
    85: ("407", "阵雪", "snow"),
    86: ("407", "强阵雪", "snow"),
    95: ("302", "雷阵雨", "rainy"),
    96: ("304", "雷雨冰雹", "rainy"),
    99: ("304", "强雷阵雨", "rainy")
}

def wind_direction_from_degrees(deg):
    directions = ["北风", "东北风", "东风", "东南风", "南风", "西南风", "西风", "西北风"]
    try:
        idx = round(float(deg) / 45.0) % 8
        return directions[idx]
    except Exception:
        return "微风"

def wind_scale_from_kmh(kmh):
    try:
        ms = float(kmh) / 3.6
        if ms < 0.3: return "0"
        if ms < 1.6: return "1"
        if ms < 3.4: return "2"
        if ms < 5.5: return "3"
        if ms < 8.0: return "4"
        if ms < 10.8: return "5"
        if ms < 13.9: return "6"
        return "7+"
    except Exception:
        return "1-3"

def get_random_weather_picture(condition_category):
    """Pick a matching background photo from assets/weather/pictures/<category>/."""
    pic_dir = os.path.join(WEATHER_DIR, "pictures", condition_category)
    if os.path.isdir(pic_dir):
        files = [f for f in os.listdir(pic_dir) if f.lower().endswith(('.png', '.jpg', '.jpeg'))]
        if files:
            chosen = random.choice(files)
            return f"/assets/weather/pictures/{condition_category}/{chosen}"
    # Fallback to sunny
    return "/assets/weather/pictures/sunny/sunny-1.png"

class WeatherService:
    @staticmethod
    def resolve_city_coords(location_str):
        if not location_str:
            return 36.10, 114.39, "安阳"
        loc = str(location_str).strip()
        if loc in CITY_COORDINATES:
            return CITY_COORDINATES[loc]

        # Geocode city name via Open-Meteo
        try:
            encoded = urllib.parse.quote(loc)
            url = f"https://geocoding-api.open-meteo.com/v1/search?name={encoded}&count=1&language=zh&format=json"
            req = urllib.request.Request(url, headers={"User-Agent": "ays2z-weather/2.0"})
            with urllib.request.urlopen(req, timeout=4) as resp:
                res = json.loads(resp.read().decode("utf-8"))
                if res.get("results") and len(res["results"]) > 0:
                    r = res["results"][0]
                    return float(r["latitude"]), float(r["longitude"]), r.get("name", loc)
        except Exception:
            pass

        # Default fallback to Anyang
        return 36.10, 114.39, "安阳"

    @classmethod
    def fetch_live_weather(cls, location_id="101180201", **kwargs):
        """
        Unified keyless weather fetcher:
        1. Seamlessly uses MSN Weather Crawler for rich real-time & hourly meteorological telemetry.
        2. Fallback to Open-Meteo free live API.
        3. Zero API Key required.
        """
        lat, lon, city_name = cls.resolve_city_coords(location_id)

        # Primary rich meteorological source: MSN Weather Crawler
        try:
            msn_data = MSNWeatherCrawler.crawl_city_weather(city_name)
            if msn_data and msn_data.get("current"):
                cur = msn_data["current"]
                c_icon = int(cur.get("iconCode", 100)) if str(cur.get("iconCode", 100)).isdigit() else 100
                category = "sunny"
                if 300 <= c_icon < 400: category = "rainy"
                elif 400 <= c_icon < 500: category = "snow"
                elif 500 <= c_icon < 600: category = "fog"
                
                cur["category"] = category
                cur["temp"] = cur.get("temperature", cur.get("temp", 0))
                cur["feelsLike"] = cur.get("feels", cur.get("feelsLike", -3))
                cur["text"] = cur.get("condition", "晴朗")
                cur["icon"] = cur.get("iconCode", "100")
                
                msn_data["code"] = "200"
                msn_data["isLive"] = not msn_data.get("isFallback", False)
                msn_data["backgroundPicture"] = get_random_weather_picture(category)
                cls._save_cached_weather(msn_data)
                return msn_data
        except Exception as e:
            print(f"[WeatherService] MSN Crawler failed: {e}")

        # Fallback to Open-Meteo free live API
        try:
            live_data = cls._fetch_open_meteo(location_id)
            if live_data:
                cls._save_cached_weather(live_data)
                return live_data
        except Exception as e:
            print(f"[WeatherService] Open-Meteo request failed: {e}")

        # Guaranteed fallback dataset
        fallback = MSNWeatherCrawler.get_fallback_data(city_name)
        fallback["code"] = "200"
        fallback["isLive"] = False
        fallback["backgroundPicture"] = get_random_weather_picture("sunny")
        return fallback

    @classmethod
    def _fetch_open_meteo(cls, location_id):
        lat, lon, city_name = cls.resolve_city_coords(location_id)
        url = (
            f"https://api.open-meteo.com/v1/forecast?"
            f"latitude={lat}&longitude={lon}&"
            f"current=temperature_2m,relative_humidity_2m,apparent_temperature,weather_code,wind_speed_10m,wind_direction_10m&"
            f"daily=weather_code,temperature_2m_max,temperature_2m_min,wind_speed_10m_max,wind_direction_10m_dominant&"
            f"timezone=Asia%2FShanghai"
        )

        req = urllib.request.Request(url, headers={"User-Agent": "ays2z-233-client/2.0"})
        with urllib.request.urlopen(req, timeout=6) as resp:
            raw = json.loads(resp.read().decode("utf-8"))

        current = raw.get("current", {})
        daily = raw.get("daily", {})

        cur_code = int(current.get("weather_code", 0))
        icon_code, cond_text, category = WMO_CODE_MAP.get(cur_code, ("100", "晴", "sunny"))

        cur_temp = round(current.get("temperature_2m", 15), 1)
        feels_like = round(current.get("apparent_temperature", cur_temp), 1)
        humidity = current.get("relative_humidity_2m", 50)
        wind_speed_kmh = round(current.get("wind_speed_10m", 10), 1)
        wind_dir = wind_direction_from_degrees(current.get("wind_direction_10m", 0))
        wind_scale = wind_scale_from_kmh(wind_speed_kmh)

        # Build daily list
        dates = daily.get("time", [datetime.now().strftime("%Y-%m-%d")])
        max_temps = daily.get("temperature_2m_max", [cur_temp])
        min_temps = daily.get("temperature_2m_min", [cur_temp])
        daily_codes = daily.get("weather_code", [cur_code])

        daily_list = []
        for i in range(len(dates)):
            d_code = int(daily_codes[i]) if i < len(daily_codes) else cur_code
            d_icon, d_text, _ = WMO_CODE_MAP.get(d_code, ("100", "晴", "sunny"))
            daily_list.append({
                "fxDate": dates[i],
                "tempMax": str(round(max_temps[i])),
                "tempMin": str(round(min_temps[i])),
                "textDay": d_text,
                "textNight": d_text,
                "iconDay": d_icon,
                "iconNight": d_icon,
                "windDirDay": wind_dir,
                "windScaleDay": wind_scale,
                "windSpeedDay": str(round(wind_speed_kmh)),
                "humidity": str(humidity)
            })

        bg_pic = get_random_weather_picture(category)
        now_iso = datetime.now().strftime("%Y-%m-%dT%H:%M+08:00")

        return {
            "code": "200",
            "cityName": city_name,
            "source": "Open-Meteo 实时气象",
            "updateTime": now_iso,
            "current": {
                "temp": cur_temp,
                "feelsLike": feels_like,
                "text": cond_text,
                "icon": icon_code,
                "humidity": humidity,
                "windDir": wind_dir,
                "windScale": wind_scale,
                "windSpeed": wind_speed_kmh,
                "category": category
            },
            "daily": daily_list,
            "backgroundPicture": bg_pic,
            "isLive": True
        }

    @classmethod
    def _save_cached_weather(cls, data):
        try:
            os.makedirs(WEATHER_DIR, exist_ok=True)
            with open(WEATHER_JSON_PATH, "w", encoding="utf-8") as f:
                json.dump(data, f, ensure_ascii=False, indent=2)
        except Exception as e:
            print(f"[WeatherService] Failed to cache weather.json: {e}")

    @classmethod
    def _read_cached_weather(cls):
        if os.path.isfile(WEATHER_JSON_PATH):
            try:
                with open(WEATHER_JSON_PATH, "r", encoding="utf-8") as f:
                    data = json.load(f)
                    data["isLive"] = False
                    return data
            except Exception:
                pass
        return None
