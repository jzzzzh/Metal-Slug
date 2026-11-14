// Run the real entry point with a deterministic clock and stop after 120 frames.
#include <graphics.h>
struct StopLoop {};
static int renderFrames = 0;
static void testSleep(DWORD)
{
    if (++renderFrames == 120) throw StopLoop{};
}
#define Sleep testSleep
#define main gameMain
#include "../HeJinDanTou.cpp"
#undef main
#undef Sleep

bool g_stubKeyState[256] = {};
int g_stubScreenW = 320, g_stubScreenH = 240;
bool g_stubMouseHit = false;
int g_stubMouseX = 0, g_stubMouseY = 0;

int main()
{
    try { gameMain(); } catch (const StopLoop&) {}
    printf("120 render frames / %d logic ticks\n", g_game.frameClock);
    const bool ok = g_game.frameClock >= 118 && g_game.frameClock <= 120;
    closegraph();
    return ok ? 0 : 1;
}
