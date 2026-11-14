// ============================================================================
//  mmsystem.h 桩
// ============================================================================
#pragma once

#include "Windows.h"

static bool g_stubMusicOpened = false;
static bool g_stubMusicPlaying = false;
inline bool stubCommandStartsWith(LPCTSTR command, LPCTSTR prefix)
{
    while (*prefix) if (*command++ != *prefix++) return false;
    return true;
}

inline int mciSendString(LPCTSTR command, TCHAR*, UINT, HWND)
{
    if (stubCommandStartsWith(command, _T("open "))) g_stubMusicOpened = true;
    if (stubCommandStartsWith(command, _T("play "))) g_stubMusicPlaying = g_stubMusicOpened;
    if (stubCommandStartsWith(command, _T("stop "))) g_stubMusicPlaying = false;
    if (stubCommandStartsWith(command, _T("close "))) g_stubMusicOpened = g_stubMusicPlaying = false;
    return 0;
}
