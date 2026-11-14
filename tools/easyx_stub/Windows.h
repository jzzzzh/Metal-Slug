// ============================================================================
//  Windows.h 桩 —— 仅用于在非 Windows 平台做编译期检查 / 无头测试
//  只实现本项目用到的 API，不代表真实 Windows 行为。
// ============================================================================
#pragma once

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <cstdarg>

typedef unsigned char       BYTE;
typedef unsigned short      WORD;
typedef unsigned int        DWORD;
typedef int                 BOOL;
typedef unsigned int        UINT;
typedef long                LONG;
typedef long long           LONG_PTR;
typedef unsigned long long  UINT_PTR;
typedef void*               HANDLE;
typedef void*               HWND;
static bool g_stubWindowOpen = false;
inline BOOL IsWindow(HWND window) { return window != NULL && g_stubWindowOpen; }
typedef void*               HDC;
typedef void*               HGDIOBJ;
typedef void*               HINSTANCE;
typedef void*               HMODULE;
typedef void*               LPVOID;
typedef int                 INT;
typedef unsigned int        COLORREF;
typedef float               FLOAT;

#ifndef TRUE
#define TRUE  1
#define FALSE 0
#endif

#ifndef MAX_PATH
#define MAX_PATH 260
#endif

#ifdef UNICODE
typedef wchar_t TCHAR;
typedef wchar_t WCHAR;
#define _T(x) L##x
#define LPCTSTR const wchar_t*
#else
typedef char    TCHAR;
typedef char    WCHAR;
#define _T(x) x
#define LPCTSTR const char*
#endif

// ---- 虚拟键码 ----
#define VK_LBUTTON      0x01
#define VK_RBUTTON      0x02
#define VK_BACK         0x08
#define VK_TAB          0x09
#define VK_RETURN       0x0D
#define VK_ESCAPE       0x1B
#define VK_SPACE        0x20
#define VK_PRIOR        0x21
#define VK_NEXT         0x22
#define VK_END          0x23
#define VK_HOME         0x24
#define VK_LEFT         0x25
#define VK_UP           0x26
#define VK_RIGHT        0x27
#define VK_DOWN         0x28
#define VK_0            0x30
#define VK_1            0x31
#define VK_2            0x32
#define VK_3            0x33
#define VK_4            0x34
#define VK_5            0x35
#define VK_6            0x36
#define VK_7            0x37
#define VK_8            0x38
#define VK_9            0x39
#define VK_A            0x41
#define VK_B            0x42
#define VK_C            0x43
#define VK_D            0x44
#define VK_E            0x45
#define VK_F            0x46
#define VK_G            0x47
#define VK_H            0x48
#define VK_I            0x49
#define VK_J            0x4A
#define VK_K            0x4B
#define VK_L            0x4C
#define VK_M            0x4D
#define VK_N            0x4E
#define VK_O            0x4F
#define VK_P            0x50
#define VK_Q            0x51
#define VK_R            0x52
#define VK_S            0x53
#define VK_T            0x54
#define VK_U            0x55
#define VK_V            0x56
#define VK_W            0x57
#define VK_X            0x58
#define VK_Y            0x59
#define VK_Z            0x5A

// ---- 系统度量 ----
#define SM_CXSCREEN      0
#define SM_CYSCREEN      1
#define SM_CXFULLSCREEN  0
#define SM_CYFULLSCREEN  1

typedef struct tagPOINT { int x, y; } POINT;
typedef struct _LARGE_INTEGER { long long QuadPart; } LARGE_INTEGER;

// ---- 测试可注入的状态 ----
extern bool  g_stubKeyState[256];
extern int   g_stubScreenW;
extern int   g_stubScreenH;

inline short GetAsyncKeyState(int vKey)
{
    return g_stubKeyState[vKey & 0xFF] ? (short)0x8000 : (short)0;
}

inline void Sleep(DWORD ms) { (void)ms; }

inline int GetSystemMetrics(int index)
{
    // SM_CYFULLSCREEN 与 SM_CYSCREEN 同为 1
    return (index == SM_CYFULLSCREEN) ? g_stubScreenH : g_stubScreenW;
}

inline BOOL QueryPerformanceFrequency(LARGE_INTEGER* freq)
{
    freq->QuadPart = 1000000LL;
    return TRUE;
}

inline BOOL QueryPerformanceCounter(LARGE_INTEGER* ctr)
{
    static long long fake = 0;
    fake += 16666;            // 每次调用推进约 1/60 秒
    ctr->QuadPart = fake;
    return TRUE;
}

inline BOOL GetCursorPos(POINT* pt) { pt->x = 0; pt->y = 0; return TRUE; }
inline BOOL ScreenToClient(HWND, POINT* pt) { (void)pt; return TRUE; }

// ---- 格式化（桩实现：支持 %d / %s / %%）----
inline int vswprintf_s(wchar_t* buf, size_t cap, const wchar_t* fmt, va_list ap)
{
    wchar_t* out = buf;
    size_t left = cap;
    for (const wchar_t* p = fmt; *p; ++p)
    {
        if (*p != L'%') { if (left > 1) { *out++ = *p; --left; } continue; }
        ++p;
        if (*p == L'%') { if (left > 1) { *out++ = L'%'; --left; } continue; }
        if (*p == L'd')
        {
            const int v = va_arg(ap, int);
            char tmp[32];
            snprintf(tmp, sizeof(tmp), "%d", v);
            for (const char* c = tmp; *c; ++c)
                if (left > 1) { *out++ = (wchar_t)*c; --left; }
            continue;
        }
        if (*p == L's')
        {
            const wchar_t* s = va_arg(ap, const wchar_t*);
            if (!s) s = L"(null)";
            for (; *s; ++s)
                if (left > 1) { *out++ = *s; --left; }
            continue;
        }
        if (left > 1) { *out++ = L'%'; --left; }
        if (*p && left > 1) { *out++ = *p; --left; }
    }
    if (cap) *out = 0;
    return (int)(out - buf);
}

inline int swprintf_s(wchar_t* buf, size_t cap, const wchar_t* fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    const int n = vswprintf_s(buf, cap, fmt, ap);
    va_end(ap);
    return n;
}

inline int vsprintf_s(char* buf, size_t cap, const char* fmt, va_list ap)
{
    return vsnprintf(buf, cap, fmt, ap);
}

inline int sprintf_s(char* buf, size_t cap, const char* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, cap, fmt, ap);
    va_end(ap);
    return n;
}

template <size_t N>
inline int swprintf_s(wchar_t (&buf)[N], const wchar_t* fmt, ...)
{
    va_list ap; va_start(ap, fmt);
    wchar_t* out = buf;
    size_t left = N;
    for (const wchar_t* p = fmt; *p; ++p)
    {
        if (*p != L'%') { if (left > 1) { *out++ = *p; --left; } continue; }
        ++p;
        if (*p == L'%') { if (left > 1) { *out++ = L'%'; --left; } continue; }
        if (*p == L'd')
        {
            const int v = va_arg(ap, int);
            char tmp[32];
            snprintf(tmp, sizeof(tmp), "%d", v);
            for (const char* c = tmp; *c; ++c)
                if (left > 1) { *out++ = (wchar_t)*c; --left; }
            continue;
        }
        if (left > 1) { *out++ = L'%'; --left; }
        if (*p && left > 1) { *out++ = *p; --left; }
    }
    va_end(ap);
    if (N) *out = 0;
    return (int)(out - buf);
}
