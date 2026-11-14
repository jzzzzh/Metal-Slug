// ============================================================================
//  graphics.h 桩 —— EasyX 的最小子集实现
//
//  用途：在非 Windows 平台上对游戏代码做编译期检查，并跑无头测试。
//  关键点：loadimage 会真的去读 PNG 文件头，拿到真实宽高并分配像素缓冲，
//          这样碰撞盒、绘制裁剪、alpha 混合都跑在真实尺寸上，
//          AddressSanitizer 能真实地盯住 EasyXPng.h 里的逐像素访问。
// ============================================================================
#pragma once

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "Windows.h"

// ---- 常量 ----
#define TRANSPARENT 1
#define OPAQUE      2
#define SRCCOPY     0x00CC0020

inline COLORREF RGB(int r, int g, int b)
{
    return (COLORREF)(((DWORD)(r) & 0xFF) | (((DWORD)(g) & 0xFF) << 8) | (((DWORD)(b) & 0xFF) << 16));
}

// Windows COLORREF is 0xBBGGRR; DIB pixels exposed by GetImageBuffer are 0xRRGGBB.
inline DWORD stubDrawingPixel(COLORREF c)
{
    return ((c & 0xff) << 16) | (c & 0xff00) | ((c >> 16) & 0xff);
}

// ---- IMAGE ----
class IMAGE
{
public:
    IMAGE() : m_w(0), m_h(0), m_buf(NULL) {}

    ~IMAGE()
    {
        if (m_buf) free(m_buf);
        m_buf = NULL;
    }

    int  getwidth()  const { return m_w; }
    int  getheight() const { return m_h; }
    DWORD* buffer() const { return m_buf; }

    void alloc(int w, int h)
    {
        if (m_buf) { free(m_buf); m_buf = NULL; }
        m_w = w > 0 ? w : 0;
        m_h = h > 0 ? h : 0;
        if (m_w && m_h)
            m_buf = (DWORD*)calloc((size_t)m_w * (size_t)m_h, sizeof(DWORD));
    }

private:
    IMAGE(const IMAGE&);
    IMAGE& operator=(const IMAGE&);

    int    m_w;
    int    m_h;
    DWORD* m_buf;
};

// ---- 全局状态 ----
extern bool  g_stubKeyState[256];
extern int   g_stubScreenW;
extern int   g_stubScreenH;
extern bool  g_stubMouseHit;
extern int   g_stubMouseX;
extern int   g_stubMouseY;

static DWORD* g_stubFrame = NULL;
static int    g_stubFbW = 0;
static int    g_stubFbH = 0;
static COLORREF g_stubFill = RGB(255, 255, 255);
static COLORREF g_stubLine = RGB(255, 255, 255);
static COLORREF g_stubText = RGB(255, 255, 255);

// ---- 画布 ----
inline void initgraph(int w, int h)
{
    g_stubWindowOpen = true;
    g_stubScreenW = w > 0 ? w : 1920;
    g_stubScreenH = h > 0 ? h : 1080;
    g_stubFbW = g_stubScreenW;
    g_stubFbH = g_stubScreenH;
    if (g_stubFrame) free(g_stubFrame);
    g_stubFrame = (DWORD*)calloc((size_t)g_stubFbW * (size_t)g_stubFbH, sizeof(DWORD));
}

inline void closegraph()
{
    g_stubWindowOpen = false;
    if (g_stubFrame) { free(g_stubFrame); g_stubFrame = NULL; }
}

inline HWND GetHWnd() { return g_stubWindowOpen ? (HWND)&g_stubWindowOpen : NULL; }

inline void BeginBatchDraw() {}
inline void FlushBatchDraw() {}
inline void EndBatchDraw() {}
inline void cleardevice() { if (g_stubFrame) memset(g_stubFrame, 0, (size_t)g_stubFbW * g_stubFbH * sizeof(DWORD)); }

inline int getwidth()  { return g_stubFbW; }
inline int getheight() { return g_stubFbH; }

inline DWORD* GetImageBuffer()             { return g_stubFrame; }
inline DWORD* GetImageBuffer(IMAGE* img)   { return img ? img->buffer() : NULL; }

// ---- PNG 头解析 + 伪随机像素填充 ----
static inline void stubToNarrow(const TCHAR* src, char* dst, size_t cap)
{
    size_t i = 0;
    for (; src && src[i] && i + 1 < cap; ++i)
    {
#ifdef UNICODE
        dst[i] = (src[i] < 128) ? (char)src[i] : '_';
#else
        dst[i] = src[i];
#endif
    }
    dst[i] = 0;
}

// 读 PNG 文件头拿真实宽高。
// 返回 false 表示文件不存在 —— 此时 w/h 保持 0，
// 与真实 EasyX 加载失败得到 0x0 空图的行为一致（游戏的素材缺失校验依赖这一点）。
static inline bool stubPngSize(const TCHAR* path, int& w, int& h)
{
    w = 0; h = 0;
    char narrow[512];
    stubToNarrow(path, narrow, sizeof(narrow));
    FILE* f = fopen(narrow, "rb");
    if (!f) return false;
    unsigned char sig[8], len[4], type[4], data[8];
    if (fread(sig, 1, 8, f) == 8 &&
        fread(len, 1, 4, f) == 4 &&
        fread(type, 1, 4, f) == 4 &&
        fread(data, 1, 8, f) == 8)
    {
        w = (int)(((DWORD)data[0] << 24) | ((DWORD)data[1] << 16) | ((DWORD)data[2] << 8) | data[3]);
        h = (int)(((DWORD)data[4] << 24) | ((DWORD)data[5] << 16) | ((DWORD)data[6] << 8) | data[7]);
    }
    fclose(f);
    return true;   // 文件存在（尺寸可能仍是 0，例如 JPG 头不是 PNG）
}

static inline DWORD stubPixel(int x, int y, unsigned seed)
{
    unsigned v = ((unsigned)x * 73856093u) ^ ((unsigned)y * 19349663u) ^ (seed * 83492791u);
    DWORD r = v & 0xFF, g = (v >> 8) & 0xFF, b = (v >> 16) & 0xFF, a = (v >> 24) & 0xFF;
    if ((v & 7) == 0) a = 255;      // 造出全不透明像素
    if ((v & 15) == 1) a = 0;       // 造出全透明像素
    return (a << 24) | (r << 16) | (g << 8) | b;
}

// 扁平化路径键，与 tools/convert_textures.py 保持一致
static inline void stubFlatKey(const TCHAR* path, char* out, size_t cap)
{
    char narrow[512];
    stubToNarrow(path, narrow, sizeof(narrow));
    const char* k = narrow;
    while (k[0] == '.' && k[1] == '/') k += 2;
    size_t i = 0;
    for (; k[i] && i + 1 < cap; ++i)
        out[i] = (k[i] == '/') ? '_' : k[i];
    out[i] = 0;
}

// 优先读 tools/.texcache 里的真实素材（离线目视检查用），否则回退到随机噪声
inline void loadimage(IMAGE* img, LPCTSTR path, int width = 0, int height = 0, bool resize = false)
{
    (void)resize;
    if (!img) return;

    char key[512];
    stubFlatKey(path, key, sizeof(key));
    char cache[640];
    snprintf(cache, sizeof(cache), "tools/.texcache/%s.raw", key);

    FILE* f = fopen(cache, "rb");
    if (f)
    {
        // 宽高按大端读取（与 convert_textures.py 的 ">ii" 一致）
        unsigned char hdr[8];
        int w = 0, h = 0;
        if (fread(hdr, 1, 8, f) == 8)
        {
            w = (int)(((DWORD)hdr[0] << 24) | ((DWORD)hdr[1] << 16) | ((DWORD)hdr[2] << 8) | hdr[3]);
            h = (int)(((DWORD)hdr[4] << 24) | ((DWORD)hdr[5] << 16) | ((DWORD)hdr[6] << 8) | hdr[7]);
        }
        if (w > 0 && h > 0)
        {
            img->alloc(w, h);
            DWORD* buf = img->buffer();
            if (buf)
            {
                unsigned char* raw = (unsigned char*)malloc((size_t)w * h * 4);
                if (raw && fread(raw, 4, (size_t)w * h, f) == (size_t)w * h)
                {
                    if (width > 0 && height > 0 && (width != w || height != h))
                    {
                        // 按请求尺寸做最近邻缩放（对应 EasyX 的 bResize 行为）
                        img->alloc(width, height);
                        DWORD* dst = img->buffer();
                        if (dst)
                        {
                            for (int y = 0; y < height; ++y)
                            {
                                const int sy = (int)((long long)y * h / height);
                                for (int x = 0; x < width; ++x)
                                {
                                    const int sx = (int)((long long)x * w / width);
                                    const size_t si = ((size_t)sy * w + sx) * 4;
                                    const size_t di = ((size_t)y * width + x);
                                    dst[di] = (DWORD)((DWORD)raw[si + 3] << 24) |
                                                     (DWORD)((DWORD)raw[si + 0] << 16) |
                                                     (DWORD)((DWORD)raw[si + 1] << 8) |
                                                      (DWORD)raw[si + 2];
                                }
                            }
                        }
                    }
                    else
                    {
                        for (size_t i = 0; i < (size_t)w * h; ++i)
                        {
                            const DWORD a = raw[i * 4 + 3];
                            const DWORD r = raw[i * 4 + 0];
                            const DWORD g = raw[i * 4 + 1];
                            const DWORD b = raw[i * 4 + 2];
                            buf[i] = (a << 24) | (r << 16) | (g << 8) | b;
                        }
                    }
                }
                if (raw) free(raw);
            }
        }
        fclose(f);
        return;
    }

    int nw = 0, nh = 0;
    const bool found = stubPngSize(path, nw, nh);
    if (!found)
    {
        // 素材缺失：和真实 EasyX 一样给出 0x0 的空图（getwidth() == 0），
        // 这样游戏里的"素材缺失校验"分支才能被真正走到。
        img->alloc(0, 0);
        return;
    }
    if (width > 0 && height > 0) { nw = width; nh = height; }
    if (nw <= 0) nw = 64;
    if (nh <= 0) nh = 64;

    img->alloc(nw, nh);
    DWORD* buf = img->buffer();
    if (!buf) return;
    unsigned seed = (unsigned)(reinterpret_cast<size_t>(path) & 0xFFFF);
    for (int y = 0; y < nh; ++y)
        for (int x = 0; x < nw; ++x)
            buf[(size_t)y * nw + x] = stubPixel(x, y, seed + (unsigned)(x * 7 + y * 13));
}

inline void putimage(int dstX, int dstY, IMAGE* src, DWORD dwRop = SRCCOPY)
{
    (void)dwRop;
    if (!src || !g_stubFrame) return;
    const int sw = src->getwidth(), sh = src->getheight();
    const DWORD* sb = src->buffer();
    if (!sb) return;
    for (int y = 0; y < sh; ++y)
    {
        const int dy = dstY + y;
        if (dy < 0 || dy >= g_stubFbH) continue;
        for (int x = 0; x < sw; ++x)
        {
            const int dx = dstX + x;
            if (dx < 0 || dx >= g_stubFbW) continue;
            g_stubFrame[(size_t)dy * g_stubFbW + dx] = sb[(size_t)y * sw + x];
        }
    }
}

// ---- 绘图 ----
inline void setfillcolor(COLORREF c) { g_stubFill = c; }
inline void setlinecolor(COLORREF c) { g_stubLine = c; }
inline void settextcolor(COLORREF c) { g_stubText = c; }
inline void setbkmode(int) {}
inline void settextstyle(int, int, LPCTSTR) {}

inline void fillrectangle(int l, int t, int r, int b)
{
    if (!g_stubFrame) return;
    if (l > r) { const int tmp = l; l = r; r = tmp; }
    if (t > b) { const int tmp = t; t = b; b = tmp; }
    for (int y = t; y <= b; ++y)
    {
        if (y < 0 || y >= g_stubFbH) continue;
        for (int x = l; x <= r; ++x)
        {
            if (x < 0 || x >= g_stubFbW) continue;
            g_stubFrame[(size_t)y * g_stubFbW + x] = stubDrawingPixel(g_stubFill);
        }
    }
}

inline void line(int x1, int y1, int x2, int y2)
{
    const int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    const int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int error = dx + dy;
    for (;;)
    {
        if (g_stubFrame && x1 >= 0 && x1 < g_stubFbW && y1 >= 0 && y1 < g_stubFbH)
            g_stubFrame[(size_t)y1 * g_stubFbW + x1] = stubDrawingPixel(g_stubLine);
        if (x1 == x2 && y1 == y2) break;
        const int twice = 2 * error;
        if (twice >= dy) { error += dy; x1 += sx; }
        if (twice <= dx) { error += dx; y1 += sy; }
    }
}

inline void outtextxy(int x, int y, LPCTSTR str)
{
    (void)x; (void)y; (void)str;              // 桩里不画字
}

// ---- 鼠标 ----
struct MOUSEMSG
{
    int  x, y, wheel, message;
    bool ctrl, shift, lbutton, mbutton, rbutton;
};

inline bool MouseHit() { return g_stubMouseHit; }

inline MOUSEMSG GetMouseMsg()
{
    MOUSEMSG m;
    memset(&m, 0, sizeof(m));
    m.x = g_stubMouseX;
    m.y = g_stubMouseY;
    m.message = 0x0200;                        // WM_MOUSEMOVE
    m.lbutton = g_stubKeyState[VK_LBUTTON];
    g_stubMouseHit = false;
    return m;
}
