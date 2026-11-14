// ============================================================================
//  《合金弹头》Metal Slug —— 重构版
//  语言：C++ (C++11 兼容，MSVC 2019+ / EasyX)
//  平台：Windows
//
//  ── 结构重构 ─────────────────────────────────────────────────────────────
//   · 去掉全部全局可变状态，收敛到 Game 结构体（唯一全局是只读资源表）
//   · 5 个近乎相同的子弹类  -> 1 个 Bullet + BulletKind
//   · 5 类敌人              -> 1 个 Enemy + EnemyKind + 配置表
//   · 主角绘制 64 个分支    -> 表驱动（4 姿态 x 2 武器 x 2 层级）
//   · 15 份重复碰撞代码     -> 1 套统一碰撞管线
//   · 3 份重复的 Boss 逻辑  -> 1 套 Boss 状态机
//   · 固定 Sleep(100) 轮询  -> 60FPS 固定步长逻辑 + 独立渲染
//
//  ── 修复的原版缺陷 ───────────────────────────────────────────────────────
//   B1 range-for 中 erase 当前迭代器（迭代器失效 = 未定义行为）
//   B2 子弹出界/命中后永不回收，无尽模式内存无限增长
//   B3 输入包在 _kbhit() 里，"按住"键位经常按不下去
//   B4 渲染过程中调用 rand()，敌人每帧随机取帧导致画面闪烁
//   B5 敌人士兵朝向画反了（永远背对玩家）
//   B6 子弹瞄准角在朝左时被整体取反，子弹往反方向飞
//   B7 putToggleImagePng 镜像索引用错宽高，贴边时越界读
//   B8 无敌帧缺失，被一发 grenade 直接秒杀
//   B9 帧率被 Sleep(100) 锁死在 10FPS
//
//  ── 玩法调整 ─────────────────────────────────────────────────────────────
//   · 相机跟随主角，关卡可真正横向推进
//   · 炮台有弹药上限，HUD 显示当前武器与余弹
//   · 受击无敌帧 + 击退 + 命中闪白 + 粒子 + 屏震
//   · Boss 会真的挥刀攻击，带血条 / 狂暴阶段 / 死亡动画
//   · HUD：血条、武器、分数、波次、关卡进度、Boss 血条、提示气泡
// ============================================================================

#include <graphics.h>
#include <conio.h>
#include <time.h>
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>     // printf：素材缺失提示
#include <cstdarg>    // va_list：tsprintf 变参
#include <cstring>
#include <mmsystem.h>
#include <Windows.h>
#include "EasyXPng.h"
#include "toggle.h"

#pragma warning(disable : 4996)
#pragma comment(lib, "Winmm.lib")

using namespace std;

// ============================================================================
//  0. 通用工具
// ============================================================================

static const float PI_F = 3.14159265f;

static inline int   clampi(int v, int lo, int hi)   { return v < lo ? lo : (v > hi ? hi : v); }
static inline float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }
static inline float lerpf(float a, float b, float t) { return a + (b - a) * t; }

// 宽字符格式化，兼容 Unicode / ANSI 两种编译配置
static inline void tsprintf(TCHAR* buf, size_t cap, const TCHAR* fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
#ifdef UNICODE
    vswprintf_s(buf, cap, fmt, ap);
#else
    vsprintf_s(buf, cap, fmt, ap);
#endif
    va_end(ap);
}

// ============================================================================
//  1. 配置（数值平衡集中在这里，方便调参）
// ============================================================================

namespace Cfg
{
    const int   LOGIC_HZ      = 60;     // 逻辑帧率
    const float DT           = 1.0f / 60.0f;
    const int   ANIM_DIV     = 6;      // 动画时钟 = 60 / 6 = 10 帧/秒，与原版一致

    // ---- 主角 ----
    const float PLAYER_SPEED      = 240.f;
    const float PLAYER_JUMP_V     = 480.f;
    const float PLAYER_GRAVITY    = 1200.f; // 约 0.8 秒落地，避免跳跃长时间漂浮
    const int   PLAYER_MAX_HP     = 100;
    const int   PLAYER_INVULN     = 45;    // 受击后无敌帧
    const int   PLAYER_RIFLE_CD   = 8;     // 步枪冷却（tick）
    const int   PLAYER_CANNON_CD  = 22;    // 炮台冷却（tick）
    const int   RIFLE_DAMAGE      = 1;
    const int   CANNON_DAMAGE     = 5;
    const float RIFLE_SPEED       = 560.f;
    const float CANNON_SPEED      = 480.f;
    const int   CANNON_AMMO_MAX   = 60;
    const int   FOOT_OVERLAP      = 7;     // 身体与脚部贴图的重叠量
    const int   MUZZLE_FRAMES     = 5;     // 开火时切换成"重武器"贴图的时长

    // 固定碰撞基准：主角的受击盒必须恒定，不能从「会随开火/换枪变化的贴图」推导，
    // 否则开火瞬间主角的受击盒会突变（详见 README 缺陷表 B2）。
    const int   PLAYER_HIT_W      = 62;    // 躯干碰撞宽（= 步枪身体贴图宽）
    const int   PLAYER_HIT_H      = 56;    // 躯干碰撞高（= 步枪身体贴图高）
    const float PLAYER_HIT_LR     = 0.16f; // 左右收缩比例
    const float PLAYER_HIT_TR     = 0.12f; // 上收缩比例

    // ---- 敌人 ----
    const float ENEMY_AGGRO_RANGE = 620.f; // 超出这个距离不开火
    const float ENEMY_KNIFE_STOP  = 30.f;  // 刀兵贴到这个距离就停下突刺
    const float ENEMY_KNIFE_RANGE = 85.f;  // 刀兵命中判定范围（要容得下 16px 击退）
    const int   ENEMY_KNIFE_CD    = 50;    // 刀兵攻击冷却
    const int   ENEMY_BOMB_CD     = 110;   // 敌机投弹间隔

    // ---- 关卡 ----
    const int   BOSS_WAVE_INTERVAL = 12;   // 无尽模式：每 12 波一场 Boss

    // ---- 道具 ----
    const int   BARREL_HP         = 12;    // 油桶血量（步枪约 12 发、炮台 3 发打爆）

    // ---- 关卡 ----
    const int   WAVE_INTERVAL_TICK = 70;   // 每波间隔
    const int   ENDLESS_MAX_ALIVE = 64;    // 无尽模式在场敌人的硬上限
}

// ============================================================================
//  2. 确定性随机数（原版在渲染里调 rand()，会让人物每帧乱跳）
// ============================================================================

struct Rng
{
    unsigned int s;

    void seed(unsigned int v) { s = v ? v : 0x9E3779B9u; }

    unsigned int next()
    {
        s ^= s << 13; s ^= s >> 17; s ^= s << 5;
        return s;
    }
    int range(int lo, int hi)              // 闭区间
    {
        if (hi <= lo) return lo;
        return lo + (int)(next() % (unsigned int)(hi - lo + 1));
    }
    float unit() { return (float)(next() & 0xFFFFFF) / (float)0x1000000; }
};

// ============================================================================
//  3. 输入（边缘触发 + 持续按住，彻底修掉 _kbhit() 吞键的问题）
// ============================================================================

struct Input
{
    bool cur[256];
    bool prev[256];

    void update()
    {
        // 只轮询游戏真正用到的按键
        static const int kTracked[] = {
            VK_LEFT, VK_RIGHT, VK_UP, VK_DOWN, VK_SPACE, VK_RETURN, VK_ESCAPE,
            VK_LBUTTON, '1', '2', 'A', 'D', 'W', 'S', 'E'
        };
        for (int i = 0; i < 256; ++i)
            prev[i] = cur[i];
        for (size_t k = 0; k < sizeof(kTracked) / sizeof(kTracked[0]); ++k)
            cur[kTracked[k]] = (GetAsyncKeyState(kTracked[k]) & 0x8000) != 0;
    }
    bool held(int vk)  const { return cur[vk & 0xFF]; }
    bool pressed(int vk) const { return cur[vk & 0xFF] && !prev[vk & 0xFF]; }
    void reset() { memset(cur, 0, sizeof(cur)); memset(prev, 0, sizeof(prev)); }
};

// ============================================================================
//  4. 资源表（只读全局，唯一保留的全局对象）
// ============================================================================

enum EnemyKind { EK_RIFLEMAN = 0, EK_KNIFE, EK_ROCKET, EK_THROWER, EK_PLANE, EK_COUNT };
enum BodyTier  { BT_NORMAL = 0, BT_HEAVY = 1 };
enum WeaponId  { WP_RIFLE = 0, WP_CANNON = 1 };
enum StanceId  { ST_STAND = 0, ST_RUN, ST_JUMP, ST_DOWN, ST_COUNT };

struct Assets
{
    IMAGE bg[4];                       // 4 张背景（3 关 + 无尽）

    IMAGE playerBody[2][2][4];         // [层级][武器][帧]
    int   playerBodyCount[2][2];
    IMAGE playerFoot[ST_COUNT][9];     // [姿态][帧]
    int   playerFootCount[ST_COUNT];

    IMAGE enemy[EK_COUNT][9];
    int   enemyCount[EK_COUNT];

    IMAGE deathA[5];                   // 士兵死亡动画 A
    IMAGE deathB[8];                   // 士兵死亡动画 B
    IMAGE bombBoom[8];                 // 炸弹爆炸动画
    IMAGE bossAttack[10];
    IMAGE bossAnger[2];
    IMAGE bossStand[4];
    IMAGE bossDie[5];
    IMAGE oldManBound[3];
    IMAGE oldManFree[4];

    IMAGE planePlayer;
    IMAGE bulletPlayer, bulletHeavy, bulletEnemyRifle, bulletEnemyRocket;
    IMAGE bulletGrenade, planeBomb, impact, letterE;

    IMAGE barrel, barrelBroken, sandbag;
    IMAGE win, fail;
    IMAGE splash;
    IMAGE menu, modeNormal, modeEndless, select;
    IMAGE roundBadge[3];
};

static Assets g_assets;

// 背景原始尺寸（用于按屏幕高度等比缩放）
static const int kBgNativeW[4] = { 8800, 1910, 1298, 624 };
static const int kBgNativeH[4] = { 1080, 224,  222,  219 };

// ============================================================================
//  5. 实体
// ============================================================================

enum BulletKind { BLK_RIFLE = 0, BLK_CANNON, BLK_ENEMY_RIFLE, BLK_ENEMY_ROCKET, BLK_GRENADE, BLK_BOMB, BLK_ENEMY_BOMB };

struct Bullet
{
    bool       alive;
    BulletKind kind;
    float      x, y;
    float      vx, vy;
    float      gravity;      // 加速度 px/s^2
    int        damage;
    bool       fromPlayer;
    int        age;
    int        maxAge;
};

struct Enemy
{
    EnemyKind kind;
    bool      alive;
    float     x, y;
    float     vx;
    int       hp, maxHp;
    bool      faceRight;
    int       anim;
    int       fireTimer;
    int       atkTicks;
    bool      atkHit;      // 本次攻击是否已结算伤害
    int       flash;
    int       muzzle;
    int       hurtCd;
};

struct Obstacle
{
    bool  alive;
    float x, y;
    int   hp, maxHp;
    bool  destructible;
    int   flash;
};

struct Particle
{
    bool  alive;
    float x, y, vx, vy;
    float life, maxLife;
    float size;
    float gravity;
    DWORD color;
};

struct FxAnim
{
    bool  alive;
    float x, y;
    int   set;      // 0 = deathA, 1 = deathB, 2 = bombBoom
    int   frame;
    int   ticks;
    bool  flip;
};

struct Player
{
    float x, y;                 // 世界坐标，(x,y) 为身体贴图左上角
    float vy;
    int   hp;
    bool  faceRight;
    int   stance;
    int   weapon;
    int   cannonAmmo;
    int   fireCooldown;
    int   invuln;
    int   muzzleFlash;
    bool  inPlane;
};

struct Plane
{
    float x, y;
    bool  available;
    bool  carried;              // 玩家正坐在机上
};

enum BossState { BSS_NONE = 0, BSS_ENTER, BSS_IDLE, BSS_ATTACK, BSS_DIE, BSS_DEAD };

struct Boss
{
    BossState state;
    float x, y;
    int   hp, maxHp;
    int   anim;
    int   atkTicks;
    int   cool;
    int   flash;
    bool  angry;
    bool  faceRight;
};

struct OldMan
{
    bool  active;
    bool  freed;
    float x, y;
    int   anim;
};

struct Toast
{
    bool  active;
    TCHAR text[48];
    float life;
};

// ============================================================================
//  6. 敌人配置表
// ============================================================================

struct EnemySpec
{
    int   hp;
    float speed;
    float stopRange;       // 靠近到这个距离就停下（枪兵停下开火，刀兵要贴脸）
    int   fireInterval;     // tick，0 表示不开火
    int   damage;
    int   score;
    int   idleFrames;       // 待机动画帧数
    int   atkStart;         // 攻击动画起始帧
    int   atkFrames;        // 攻击动画帧数
    int   atkTicks;         // 攻击动画时长
    bool  artFacesRight;    // 素材本身是否朝右
    bool  isPlane;
};

static const EnemySpec kEnemySpec[EK_COUNT] =
{
    // hp  speed  stop  fire  dmg score idle atkStart atkFrames atkTicks artRight isPlane
    {   3,  55.f, 300.f,  85,   8,  100,  4,   4,   3,   12,  false, false },  // 原图朝左
    {   2, 105.f,  30.f,   0,  12,  120,  5,   5,   2,   14,  false, false },
    {   4,  45.f, 300.f, 130,  18,  160,  2,   2,   3,   16,  false, false },
    {   3,  60.f, 300.f, 120,  14,  140,  5,   5,   4,   16,  false, false },
    {   6, 130.f,   0.f,   0,   0,  200,  1,   0,   0,    0,  false, true  },  // 敌机
};

// ============================================================================
//  7. 游戏主体
// ============================================================================

enum Phase { PH_SPLASH = 0, PH_MENU, PH_ROUND_SEL, PH_PLAY, PH_CLEAR, PH_WIN, PH_OVER };

struct LevelConfig
{
    int   background;
    int   enemyHpBonus;
    int   maxAlive;
    int   waveInterval;
    bool  hasBoss;
    bool  hasPlane;
    int   obstacleCount;      // 场上障碍数量（第一关为 0，与原版一致）
    int   weight[EK_COUNT];   // 随机波的敌人权重
    int   opening[EK_COUNT];  // 开局固定编队
};

static const LevelConfig kLevel[3] =
{
    //            bg hp alive wave boss plane obst   权重[步枪 刀 火箭 手雷 敌机]      开局编队
    { 0, 0, 6, 80, true, false, 0, { 5, 3, 2, 1, 0 }, { 1, 1, 1, 0, 0 } },
    { 1, 1, 8, 70, true, true,  5, { 3, 2, 3, 4, 2 }, { 1, 0, 1, 2, 1 } },
    { 2, 2, 9, 62, true, true,  6, { 4, 3, 3, 3, 4 }, { 1, 1, 1, 1, 2 } },
};

// 无尽模式：敌人构成与关卡无关，各类型均衡
static const int kEndlessWeight[EK_COUNT]  = { 4, 3, 3, 3, 3 };
static const int kEndlessOpening[EK_COUNT] = { 1, 1, 1, 1, 1 };

struct Game
{
    // ---- 屏幕 ----
    int   screenW, screenH;
    int   groundY;

    // ---- 状态机 ----
    Phase phase;
    int   round;             // 0..2
    int   menuSel;
    int   roundSel;
    int   frameClock;        // 逻辑帧计数
    float elapsed;
    float splashTime;        // 标题闪屏计时

    // ---- 世界 ----
    int   level;
    bool  endless;           // 是否无尽模式（由 resetRound 统一设定）
    float levelWidth;
    float cameraX;

    // ---- 实体 ----
    Player player;
    Plane  plane;
    Boss   boss;
    OldMan oldMan;
    vector<Enemy>     enemies;
    vector<Bullet>    bullets;
    vector<Obstacle>  obstacles;
    vector<Particle>  particles;
    vector<FxAnim>    fx;

    // ---- 统计 ----
    int   score, kills, waveIndex, waveTimer;
    int   enemyHpScale;
    int   nextBossWave;      // 无尽模式：下一场 Boss 在第几波出现
    int   bossCount;         // 本轮已经出现的 Boss 数量
    int   missingAssets;     // 加载失败的素材数

    // ---- 表现 ----
    float shakeX, shakeY, shakeAmp, shakeTime, shakeDur;
    Toast toast;
    Rng   rng;
    Input input;

    int   mouseX, mouseY;
    bool  mouseValid;
    bool  musicOn;

    // ---- 生命周期 ----
    void init();
    void loadAssets();
    void loadOne(IMAGE* dst, const TCHAR* path, int w = 0, int h = 0, bool resize = false);
    void loadSeries(IMAGE* dst, int count, const TCHAR* fmt, int first);
    void computeScreen();
    void resetRound(int roundIndex, bool endless);
    void spawnWave();
    void spawnOpeningWave();
    void spawnObstacles();
    void spawnEnemy(EnemyKind kind, float x);
    void spawnBullet(BulletKind kind, float x, float y, float vx, float vy, int damage, bool fromPlayer);
    void spawnFx(int set, float x, float y, bool flip);
    void spawnParticles(float x, float y, int count, DWORD color, float speed, float size);
    void explodeAt(float x, float y, int damage, bool fromPlayer, float radius);
    void endlessCycle();
    void shake(float amp, float dur);
    void showToast(const TCHAR* msg);

    // ---- 更新 ----
    void update();
    void updateMenu();
    void updateRoundSelect();
    void updatePlay();
    void updatePlayer(float dt);
    void updateEnemies(float dt);
    void updateBullets(float dt);
    void updateObstacles();
    void updateBoss(float dt);
    void updateCamera();
    void updateParticles(float dt);
    void updateFx();

    // ---- 绘制 ----
    void draw();
    void drawMenu();
    void drawRoundSelect();
    void drawWorld(int sx, int sy);
    void drawHud();
    void drawToast();
    void drawEndScreen();

    // ---- 碰撞 ----
    bool  boxHit(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) const;
    void  playerBox(float px, float py, float& l, float& t, float& r, float& b) const;
    void  enemyBox(const Enemy& e, float& l, float& t, float& r, float& b) const;
    void  enemyBoxAt(const Enemy& e, float x, float& l, float& t, float& r, float& b) const;
    void  bossAttackBox(float& l, float& t, float& r, float& b) const;
    bool  boxBlockedByObstacle(float l, float t, float r, float b) const;
    bool  playerBlocked(float px, float py) const;
    bool  bulletHitsObstacle(const Bullet& b, float prevY);
    void  fireWeapon();
    void  damagePlayer(int dmg, float fromX);
    void  killEnemy(Enemy& e);

    // ---- 辅助 ----
    bool  canInteract() const;
    float interactX() const;
    float aimAngle() const;
    void   muzzlePos(float& mx, float& my) const;
    int    playerBodyWidth() const;
    int    playerBodyHeight() const;
};

static Game g_game;

// ---- 资源加载 -------------------------------------------------------------

void Game::computeScreen()
{
    screenW = GetSystemMetrics(SM_CXFULLSCREEN);
    screenH = GetSystemMetrics(SM_CYFULLSCREEN);
    if (screenW <= 0) screenW = 1920;
    if (screenH <= 0) screenH = 1080;
    groundY = (int)(0.80f * screenH);
}

// 素材加载：EasyX 的 loadimage 失败时不会崩溃，只会让 IMAGE 宽高为 0，
// 表现出来就是"某块东西凭空消失"。这里统一记录并打印出来。
// 素材路径都是 ASCII，直接转成窄字符打印，避免 UNICODE 下 printf 的坑。
static void printAssetPath(const TCHAR* path)
{
    char buf[512];
    int i = 0;
    for (; path && path[i] && i < 511; ++i)
    {
#ifdef UNICODE
        // 宽字符转窄字符：素材路径都是 ASCII，超出范围的一律替换成 '?'
        buf[i] = (char)((path[i] < 128) ? path[i] : '?');
#else
        buf[i] = path[i];
#endif
    }
    buf[i] = 0;          // 必须在字符串末尾截断，否则会把栈上的垃圾字符一起打印出来
    printf("[素材缺失] %s\n", buf);
}

void Game::loadOne(IMAGE* dst, const TCHAR* path, int w, int h, bool resize)
{
    loadimage(dst, path, w, h, resize);
    if (dst->getwidth() <= 0 || dst->getheight() <= 0)
    {
        ++missingAssets;
        printAssetPath(path);
    }
}

void Game::loadSeries(IMAGE* dst, int count, const TCHAR* fmt, int first)
{
    TCHAR path[MAX_PATH];
    for (int i = 0; i < count; ++i)
    {
#ifdef UNICODE
        swprintf_s(path, MAX_PATH, fmt, first + i);
#else
        sprintf_s(path, MAX_PATH, fmt, first + i);
#endif
        loadOne(&dst[i], path);
    }
}

void Game::loadAssets()
{
    // 背景
    for (int i = 0; i < 4; ++i)
    {
        const TCHAR* name[4] = { _T("background1.png"), _T("background2.png"),
                                 _T("background3.png"), _T("bk4.png") };
        int w = kBgNativeW[i] * screenH / kBgNativeH[i];
        if (w < screenW) w = screenW;
        loadOne(&g_assets.bg[i], name[i], w, screenH, true);
        if (i == 2)
        {
            // 第三关原图的紫红色是天空底色键，显示时应替换为白色。
            DWORD* pixels = GetImageBuffer(&g_assets.bg[i]);
            const size_t count = (size_t)g_assets.bg[i].getwidth() * g_assets.bg[i].getheight();
            for (size_t p = 0; pixels && p < count; ++p)
            {
                const DWORD c = pixels[p];
                if (((c >> 16) & 255) >= 224 && ((c >> 8) & 255) <= 32 && (c & 255) >= 224)
                    pixels[p] = 0xffffffffu;
            }
        }
    }

    // 主角身体：[层级][武器][帧]
    loadSeries(g_assets.playerBody[BT_NORMAL][WP_RIFLE],  3, _T("./zhujue/body/leftqiang (%d).png"),   1);
    loadSeries(g_assets.playerBody[BT_NORMAL][WP_CANNON], 3, _T("./zhujue/body/leftpao (%d).png"),    1);
    loadSeries(g_assets.playerBody[BT_HEAVY][WP_RIFLE],  3, _T("./zhujue/body/leftdaqiang (%d).png"), 1);
    loadSeries(g_assets.playerBody[BT_HEAVY][WP_CANNON], 4, _T("./zhujue/body/leftdapao (%d).png"),  1);
    g_assets.playerBodyCount[BT_NORMAL][WP_RIFLE]  = 3;
    g_assets.playerBodyCount[BT_NORMAL][WP_CANNON] = 3;
    g_assets.playerBodyCount[BT_HEAVY][WP_RIFLE]  = 3;
    g_assets.playerBodyCount[BT_HEAVY][WP_CANNON] = 4;

    // 主角脚部：[姿态][帧]
    loadSeries(g_assets.playerFoot[ST_STAND], 1, _T("./zhujue/foot/leftstand.png"), 1);
    loadSeries(g_assets.playerFoot[ST_RUN],   9, _T("./zhujue/foot/leftrun (%d).png"),  1);
    loadSeries(g_assets.playerFoot[ST_JUMP],  5, _T("./zhujue/foot/leftjump (%d).png"), 1);
    loadSeries(g_assets.playerFoot[ST_DOWN],  6, _T("./zhujue/foot/leftdun (%d).png"), 1);
    g_assets.playerFootCount[ST_STAND] = 1;
    g_assets.playerFootCount[ST_RUN]   = 9;
    g_assets.playerFootCount[ST_JUMP]  = 5;
    g_assets.playerFootCount[ST_DOWN]  = 6;

    // 敌人
    loadSeries(g_assets.enemy[EK_RIFLEMAN], 7, _T("./qiang/qiang (%d).png"), 1);
    loadSeries(g_assets.enemy[EK_KNIFE],    7, _T("./dao/dao (%d).png"), 1);
    loadSeries(g_assets.enemy[EK_ROCKET],   5, _T("./liudan/liudan (%d).png"), 1);
    loadSeries(g_assets.enemy[EK_THROWER],  9, _T("./shouliudan/sld (%d).png"), 1);
    loadOne(&g_assets.enemy[EK_PLANE][0], _T("./feiji/feiji.png"));
    g_assets.enemyCount[EK_RIFLEMAN] = 7;
    g_assets.enemyCount[EK_KNIFE]    = 7;
    g_assets.enemyCount[EK_ROCKET]   = 5;
    g_assets.enemyCount[EK_THROWER]  = 9;
    g_assets.enemyCount[EK_PLANE]    = 1;

    // 死亡 / 爆炸
    loadSeries(g_assets.deathA,   5, _T("./die/die (%d).png"),  1);
    loadSeries(g_assets.deathB,   8, _T("./die2/die (%d).png"), 1);
    loadSeries(g_assets.bombBoom, 8, _T("./feiji/bomb (%d).png"), 1);

    // Boss
    loadSeries(g_assets.bossAttack, 10, _T("./boss/boss (%d).png"), 1);
    loadSeries(g_assets.bossAnger,   2, _T("./boss/bossanger (%d).png"), 1);
    loadSeries(g_assets.bossStand,   4, _T("./boss/bossstand (%d).png"), 1);
    loadSeries(g_assets.bossDie,     5, _T("./boss/bossdie (%d).png"), 1);

    // 老人
    loadSeries(g_assets.oldManBound, 3, _T("./oldman/laorenbang (%d).png"), 1);
    loadSeries(g_assets.oldManFree,  4, _T("./oldman/laorenfree (%d).png"), 1);

    // 单张资源
    loadOne(&g_assets.planePlayer,      _T("myFeiji.png"));
    loadOne(&g_assets.bulletPlayer,     _T("zidan.png"));
    loadOne(&g_assets.bulletHeavy,      _T("dazidan.png"));
    loadOne(&g_assets.bulletEnemyRifle, _T("enemyzidan.png"));
    loadOne(&g_assets.bulletEnemyRocket,_T("liudan.png"));
    loadOne(&g_assets.bulletGrenade,    _T("./shouliudan/sld.png"));
    loadOne(&g_assets.planeBomb,        _T("./feiji/zhadan.png"));
    loadOne(&g_assets.impact,           _T("boom.png"));
    loadOne(&g_assets.letterE,          _T("E.png"));
    loadOne(&g_assets.barrel,           _T("tong.png"));
    loadOne(&g_assets.barrelBroken,     _T("potong.png"));
    loadOne(&g_assets.sandbag,          _T("shadai.png"));
    loadOne(&g_assets.win,              _T("win.png"));
    loadOne(&g_assets.splash,            _T("firstImg.jpg"), screenW, screenH, true);
    loadOne(&g_assets.fail,             _T("fail.png"));
    loadOne(&g_assets.menu,             _T("menu.png"), screenW, screenH, true);
    loadOne(&g_assets.modeNormal,       _T("putong.png"));
    loadOne(&g_assets.modeEndless,      _T("wujing.png"));
    loadOne(&g_assets.select,           _T("select.png"));
    loadOne(&g_assets.roundBadge[0],    _T("round1.png"));
    loadOne(&g_assets.roundBadge[1],    _T("round2.png"));
    loadOne(&g_assets.roundBadge[2],    _T("round3.png"));

    // 素材自检：把结果打印出来，避免"素材没找到但画面上什么都没有"这种哑错误
    printf("[资源] 缺失素材 %d 个\n", missingAssets);
}

// ---- 关卡初始化 -----------------------------------------------------------

void Game::resetRound(int roundIndex, bool endlessMode)
{
    endless = endlessMode;
    const bool isEndless = endlessMode;
    level = isEndless ? 3 : roundIndex;
    // 关卡索引在这里统一，避免后续用 round 读配置时和 roundIndex 不一致
    if (!endless)
        round = roundIndex;
    const float bgScale = (float)screenH / (float)kBgNativeH[level];
    levelWidth = (float)kBgNativeW[level] * bgScale;
    if (levelWidth < (float)screenW + 800.f)
        levelWidth = (float)screenW + 800.f;

    cameraX = 0.f;
    enemies.clear();
    bullets.clear();
    obstacles.clear();
    particles.clear();
    fx.clear();

    waveIndex = 0;
    waveTimer = 40;
    nextBossWave = Cfg::BOSS_WAVE_INTERVAL;
    bossCount = 0;              // F4：Boss 计数按轮重置，避免跨轮/跨模式泄漏
    elapsed = 0.f;

    // 表现状态同样要清干净，别把上一轮的残影带进新一关
    toast.active = false;
    shakeAmp = 0.f;
    shakeTime = 0.f;
    shakeX = 0.f;
    shakeY = 0.f;

    // 主角
    player.hp = (player.hp > 0) ? player.hp : Cfg::PLAYER_MAX_HP;   // 兜底：血量非法时给满
    player.x = 220.f;
    player.y = (float)groundY;
    player.vy = 0.f;
    player.stance = ST_STAND;
    player.faceRight = true;
    player.weapon = WP_RIFLE;
    player.cannonAmmo = Cfg::CANNON_AMMO_MAX;
    player.fireCooldown = 0;
    player.invuln = 0;
    player.muzzleFlash = 0;
    player.inPlane = false;

    // 玩家飞机
    plane.available = isEndless || (roundIndex >= 1);
    plane.x = isEndless ? 520.f : 900.f;
    plane.y = (float)(groundY - 40);
    plane.carried = false;
    if (roundIndex == 1)                 // 第二关开局就在机上
    {
        plane.carried = true;
        player.inPlane = true;
        player.x = plane.x;
        player.y = plane.y;
    }

    // Boss / 老人
    boss.state = BSS_NONE;
    boss.faceRight = false;
    boss.hp = boss.maxHp = 0;
    oldMan.active = false;
    oldMan.freed = false;

    // 障碍：按关卡配置摆放（原版第一关无障碍，第二关 5 个，第三关 6 个）
    spawnObstacles();

    enemyHpScale = isEndless ? 0 : kLevel[roundIndex].enemyHpBonus;

    // 开局编队（每关构成不同）
    spawnOpeningWave();

    showToast(isEndless ? _T("无尽模式 开始！") : (roundIndex == 0 ? _T("第一关") : (roundIndex == 1 ? _T("第二关") : _T("第三关"))));
}

// 障碍摆放：按关卡配置生成掩体。
// 原版：第一关无障碍，第二关 2 沙袋 + 3 油桶，第三关 1 沙袋 + 5 油桶。
// 障碍必须落在关卡宽度内 —— 无尽模式的关卡宽度比普通关卡小得多，
// 固定间距会把掩体摆到关卡外，玩家永远碰不到（F3 / 越界问题）。
void Game::spawnObstacles()
{
    obstacles.clear();
    const int count = endless ? 8 : kLevel[round].obstacleCount;
    if (count <= 0)
        return;

    const float first = 900.f;
    const float last  = max(first + 120.f, levelWidth - 420.f);
    const float step  = (count > 1) ? (last - first) / (float)(count - 1) : 0.f;
    for (int i = 0; i < count; ++i)
    {
        Obstacle o;
        o.alive = true;
        // 2/3 是可破坏的油桶，1/3 是挡子弹的沙袋
        o.destructible = (i % 3 != 2);
        o.x = clampf(first + (float)i * step + (float)rng.range(-70, 70), first, last);
        o.y = (float)(groundY + (o.destructible ? 10 : 20));
        o.maxHp = o.hp = o.destructible ? Cfg::BARREL_HP : 0;
        o.flash = 0;
        obstacles.push_back(o);
    }
}

// ---- 通用生成 -------------------------------------------------------------

void Game::spawnBullet(BulletKind kind, float x, float y, float vx, float vy, int damage, bool fromPlayer)
{
    Bullet b;
    b.alive = true;
    b.kind = kind;
    b.x = x; b.y = y;
    b.vx = vx; b.vy = vy;
    b.gravity = 0.f;
    b.damage = damage;
    b.fromPlayer = fromPlayer;
    b.age = 0;
    b.maxAge = 600;

    switch (kind)
    {
    case BLK_RIFLE:
        b.maxAge = 120; break;
    case BLK_CANNON:
        b.maxAge = 120; break;
    case BLK_ENEMY_RIFLE:
        b.maxAge = 400; break;
    case BLK_ENEMY_ROCKET:
        b.maxAge = 400; break;
    case BLK_GRENADE:
        b.gravity = 200.f;
        b.vy = -180.f;
        b.maxAge = 400; break;
    case BLK_BOMB:
        b.gravity = 0.f;
        b.vy = 160.f;
        b.maxAge = 300; break;
    case BLK_ENEMY_BOMB:
        b.gravity = 0.f;
        b.vy = 170.f;
        b.maxAge = 300; break;
    }

    bullets.push_back(b);
}

void Game::spawnEnemy(EnemyKind kind, float x)
{
    Enemy e;
    e.kind = kind;
    e.alive = true;
    e.x = x;
    e.y = (float)(kind == EK_PLANE ? (int)(0.18f * screenH) : groundY);
    e.vx = (kind == EK_PLANE) ? -130.f : 0.f;   // 敌机向左横穿
    e.maxHp = kEnemySpec[kind].hp + enemyHpScale;
    e.hp = e.maxHp;
    e.faceRight = false;
    e.anim = 0;
    e.fireTimer = rng.range(20, 70);
    e.atkTicks = 0;
    e.atkHit = true;
    e.flash = 0;
    e.muzzle = 0;
    e.hurtCd = 0;
    enemies.push_back(e);
}

// 开局固定编队：每关的起始敌人构成不同（A7）
void Game::spawnOpeningWave()
{
    const int* opening = endless ? kEndlessOpening : kLevel[round].opening;
    int slot = 0;
    for (int k = 0; k < EK_COUNT; ++k)
    {
        for (int i = 0; i < opening[k]; ++i)
        {
            const float x = cameraX + 520.f + (float)slot * 180.f;
            spawnEnemy((EnemyKind)k, x);
            ++slot;
        }
    }
}

void Game::spawnWave()
{
    int maxAlive = endless ? min(Cfg::ENDLESS_MAX_ALIVE, 7 + waveIndex / 6) : kLevel[round].maxAlive;
    if ((int)enemies.size() >= maxAlive)
        return;

    const int* weight = endless ? kEndlessWeight : kLevel[round].weight;
    int totalWeight = 0;
    for (int k = 0; k < EK_COUNT; ++k)
        totalWeight += weight[k];

    const int n = min(rng.range(1, 3), maxAlive - (int)enemies.size());
    for (int i = 0; i < n; ++i)
    {
        // 按关卡权重抽敌人类型（各关构成不同）
        int r = rng.range(0, totalWeight - 1);
        EnemyKind kind = EK_RIFLEMAN;
        for (int k = 0; k < EK_COUNT; ++k)
        {
            if (r < weight[k]) { kind = (EnemyKind)k; break; }
            r -= weight[k];
        }
        const float x = cameraX + (float)screenW + (float)rng.range(20, 160) + (float)i * 70.f;
        spawnEnemy(kind, x);
    }
}

void Game::spawnFx(int set, float x, float y, bool flip)
{
    FxAnim f;
    f.alive = true;
    f.x = x; f.y = y;
    f.set = set;
    f.frame = 0;
    f.ticks = 0;
    f.flip = flip;
    fx.push_back(f);
}

void Game::spawnParticles(float x, float y, int count, DWORD color, float speed, float size)
{
    for (int i = 0; i < count; ++i)
    {
        Particle p;
        p.alive = true;
        p.x = x; p.y = y;
        const float a = rng.unit() * 2.f * PI_F;
        const float sp = speed * (0.35f + 0.65f * rng.unit());
        p.vx = (float)cos(a) * sp;
        p.vy = (float)sin(a) * sp - speed * 0.35f;
        p.maxLife = p.life = 0.30f + 0.35f * rng.unit();
        p.size = size * (0.6f + 0.8f * rng.unit());
        p.gravity = 420.f;
        p.color = color;
        particles.push_back(p);
    }
}

void Game::shake(float amp, float dur)
{
    if (amp > shakeAmp)
    {
        shakeAmp = amp;
        shakeTime = dur;
        shakeDur = dur;
    }
}

void Game::showToast(const TCHAR* msg)
{
    toast.active = true;
    toast.life = 2.0f;
    int i = 0;
    for (; i < 47 && msg[i]; ++i)
        toast.text[i] = msg[i];
    toast.text[i] = 0;
}

// ---- 碰撞辅助 -------------------------------------------------------------

// 主角碰撞盒：用固定常量，而不是从"当前正在画的贴图"推导。
// 否则开火瞬间贴图切到重武器、尺寸一变，主角的受击盒就会突变（B2）。
void Game::playerBox(float px, float py, float& l, float& t, float& r, float& b) const
{
    if (player.inPlane)
    {
        const float w = (float)g_assets.planePlayer.getwidth();
        const float h = (float)g_assets.planePlayer.getheight();
        l = px + w * 0.10f; r = px + w * 0.90f;
        t = py + h * 0.15f; b = py + h * 0.85f;
        return;
    }
    const int fh = g_assets.playerFootCount[player.stance] ? g_assets.playerFoot[player.stance][0].getheight() : 30;
    l = px + Cfg::PLAYER_HIT_W * Cfg::PLAYER_HIT_LR;
    r = px + Cfg::PLAYER_HIT_W * (1.f - Cfg::PLAYER_HIT_LR);
    t = py + Cfg::PLAYER_HIT_H * Cfg::PLAYER_HIT_TR;
    b = py + Cfg::PLAYER_HIT_H - Cfg::FOOT_OVERLAP + fh * 0.45f;
}

void Game::enemyBoxAt(const Enemy& e, float x, float& l, float& t, float& r, float& b) const
{
    const IMAGE& img = g_assets.enemy[e.kind][0];
    const float w = (float)img.getwidth();
    const float h = (float)img.getheight();
    l = x + w * 0.22f;
    r = x + w * 0.78f;
    t = e.y + h * 0.20f;
    b = e.y + h * 0.92f;
}

void Game::enemyBox(const Enemy& e, float& l, float& t, float& r, float& b) const
{
    enemyBoxAt(e, e.x, l, t, r, b);
}

bool Game::boxHit(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) const
{
    return (ax < bx + bw) && (ax + aw > bx) && (ay < by + bh) && (ay + ah > by);
}

// 子弹与掩体的碰撞。
// 返回 true 表示子弹被掩体挡住（不论油桶还是沙袋）。
// 可破坏的油桶会掉血并在被打爆时炸出碎片；沙袋不可破坏，但同样会吃掉子弹。
//
// prevY 是子弹上一帧的 y：手雷这类高速下落的物体必须用"这一帧走过的区间"去撞掩体，
// 否则它会一步跨过掩体的高度带，碰撞形同虚设。
bool Game::bulletHitsObstacle(const Bullet& b, float prevY)
{
    const float yTop = min(prevY, b.y);
    const float yBot = max(prevY, b.y);
    for (size_t i = 0; i < obstacles.size(); ++i)
    {
        Obstacle& o = obstacles[i];
        if (!o.alive) continue;
        const IMAGE& img = o.destructible ? g_assets.barrel : g_assets.sandbag;
        const float ow = (float)img.getwidth();
        const float oh = (float)img.getheight();
        if (b.x < o.x || b.x > o.x + ow) continue;
        if (yBot < o.y - oh * 0.45f || yTop > o.y + oh * 0.45f) continue;

        if (o.destructible)
        {
            o.hp -= b.damage;
            o.flash = 5;
            if (o.hp <= 0)
            {
                o.alive = false;
                spawnParticles(o.x + ow * 0.5f, o.y, 28, RGB(255, 170, 60), 280.f, 5.f);
                shake(10.f, 0.30f);
            }
        }
        return true;
    }
    return false;
}

// 统一的爆炸处理（炸弹、手雷、炮弹爆炸共用）
void Game::explodeAt(float x, float y, int damage, bool fromPlayer, float radius)
{
    spawnFx(2, x - 40.f, y - 60.f, false);
    spawnParticles(x, y, 24, RGB(255, 170, 60), 280.f, 5.f);
    shake(10.f, 0.30f);

    if (fromPlayer)
    {
        for (size_t j = 0; j < enemies.size(); ++j)
        {
            Enemy& e = enemies[j];
            if (!e.alive) continue;
            const float ex = e.x + 40.f;
            const float ey = e.y + 30.f;
            const float ddx = ex - x;
            const float ddy = ey - y;
            if (ddx * ddx + ddy * ddy < radius * radius)
            {
                e.hp -= damage;
                e.flash = 8;
                if (e.hp <= 0)
                    killEnemy(e);
            }
        }
    }
    else
    {
        float l, t, r, b;
        playerBox(player.x, player.y, l, t, r, b);
        const float ddx = (l + r) * 0.5f - x;
        const float ddy = (t + b) * 0.5f - y;
        if (ddx * ddx + ddy * ddy < radius * radius)
            damagePlayer(damage, x);
    }
}

// 障碍碰撞：给一个盒子和障碍比，判定是否被挡住。
// 碰撞盒与贴图对齐（贴图画在 [o.x, o.x+ow] x [o.y, o.y+oh]）。
bool Game::boxBlockedByObstacle(float l, float t, float r, float b) const
{
    for (size_t i = 0; i < obstacles.size(); ++i)
    {
        const Obstacle& o = obstacles[i];
        if (!o.alive) continue;
        const IMAGE& img = o.destructible ? g_assets.barrel : g_assets.sandbag;
        const float ow = (float)img.getwidth();
        const float oh = (float)img.getheight();
        if (boxHit(l, t, r - l, b - t, o.x, o.y, ow, oh))
            return true;
    }
    return false;
}

// 主角避障：用主角自己的碰撞盒。
// 油桶、沙袋都会挡住主角，跳跃越过沙袋后即可穿行。
bool Game::playerBlocked(float px, float py) const
{
    float l, t, r, b;
    playerBox(px, py, l, t, r, b);
    return boxBlockedByObstacle(l, t, r, b);
}

// 绘制用的贴图尺寸（会随武器/开火变化），碰撞盒请用 playerBox()
int Game::playerBodyWidth() const
{
    const IMAGE& img = g_assets.playerBody[player.muzzleFlash > 0 ? BT_HEAVY : BT_NORMAL][player.weapon][0];
    return img.getwidth() ? img.getwidth() : Cfg::PLAYER_HIT_W;
}

int Game::playerBodyHeight() const
{
    const IMAGE& img = g_assets.playerBody[player.muzzleFlash > 0 ? BT_HEAVY : BT_NORMAL][player.weapon][0];
    return img.getheight() ? img.getheight() : Cfg::PLAYER_HIT_H;
}

void Game::muzzlePos(float& mx, float& my) const
{
    if (player.inPlane)
    {
        const int pw = g_assets.planePlayer.getwidth();
        const int ph = g_assets.planePlayer.getheight();
        mx = player.faceRight ? player.x + pw * 0.92f : player.x + pw * 0.08f;
        my = player.y + ph * 0.45f;
        return;
    }
    const int bw = playerBodyWidth();
    const int bh = playerBodyHeight();
    mx = player.faceRight ? player.x + bw * 0.92f : player.x + bw * 0.08f;
    my = player.y + bh * 0.45f;
}

float Game::aimAngle() const
{
    float mx, my;
    muzzlePos(mx, my);
    if (!mouseValid)
        return player.faceRight ? 0.f : PI_F;
    // 鼠标落在身体与枪口之间时，不允许子弹从向右的枪口反向飞出。
    const float rawDx = (float)mouseX + cameraX - mx;
    const float dx = player.faceRight ? max(1.f, rawDx) : min(-1.f, rawDx);
    const float dy = (float)mouseY - my;
    if (dx * dx + dy * dy < 1.f)
        return player.faceRight ? 0.f : PI_F;
    return (float)atan2(dy, dx);
}

// ---- 初始化 ---------------------------------------------------------------

// 无尽模式的一轮结束：救完人后重置状态，继续下一轮
void Game::endlessCycle()
{
    boss.state = BSS_NONE;
    boss.hp = boss.maxHp = 0;
    boss.angry = false;
    oldMan.active = false;
    oldMan.freed = false;

    // 回到起点开新一段，并把 Boss 触发计数重置，
    // 这样下一场 Boss 会在新的 12 波节点再次出现。
    player.x = 220.f;
    player.y = (float)groundY;
    player.vy = 0.f;
    player.inPlane = false;
    plane.carried = false;
    plane.x = 520.f;
    cameraX = 0.f;
    waveIndex = 0;
    waveTimer = 40;
    nextBossWave = Cfg::BOSS_WAVE_INTERVAL;
    enemies.clear();
    bullets.clear();
    spawnObstacles();          // F3：新一轮必须重新铺掩体，否则第二轮起掩体全失
    showToast(_T("下一轮开始！"));
}

void Game::init()
{
    rng.seed((unsigned int)time(0));
    computeScreen();
    loadAssets();

    phase = PH_SPLASH;
    splashTime = 0.f;
    round = 0;
    menuSel = 0;
    roundSel = 0;
    score = 0;
    kills = 0;
    musicOn = true;
    mouseValid = false;
    input.reset();
}


// ============================================================================
//  8. 更新
// ============================================================================

static inline float fabsf_(float v) { return v < 0.f ? -v : v; }

void Game::update()
{
    input.update();
    ++frameClock;

    // EasyX 的鼠标位置只在有消息时更新，这里保留最后一次位置
    while (MouseHit())
    {
        MOUSEMSG m = GetMouseMsg();
        mouseX = m.x;
        mouseY = m.y;
        mouseValid = true;
    }

    if (input.pressed(VK_ESCAPE) &&
        (phase == PH_PLAY || phase == PH_CLEAR || phase == PH_WIN || phase == PH_OVER))
    {
        phase = endless ? PH_MENU : PH_ROUND_SEL;
        if (endless) menuSel = 1;
        else roundSel = round;
        return;
    }

    switch (phase)
    {
    case PH_SPLASH:
        splashTime += Cfg::DT;
        if (splashTime > 1.6f || input.pressed(VK_RETURN) || input.pressed(VK_SPACE))
            phase = PH_MENU;
        break;
    case PH_MENU:       updateMenu(); break;
    case PH_ROUND_SEL:  updateRoundSelect(); break;
    case PH_PLAY:
    case PH_CLEAR:      updatePlay(); break;
    case PH_WIN:
    case PH_OVER:
        if (input.pressed(VK_RETURN) || input.pressed(VK_SPACE))
        {
            // 原版行为：普通模式失败后回到选关界面，无尽模式回到主菜单（A8）
            if (!endless)
            {
                phase = PH_ROUND_SEL;
                roundSel = round;
            }
            else
            {
                phase = PH_MENU;
                menuSel = 1;
            }
        }
        break;
    }

    updateParticles(Cfg::DT);
    updateFx();

    // 提示气泡的寿命在逻辑帧里递减，与渲染帧率解耦
    if (toast.active)
    {
        toast.life -= Cfg::DT;
        if (toast.life <= 0.f)
            toast.active = false;
    }

    // 屏幕震动
    if (shakeTime > 0.f)
    {
        shakeTime -= Cfg::DT;
        const float dur = (shakeDur > 0.f) ? shakeDur : 1.f;
        const float k = clampf(shakeTime / dur, 0.f, 1.f);
        shakeX = (rng.unit() * 2.f - 1.f) * shakeAmp * k;
        shakeY = (rng.unit() * 2.f - 1.f) * shakeAmp * k;
        if (shakeTime <= 0.f)
        {
            shakeAmp = 0.f;
            shakeX = shakeY = 0.f;
        }
    }
}

void Game::updateMenu()
{
    if (input.pressed(VK_SPACE))
        menuSel = 1 - menuSel;

    if (input.pressed(VK_RETURN))
    {
        score = 0;
        kills = 0;
        player.hp = Cfg::PLAYER_MAX_HP;
        if (menuSel == 0)
        {
            endless = false;
            phase = PH_ROUND_SEL;
            roundSel = 0;
        }
        else
        {
            resetRound(0, true);
            phase = PH_PLAY;
        }
    }
}

void Game::updateRoundSelect()
{
    if (input.pressed(VK_SPACE))
        roundSel = (roundSel + 1) % 3;
    if (input.pressed(VK_ESCAPE))
        phase = PH_MENU;
    if (input.pressed(VK_RETURN))
    {
        round = roundSel;
        score = 0;
        kills = 0;
        player.hp = Cfg::PLAYER_MAX_HP;
        resetRound(round, false);
        phase = PH_PLAY;
    }
}

void Game::updatePlay()
{
    const float dt = Cfg::DT;
    elapsed += dt;

    // 关卡已完成：等待确认进入下一关
    if (phase == PH_CLEAR)
    {
        if (input.pressed(VK_RETURN) || input.pressed(VK_SPACE) || input.pressed('E'))
        {
            if (!endless)
            {
                if (round < 2)
                {
                    round++;
                    player.hp = min(Cfg::PLAYER_MAX_HP, player.hp + 30);
                    resetRound(round, false);
                    phase = PH_PLAY;
                }
                else
                {
                    phase = PH_WIN;
                }
            }
            else
            {
                phase = PH_OVER;
            }
        }
        return;
    }

    updatePlayer(dt);
    updateEnemies(dt);
    updateBullets(dt);
    updateObstacles();
    updateBoss(dt);
    updateCamera();

    // 过关时清空残留弹幕（B4）：必须放在所有实体更新之后，
    // 否则同一帧里敌人刚发射的子弹会留到下一帧被冻在过关画面上（F2）。
    if (phase == PH_CLEAR)
        bullets.clear();

    // Boss 触发
    if (phase == PH_PLAY && boss.state == BSS_NONE)
    {
        bool wantsBoss = false;
        if (!endless)
            wantsBoss = kLevel[round].hasBoss;
        else
            wantsBoss = (waveIndex >= nextBossWave);

        if (wantsBoss && cameraX > levelWidth - (float)screenW - 760.f)
        {
            boss.state = BSS_ENTER;
            boss.x = levelWidth - 300.f;
            ++bossCount;
            if (endless)
                nextBossWave = waveIndex + Cfg::BOSS_WAVE_INTERVAL;
            // 让 Boss 的脚和主角站在同一条地平线上
            {
                const int bossH = g_assets.bossAttack[0].getheight();
                const int footH = (g_assets.playerFootCount[ST_STAND] > 0)
                    ? g_assets.playerFoot[ST_STAND][0].getheight() : 33;
                const int playerBottom = groundY + playerBodyHeight() - Cfg::FOOT_OVERLAP + footH;
                boss.y = (float)(playerBottom - bossH);
            }
            boss.maxHp = 80 + enemyHpScale * 20;
            boss.hp = boss.maxHp;
            boss.anim = 0;
            boss.atkTicks = 0;
            boss.cool = 50;
            boss.flash = 0;
            boss.angry = false;
            boss.faceRight = false;
            showToast(endless ? _T("BOSS 出现！无尽模式") : _T("BOSS 出现！"));
        }
    }

    // 波次
    if (phase == PH_PLAY)
    {
        if (waveTimer > 0)
        {
            waveTimer--;
        }
        else
        {
            waveTimer = endless
                ? max(28, Cfg::WAVE_INTERVAL_TICK - waveIndex * 2)
                : kLevel[round].waveInterval;
            spawnWave();
            waveIndex++;
        }
    }

    if (player.hp <= 0 && phase != PH_OVER)
    {
        player.hp = 0;
        phase = PH_OVER;
        showToast(_T("任务失败"));
    }
}

void Game::updatePlayer(float dt)
{
    Player& p = player;
    if (p.invuln > 0) p.invuln--;
    if (p.fireCooldown > 0) p.fireCooldown--;
    if (p.muzzleFlash > 0) p.muzzleFlash--;

    // 武器切换（边缘触发，不会连发）
    if (input.pressed('1'))
        p.weapon = WP_RIFLE;
    if (input.pressed('2'))
    {
        if (p.cannonAmmo > 0)
            p.weapon = WP_CANNON;
        else
            showToast(_T("炮弹已耗尽"));
    }
    if (p.weapon == WP_CANNON && p.cannonAmmo <= 0)
    {
        p.weapon = WP_RIFLE;
        showToast(_T("炮弹已耗尽"));
    }

    const bool left  = input.held(VK_LEFT)  || input.held('A');
    const bool right = input.held(VK_RIGHT) || input.held('D');
    const bool up    = input.held(VK_UP)    || input.held('W');
    const bool down  = input.held(VK_DOWN)  || input.held('S');

    // ---- 机上 ----
    if (p.inPlane)
    {
        const int moveX = (right ? 1 : 0) - (left ? 1 : 0);
        const int moveY = (down ? 1 : 0) - (up ? 1 : 0);
        const float diagonal = moveX != 0 && moveY != 0 ? 0.70710678f : 1.f;
        p.x += moveX * Cfg::PLAYER_SPEED * 1.15f * diagonal * dt;
        p.y += moveY * 210.f * diagonal * dt;
        if (moveX != 0) p.faceRight = moveX > 0;

        p.x = clampf(p.x, cameraX + 40.f, cameraX + (float)screenW - 40.f);
        p.x = clampf(p.x, 60.f, levelWidth - 60.f);
        p.y = clampf(p.y, 40.f, (float)groundY - 60.f);
        plane.x = p.x;
        plane.y = p.y;

        if (input.pressed(VK_SPACE))
        {
            spawnBullet(BLK_BOMB, p.x, p.y + 24.f, 0.f, 170.f, 12, true);
            p.muzzleFlash = 8;
        }
        // 机上同样可以开火
        if (input.held(VK_LBUTTON))
            fireWeapon();
        if (input.pressed('E'))
        {
            p.inPlane = false;
            plane.carried = false;
            p.x = plane.x;
            p.y = (float)groundY;
            p.vy = 0.f;
            showToast(_T("已离开飞机"));
        }
        return;
    }

    // ---- 地面 ----
    const bool onGround = (p.y >= (float)groundY - 0.5f);
    bool moving = false;

    if (left && !right)
    {
        const float nx = p.x - Cfg::PLAYER_SPEED * dt;
        if (!playerBlocked(nx, p.y)) { p.x = nx; moving = true; }
        p.faceRight = false;
    }
    if (right && !left)
    {
        const float nx = p.x + Cfg::PLAYER_SPEED * dt;
        if (!playerBlocked(nx, p.y)) { p.x = nx; moving = true; }
        p.faceRight = true;
    }

    if ((input.pressed(VK_UP) || input.pressed('W')) && onGround)
        p.vy = -Cfg::PLAYER_JUMP_V;

    p.vy += Cfg::PLAYER_GRAVITY * dt;
    p.y += p.vy * dt;
    if (p.y >= (float)groundY)
    {
        p.y = (float)groundY;
        p.vy = 0.f;
    }
    p.x = clampf(p.x, 60.f, levelWidth - 60.f);

    // 姿态
    if (p.y < (float)groundY - 0.5f)
        p.stance = ST_JUMP;
    else if (down)
        p.stance = ST_DOWN;
    else if (moving)
        p.stance = ST_RUN;
    else
        p.stance = ST_STAND;

    // 射击
    if (input.held(VK_LBUTTON))
        fireWeapon();

    // 交互
    if (input.pressed('E'))
    {
        if (oldMan.active && !oldMan.freed && fabsf_(p.x - oldMan.x) < 95.f)
        {
            oldMan.freed = true;
            score += 500;
            showToast(_T("人质已获救，按 E 或回车继续"));
            if (endless) endlessCycle();
            else phase = PH_CLEAR;
        }
        else if (plane.available && !plane.carried &&
            fabsf_(p.x - plane.x) < 95.f && p.y >= (float)groundY - 90.f)
        {
            p.inPlane = true;
            plane.carried = true;
            p.x = plane.x;
            p.y = plane.y;
            p.vy = 0.f;
            showToast(_T("登上飞机"));
        }
    }
}

void Game::updateEnemies(float dt)
{
    for (size_t i = 0; i < enemies.size(); ++i)
    {
        Enemy& e = enemies[i];
        if (!e.alive) continue;

        if (e.flash > 0) e.flash--;
        if (e.muzzle > 0) e.muzzle--;
        if (e.hurtCd > 0) e.hurtCd--;

        const EnemySpec& sp = kEnemySpec[e.kind];
        const float dx = player.x - e.x;
        const float adx = fabsf_(dx);

        // ---- 敌机 ----
        if (sp.isPlane)
        {
            // 敌机恒定向左飞行，朝向必须由速度决定（A5）。
            // 若按"朝玩家"推导，飞过玩家之后会被镜像成朝右，像是掉头。
            e.faceRight = (e.vx > 0.f);
            e.x += e.vx * dt;
            if (e.fireTimer > 0)
            {
                e.fireTimer--;
            }
            else
            {
                e.fireTimer = Cfg::ENEMY_BOMB_CD;
                spawnBullet(BLK_ENEMY_BOMB, e.x, e.y + 40.f, 0.f, 170.f, 10, false);
            }
            e.anim = 0;
            if (e.x < cameraX - 500.f || e.x > cameraX + (float)screenW + 700.f)
                e.alive = false;
            continue;
        }

        // ---- 地面敌人 ----
        e.faceRight = (dx > 0);

        // 每种敌人有自己的"停下距离"（A1）：
        // 枪兵停在远处开火，刀兵必须贴到脸上才能砍人。
        // 敌人移动不与掩体做碰撞（F1）：掩体只挡子弹和主角。
        // 原版用 checkIsDangZhu() 把"整个 update"冻结在障碍前，
        // 结果是刀兵永远走不到玩家面前、近战技能形同虚设。
        // 这里改成敌人直接穿过掩体：既保住了近战，也彻底消灭"卡死"这一类缺陷。
        if (adx > sp.stopRange)
            e.x += (dx > 0 ? sp.speed : -sp.speed) * dt;
        e.x = clampf(e.x, cameraX - 200.f, cameraX + (float)screenW + 300.f);

        if (e.fireTimer > 0)
            e.fireTimer--;

        if (sp.fireInterval > 0 && adx < Cfg::ENEMY_AGGRO_RANGE && e.fireTimer == 0)
        {
            e.fireTimer = sp.fireInterval;
            e.atkTicks = sp.atkTicks;
            e.muzzle = 8;
            const float spriteW = (float)g_assets.enemy[e.kind][sp.atkStart].getwidth();
            const float mx = e.x + (e.faceRight ? spriteW : 0.f);
            const float my = e.y + 34.f;
            float pl, pt, pr, pb;
            playerBox(player.x, player.y, pl, pt, pr, pb);
            const float aim = (float)atan2((pt + pb) * 0.5f - my, (pl + pr) * 0.5f - mx);

            if (e.kind == EK_RIFLEMAN)
                spawnBullet(BLK_ENEMY_RIFLE, mx, my, 240.f * (float)cos(aim), 240.f * (float)sin(aim), sp.damage, false);
            else if (e.kind == EK_ROCKET)
                spawnBullet(BLK_ENEMY_ROCKET, mx, my, 200.f * (float)cos(aim), 200.f * (float)sin(aim), sp.damage, false);
            else if (e.kind == EK_THROWER)
                spawnBullet(BLK_GRENADE, mx, my, e.faceRight ? 240.f : -240.f, 0.f, sp.damage, false);
        }

        // 刀兵近战：贴身后起手，伤害在挥砍动画中段结算
        if (e.kind == EK_KNIFE && adx <= Cfg::ENEMY_KNIFE_STOP + 30.f &&
            e.hurtCd <= 0 && e.atkTicks <= 0)
        {
            e.atkTicks = sp.atkTicks;
            e.atkHit = false;
            e.hurtCd = Cfg::ENEMY_KNIFE_CD;
        }

        // 动画：待机循环，攻击时切到攻击帧
        if (e.atkTicks > 0)
        {
            const int atkElapsed = sp.atkTicks - e.atkTicks;
            e.anim = sp.atkStart + (atkElapsed * sp.atkFrames) / sp.atkTicks;
            // 挥砍到一半才判定命中，手感更准
            if (e.kind == EK_KNIFE && !e.atkHit && atkElapsed * 2 >= sp.atkTicks)
            {
                e.atkHit = true;
                if (adx <= Cfg::ENEMY_KNIFE_RANGE)
                {
                    float pl, pt, pr, pb, el, et, er, eb;
                    playerBox(player.x, player.y, pl, pt, pr, pb);
                    enemyBox(e, el, et, er, eb);
                    if (pb > et && pt < eb) damagePlayer(sp.damage, e.x);
                    spawnParticles(e.x + 20.f, e.y + 40.f, 6, RGB(255, 240, 200), 150.f, 3.f);
                }
            }
            e.atkTicks--;
        }
        else
        {
            e.anim = (int)(frameClock / Cfg::ANIM_DIV) % sp.idleFrames;
        }
    }
}

void Game::updateBullets(float dt)
{
    for (size_t i = 0; i < bullets.size(); ++i)
    {
        Bullet& b = bullets[i];
        if (!b.alive) continue;

        const float prevY = b.y;
        b.vy += b.gravity * dt;
        b.x += b.vx * dt;
        b.y += b.vy * dt;
        b.age++;

        // 机上玩家使用飞机碰撞盒；炸弹也可在空中直接命中飞机。
        if (!b.fromPlayer && player.inPlane)
        {
            float l, t, r, bottom;
            playerBox(player.x, player.y, l, t, r, bottom);
            if (b.x >= l && b.x <= r && b.y >= t && b.y <= bottom)
            {
                damagePlayer(b.damage, b.x);
                spawnParticles(b.x, b.y, 8, RGB(255, 120, 60), 180.f, 4.f);
                b.alive = false;
                continue;
            }
        }

        if (b.age > b.maxAge ||
            b.y > (float)screenH + 300.f || b.y < -500.f ||
            b.x < cameraX - 600.f || b.x > cameraX + (float)screenW + 600.f)
        {
            b.alive = false;
            continue;
        }

        // ---- 炸弹：触地爆炸（原版的飞机炸弹不参与掩体判定）----
        if (b.kind == BLK_BOMB || b.kind == BLK_ENEMY_BOMB)
        {
            if (b.y >= (float)groundY)
            {
                b.alive = false;
                explodeAt(b.x, b.y, b.damage, b.fromPlayer, 120.f);
            }
            continue;
        }

        // ---- 手雷：撞到掩体就哑掉（同原版：掩体挡手雷），落到地面才爆炸 ----
        // 掩体判定必须排在地面判定前面，否则手雷永远先落地爆炸，撞掩体的分支形同虚设。
        if (b.kind == BLK_GRENADE)
        {
            if (bulletHitsObstacle(b, prevY))
            {
                b.alive = false;
                continue;
            }
            if (b.y >= (float)groundY)
            {
                b.alive = false;
                explodeAt(b.x, b.y, b.damage, b.fromPlayer, 90.f);
            }
            continue;
        }

        // ---- 掩体优先（原版 checkZhangAiWu 在敌人判定之前执行）----
        // 子弹打在油桶/沙袋上就停下，不会穿过去打死躲在后面的敌人。
        // 不可破坏的沙袋也会挡子弹（原版沙袋同样会吃掉子弹）。
        if (bulletHitsObstacle(b, prevY))
        {
            b.alive = false;
            continue;
        }

        if (b.fromPlayer)
        {
            bool consumed = false;

            for (size_t j = 0; j < enemies.size() && !consumed; ++j)
            {
                Enemy& e = enemies[j];
                if (!e.alive) continue;
                float l, t, r, bo;
                enemyBox(e, l, t, r, bo);
                if (b.x >= l && b.x <= r && b.y >= t && b.y <= bo)
                {
                    e.hp -= b.damage;
                    e.flash = 6;
                    spawnParticles(b.x, b.y, 5, RGB(255, 210, 130), 150.f, 3.f);
                    spawnFx(3, b.x - 19.f, b.y - 10.f, false);
                    if (e.hp <= 0)
                        killEnemy(e);
                    consumed = true;
                }
            }
            if (consumed) { b.alive = false; continue; }

            // ---- 命中 Boss ----
            if (boss.state != BSS_NONE && boss.state != BSS_DIE && boss.state != BSS_DEAD)
            {
                const float bw = (float)g_assets.bossAttack[0].getwidth();
                const float bh = (float)g_assets.bossAttack[0].getheight();
                const float bl = boss.x + bw * 0.15f;
                const float br = boss.x + bw * 0.85f;
                const float bt = boss.y + bh * 0.15f;
                const float bb = boss.y + bh * 0.90f;
                if (b.x >= bl && b.x <= br && b.y >= bt && b.y <= bb)
                {
                    boss.hp -= b.damage;
                    boss.flash = 6;
                    spawnParticles(b.x, b.y, 6, RGB(255, 210, 130), 160.f, 3.f);
                    spawnFx(3, b.x - 19.f, b.y - 10.f, false);
                    consumed = true;
                }
            }
            if (consumed) { b.alive = false; continue; }
        }
        else
        {
            float l, t, r, bo;
            playerBox(player.x, player.y, l, t, r, bo);
            if (b.x >= l && b.x <= r && b.y >= t && b.y <= bo)
            {
                damagePlayer(b.damage, b.x);
                spawnParticles(b.x, b.y, 6, RGB(255, 90, 70), 160.f, 3.f);
                b.alive = false;
            }
        }
    }

    // 统一回收（本帧标记为失效的实体在这里被移除，不会残留）
    bullets.erase(remove_if(bullets.begin(), bullets.end(),
                [](const Bullet& b) { return !b.alive; }), bullets.end());
    enemies.erase(remove_if(enemies.begin(), enemies.end(),
                [](const Enemy& e) { return !e.alive; }), enemies.end());
}

void Game::updateObstacles()
{
    for (size_t i = 0; i < obstacles.size(); ++i)
        if (obstacles[i].flash > 0)
            obstacles[i].flash--;
}

void Game::bossAttackBox(float& l, float& t, float& r, float& b) const
{
    const float cx = boss.x + g_assets.bossAttack[0].getwidth() * 0.5f;
    l = boss.faceRight ? cx : cx - 320.f;
    r = boss.faceRight ? cx + 320.f : cx;
    t = (float)groundY + 10.f; // 腰部横斩：及时跳跃能越过刀光
    b = (float)groundY + 90.f;
}

void Game::updateBoss(float dt)
{
    if (boss.state == BSS_NONE || boss.state == BSS_DEAD)
        return;

    if (boss.flash > 0)
        boss.flash--;

    // 狂暴：半血后进入
    if (!boss.angry && boss.maxHp > 0 && boss.hp > 0 && boss.hp * 2 <= boss.maxHp)
    {
        boss.angry = true;
        showToast(_T("BOSS 狂暴了！"));
    }

    // 血量归零 -> 死亡动画
    if (boss.hp <= 0 && boss.state != BSS_DIE && boss.state != BSS_DEAD)
    {
        boss.state = BSS_DIE;
        boss.atkTicks = 0;
        boss.hp = 0;
        spawnParticles(boss.x + 60.f, boss.y - 60.f, 30, RGB(255, 190, 90), 280.f, 6.f);
        shake(12.f, 0.40f);
    }

    switch (boss.state)
    {
    case BSS_ENTER:
        boss.x -= 45.f * dt;
        boss.anim = (int)(frameClock / Cfg::ANIM_DIV) % 4;
        if (boss.x <= levelWidth - 560.f)
        {
            boss.state = BSS_IDLE;
            boss.cool = 60;
        }
        break;

    case BSS_IDLE:
    {
        boss.anim = (int)(frameClock / Cfg::ANIM_DIV) % 4;
        float pl, pt, pr, pb;
        playerBox(player.x, player.y, pl, pt, pr, pb);
        const float dx = (pl + pr) * 0.5f - (boss.x + g_assets.bossAttack[0].getwidth() * 0.5f);
        boss.faceRight = dx > 0.f;
        if (fabsf_(dx) > 230.f)
        {
            boss.x += (boss.faceRight ? 110.f : -110.f) * dt;
            boss.x = clampf(boss.x, 60.f, levelWidth - (float)g_assets.bossAttack[0].getwidth());
            break;
        }
        if (--boss.cool <= 0)
        {
            boss.state = BSS_ATTACK;
            boss.atkTicks = 0;
            boss.anim = 0;
        }
        break;
    }

    case BSS_ATTACK:
    {
        const int total = boss.angry ? 32 : 46;
        boss.atkTicks++;
        boss.anim = (boss.atkTicks * 10) / total;
        if (boss.atkTicks == total / 2)
        {
            // 刀光判定
            float l, t, r, b;
            bossAttackBox(l, t, r, b);
            float pl, pt, pr, pb;
            playerBox(player.x, player.y, pl, pt, pr, pb);
            if (pr > l && pl < r && pb > t && pt < b)
                damagePlayer(16, boss.x);
            spawnParticles((l + r) * 0.5f, (t + b) * 0.5f, 14, RGB(255, 255, 255), 240.f, 5.f);
            shake(7.f, 0.20f);
        }
        if (boss.atkTicks >= total)
        {
            boss.state = BSS_IDLE;
            boss.anim = 0;
            boss.cool = boss.angry ? 45 : 90;
        }
        break;
    }

    case BSS_DIE:
        boss.anim = boss.atkTicks / Cfg::ANIM_DIV;
        if (++boss.atkTicks >= 5 * Cfg::ANIM_DIV)
        {
            boss.state = BSS_DEAD;
            boss.hp = 0;
            score += 3000;
            spawnParticles(boss.x + 60.f, boss.y - 60.f, 40, RGB(255, 190, 90), 320.f, 7.f);
            shake(16.f, 0.6f);
            oldMan.active = true;
            oldMan.freed = false;
            oldMan.x = boss.x - 30.f;
            oldMan.y = (float)groundY;
            oldMan.anim = 0;
            showToast(_T("BOSS 已击败！去解救老人"));
        }
        break;

    default:
        break;
    }
}

void Game::updateCamera()
{
    const float maxX = levelWidth - (float)screenW;
    float target = player.x - (float)screenW * 0.38f;
    target = clampf(target, 0.f, (maxX > 0.f) ? maxX : 0.f);
    cameraX = lerpf(cameraX, target, 0.22f);
    if (maxX <= 0.f) cameraX = 0.f;
}

void Game::updateParticles(float dt)
{
    for (size_t i = 0; i < particles.size(); ++i)
    {
        Particle& p = particles[i];
        if (!p.alive) continue;
        p.vy += p.gravity * dt;
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.life -= dt;
        if (p.life <= 0.f)
            p.alive = false;
    }
    particles.erase(remove_if(particles.begin(), particles.end(),
                [](const Particle& p) { return !p.alive; }), particles.end());
}

void Game::updateFx()
{
    for (size_t i = 0; i < fx.size(); ++i)
    {
        FxAnim& f = fx[i];
        if (!f.alive) continue;
        if (++f.ticks < Cfg::ANIM_DIV) continue;
        f.ticks = 0;
        f.frame++;
        const int total = (f.set == 0) ? 5 : ((f.set == 3) ? 2 : 8);
        if (f.frame >= total)
            f.alive = false;
    }
    fx.erase(remove_if(fx.begin(), fx.end(),
                [](const FxAnim& f) { return !f.alive; }), fx.end());
}

void Game::fireWeapon()
{
    Player& p = player;
    if (mouseValid)
    {
        const float width = p.inPlane ? (float)g_assets.planePlayer.getwidth() : (float)Cfg::PLAYER_HIT_W;
        p.faceRight = (float)mouseX + cameraX >= p.x + width * 0.5f;
    }
    if (p.fireCooldown > 0)
        return;

    p.muzzleFlash = Cfg::MUZZLE_FRAMES; // 枪口和本次绘制的开火贴图使用同一尺寸
    float mx, my;
    muzzlePos(mx, my);
    const float ang = aimAngle();
    const float ca = (float)cos(ang);
    const float sa = (float)sin(ang);

    if (p.weapon == WP_CANNON)
    {
        spawnBullet(BLK_CANNON, mx, my, ca * Cfg::CANNON_SPEED, sa * Cfg::CANNON_SPEED,
                    Cfg::CANNON_DAMAGE, true);
        p.fireCooldown = Cfg::PLAYER_CANNON_CD;
        p.cannonAmmo--;
        if (p.cannonAmmo <= 0)
        {
            p.cannonAmmo = 0;
            p.weapon = WP_RIFLE;
            showToast(_T("炮弹已耗尽"));
        }
    }
    else
    {
        spawnBullet(BLK_RIFLE, mx, my, ca * Cfg::RIFLE_SPEED, sa * Cfg::RIFLE_SPEED,
                    Cfg::RIFLE_DAMAGE, true);
        p.fireCooldown = Cfg::PLAYER_RIFLE_CD;
    }
    p.muzzleFlash = Cfg::MUZZLE_FRAMES;
}

void Game::killEnemy(Enemy& e)
{
    if (!e.alive) return;
    e.alive = false;
    kills++;
    score += kEnemySpec[e.kind].score;
    if (kills % 10 == 0 && player.hp > 0)
    {
        player.hp = min(Cfg::PLAYER_MAX_HP, player.hp + 12);
        player.cannonAmmo = min(Cfg::CANNON_AMMO_MAX, player.cannonAmmo + 12);
        showToast(_T("击杀补给：生命 +12，炮弹 +12"));
    }
    const IMAGE& img = g_assets.enemy[e.kind][0];
    spawnFx(rng.range(0, 1), e.x, e.y, e.faceRight);
    spawnParticles(e.x + (float)img.getwidth() * 0.5f, e.y + (float)img.getheight() * 0.45f,
                   16, RGB(210, 40, 40), 190.f, 4.f);
    shake(4.f, 0.14f);
}

void Game::damagePlayer(int dmg, float fromX)
{
    Player& p = player;
    if (p.invuln > 0) return;
    if (phase != PH_PLAY) return;

    p.hp -= dmg;
    p.invuln = Cfg::PLAYER_INVULN;
    const float dir = (p.x < fromX) ? -1.f : 1.f;
    p.x += dir * 16.f;
    p.x = clampf(p.x, 60.f, levelWidth - 60.f);
    if (p.inPlane) plane.x = p.x;
    shake(9.f, 0.26f);
    spawnParticles(p.x + 30.f, p.y + 30.f, 12, RGB(255, 90, 70), 180.f, 4.f);
    if (p.hp <= 0)
    {
        p.hp = 0;
        showToast(_T("任务失败"));
    }
}

bool Game::canInteract() const
{
    if (player.inPlane) return false;
    if (plane.available && !plane.carried &&
        fabsf_(player.x - plane.x) < 95.f && player.y >= (float)groundY - 90.f)
        return true;
    if (oldMan.active && !oldMan.freed && fabsf_(player.x - oldMan.x) < 95.f)
        return true;
    return false;
}

float Game::interactX() const
{
    if (oldMan.active && !oldMan.freed && fabsf_(player.x - oldMan.x) < 95.f)
        return oldMan.x;
    if (plane.available && !plane.carried && fabsf_(player.x - plane.x) < 95.f)
        return plane.x;
    if (oldMan.active && !oldMan.freed)
        return oldMan.x;
    return player.x;
}

// ============================================================================
//  9. 绘制
// ============================================================================

void Game::draw()
{
    const int sx = (int)shakeX;
    const int sy = (int)shakeY;

    cleardevice();
    setbkmode(TRANSPARENT);
    settextstyle(24, 0, _T("Microsoft YaHei"));
    settextcolor(RGB(255, 255, 255));

    switch (phase)
    {
    case PH_SPLASH:
        putimage(0, 0, &g_assets.splash);
        break;
    case PH_MENU:       drawMenu(); break;
    case PH_ROUND_SEL:  drawRoundSelect(); break;
    default:
        drawWorld(sx, sy);
        drawHud();
        drawToast();
        if (phase == PH_CLEAR)
        {
            const int x = (screenW - 680) / 2, y = (int)(screenH * 0.30f);
            setfillcolor(RGB(20, 24, 32));
            fillrectangle(x - 20, y - 20, x + 700, y + 90);
            settextcolor(RGB(255, 255, 255));
            outtextxy(x, y, _T("人质已获救！关卡完成"));
            outtextxy(x, y + 36, _T("按 E / 回车 / 空格 继续，ESC 返回选关"));
        }
        if (phase == PH_WIN || phase == PH_OVER)
            drawEndScreen();
        break;
    }

    FlushBatchDraw();
}

void Game::drawMenu()
{
    putimage(0, 0, &g_assets.menu);
    putimagePng((int)(screenW * 0.40f), (int)(screenH * 0.50f), &g_assets.modeNormal);
    putimagePng((int)(screenW * 0.60f), (int)(screenH * 0.50f), &g_assets.modeEndless);

    const int selX = (menuSel == 0)
        ? (int)(screenW * 0.40f) + 30
        : (int)(screenW * 0.60f) + 30;
    putimagePng(selX, (int)(screenH * 0.50f) - 50, &g_assets.select);

    outtextxy((int)(screenW * 0.03f), (int)(screenH * 0.88f), _T("空格 切换模式    回车 确认"));
    outtextxy((int)(screenW * 0.03f), (int)(screenH * 0.92f), _T("WASD 移动   鼠标瞄准   左键射击   E 交互"));
}

void Game::drawRoundSelect()
{
    putimage(0, 0, &g_assets.menu);
    putimagePng((int)(screenW * 0.40f), (int)(screenH * 0.50f), &g_assets.modeNormal);
    putimagePng((int)(screenW * 0.40f) + 30, (int)(screenH * 0.50f) - 50, &g_assets.select);

    for (int i = 0; i < 3; ++i)
        putimagePng((int)((0.30f + 0.20f * i) * screenW), (int)(screenH * 0.70f), &g_assets.roundBadge[i]);

    const int selX = (int)((0.30f + 0.20f * roundSel) * screenW) + 30;
    putimagePng(selX, (int)(screenH * 0.70f) - 50, &g_assets.select);

    outtextxy((int)(screenW * 0.03f), (int)(screenH * 0.88f), _T("空格 切换关卡    回车 开始    ESC 返回"));
}

void Game::drawWorld(int sx, int sy)
{
    const int camX = (int)cameraX;

    // ---- 背景 ----
    putimage(sx - camX, sy, &g_assets.bg[level]);

    // ---- 障碍 ----
    for (size_t i = 0; i < obstacles.size(); ++i)
    {
        const Obstacle& o = obstacles[i];
        if (!o.alive) continue;
        const int ix = sx + (int)o.x - camX;
        if (ix < -220 || ix > screenW + 220) continue;
        if (o.destructible)
        {
            // 油桶被打伤后换成破损贴图
            const bool broken = (o.hp * 2 <= o.maxHp);
            putimagePngFlash(ix, sy + (int)o.y,
                             broken ? &g_assets.barrelBroken : &g_assets.barrel,
                             o.flash > 0 ? 190 : 0);
        }
        else
            putimagePng(ix, sy + (int)o.y, &g_assets.sandbag);
    }

    // ---- 敌人 ----
    for (size_t i = 0; i < enemies.size(); ++i)
    {
        const Enemy& e = enemies[i];
        if (!e.alive) continue;
        const int count = g_assets.enemyCount[e.kind];
        if (count <= 0) continue;
        int frame = e.anim % count;
        if (frame < 0) frame = 0;
        IMAGE* img = &g_assets.enemy[e.kind][frame];
        const int ix = sx + (int)e.x - camX;
        if (ix < -320 || ix > screenW + 320) continue;
        const bool flip = (e.faceRight != kEnemySpec[e.kind].artFacesRight);
        const int flash = (e.flash > 0) ? 210 : 0;
        if (flip)
            putToggleImagePngFlash(ix, sy + (int)e.y, img, flash);
        else
            putimagePngFlash(ix, sy + (int)e.y, img, flash);
    }

    // ---- Boss ----
    if (boss.state != BSS_NONE && boss.state != BSS_DEAD)
    {
        IMAGE* img = nullptr;
        if (boss.state == BSS_DIE)
            img = &g_assets.bossDie[clampi(boss.anim, 0, 4)];
        else if (boss.state == BSS_ATTACK)
            img = &g_assets.bossAttack[clampi(boss.anim, 0, 9)];
        else if (boss.angry)
            img = &g_assets.bossAnger[(int)(frameClock / Cfg::ANIM_DIV) % 2];
        else
            img = &g_assets.bossStand[clampi(boss.anim, 0, 3)];

        const int ix = sx + (int)boss.x - camX;
        if (ix > -400 && ix < screenW + 400)
        {
            if (boss.faceRight) putToggleImagePngFlash(ix, sy + (int)boss.y, img, boss.flash > 0 ? 200 : 0);
            else putimagePngFlash(ix, sy + (int)boss.y, img, boss.flash > 0 ? 200 : 0);
        }
        // 刀光与伤害共用范围，并在伤害结算的中段出现。
        const int total = boss.angry ? 32 : 46;
        if (boss.state == BSS_ATTACK && boss.atkTicks < total / 2 - 4)
        {
            float l, t, r, b;
            bossAttackBox(l, t, r, b);
            setfillcolor(RGB(220, 90, 30));
            fillrectangle(sx - camX + (int)l, sy + (int)b - 4,
                          sx - camX + (int)r, sy + (int)b);
        }
        if (boss.state == BSS_ATTACK && abs(boss.atkTicks - total / 2) <= 4)
        {
            float l, t, r, b;
            bossAttackBox(l, t, r, b);
            const float cx = boss.faceRight ? l : r;
            const float cy = (t + b) * 0.5f;
            const float dir = boss.faceRight ? 1.f : -1.f;
            for (int trail = 0; trail < 4; ++trail)
            {
                setlinecolor(trail == 0 ? RGB(255, 255, 255) : RGB(120, 220, 255));
                for (int seg = 0; seg < 24; ++seg)
                {
                    const float a = -PI_F * 0.5f + PI_F * (float)seg / 24.f;
                    const float a2 = -PI_F * 0.5f + PI_F * (float)(seg + 1) / 24.f;
                    const float reach = 320.f - trail * 8.f;
                    const float height = (b - t) * 0.5f - trail * 3.f;
                    line(sx - camX + (int)(cx + dir * reach * cos(a)), sy + (int)(cy + height * sin(a)),
                         sx - camX + (int)(cx + dir * reach * cos(a2)), sy + (int)(cy + height * sin(a2)));
                }
            }
            setlinecolor(RGB(255, 255, 255));
        }
    }

    // ---- 老人 ----
    if (oldMan.active)
    {
        const int ix = sx + (int)oldMan.x - camX;
        const int iy = sy + (int)oldMan.y;
        if (oldMan.freed)
            putimagePng(ix, iy, &g_assets.oldManFree[(int)(frameClock / Cfg::ANIM_DIV) % 4]);
        else
            putimagePng(ix, iy, &g_assets.oldManBound[(int)(frameClock / Cfg::ANIM_DIV) % 3]);
    }

    // ---- 子弹 ----
    for (size_t i = 0; i < bullets.size(); ++i)
    {
        const Bullet& b = bullets[i];
        const int ix = sx + (int)b.x - camX;
        const int iy = sy + (int)b.y;
        const bool left = (b.vx < 0.f);
        switch (b.kind)
        {
        case BLK_RIFLE:
            if (left) putToggleImagePng(ix, iy, &g_assets.bulletPlayer);
            else      putimagePng(ix, iy, &g_assets.bulletPlayer);
            break;
        case BLK_CANNON:
            if (left) putToggleImagePng(ix, iy, &g_assets.bulletHeavy);
            else      putimagePng(ix, iy, &g_assets.bulletHeavy);
            break;
        case BLK_ENEMY_RIFLE:
            putimagePng(ix, iy, &g_assets.bulletEnemyRifle);
            break;
        case BLK_ENEMY_ROCKET:
            // 火箭弹原图弹头朝左，和主角的子弹素材方向相反。
            if (left) putimagePng(ix, iy, &g_assets.bulletEnemyRocket);
            else      putToggleImagePng(ix, iy, &g_assets.bulletEnemyRocket);
            break;
        case BLK_GRENADE:
            putimagePng(ix, iy, &g_assets.bulletGrenade);
            break;
        default:
            putimagePng(ix, iy, &g_assets.planeBomb);
            break;
        }
    }

    // ---- 玩家飞机 ----
    if (plane.available)
    {
        const int ix = sx + (int)plane.x - camX;
        const int iy = sy + (int)plane.y;
        const int flash = player.inPlane && player.invuln > 0 && ((frameClock / 3) & 1) ? 150 : 0;
        if (player.faceRight)
            putimagePngFlash(ix, iy, &g_assets.planePlayer, flash);
        else
            putToggleImagePngFlash(ix, iy, &g_assets.planePlayer, flash);
    }

    // ---- 主角 ----
    if (!player.inPlane)
    {
        const int tier = (player.muzzleFlash > 0) ? BT_HEAVY : BT_NORMAL;
        const int bc = g_assets.playerBodyCount[tier][player.weapon];
        const int fc = g_assets.playerFootCount[player.stance];
        if (bc > 0 && fc > 0)
        {
            const int anim = (int)(frameClock / Cfg::ANIM_DIV);
            IMAGE* body = &g_assets.playerBody[tier][player.weapon][anim % bc];
            IMAGE* foot = &g_assets.playerFoot[player.stance][anim % fc];
            const int ix = sx + (int)player.x - camX;
            const int iy = sy + (int)player.y;
            const int fy = iy + body->getheight() - Cfg::FOOT_OVERLAP;
            // 主角素材本身朝左，朝右时需要镜像
            const int flash = (player.invuln > 0 && ((frameClock / 3) & 1)) ? 150 : 0;
            if (player.faceRight)
            {
                putToggleImagePngFlash(ix, iy, body, flash);
                putToggleImagePngFlash(ix, fy, foot, flash);
            }
            else
            {
                putimagePngFlash(ix, iy, body, flash);
                putimagePngFlash(ix, fy, foot, flash);
            }
        }
    }

    // ---- 特效动画 ----
    for (size_t i = 0; i < fx.size(); ++i)
    {
        const FxAnim& f = fx[i];
        IMAGE* img = nullptr;
        if (f.set == 0)      img = &g_assets.deathA[clampi(f.frame, 0, 4)];
        else if (f.set == 1) img = &g_assets.deathB[clampi(f.frame, 0, 7)];
        else if (f.set == 2) img = &g_assets.bombBoom[clampi(f.frame, 0, 7)];
        else                 img = &g_assets.impact;
        const int ix = sx + (int)f.x - camX;
        if (ix < -300 || ix > screenW + 300) continue;
        if (f.flip) putToggleImagePng(ix, sy + (int)f.y, img);
        else        putimagePng(ix, sy + (int)f.y, img);
    }

    // ---- 粒子 ----
    for (size_t i = 0; i < particles.size(); ++i)
    {
        const Particle& p = particles[i];
        const float k = clampf(p.life / p.maxLife, 0.f, 1.f);
        const int s = (int)(p.size * (0.35f + 0.65f * k));
        if (s <= 0) continue;
        const int ix = sx + (int)p.x - camX;
        const int iy = sy + (int)p.y;
        setfillcolor(p.color);
        fillrectangle(ix - s / 2, iy - s / 2, ix + s / 2, iy + s / 2);
    }
    setfillcolor(RGB(255, 255, 255));

    // ---- 交互提示 ----
    if (canInteract())
    {
        const int ix = sx + (int)interactX() - camX;
        const int iy = sy + groundY - 150 + (int)(4.0f * (float)sin((float)frameClock * 0.12f));
        putimagePng(ix - 10, iy, &g_assets.letterE);
    }
}

void Game::drawHud()
{
    TCHAR buf[64];

    // ---- 血条 ----
    const int barX = 26, barY = 24, barW = 300, barH = 20;
    setlinecolor(RGB(0, 0, 0));
    setfillcolor(RGB(18, 18, 18));
    fillrectangle(barX - 3, barY - 3, barX + barW + 3, barY + barH + 3);

    const int hp = clampi(player.hp, 0, Cfg::PLAYER_MAX_HP);
    setfillcolor(hp > 40 ? RGB(40, 200, 90) : (hp > 15 ? RGB(240, 200, 40) : RGB(230, 60, 50)));
    fillrectangle(barX, barY, barX + (barW * hp) / Cfg::PLAYER_MAX_HP, barY + barH);

    setlinecolor(RGB(255, 255, 255));
    for (int i = 1; i < 4; ++i)
    {
        const int tx = barX + (barW * i) / 4;
        line(tx, barY, tx, barY + barH);
    }

    tsprintf(buf, 64, _T("HP %d / %d"), hp, Cfg::PLAYER_MAX_HP);
    outtextxy(barX, barY + barH + 8, buf);

    // ---- 武器 ----
    if (player.weapon == WP_CANNON)
        tsprintf(buf, 64, _T("武器 炮台   弹药 %d"), player.cannonAmmo);
    else
        tsprintf(buf, 64, _T("武器 步枪   弹药 无穷"));
    outtextxy(barX, barY + barH + 32, buf);

    // ---- 右上角统计 ----
    tsprintf(buf, 64, _T("分数 %d"), score);
    outtextxy(screenW - 220, barY, buf);
    tsprintf(buf, 64, _T("击杀 %d"), kills);
    outtextxy(screenW - 220, barY + 26, buf);
    tsprintf(buf, 64, _T("波次 %d"), waveIndex);
    outtextxy(screenW - 220, barY + 52, buf);
    if (!endless)
        tsprintf(buf, 64, _T("第 %d 关"), round + 1);
    else
        tsprintf(buf, 64, _T("无尽模式  Boss %d"), bossCount);
    outtextxy(screenW - 220, barY + 78, buf);

    // ---- Boss 血条 ----
    if (boss.state != BSS_NONE && boss.state != BSS_DEAD && boss.maxHp > 0)
    {
        const int bw = 560, bx = (screenW - bw) / 2, by = 46;
        setlinecolor(RGB(0, 0, 0));
        setfillcolor(RGB(18, 18, 18));
        fillrectangle(bx - 3, by - 3, bx + bw + 3, by + 18);
        setfillcolor(boss.angry ? RGB(240, 90, 50) : RGB(200, 40, 50));
        fillrectangle(bx, by, bx + (bw * clampi(boss.hp, 0, boss.maxHp)) / boss.maxHp, by + 15);
        settextcolor(RGB(255, 80, 80));
        outtextxy(bx, by - 24, _T("BOSS"));
        settextcolor(RGB(255, 255, 255));
    }

    // ---- 当前目标：持续显示，避免只靠两秒提示气泡引导 ----
    const TCHAR* objective = oldMan.active && !oldMan.freed ? _T("目标：靠近人质，按 E 解救")
        : (boss.state != BSS_NONE && boss.state != BSS_DEAD ? _T("目标：击败 BOSS，留意挥刀预备动作")
        : _T("目标：向右推进；每 10 次击杀获得补给"));
    const int objectiveX = (screenW - 560) / 2;
    setfillcolor(RGB(20, 24, 32));
    fillrectangle(objectiveX - 8, 112, objectiveX + 560, 148);
    outtextxy(objectiveX, 116, objective);

    // ---- 关卡进度条 ----
    const int pW = screenW - 240, pX = 120, pY = screenH - 40;
    setlinecolor(RGB(0, 0, 0));
    setfillcolor(RGB(18, 18, 18));
    fillrectangle(pX - 3, pY - 3, pX + pW + 3, pY + 14);
    setfillcolor(RGB(80, 140, 230));
    const float prog = clampf(player.x / (levelWidth > 1.f ? levelWidth : 1.f), 0.f, 1.f);
    fillrectangle(pX, pY, pX + (int)(pW * prog), pY + 11);

    // Boss 位置标记
    {
        const float bx = (boss.state != BSS_NONE) ? boss.x : levelWidth - 560.f;
        const int mx = pX + (int)(pW * clampf(bx / (levelWidth > 1.f ? levelWidth : 1.f), 0.f, 1.f));
        setfillcolor(RGB(240, 70, 70));
        fillrectangle(mx - 4, pY - 4, mx + 4, pY + 15);
    }

    outtextxy(pX, screenH - 26, _T("1 步枪   2 炮台   E 交互   空格 投弹"));
}

void Game::drawToast()
{
    if (!toast.active) return;
    const int a = (int)(255.f * clampf(toast.life, 0.f, 1.f));
    settextcolor(RGB(a, a, a));
    outtextxy((screenW - 360) / 2, (int)(screenH * 0.16f), toast.text);
    settextcolor(RGB(255, 255, 255));
}

void Game::drawEndScreen()
{
    IMAGE* img = (phase == PH_WIN) ? &g_assets.win : &g_assets.fail;
    putimage((screenW - img->getwidth()) / 2, (screenH - img->getheight()) / 2 - 90, img);

    TCHAR buf[64];
    tsprintf(buf, 64, _T("最终得分 %d"), score);
    outtextxy((screenW - 160) / 2, (screenH + img->getheight()) / 2 + 20, buf);
    tsprintf(buf, 64, _T("击杀数 %d"), kills);
    outtextxy((screenW - 160) / 2, (screenH + img->getheight()) / 2 + 52, buf);
    outtextxy((screenW - 280) / 2, (screenH + img->getheight()) / 2 + 92,
              endless ? _T("按 回车 返回主菜单") : _T("按 回车 返回选关界面"));
}

// ============================================================================
//  10. 入口
// ============================================================================

#ifndef METAL_SLUG_NO_MAIN
int main()
{
    g_game.init();

    initgraph(g_game.screenW, g_game.screenH);
    BeginBatchDraw();
    setbkmode(TRANSPARENT);

    mciSendString(_T("open Market.mp3 alias bkmusic"), NULL, 0, NULL);
    mciSendString(_T("play bkmusic repeat"), NULL, 0, NULL);

    // 固定步长逻辑 + 独立渲染
    const double target = 1.0 / (double)Cfg::LOGIC_HZ;  // 性能计数器换算的是秒
    LARGE_INTEGER freq, last, now;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&last);
    double acc = 0.0;

    while (true)
    {
        QueryPerformanceCounter(&now);
        double dt = (double)(now.QuadPart - last.QuadPart) / (double)freq.QuadPart;
        last = now;
        if (dt > 0.25) dt = 0.25;      // 卡顿后不要暴走
        acc += dt;

        int guard = 0;
        while (acc >= target && guard < 8)
        {
            g_game.update();
            acc -= target;
            ++guard;
        }
        if (guard >= 8) acc = 0.0;

        g_game.draw();
        FlushBatchDraw();
        Sleep(1);
    }

    EndBatchDraw();
    closegraph();
    return 0;
}
#endif
