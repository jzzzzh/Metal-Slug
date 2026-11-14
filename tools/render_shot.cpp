// ============================================================================
//  render_shot.cpp —— 无头渲染截图（离线目视检查用）
//
//  把真实素材接进桩头文件后，可以把游戏画面导出成 raw 帧缓冲，
//  再用 tools/raw_to_png.py 转成 PNG，用来检查构图是否正确。
//
//  用法：
//    python3 tools/convert_textures.py
//    clang++ -std=c++14 -O2 -DUNICODE -D_UNICODE -I tools/easyx_stub \
//            tools/render_shot.cpp -o /tmp/ms_shot
//    /tmp/ms_shot && python3 tools/raw_to_png.py /tmp/ms_shot
// ============================================================================

#define METAL_SLUG_NO_MAIN
#include "../HeJinDanTou.cpp"

#include <cstdio>

bool g_stubKeyState[256] = { false };
int  g_stubScreenW = 1920;
int  g_stubScreenH = 1080;
bool g_stubMouseHit = false;
int  g_stubMouseX = 0;
int  g_stubMouseY = 0;

static void dump(const char* path)
{
    FILE* f = fopen(path, "wb");
    if (!f) { printf("无法写入 %s\n", path); return; }
    fwrite(g_stubFrame, 4, (size_t)g_stubFbW * (size_t)g_stubFbH, f);
    fclose(f);
    printf("  -> %s (%dx%d)\n", path, g_stubFbW, g_stubFbH);
}

static void step(int n = 1)
{
    for (int i = 0; i < n; ++i) { g_game.update(); g_game.draw(); }
}

int main()
{
    initgraph(g_stubScreenW, g_stubScreenH);
    BeginBatchDraw();
    g_game.init();

    // ---- 1. 标题闪屏 ----
    printf("渲染标题闪屏\n");
    step(1);
    dump("/tmp/ms_shot_0.raw");

    // ---- 2. 主菜单 ----
    printf("渲染主菜单\n");
    g_game.phase = PH_MENU;
    step(1);
    dump("/tmp/ms_shot_1.raw");

    // ---- 3. 选关 ----
    printf("渲染选关界面\n");
    g_game.phase = PH_ROUND_SEL;
    g_game.roundSel = 1;
    step(1);
    dump("/tmp/ms_shot_2.raw");

    // ---- 4. 第一关战斗画面 ----
    printf("渲染战斗画面\n");
    g_game.phase = PH_PLAY;
    g_game.endless = false;
    g_game.round = 0;
    g_game.resetRound(0, false);
    g_game.player.hp = Cfg::PLAYER_MAX_HP;
    g_game.player.x = 520.f;
    g_game.cameraX = 0.f;

    g_game.spawnEnemy(EK_RIFLEMAN, 900.f);
    g_game.spawnEnemy(EK_RIFLEMAN, 1080.f);
    g_game.spawnEnemy(EK_KNIFE,    1260.f);
    g_game.spawnEnemy(EK_ROCKET,   1460.f);
    g_game.spawnEnemy(EK_THROWER,  1660.f);
    g_game.spawnEnemy(EK_PLANE,    1500.f);
    g_game.spawnBullet(BLK_RIFLE, 640.f, 830.f, 560.f, -80.f, 1, true);
    g_game.spawnBullet(BLK_CANNON, 640.f, 840.f, 480.f, 20.f, 5, true);
    g_game.spawnBullet(BLK_ENEMY_RIFLE, 1200.f, 900.f, -240.f, 0.f, 1, false);
    g_game.spawnBullet(BLK_ENEMY_ROCKET, 1500.f, 900.f, -200.f, 0.f, 3, false);
    g_game.spawnBullet(BLK_GRENADE, 1300.f, 800.f, 240.f, 0.f, 1, false);
    g_game.spawnParticles(700.f, 880.f, 14, RGB(220, 60, 60), 180.f, 4.f);
    g_game.spawnParticles(1000.f, 880.f, 10, RGB(255, 180, 80), 150.f, 4.f);
    g_game.showToast(_T("第一关"));

    g_stubMouseHit = true;
    g_stubMouseX = 1200;
    g_stubMouseY = 700;
    step(2);
    dump("/tmp/ms_shot_3.raw");

    // ---- 5. Boss 战画面 ----
    printf("渲染 Boss 战\n");
    g_game.cameraX = g_game.levelWidth - 1920.f;
    g_game.player.x = g_game.cameraX + 700.f;
    g_game.boss.state = BSS_IDLE;
    g_game.boss.x = g_game.levelWidth - 420.f;
    {
        const int bossH = g_assets.bossAttack[0].getheight();
        const int footH = g_assets.playerFoot[ST_STAND][0].getheight();
        g_game.boss.y = (float)(g_game.groundY + g_game.playerBodyHeight() - Cfg::FOOT_OVERLAP + footH - bossH);
    }
    g_game.boss.hp = g_game.boss.maxHp = 80;
    g_game.boss.anim = 0;
    g_game.oldMan.active = true;
    g_game.oldMan.freed = false;
    g_game.oldMan.x = g_game.boss.x - 260.f;
    g_game.oldMan.y = (float)g_game.groundY;
    g_game.showToast(_T("BOSS 出现！"));
    g_stubMouseX = (int)(g_game.boss.x - g_game.cameraX) - 200;
    g_stubMouseY = (int)g_game.boss.y + 60;
    step(2);
    dump("/tmp/ms_shot_4.raw");

    // ---- 5b. 第二关掩体（A7：第二关有油桶/沙袋，第一关没有）----
    printf("渲染第二关掩体\n");
    g_game.resetRound(1, false);
    g_game.phase = PH_PLAY;
    g_game.player.x = 700.f;
    g_game.cameraX = 300.f;
    step(2);
    dump("/tmp/ms_shot_7.raw");

    // ---- 6. 第二关（开局在飞机上）----
    printf("渲染第二关（飞机）\n");
    g_game.round = 1;
    g_game.resetRound(1, false);
    g_game.phase = PH_PLAY;
    g_game.cameraX = 0.f;
    step(2);
    dump("/tmp/ms_shot_5.raw");

    // ---- 7. 胜利画面 ----
    printf("渲染胜利画面\n");
    g_game.phase = PH_WIN;
    g_game.score = 12345;
    g_game.kills = 87;
    step(1);
    dump("/tmp/ms_shot_6.raw");

    return 0;
}
