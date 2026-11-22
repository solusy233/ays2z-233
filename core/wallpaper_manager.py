"""
High-Performance Modular Wallpaper Manager (C-Level Acceleration)
Only manages Image (.jpg, .jpeg, .png, .webp, .bmp, .gif) and Video (.mp4, .webm, .mkv, .avi)
formats for weather background switching and user custom media.
"""

import os
import json
import ctypes
from typing import Dict, List, Any

BASE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
PICTURES_DIR = os.path.join(BASE_DIR, "assets", "weather", "pictures")
IMAGES_DIR = os.path.join(BASE_DIR, "assets", "images")
NATIVE_DLL_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "native_core.dll")

# Load Native C DLL for ultra-fast C-level media scanning
_native_dll = None
if os.path.isfile(NATIVE_DLL_PATH):
    try:
        _native_dll = ctypes.CDLL(NATIVE_DLL_PATH)
    except Exception as e:
        print(f"[WallpaperManager] Native DLL load warning: {e}")

CATEGORY_NAMES = {
    "sunny": "晴天",
    "rainy": "雨天",
    "snow": "雪天",
    "fog": "雾天",
    "video": "视频壁纸",
    "custom": "自定义"
}

IMAGE_EXTENSIONS = ('.jpg', '.jpeg', '.png', '.webp', '.bmp', '.gif')
VIDEO_EXTENSIONS = ('.mp4', '.webm', '.mkv', '.avi')
MEDIA_EXTENSIONS = IMAGE_EXTENSIONS + VIDEO_EXTENSIONS


def _fast_scan_dir(dir_path: str, recursive: bool = True) -> List[Dict[str, Any]]:
    """Scan directory for image and video files using C native core if available."""
    if not os.path.isdir(dir_path):
        return []

    # 1. C-Level Native Scanning
    if _native_dll and hasattr(_native_dll, 'fast_scan_media'):
        try:
            buf_size = 1024 * 1024  # 1MB buffer for fast JSON
            buf = ctypes.create_unicode_buffer(buf_size)
            count = _native_dll.fast_scan_media(dir_path, buf, buf_size, 1 if recursive else 0)
            if count > 0 and buf.value:
                return json.loads(buf.value)
        except Exception as e:
            print(f"[WallpaperManager] Native scan fallback: {e}")

    # 2. Optimized Python fallback
    results = []
    walker = os.walk(dir_path) if recursive else [(dir_path, [], os.listdir(dir_path))]
    for root, _, files in walker:
        for f in files:
            lower = f.lower()
            is_vid = lower.endswith(VIDEO_EXTENSIONS)
            if is_vid or lower.endswith(IMAGE_EXTENSIONS):
                full = os.path.join(root, f).replace("\\", "/")
                rel = os.path.relpath(full, BASE_DIR).replace("\\", "/")
                results.append({
                    "name": f,
                    "path": full,
                    "rel": rel,
                    "type": "video" if is_vid else "image",
                    "size": os.path.getsize(full) if os.path.exists(full) else 0
                })
    return results


class WallpaperManagerService:
    @staticmethod
    def get_categorized_wallpapers(custom_paths=None) -> Dict[str, List[Dict[str, Any]]]:
        """
        Scan and return all wallpapers grouped strictly by weather (sunny, rainy, snow, fog)
        and media types (video, custom). Zero bloated web/html/scene parsing.
        """
        result = {
            "all": [],
            "sunny": [],
            "rainy": [],
            "snow": [],
            "fog": [],
            "video": [],
            "custom": []
        }

        # 1. Rainy category (Default anime rainy night + stock pictures/videos)
        anime_rainy_path = os.path.join(IMAGES_DIR, "anime_rainy_night.jpg")
        if os.path.isfile(anime_rainy_path):
            item = {
                "id": "rainy-anime",
                "name": "雨夜月色动漫",
                "path": "/assets/images/anime_rainy_night.jpg",
                "localPath": anime_rainy_path.replace("\\", "/"),
                "type": "image",
                "category": "rainy",
                "categoryName": "雨天",
                "thumb": "/assets/images/anime_rainy_night.jpg",
                "isDefault": True,
                "desc": "雨夜车场与动漫少女，磨砂玻璃拟态专属背景"
            }
            result["rainy"].append(item)
            result["all"].append(item)

        rainy_dir = os.path.join(PICTURES_DIR, "rainy")
        for idx, media in enumerate(_fast_scan_dir(rainy_dir, recursive=False), 1):
            is_vid = media["type"] == "video"
            item = {
                "id": f"rainy-stock-{idx}",
                "name": f"雨景{'视频' if is_vid else '摄影'} 0{idx}",
                "path": f"/assets/weather/pictures/rainy/{media['name']}",
                "localPath": media["path"],
                "type": media["type"],
                "category": "rainy",
                "categoryName": "雨天",
                "thumb": f"/assets/weather/pictures/rainy/{media['name']}" if not is_vid else "/assets/images/settings.png",
                "desc": f"实景降水天气{'视频' if is_vid else '图片'}背景"
            }
            result["rainy"].append(item)
            result["all"].append(item)
            if is_vid:
                result["video"].append(item)

        # 2. Sunny category
        sunny_dir = os.path.join(PICTURES_DIR, "sunny")
        for idx, media in enumerate(_fast_scan_dir(sunny_dir, recursive=False), 1):
            is_vid = media["type"] == "video"
            item = {
                "id": f"sunny-{idx}",
                "name": f"晴朗日光{'视频' if is_vid else '摄影'} 0{idx}",
                "path": f"/assets/weather/pictures/sunny/{media['name']}",
                "localPath": media["path"],
                "type": media["type"],
                "category": "sunny",
                "categoryName": "晴天",
                "thumb": f"/assets/weather/pictures/sunny/{media['name']}" if not is_vid else "/assets/images/settings.png",
                "desc": f"阳光明媚晴空{'视频' if is_vid else '图片'}背景"
            }
            result["sunny"].append(item)
            result["all"].append(item)
            if is_vid:
                result["video"].append(item)

        # 3. Snow category
        snow_dir = os.path.join(PICTURES_DIR, "snow")
        for idx, media in enumerate(_fast_scan_dir(snow_dir, recursive=False), 1):
            is_vid = media["type"] == "video"
            item = {
                "id": f"snow-{idx}",
                "name": f"冬日飘雪{'视频' if is_vid else '摄影'} 0{idx}",
                "path": f"/assets/weather/pictures/snow/{media['name']}",
                "localPath": media["path"],
                "type": media["type"],
                "category": "snow",
                "categoryName": "雪天",
                "thumb": f"/assets/weather/pictures/snow/{media['name']}" if not is_vid else "/assets/images/settings.png",
                "desc": f"洁白雪景静谧{'视频' if is_vid else '图片'}壁纸"
            }
            result["snow"].append(item)
            result["all"].append(item)
            if is_vid:
                result["video"].append(item)

        # 4. Fog category
        fog_dir = os.path.join(PICTURES_DIR, "fog")
        for idx, media in enumerate(_fast_scan_dir(fog_dir, recursive=False), 1):
            is_vid = media["type"] == "video"
            item = {
                "id": f"fog-{idx}",
                "name": f"晨雾微澜{'视频' if is_vid else '摄影'} 0{idx}",
                "path": f"/assets/weather/pictures/fog/{media['name']}",
                "localPath": media["path"],
                "type": media["type"],
                "category": "fog",
                "categoryName": "雾天",
                "thumb": f"/assets/weather/pictures/fog/{media['name']}" if not is_vid else "/assets/images/settings.png",
                "desc": f"低能见度大雾气象{'视频' if is_vid else '图片'}"
            }
            result["fog"].append(item)
            result["all"].append(item)
            if is_vid:
                result["video"].append(item)

        # 5. User Custom Paths (Fast recursive scan for images/videos)
        if custom_paths:
            if isinstance(custom_paths, str):
                custom_paths = [custom_paths]
            for c_path in custom_paths:
                if os.path.isdir(c_path):
                    for idx, media in enumerate(_fast_scan_dir(c_path, recursive=True), 1):
                        is_vid = media["type"] == "video"
                        item = {
                            "id": f"custom-{idx}",
                            "name": media["name"],
                            "path": f"/stream?path={media['path']}",
                            "localPath": media["path"],
                            "type": media["type"],
                            "category": "custom",
                            "categoryName": "自定义",
                            "thumb": f"/stream?path={media['path']}" if not is_vid else "/assets/images/settings.png",
                            "desc": f"本地自定义{'视频' if is_vid else '图片'}壁纸"
                        }
                        result["custom"].append(item)
                        result["all"].append(item)
                        if is_vid:
                            result["video"].append(item)

        return result
