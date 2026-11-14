// ============================================================================
//  headless_test.cpp —— 无头测试台
//
//  用 tools/easyx_stub 里的 EasyX 桩，在任意平台跑真实游戏逻辑：
//    · 真实读 PNG 文件头拿贴图尺寸（碰撞盒/绘制裁剪都按真实尺寸跑）
//    · 真实执行 EasyXPng.h 的逐像素 alpha 混合（ASan 可盯住越界）
//    · 覆盖：菜单 / 选关 / 战斗 / Boss / 解救 / 通关 / 死亡 / 无尽模式
//
//  编译：
//    clang++ -std=c++14 -fsanitize=address,undefined //            -I tools/easyx_stub tools/headless_test.cpp -o /tmp/ms_test
// ============================================================================

#define METAL_SLUG_NO_MAIN
#include "../HeJinDanTou.cpp"

#include <cstdio>

// ---- 桩的全局状态 ----
bool g_stubKeyState[256] = { false };
int  g_stubScreenW = 1920;
int  g_stubScreenH = 1080;
bool g_stubMouseHit = false;
int  g_stubMouseX = 0;
int  g_stubMouseY = 0;

static int g_failures = 0;
static int g_checks = 0;

#define CHECK(cond, msg)                                                     \
    do {                                                                     \
        ++g_checks;                                                          \
        if (!(cond)) {                                                       \
            printf("  [FAIL] %s   (line %d)\n", (msg), __LINE__);           \
            ++g_failures;                                                    \
        }                                                                    \
    } while (0)

static void step(int n = 1)
{
    for (int i = 0; i < n; ++i)
    {
        g_game.update();
        g_game.draw();
    }
}

static void tap(int vk)
{
    g_stubKeyState[vk] = true;
    step(1);
    g_stubKeyState[vk] = false;
    step(1);
}

static bool finite(float v)
{
    return (v == v) && (v > -1e30f) && (v < 1e30f);
}

// 推进到 Boss 出现：主角走到关卡最右端，镜头跟到边界
static void triggerBoss()
{
    for (int i = 0; i < 240; ++i)
    {
        g_game.player.x = g_game.levelWidth - 100.f;
        g_game.player.hp = Cfg::PLAYER_MAX_HP;
        step(1);
    }
}

// 打 Boss：先清掉路上的杂兵（否则子弹会先被小兵吃掉），再贴脸开火
static void fightBoss(int frames)
{
    // 第二关开局主角在飞机上，先落地再打地面 Boss
    if (g_game.player.inPlane)
    {
        tap('E');
        CHECK(!g_game.player.inPlane, "按 E 应能离开飞机");
    }
    for (int i = 0; i < frames; ++i)
    {
        g_game.enemies.clear();                     // 清场
        if (g_game.boss.state == BSS_ENTER)         // 等 Boss 走进来
        {
            g_game.player.hp = Cfg::PLAYER_MAX_HP;
            step(1);
            continue;
        }
        g_game.player.x = g_game.boss.x - 320.f;
        g_game.player.y = (float)g_game.groundY;
        g_game.player.hp = Cfg::PLAYER_MAX_HP;
        g_stubMouseHit = true;
        g_stubMouseX = (int)g_game.boss.x - 40;
        g_stubMouseY = (int)g_game.boss.y + 60;     // 瞄准躯干
        g_stubKeyState[VK_LBUTTON] = true;
        step(1);
    }
    g_stubKeyState[VK_LBUTTON] = false;
}

// 打完 Boss 并救出老人；expectNext 为回车后应进入的阶段
static void clearRound(Phase expectNext)
{
    // 长跑阶段 Boss 可能已经被触发并击破，这里复位后单独验证 Boss 战流程
    g_game.boss.state = BSS_NONE;
    g_game.boss.hp = 0;
    g_game.boss.maxHp = 0;
    g_game.oldMan.active = false;
    g_game.oldMan.freed = false;

    triggerBoss();
    CHECK(g_game.boss.state != BSS_NONE, "应触发 Boss 出现");

    // 等 Boss 走入动画结束、站定
    for (int i = 0; i < 600 && g_game.boss.state == BSS_ENTER; ++i)
    {
        g_game.enemies.clear();
        g_game.player.hp = Cfg::PLAYER_MAX_HP;
        step(1);
    }
    CHECK(g_game.boss.state != BSS_ENTER, "Boss 应走到位");

    const int hpBefore = g_game.boss.hp;
    fightBoss(150);
    CHECK(g_game.boss.hp < hpBefore, "玩家子弹应能对 Boss 造成伤害");

    // 直接压血，加速验证死亡流程
    g_game.boss.hp = 1;
    fightBoss(120);
    CHECK(g_game.boss.state == BSS_DEAD || g_game.boss.state == BSS_DIE, "Boss 应进入死亡流程");
    CHECK(g_game.oldMan.active, "Boss 死亡后应出现被绑的老人");

    g_game.player.x = g_game.oldMan.x;
    g_game.player.y = (float)g_game.groundY;
    step(2);
    CHECK(g_game.canInteract(), "靠近老人应出现交互提示");

    tap('E');
    CHECK(g_game.phase == PH_CLEAR, "救出老人后应进入关卡完成状态");
    tap(VK_RETURN);
    CHECK(g_game.phase == expectNext, "回车后应进入下一阶段");
}

// ===========================================================================
//  专项回归：覆盖本轮修复的每一条行为
// ===========================================================================

// A1：刀兵必须真的能打到玩家（原来枪兵 300px 停下 / 刀兵 62px 判定，两个阈值互斥）
static void testKnifeMelee()
{
    printf("== 专项 A1：刀兵近战 ==\n");
    g_game.resetRound(0, false);
    g_game.enemies.clear();
    g_game.bullets.clear();
    g_game.player.x = 400.f;
    g_game.player.y = (float)g_game.groundY;
    g_game.player.hp = Cfg::PLAYER_MAX_HP;
    g_game.player.invuln = 0;

    g_game.phase = PH_PLAY;
    g_game.spawnEnemy(EK_KNIFE, 700.f);
    bool gotHit = false;
    for (int i = 0; i < 900; ++i)
    {
        const int hp = g_game.player.hp;
        g_game.player.invuln = 0;   // 关掉无敌帧，确保能观测到伤害
        g_game.updateEnemies(Cfg::DT);
        if (g_game.player.hp < hp) { gotHit = true; break; }
    }
    CHECK(gotHit, "刀兵应该能靠近并攻击玩家");

    // 贴身起手：伤害应在挥砍动画中段结算
    g_game.enemies.clear();
    g_game.player.hp = Cfg::PLAYER_MAX_HP;
    g_game.player.invuln = 0;
    g_game.spawnEnemy(EK_KNIFE, 400.f + 40.f);
    gotHit = false;
    for (int i = 0; i < 300; ++i)
    {
        const int hp = g_game.player.hp;
        g_game.player.invuln = 0;
        g_game.updateEnemies(Cfg::DT);
        if (g_game.player.hp < hp) { gotHit = true; break; }
    }
    CHECK(gotHit, "刀兵贴身时必须能对玩家造成伤害");
}

// A2/A3：沙袋挡子弹 + 障碍优先于敌人（子弹不能穿过桶打死桶后的敌人）
static void testObstacleBlocksBullets()
{
    printf("== 专项 A2/A3：掩体挡弹与命中优先级 ==\n");
    g_game.resetRound(0, false);
    g_game.enemies.clear();
    g_game.bullets.clear();
    g_game.obstacles.clear();
    g_game.player.x = 300.f;
    g_game.player.y = (float)g_game.groundY;

    // 一个不可破坏的沙袋挡在敌人前面
    Obstacle bag;
    bag.alive = true;
    bag.destructible = false;
    bag.x = 600.f;
    bag.y = (float)(g_game.groundY + 20);
    bag.maxHp = bag.hp = 0;
    bag.flash = 0;
    g_game.obstacles.push_back(bag);

    // 敌人站在沙袋后面，而且身体与沙袋有重叠 ——
    // 这样"障碍优先于敌人"和"敌人优先于障碍"两种顺序会得到不同结果，用例才有鉴别力。
    Enemy e;
    e.kind = EK_RIFLEMAN;
    e.alive = true;
    e.x = 580.f;
    e.y = (float)g_game.groundY;
    e.vx = 0.f;
    e.maxHp = e.hp = 99;
    e.faceRight = false;
    e.anim = 0;
    e.fireTimer = 999;
    e.atkTicks = 0;
    e.atkHit = true;
    e.flash = 0;
    e.muzzle = 0;
    e.hurtCd = 0;
    g_game.enemies.push_back(e);

    // 从沙袋左边打一发子弹过去
    g_game.spawnBullet(BLK_RIFLE, 560.f, (float)g_game.groundY + 30.f, 560.f, 0.f, 1, true);
    const int hpBefore = e.hp;
    for (int i = 0; i < 30; ++i)
        g_game.updateBullets(Cfg::DT);
    CHECK(g_game.bullets.empty(), "子弹应被沙袋挡下");
    CHECK(g_game.enemies[0].hp == hpBefore, "子弹不该穿过沙袋打到后面的敌人");
    CHECK(g_game.enemies[0].hp == hpBefore && g_game.bullets.empty(),
          "障碍判定必须排在敌人判定之前（否则子弹会先打中站在沙袋上的敌人）");

    // 可破坏油桶：应能被打掉
    g_game.obstacles.clear();
    Obstacle barrel;
    barrel.alive = true;
    barrel.destructible = true;
    barrel.x = 600.f;
    barrel.y = (float)(g_game.groundY + 10);
    barrel.maxHp = barrel.hp = Cfg::BARREL_HP;
    barrel.flash = 0;
    g_game.obstacles.push_back(barrel);
    // 持续开火，直到油桶被打爆
    for (int i = 0; i < 300 && !g_game.obstacles.empty() && g_game.obstacles[0].alive; ++i)
    {
        if (i % 4 == 0)
            g_game.spawnBullet(BLK_RIFLE, 560.f, (float)g_game.groundY + 30.f, 560.f, 0.f, 1, true);
        g_game.updateBullets(Cfg::DT);
    }
    CHECK(g_game.obstacles.empty() || !g_game.obstacles[0].alive, "油桶应能被子弹打爆");
}

// B1/B2：主角碰撞盒固定 + 敌人避障用敌人自己的盒子
static void testHitboxStability()
{
    printf("== 专项 B1/B2：碰撞盒稳定性 ==\n");
    g_game.resetRound(0, false);
    g_game.player.weapon = WP_RIFLE;
    g_game.player.muzzleFlash = 0;
    g_game.player.stance = ST_STAND;

    float l0, t0, r0, b0;
    g_game.playerBox(g_game.player.x, g_game.player.y, l0, t0, r0, b0);

    // 开火后（贴图切到重武器）碰撞盒不应变化
    g_game.player.muzzleFlash = Cfg::MUZZLE_FRAMES;
    float l1, t1, r1, b1;
    g_game.playerBox(g_game.player.x, g_game.player.y, l1, t1, r1, b1);
    CHECK(l0 == l1 && t0 == t1 && r0 == r1 && b0 == b1, "开火不应改变主角的碰撞盒");

    // 换武器也不应改变碰撞盒
    g_game.player.muzzleFlash = 0;
    g_game.player.weapon = WP_CANNON;
    float l2, t2, r2, b2;
    g_game.playerBox(g_game.player.x, g_game.player.y, l2, t2, r2, b2);
    CHECK(l0 == l2 && t0 == t2 && r0 == r2 && b0 == b2, "换武器不应改变主角的碰撞盒");

    // 敌人避障必须用敌人自己的盒子（而不是主角的）
    Enemy e;
    e.kind = EK_RIFLEMAN;
    e.alive = true;
    e.x = 100.f;
    e.y = (float)g_game.groundY;
    e.vx = 0.f;
    e.maxHp = e.hp = 3;
    e.faceRight = false;
    e.anim = 0;
    e.fireTimer = 999;
    e.atkTicks = 0;
    e.atkHit = true;
    e.flash = 0;
    e.muzzle = 0;
    e.hurtCd = 0;
    float el, et, er, eb;
    g_game.enemyBox(e, el, et, er, eb);
    CHECK(el != l0 || er != r0, "敌人的碰撞盒不应与主角相同");
}

// A5：敌机朝向由速度决定（恒定向左飞，不应因为越过玩家就被镜像）
static void testPlaneFacing()
{
    printf("== 专项 A5：敌机朝向 ==\n");
    g_game.resetRound(0, false);
    g_game.enemies.clear();
    g_game.spawnEnemy(EK_PLANE, 900.f);
    Enemy& p = g_game.enemies[0];
    CHECK(p.vx < 0.f, "敌机应向左飞");
    CHECK(p.faceRight == false, "向左飞的敌机不应被画成朝右");

    // 关键场景：敌机飞到玩家左侧后（玩家在敌机右边），
    // 如果朝向按"朝玩家"推导，就会被镜像成朝右 —— 像是掉头。
    g_game.player.x = 2000.f;
    g_game.updateEnemies(Cfg::DT);
    CHECK(!g_game.enemies.empty() && g_game.enemies[0].faceRight == false,
          "敌机飞到玩家左侧后仍应保持朝左（不能被镜像成掉头）");
}

// A4/A6：无尽模式救完人继续下一轮，并且下一场 Boss 还能出现
static void testEndlessCycle()
{
    printf("== 专项 A4/A6：无尽模式循环 ==\n");
    g_game.resetRound(0, true);
    g_game.phase = PH_PLAY;
    g_game.enemies.clear();

    // 模拟"Boss 已被击败、老人出现"
    g_game.boss.state = BSS_DEAD;
    g_game.boss.hp = 0;
    g_game.oldMan.active = true;
    g_game.oldMan.freed = false;
    g_game.oldMan.x = g_game.player.x;
    g_game.oldMan.y = (float)g_game.groundY;

    tap('E');
    CHECK(g_game.phase == PH_PLAY, "无尽模式救完人后应继续游戏，而不是结束");
    CHECK(g_game.boss.state == BSS_NONE, "救完人后 Boss 状态应重置，下一场还能出现");
    CHECK(g_game.oldMan.active == false, "救完人后老人应被清掉");
    CHECK(g_game.score > 0, "救人应加分");

    // F3：进入下一轮后必须重新铺掩体，而且掩体必须落在关卡范围内
    CHECK(!g_game.obstacles.empty(), "无尽模式进入下一轮后应重新铺掩体");
    bool inBounds = true;
    for (size_t i = 0; i < g_game.obstacles.size(); ++i)
        if (g_game.obstacles[i].x + 200.f > g_game.levelWidth)
            inBounds = false;
    CHECK(inBounds, "掩体必须落在关卡宽度范围内（不能摆到关卡外）");

    // F4：Boss 计数必须按轮重置
    g_game.bossCount = 7;
    g_game.resetRound(0, true);
    CHECK(g_game.bossCount == 0, "重开关卡时 Boss 计数应归零");

    // 下一场 Boss 仍能触发
    g_game.cameraX = g_game.levelWidth - (float)g_game.screenW - 100.f;
    g_game.player.x = g_game.cameraX + 100.f;
    g_game.nextBossWave = g_game.waveIndex;   // 立刻满足触发条件
    for (int i = 0; i < 10; ++i)
    {
        g_game.player.hp = Cfg::PLAYER_MAX_HP;
        g_game.update();
    }
    CHECK(g_game.boss.state != BSS_NONE, "无尽模式下应该能再次出现 Boss");
}

// A7：各关障碍数量与敌人构成不同
static void testLevelDifference()
{
    printf("== 专项 A7：关卡差异 ==\n");
    g_game.resetRound(0, false);
    const size_t obs0 = g_game.obstacles.size();
    CHECK(obs0 == 0, "第一关不应有障碍（原版第一关无掩体）");

    g_game.resetRound(1, false);
    const size_t obs1 = g_game.obstacles.size();
    CHECK(obs1 == 5, "第二关应有 5 个障碍");

    g_game.resetRound(2, false);
    const size_t obs2 = g_game.obstacles.size();
    CHECK(obs2 == 6, "第三关应有 6 个障碍");

    // 三关的开局编队应不同
    g_game.resetRound(0, false);
    int kinds0[EK_COUNT] = { 0 };
    for (size_t i = 0; i < g_game.enemies.size(); ++i)
        kinds0[g_game.enemies[i].kind]++;
    CHECK(kinds0[EK_RIFLEMAN] > 0 && kinds0[EK_KNIFE] > 0, "第一关开局应有步枪兵和刀兵");

    g_game.resetRound(2, false);
    int kinds2[EK_COUNT] = { 0 };
    for (size_t i = 0; i < g_game.enemies.size(); ++i)
        kinds2[g_game.enemies[i].kind]++;
    CHECK(kinds2[EK_PLANE] > 0, "第三关开局应有敌机");
    CHECK(kinds0[EK_PLANE] == 0, "第一关开局不应有敌机");
    CHECK(kinds2[EK_PLANE] != kinds0[EK_PLANE], "各关开局编队应不同");
}

// F1/F7：掩体只挡子弹和主角，敌人绝不能被卡死在障碍前
// （原版用 checkIsDangZhu() 冻结整个敌人 update，刀兵永远走不到玩家面前）
static void testEnemyNotStuckByObstacle()
{
    printf("== 专项 F1/F7：敌人不被掩体卡死 ==\n");
    g_game.resetRound(0, false);
    g_game.enemies.clear();
    g_game.bullets.clear();
    g_game.obstacles.clear();

    // 油桶挡在刀兵和玩家中间
    Obstacle barrel;
    barrel.alive = true;
    barrel.destructible = true;
    barrel.x = 500.f;
    barrel.y = (float)(g_game.groundY + 10);
    barrel.maxHp = barrel.hp = Cfg::BARREL_HP;
    barrel.flash = 0;
    g_game.obstacles.push_back(barrel);

    g_game.player.x = 300.f;
    g_game.player.y = (float)g_game.groundY;
    g_game.player.hp = Cfg::PLAYER_MAX_HP;
    g_game.player.invuln = 0;
    g_game.phase = PH_PLAY;

    g_game.spawnEnemy(EK_KNIFE, 800.f);
    bool gotHit = false;
    for (int i = 0; i < 900; ++i)
    {
        const int hp = g_game.player.hp;
        g_game.player.invuln = 0;      // 关掉无敌帧，确保能观测到伤害
        g_game.updateEnemies(Cfg::DT);
        if (g_game.player.hp < hp) { gotHit = true; break; }
    }
    CHECK(gotHit, "油桶挡路时刀兵也必须能贴到玩家面前（不能被卡死）");

    // 反过来：主角会被掩体挡住（掩体仍然有意义）
    g_game.enemies.clear();
    g_game.bullets.clear();
    g_game.player.x = 380.f;
    g_game.player.y = (float)g_game.groundY;
    g_game.player.hp = Cfg::PLAYER_MAX_HP;
    g_game.waveTimer = 1000000;        // 压制刷兵，避免干扰判定
    g_stubKeyState[VK_RIGHT] = true;
    for (int i = 0; i < 180; ++i)
    {
        g_game.player.hp = Cfg::PLAYER_MAX_HP;
        step(1);
    }
    g_stubKeyState[VK_RIGHT] = false;
    CHECK(g_game.player.x < 470.f, "主角应该被油桶挡住，不能直接穿过去");
}

// F8：敌弹和手雷都必须和掩体做碰撞
static void testEnemyBulletsBlockedByCover()
{
    printf("== 专项 F8：敌弹/手雷的掩体碰撞 ==\n");
    g_game.resetRound(0, false);
    g_game.enemies.clear();
    g_game.bullets.clear();
    g_game.obstacles.clear();

    Obstacle bag;
    bag.alive = true;
    bag.destructible = false;
    bag.x = 600.f;
    bag.y = (float)(g_game.groundY + 20);
    bag.maxHp = bag.hp = 0;
    bag.flash = 0;
    g_game.obstacles.push_back(bag);

    // 敌弹从右边射向左边的玩家，应被沙袋吃掉
    g_game.player.x = 400.f;
    g_game.player.y = (float)g_game.groundY;
    g_game.player.hp = Cfg::PLAYER_MAX_HP;
    g_game.player.invuln = 0;
    g_game.spawnBullet(BLK_ENEMY_RIFLE, 700.f, (float)g_game.groundY + 30.f, -240.f, 0.f, 1, false);
    for (int i = 0; i < 120; ++i)
        g_game.updateBullets(Cfg::DT);
    CHECK(g_game.bullets.empty(), "敌弹应被沙袋吃掉");
    CHECK(g_game.player.hp == Cfg::PLAYER_MAX_HP, "敌弹被沙袋挡住时玩家不该掉血");

    // 手雷落到沙袋上：哑掉，不爆炸
    g_game.bullets.clear();
    g_game.player.x = 640.f;           // 站在沙袋正上方
    g_game.player.y = (float)g_game.groundY;
    g_game.player.hp = Cfg::PLAYER_MAX_HP;
    g_game.player.invuln = 0;
    g_game.spawnBullet(BLK_GRENADE, 640.f, (float)g_game.groundY - 120.f, 0.f, 0.f, 5, false);
    for (int i = 0; i < 240; ++i)
        g_game.updateBullets(Cfg::DT);
    CHECK(g_game.bullets.empty(), "手雷撞到掩体后应消失");
    CHECK(g_game.player.hp == Cfg::PLAYER_MAX_HP, "手雷撞到掩体后不应爆炸伤到玩家");
}

// F9：过关时要清空弹幕，而且必须覆盖"敌人同帧开火"的边界
static void testClearRemovesBullets()
{
    printf("== 专项 F9：过关清空弹幕 ==\n");
    g_game.resetRound(0, false);
    g_game.phase = PH_PLAY;
    g_game.enemies.clear();
    g_game.bullets.clear();
    g_game.player.x = 400.f;
    g_game.player.y = (float)g_game.groundY;
    g_game.player.hp = Cfg::PLAYER_MAX_HP;

    // 先确认"敌人在下一帧开火"这件事本身是成立的（否则下面的断言没有意义）
    g_game.spawnEnemy(EK_RIFLEMAN, 700.f);
    g_game.enemies[0].fireTimer = 0;
    g_game.updateEnemies(Cfg::DT);
    CHECK(!g_game.bullets.empty(), "敌人开火后应产生子弹（前置条件）");

    // 布置救人场景：老人出现 + 一个正在开火的敌人 + 场上残留子弹
    g_game.bullets.clear();
    g_game.oldMan.active = true;
    g_game.oldMan.freed = false;
    g_game.oldMan.x = g_game.player.x;
    g_game.oldMan.y = (float)g_game.groundY;
    g_game.spawnEnemy(EK_RIFLEMAN, 700.f);
    g_game.enemies[0].fireTimer = 0;
    g_game.spawnBullet(BLK_RIFLE, 500.f, (float)g_game.groundY - 120.f, 120.f, 0.f, 1, true);

    tap('E');
    CHECK(g_game.phase == PH_CLEAR, "救出老人后应进入过关状态");
    CHECK(g_game.bullets.empty(), "过关后场上不应残留任何子弹（含敌人同帧开火的）");
}

// F6：三关的兵种权重必须真的不同，并且真的按权重抽
static void testLevelWeights()
{
    printf("== 专项 F6：各关兵种权重 ==\n");
    for (int lv = 0; lv < 3; ++lv)
    {
        g_game.resetRound(lv, false);
        g_game.enemies.clear();

        int cnt[EK_COUNT] = { 0 };
        int total = 0;
        for (int i = 0; i < 500; ++i)
        {
            g_game.enemies.clear();
            g_game.spawnWave();
            for (size_t k = 0; k < g_game.enemies.size(); ++k)
            {
                ++cnt[g_game.enemies[k].kind];
                ++total;
            }
        }
        CHECK(total > 500, "抽样应生成足够多的敌人");

        int wsum = 0;
        for (int k = 0; k < EK_COUNT; ++k)
            wsum += kLevel[lv].weight[k];
        CHECK(wsum > 0, "关卡权重之和应大于 0");

        for (int k = 0; k < EK_COUNT; ++k)
        {
            const int w = kLevel[lv].weight[k];
            if (w == 0)
            {
                CHECK(cnt[k] == 0, "权重为 0 的兵种不应被抽到");
                continue;
            }
            const float expect = (float)w / (float)wsum;
            const float got = (float)cnt[k] / (float)total;
            CHECK(fabs(got - expect) < 0.09f, "兵种占比应与关卡权重一致");
        }
    }
}

// F10：素材缺失必须能被检出（桩要按真实 EasyX 的行为返回 0x0 空图）
static void testMissingAssetDetection()
{
    printf("== 专项 F10：素材缺失检测 ==\n");
    IMAGE ok;
    g_game.loadOne(&ok, _T("./zhujue/foot/leftstand.png"));
    CHECK(ok.getwidth() > 0 && ok.getheight() > 0, "存在的素材应加载出真实尺寸");

    const int before = g_game.missingAssets;
    IMAGE miss;
    g_game.loadOne(&miss, _T("./__definitely_missing__/nope.png"));
    CHECK(g_game.missingAssets == before + 1, "缺失素材必须被计数");
    CHECK(miss.getwidth() == 0, "缺失素材应得到 0x0 空图（与真实 EasyX 一致）");
}

int main()
{
    printf("== 阶段 0：初始化 ==\n");
    initgraph(g_stubScreenW, g_stubScreenH);
    BeginBatchDraw();
    g_game.init();
    CHECK(g_game.phase == PH_SPLASH, "初始应为标题闪屏");
    CHECK(g_game.screenW == 1920 && g_game.screenH == 1080, "屏幕尺寸应正确");

    printf("== 阶段 0b：标题闪屏 ==\n");
    step(120);
    CHECK(g_game.phase == PH_MENU, "闪屏结束后应自动进入主菜单");

    printf("== 阶段 1：主菜单 ==\n");
    CHECK(g_game.menuSel == 0, "默认应选中普通模式");
    tap(VK_SPACE);
    CHECK(g_game.menuSel == 1, "空格应切换到无尽模式");
    tap(VK_SPACE);
    CHECK(g_game.menuSel == 0, "空格应切回普通模式");
    tap(VK_RETURN);
    CHECK(g_game.phase == PH_ROUND_SEL, "回车应进入选关界面");
    tap(VK_ESCAPE);
    CHECK(g_game.phase == PH_MENU, "ESC 应返回主菜单");
    tap(VK_RETURN);
    CHECK(g_game.phase == PH_ROUND_SEL, "再次回车应进入选关");

    printf("== 阶段 2：选关 ==\n");
    tap(VK_SPACE);
    CHECK(g_game.roundSel == 1, "空格应切换关卡");
    tap(VK_SPACE);
    tap(VK_SPACE);
    CHECK(g_game.roundSel == 0, "关卡选择应循环回到第一关");
    tap(VK_RETURN);
    CHECK(g_game.phase == PH_PLAY, "回车应开始游戏");
    CHECK(g_game.player.hp == Cfg::PLAYER_MAX_HP, "开局应满血");
    CHECK(g_game.endless == false, "应为普通模式");

    // 压测帧数可用环境变量 METAL_SLUG_SOAK 调整（默认 3600 帧 = 60 秒）
    int soakFrames = 3600;
    if (const char* env = getenv("METAL_SLUG_SOAK"))
        soakFrames = atoi(env);
    printf("== 阶段 3：长时间战斗（%d 帧） ==\n", soakFrames);
    size_t maxBullets = 0, maxEnemies = 0, maxParticles = 0, maxFx = 0;
    for (int i = 0; i < soakFrames; ++i)
    {
        if (i % 7 == 0)
        {
            g_stubMouseHit = true;
            g_stubMouseX = 900 + (i * 13) % 900;
            g_stubMouseY = 520 + (i * 7) % 280;
        }
        g_stubKeyState[VK_RIGHT] = (i % 200) < 120;
        g_stubKeyState[VK_LEFT]  = (i % 200) >= 150;
        g_stubKeyState[VK_UP]    = (i % 90) < 12;
        g_stubKeyState[VK_DOWN]  = (i % 130) >= 108;
        g_stubKeyState[VK_LBUTTON] = true;
        step(1);

        // 保持主角存活，把测试重点放在系统稳定性上
        if (i % 30 == 0 && g_game.phase == PH_PLAY)
            g_game.player.hp = Cfg::PLAYER_MAX_HP;

        if (g_game.bullets.size()   > maxBullets)   maxBullets = g_game.bullets.size();
        if (g_game.enemies.size()   > maxEnemies)   maxEnemies = g_game.enemies.size();
        if (g_game.particles.size() > maxParticles) maxParticles = g_game.particles.size();
        if (g_game.fx.size()        > maxFx)        maxFx = g_game.fx.size();

        if (i % 600 == 0)
        {
            CHECK(finite(g_game.player.x) && finite(g_game.player.y), "主角坐标不应出现 NaN/Inf");
            CHECK(finite(g_game.cameraX), "镜头坐标不应出现 NaN/Inf");
            CHECK(g_game.player.hp >= 0 && g_game.player.hp <= Cfg::PLAYER_MAX_HP, "血量应始终合法");
            CHECK(g_game.player.x >= 0.f && g_game.player.x <= g_game.levelWidth, "主角不应越出关卡");
            for (size_t k = 0; k < g_game.enemies.size(); ++k)
                CHECK(g_game.enemies[k].alive && g_game.enemies[k].hp > 0, "不应残留已死亡的敌人");
            for (size_t k = 0; k < g_game.bullets.size(); ++k)
                CHECK(g_game.bullets[k].alive, "不应残留失效的子弹");
        }
    }
    g_stubKeyState[VK_LBUTTON] = false;
    printf("  峰值数量：子弹 %zu / 敌人 %zu / 粒子 %zu / 特效 %zu\n",
           maxBullets, maxEnemies, maxParticles, maxFx);
    CHECK(maxBullets < 400, "子弹数量必须有界（原版存在永不回收的泄漏）");
    CHECK(maxEnemies < 200, "敌人数量必须有界");
    CHECK(maxParticles < 3000, "粒子数量必须有界");
    CHECK(maxFx < 3000, "特效动画数量必须有界");

    printf("== 阶段 4：炮台与弹药 ==\n");
    tap(VK_2);
    CHECK(g_game.player.weapon == WP_CANNON, "按 2 应切到炮台");
    const int ammoBefore = g_game.player.cannonAmmo;
    g_stubKeyState[VK_LBUTTON] = true;
    step(120);
    g_stubKeyState[VK_LBUTTON] = false;
    CHECK(g_game.player.cannonAmmo < ammoBefore, "开炮应消耗弹药");
    CHECK(g_game.player.cannonAmmo >= 0, "弹药不应为负");

    printf("== 阶段 5：Boss 战与通关流程 ==\n");
    clearRound(PH_PLAY);
    CHECK(g_game.round == 1, "应进入第二关");
    CHECK(g_game.score > 0, "得分应累计");

    clearRound(PH_PLAY);
    CHECK(g_game.round == 2, "应进入第三关");

    clearRound(PH_WIN);
    CHECK(g_game.phase == PH_WIN, "第三关完成后应进入胜利画面");
    CHECK(g_game.score > 0, "胜利时得分应有效");

    printf("== 阶段 6：结束画面返回 ==\n");
    // 普通模式通关后回车应回到选关界面（原版行为）
    tap(VK_RETURN);
    CHECK(g_game.phase == PH_ROUND_SEL, "通关后回车应返回选关界面");
    tap(VK_ESCAPE);
    CHECK(g_game.phase == PH_MENU, "ESC 应返回主菜单");

    printf("== 阶段 7：无尽模式 ==\n");
    tap(VK_SPACE);
    CHECK(g_game.menuSel == 1, "应选中无尽模式");
    tap(VK_RETURN);
    CHECK(g_game.phase == PH_PLAY, "无尽模式应直接开始");
    CHECK(g_game.endless == true, "模式应是无尽");
    CHECK(g_game.level == 3, "无尽模式应使用第 4 张背景");

    for (int i = 0; i < 900; ++i)
    {
        g_stubKeyState[VK_RIGHT] = true;
        g_stubKeyState[VK_LBUTTON] = true;
        step(1);
        if (i % 30 == 0 && g_game.phase == PH_PLAY)
            g_game.player.hp = Cfg::PLAYER_MAX_HP;
    }
    g_stubKeyState[VK_RIGHT] = false;
    g_stubKeyState[VK_LBUTTON] = false;
    CHECK(g_game.phase == PH_PLAY, "无尽模式应持续进行");

    printf("== 阶段 8：死亡流程 ==\n");
    g_game.player.hp = 1;
    g_game.player.invuln = 0;
    g_game.damagePlayer(50, g_game.player.x + 10.f);
    CHECK(g_game.player.hp == 0, "致命伤后血量应归零");
    step(2);
    CHECK(g_game.phase == PH_OVER, "死亡应进入结束画面");
    tap(VK_RETURN);
    // 无尽模式结束后回车应回主菜单
    CHECK(g_game.phase == PH_MENU, "无尽模式结束后回车应返回主菜单");

    printf("== 阶段 9：专项回归 ==\n");
    testHitboxStability();
    testKnifeMelee();
    testObstacleBlocksBullets();
    testPlaneFacing();
    testLevelDifference();
    testEndlessCycle();
    testEnemyNotStuckByObstacle();
    testEnemyBulletsBlockedByCover();
    testClearRemovesBullets();
    testLevelWeights();
    testMissingAssetDetection();

    printf("\n===== 检查项 %d，失败 %d =====\n", g_checks, g_failures);
    if (g_failures == 0)
        printf("全部通过\n");
    return g_failures == 0 ? 0 : 1;
}
