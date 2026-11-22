/**
 * ays2z-233 Desktop Assistant - High Performance Native C Core
 * Compiled with GCC for C-level execution speed.
 * 
 * Features:
 *  1. Ultra-fast recursive media scanner (Win32 FindFirstFileW/FindNextFileW)
 *     for Image (.jpg, .jpeg, .png, .webp, .bmp, .gif) and Video (.mp4, .webm, .mkv, .avi).
 *  2. Native Windows Acrylic / DWM BlurBehind window composition.
 *  3. Native WorkerW desktop wallpaper pinning (Lively Wallpaper architecture in pure C).
 *  4. Fast MurmurHash3 for weather and memory caching.
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dwmapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>

#define EXPORT __declspec(dllexport)

/* =========================================================================
   1. Fast MurmurHash3 (32-bit)
   ========================================================================= */

static inline uint32_t rotl32(uint32_t x, int8_t r) {
    return (x << r) | (x >> (32 - r));
}

EXPORT uint32_t fast_murmur3_32(const uint8_t* key, size_t len, uint32_t seed) {
    uint32_t h = seed;
    const size_t nblocks = len / 4;
    const uint32_t* blocks = (const uint32_t*)key;

    for (size_t i = 0; i < nblocks; i++) {
        uint32_t k = blocks[i];
        k *= 0xcc9e2d51;
        k = rotl32(k, 15);
        k *= 0x1b873593;

        h ^= k;
        h = rotl32(h, 13);
        h = h * 5 + 0xe6546b64;
    }

    const uint8_t* tail = (const uint8_t*)(key + nblocks * 4);
    uint32_t k1 = 0;

    switch (len & 3) {
        case 3: k1 ^= tail[2] << 16; /* fallthrough */
        case 2: k1 ^= tail[1] << 8;  /* fallthrough */
        case 1: k1 ^= tail[0];
                k1 *= 0xcc9e2d51;
                k1 = rotl32(k1, 15);
                k1 *= 0x1b873593;
                h ^= k1;
    }

    h ^= (uint32_t)len;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;

    return h;
}

/* =========================================================================
   2. Ultra-Fast Media File Scanner (Win32 FindFirstFileW)
      Only scans Video and Image formats as specified by user.
   ========================================================================= */

static int is_media_file(const wchar_t* filename, int* is_video) {
    const wchar_t* dot = wcsrchr(filename, L'.');
    if (!dot) return 0;

    // Check images
    if (_wcsicmp(dot, L".jpg") == 0 ||
        _wcsicmp(dot, L".jpeg") == 0 ||
        _wcsicmp(dot, L".png") == 0 ||
        _wcsicmp(dot, L".webp") == 0 ||
        _wcsicmp(dot, L".bmp") == 0 ||
        _wcsicmp(dot, L".gif") == 0) {
        if (is_video) *is_video = 0;
        return 1;
    }

    // Check videos
    if (_wcsicmp(dot, L".mp4") == 0 ||
        _wcsicmp(dot, L".webm") == 0 ||
        _wcsicmp(dot, L".mkv") == 0 ||
        _wcsicmp(dot, L".avi") == 0) {
        if (is_video) *is_video = 1;
        return 1;
    }

    return 0;
}

static void escape_json_wstr(const wchar_t* src, wchar_t* dest, size_t dest_max) {
    size_t di = 0;
    for (size_t si = 0; src[si] && di + 4 < dest_max; si++) {
        if (src[si] == L'\\') {
            dest[di++] = L'/'; // Normalize Windows path separators for web/JSON
        } else if (src[si] == L'"') {
            dest[di++] = L'\\';
            dest[di++] = L'"';
        } else {
            dest[di++] = src[si];
        }
    }
    dest[di] = L'\0';
}

typedef struct {
    wchar_t* buffer;
    size_t capacity;
    size_t length;
    int count;
} ScanOutput;

static void scan_dir_recursive(const wchar_t* root_dir, const wchar_t* rel_dir, ScanOutput* out, int max_depth) {
    if (max_depth < 0) return;

    wchar_t search_path[MAX_PATH * 2];
    swprintf(search_path, sizeof(search_path)/sizeof(wchar_t), L"%s\\*", root_dir);

    WIN32_FIND_DATAW fd;
    HANDLE hFind = FindFirstFileW(search_path, &fd);
    if (hFind == INVALID_HANDLE_VALUE) return;

    do {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) {
            continue;
        }

        wchar_t full_item_path[MAX_PATH * 2];
        swprintf(full_item_path, sizeof(full_item_path)/sizeof(wchar_t), L"%s\\%s", root_dir, fd.cFileName);

        wchar_t rel_item_path[MAX_PATH * 2];
        if (rel_dir && wcslen(rel_dir) > 0) {
            swprintf(rel_item_path, sizeof(rel_item_path)/sizeof(wchar_t), L"%s/%s", rel_dir, fd.cFileName);
        } else {
            swprintf(rel_item_path, sizeof(rel_item_path)/sizeof(wchar_t), L"%s", fd.cFileName);
        }

        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            // Recurse into subdirectories
            scan_dir_recursive(full_item_path, rel_item_path, out, max_depth - 1);
        } else {
            int is_video = 0;
            if (is_media_file(fd.cFileName, &is_video)) {
                wchar_t clean_full[MAX_PATH * 2];
                wchar_t clean_rel[MAX_PATH * 2];
                escape_json_wstr(full_item_path, clean_full, sizeof(clean_full)/sizeof(wchar_t));
                escape_json_wstr(rel_item_path, clean_rel, sizeof(clean_rel)/sizeof(wchar_t));

                ULARGE_INTEGER fileSize;
                fileSize.LowPart = fd.nFileSizeLow;
                fileSize.HighPart = fd.nFileSizeHigh;

                wchar_t entry[MAX_PATH * 4];
                int entry_len = swprintf(entry, sizeof(entry)/sizeof(wchar_t),
                    L"%ls{\"name\":\"%ls\",\"path\":\"%ls\",\"rel\":\"%ls\",\"type\":\"%ls\",\"size\":%llu}",
                    (out->count > 0 ? L"," : L""),
                    fd.cFileName, clean_full, clean_rel,
                    (is_video ? L"video" : L"image"),
                    fileSize.QuadPart);

                if (out->length + entry_len + 32 < out->capacity) {
                    wcscpy(out->buffer + out->length, entry);
                    out->length += entry_len;
                    out->count++;
                }
            }
        }
    } while (FindNextFileW(hFind, &fd));

    FindClose(hFind);
}

/**
 * Fast C-level media scanner.
 * Output is formatted JSON array string: [{"name":"...","path":"...","type":"image|video","size":...},...]
 * Returns total media files count found.
 */
EXPORT int fast_scan_media(const wchar_t* directory, wchar_t* out_json, size_t out_max_chars, int recursive) {
    if (!directory || !out_json || out_max_chars < 16) return 0;

    ScanOutput out;
    out.buffer = out_json;
    out.capacity = out_max_chars;
    out.length = 0;
    out.count = 0;

    wcscpy(out.buffer, L"[");
    out.length = 1;

    int max_depth = recursive ? 10 : 0;
    scan_dir_recursive(directory, L"", &out, max_depth);

    if (out.length + 2 < out.capacity) {
        wcscpy(out.buffer + out.length, L"]");
        out.length += 1;
    }

    return out.count;
}

/* =========================================================================
   3. Native Windows DWM Acrylic / Glass Blur (SetWindowCompositionAttribute)
   ========================================================================= */

typedef struct {
    int AccentState;
    int AccentFlags;
    int GradientColor;
    int AnimationId;
} ACCENT_POLICY;

typedef struct {
    int Attribute;
    void* Data;
    size_t SizeOfData;
} WINCOMPATTRDATA;

typedef BOOL(WINAPI* pfnSetWindowCompositionAttribute)(HWND, WINCOMPATTRDATA*);

EXPORT int native_enable_blur(HWND hwnd, int accent_state) {
    if (!hwnd || !IsWindow(hwnd)) return 0;

    // 1. DwmExtendFrameIntoClientArea
    MARGINS margins = { -1, -1, -1, -1 };
    DwmExtendFrameIntoClientArea(hwnd, &margins);

    // 2. SetWindowCompositionAttribute
    HMODULE hUser = GetModuleHandleW(L"user32.dll");
    if (!hUser) hUser = LoadLibraryW(L"user32.dll");
    if (!hUser) return 1;

    pfnSetWindowCompositionAttribute setWindowCompAttr =
        (pfnSetWindowCompositionAttribute)GetProcAddress(hUser, "SetWindowCompositionAttribute");

    if (setWindowCompAttr) {
        ACCENT_POLICY policy;
        policy.AccentState = (accent_state > 0 ? accent_state : 4); // 4 = ACCENT_ENABLE_ACRYLICBLURBEHIND
        policy.AccentFlags = 2;
        policy.GradientColor = 0x01FFFFFF; // Ultra-clean transparent acrylic tint
        policy.AnimationId = 0;

        WINCOMPATTRDATA data;
        data.Attribute = 19; // WCA_ACCENT_POLICY
        data.Data = &policy;
        data.SizeOfData = sizeof(policy);

        return setWindowCompAttr(hwnd, &data) ? 1 : 0;
    }

    return 1;
}

/* =========================================================================
   4. Native WorkerW Desktop Attachment (Pure C Implementation)
      Embeds window directly behind desktop icons at hardware speed.
   ========================================================================= */

static HWND g_found_workerw = NULL;

static BOOL CALLBACK EnumWindowsCallback(HWND hwnd, LPARAM lParam) {
    HWND shellView = FindWindowExW(hwnd, NULL, L"SHELLDLL_DefView", NULL);
    if (shellView != NULL) {
        // The WorkerW window immediately behind desktop icons
        g_found_workerw = FindWindowExW(NULL, hwnd, L"WorkerW", NULL);
    }
    return TRUE;
}

EXPORT HWND native_find_workerw(void) {
    HWND progman = FindWindowW(L"Progman", NULL);
    if (!progman) return NULL;

    DWORD_PTR result = 0;
    // Trigger WorkerW spawn behind icons
    SendMessageTimeoutW(progman, 0x052C, 0, 0, SMTO_NORMAL, 1000, &result);

    g_found_workerw = NULL;
    EnumWindows(EnumWindowsCallback, 0);

    return g_found_workerw;
}

EXPORT int native_attach_to_desktop(HWND child_hwnd) {
    if (!child_hwnd || !IsWindow(child_hwnd)) return 0;

    HWND workerw = native_find_workerw();
    if (workerw) {
        SetParent(child_hwnd, workerw);
        return 1;
    }
    return 0;
}
