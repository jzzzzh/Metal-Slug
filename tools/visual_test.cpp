// Visual contracts use the inspected source art (soldiers and rockets face left).
#define METAL_SLUG_NO_MAIN
#include "../HeJinDanTou.cpp"

bool g_stubKeyState[256] = {};
int g_stubScreenW = 1920, g_stubScreenH = 1080;
bool g_stubMouseHit = false;
int g_stubMouseX = 0, g_stubMouseY = 0;
static int failures = 0, checks = 0;
#define CHECK(c, msg) do { ++checks; if (!(c)) { ++failures; printf("FAIL: %s (%d)\n", msg, __LINE__); } } while (0)

static void resetScene(int round = 0, bool endless = false)
{
    memset(g_stubKeyState, 0, sizeof(g_stubKeyState));
    g_stubMouseHit = false;
    g_game.input.reset();
    g_game.resetRound(round, endless);
    g_game.enemies.clear();
    g_game.obstacles.clear();
    g_game.plane.available = false;
    g_game.plane.carried = false;
    g_game.player.inPlane = false;
    g_game.player.hp = 100;
    g_game.phase = PH_PLAY;
    g_game.mouseValid = false;
}

// Compare opaque pixels of the actual drawWorld output, independently of the
// orientation configuration under test. Source PNGs were inspected facing left.
static bool matchesSprite(IMAGE& img, int x, int y, bool flip)
{
    int opaque = 0;
    const DWORD* src = GetImageBuffer(&img);
    const DWORD* dst = GetImageBuffer();
    for (int j = 0; j < img.getheight(); ++j)
        for (int i = 0; i < img.getwidth(); ++i)
        {
            const DWORD p = src[j * img.getwidth() + i];
            if ((p >> 24) != 255) continue;
            ++opaque;
            const int dx = x + (flip ? img.getwidth() - 1 - i : i);
            if ((dst[(y + j) * getwidth() + dx] & 0xffffff) != (p & 0xffffff)) return false;
        }
    return opaque > 0;
}

static void testSoldierRendering()
{
    for (int kind = EK_RIFLEMAN; kind <= EK_THROWER; ++kind)
        for (bool right : {false, true})
        {
            resetScene();
            g_game.player.x = right ? 1100.f : 200.f;
            g_game.spawnEnemy((EnemyKind)kind, 640.f);
            g_game.enemies[0].fireTimer = 1000;
            g_game.updateEnemies(0.f);
            CHECK(g_game.enemies[0].faceRight == right, "soldier targets player on either side");
            for (int frame = 0; frame < g_assets.enemyCount[kind]; ++frame)
            {
                g_game.enemies[0].anim = frame;
                g_game.drawWorld(0, 0);
                CHECK(matchesSprite(g_assets.enemy[kind][frame], 640, g_game.groundY, right),
                      "soldier idle/attack pixels face the player");
            }
        }
}

static void testWeaponRendering()
{
    for (bool right : {false, true})
    {
        resetScene();
        g_game.spawnBullet(BLK_ENEMY_ROCKET, 600.f, 500.f, right ? 200.f : -200.f, 0.f, 3, false);
        g_game.drawWorld(0, 0);
        CHECK(matchesSprite(g_assets.bulletEnemyRocket, 600, 500, right), "rocket nose follows velocity");
    }
    for (int weapon : {WP_RIFLE, WP_CANNON})
        for (bool inPlane : {false, true})
            for (bool right : {false, true})
            {
                resetScene();
                g_game.player.x = 600.f;
                g_game.player.weapon = weapon;
                g_game.player.inPlane = inPlane;
                g_game.player.faceRight = !right;
                g_game.mouseValid = true;
                g_game.mouseX = right ? 1000 : 200;
                g_game.mouseY = g_game.groundY + 20;
                g_game.input.cur[VK_LBUTTON] = true;
                g_game.input.cur[right ? VK_LEFT : VK_RIGHT] = true;
                g_game.updatePlayer(0.f);
                CHECK(g_game.player.faceRight == right, "firing weapon faces mouse even while moving away");
                CHECK(g_game.bullets.size() == 1 && (g_game.bullets[0].vx > 0.f) == right,
                      "muzzle and projectile direction agree");
            }
    for (EnemyKind kind : {EK_RIFLEMAN, EK_ROCKET, EK_THROWER})
        for (bool right : {false, true})
        {
            resetScene();
            g_game.player.x = right ? 1000.f : 300.f;
            g_game.spawnEnemy(kind, 600.f);
            g_game.enemies[0].fireTimer = 0;
            g_game.updateEnemies(0.f);
            const int w = g_assets.enemy[kind][kEnemySpec[kind].atkStart].getwidth();
            CHECK(g_game.bullets.size() == 1, "ranged soldier fires");
            if (!g_game.bullets.empty())
                CHECK(fabsf_(g_game.bullets[0].x - (600.f + (right ? w : 0))) < 1.f,
                      "enemy projectile starts at weapon side of sprite");
        }
    for (bool inPlane : {false, true})
    {
        resetScene();
        g_game.player.x = 600.f;
        g_game.player.inPlane = inPlane;
        g_game.mouseValid = true;
        g_game.mouseX = inPlane ? 680 : 640; // inside right half, behind right muzzle
        g_game.mouseY = (int)g_game.player.y + 25;
        g_game.fireWeapon();
        CHECK(g_game.player.faceRight && g_game.bullets[0].vx >= 0.f,
              "aiming inside sprite cannot fire backwards out of a forward weapon");
    }
}

static void press(int key)
{
    g_stubKeyState[key] = true; g_game.update();
    g_stubKeyState[key] = false; g_game.update();
}

static void testRescueContinuation()
{
    for (int round = 0; round < 3; ++round)
        for (int key : {int('E'), VK_RETURN, VK_SPACE})
        {
            resetScene(round);
            g_game.boss.state = BSS_DEAD;
            g_game.oldMan.active = true;
            g_game.oldMan.x = g_game.player.x;
            g_game.oldMan.y = g_game.player.y;
            // A parked plane must not intercept a rescue interaction.
            g_game.plane.available = true;
            g_game.plane.x = g_game.player.x;
            g_game.plane.y = g_game.player.y;
            press('E');
            CHECK(g_game.oldMan.freed && g_game.phase == PH_CLEAR, "E rescues hostage before boarding plane");
            CHECK(!g_game.player.inPlane, "rescue does not board plane");
            const int score = g_game.score;
            press(key);
            CHECK(g_game.phase == (round < 2 ? PH_PLAY : PH_WIN), "rescue continues with E/Enter/Space");
            CHECK(round == 2 || g_game.round == round + 1, "rescue advances to next round");
            CHECK(g_game.score == score, "continuing must not award rescue twice");
        }
}

static void testAnimationTiming()
{
    resetScene();
    g_game.spawnFx(0, 500.f, 800.f, false);
    for (int i = 0; i < 5; ++i) g_game.updateFx();
    CHECK(g_game.fx.size() == 1 && g_game.fx[0].frame == 0, "death first frame lasts six logic ticks");
    for (int i = 5; i < 29; ++i) g_game.updateFx();
    CHECK(g_game.fx.size() == 1 && g_game.fx[0].frame == 4, "death animation reaches final frame");
    g_game.updateFx();
    CHECK(g_game.fx.empty(), "death effect ends after half a second");
    g_game.boss.state = BSS_IDLE;
    g_game.boss.hp = 0;
    g_game.boss.x = 1000.f;
    for (int i = 0; i < 5; ++i) g_game.updateBoss(Cfg::DT);
    CHECK(g_game.boss.state == BSS_DIE && !g_game.oldMan.active, "boss death animation must not disappear in 5 ticks");
    for (int i = 5; i < 30; ++i) g_game.updateBoss(Cfg::DT);
    CHECK(g_game.boss.state == BSS_DEAD && g_game.oldMan.active, "hostage appears after complete boss death animation");
}

static void testAircraftDamage()
{
    for (BulletKind kind : {BLK_ENEMY_RIFLE, BLK_ENEMY_ROCKET, BLK_ENEMY_BOMB})
    {
        resetScene();
        g_game.player.inPlane = true;
        g_game.player.x = 600.f;
        g_game.player.y = 300.f;
        g_game.spawnBullet(kind, 670.f, 340.f, 0.f, 0.f, 10, false);
        g_game.updateBullets(0.f);
        CHECK(g_game.player.hp == 90, "enemy bullets/rockets/bombs damage occupied aircraft");
        CHECK(g_game.bullets.empty(), "aircraft absorbs projectile on contact");
        g_game.damagePlayer(10, 600.f);
        CHECK(g_game.player.hp == 90, "aircraft damage retains invulnerability frames");
    }
    resetScene();
    g_game.player.inPlane = true;
    g_game.player.x = 600.f;
    g_game.player.y = 300.f;
    g_game.spawnEnemy(EK_RIFLEMAN, 900.f);
    g_game.enemies[0].fireTimer = 0;
    g_game.updateEnemies(0.f);
    CHECK(g_game.bullets.size() == 1 && g_game.bullets[0].vy < -1.f,
          "rifleman aims upwards at aircraft");
    for (int i = 0; i < 200 && g_game.player.hp == 100; ++i) g_game.updateBullets(Cfg::DT);
    CHECK(g_game.player.hp < 100, "rifleman shot actually reaches aircraft hitbox");
}

static void testBossCombat()
{
    resetScene();
    g_game.boss.state = BSS_IDLE;
    g_game.boss.hp = g_game.boss.maxHp = 80;
    g_game.boss.x = 1100.f;
    g_game.boss.y = (float)g_game.groundY - 40.f;
    g_game.boss.cool = 0;
    g_game.player.x = 200.f;
    for (int i = 0; i < 60; ++i) g_game.updateBoss(Cfg::DT);
    CHECK(g_game.boss.x < 1050.f, "boss closes distance instead of slashing empty air");

    for (bool right : {false, true})
        for (bool inPlane : {false, true})
        {
            resetScene();
            g_game.boss.state = BSS_IDLE;
            g_game.boss.hp = g_game.boss.maxHp = 80;
            g_game.boss.x = 1000.f;
            g_game.boss.y = (float)g_game.groundY - 40.f;
            g_game.boss.cool = 1;
            g_game.player.x = right ? 1180.f : 820.f;
            g_game.player.inPlane = inPlane;
            if (inPlane) g_game.player.y = (float)g_game.groundY - 40.f;
            g_game.updateBoss(0.f);
            for (int i = 0; i < 23; ++i) g_game.updateBoss(0.f);
            CHECK(g_game.player.hp < 100, "boss slash hits player or low aircraft on either side");
        }
    resetScene();
    g_game.boss.state = BSS_IDLE;
    g_game.boss.hp = g_game.boss.maxHp = 80;
    g_game.boss.x = 1000.f;
    g_game.boss.y = (float)g_game.groundY - 40.f;
    g_game.boss.cool = 1;
    g_game.player.x = 820.f;
    g_game.updateBoss(0.f);
    g_game.input.cur[VK_UP] = true;
    g_game.input.prev[VK_UP] = false;
    for (int i = 0; i < 23; ++i)
    {
        g_game.updatePlayer(Cfg::DT);
        g_game.updateBoss(Cfg::DT);
        g_game.input.reset();
    }
    CHECK(g_game.player.hp == 100, "jump during boss windup dodges waist-height slash");
}

static void testThirdLevelSky()
{
    const IMAGE& bg = g_assets.bg[2];
    const DWORD* pixels = GetImageBuffer(&g_assets.bg[2]);
    // The inspected PNG has a magenta key at (760, 0), building at (0, 0).
    const int skyX = 760 * bg.getwidth() / 1298;
    FILE* cached = fopen("tools/.texcache/background3.png.raw", "rb");
    if (cached)
    {
        fclose(cached);
        CHECK((pixels[skyX] & 0xffffff) == 0xffffff, "third level keyed sky must be white");
    }
    else printf("SKIP original sky sample: convert textures to enable real-art check\n");
    bool hasKey = false;
    for (int i = 0; i < bg.getwidth() * bg.getheight(); ++i)
    {
        const DWORD c = pixels[i];
        if (((c >> 16) & 255) >= 224 && ((c >> 8) & 255) <= 32 && (c & 255) >= 224) hasKey = true;
    }
    CHECK(!hasKey, "third level contains no leftover magenta key pixels");
    CHECK((pixels[0] & 0xffffff) != 0xffffff, "third level buildings retain original colors");
}

static void testPacingAndSupplies()
{
    resetScene();
    g_game.player.inPlane = true;
    g_game.player.x = 600.f;
    g_game.player.y = 300.f;
    g_game.input.cur[VK_RIGHT] = true;
    g_game.input.cur[VK_UP] = true;
    g_game.updatePlayer(Cfg::DT);
    const float dx = g_game.player.x - 600.f, dy = g_game.player.y - 300.f;
    CHECK(dx * dx + dy * dy <= 4.61f * 4.61f, "diagonal aircraft movement has no speed bonus");
    resetScene();
    g_game.input.cur[VK_UP] = true;
    g_game.updatePlayer(Cfg::DT);
    g_game.input.reset();
    int airborne = 1;
    while (g_game.player.y < g_game.groundY && airborne < 200)
    {
        g_game.updatePlayer(Cfg::DT);
        ++airborne;
    }
    CHECK(airborne >= 35 && airborne <= 60, "jump lands within one second for responsive control");
    g_game.input.cur[VK_UP] = true;
    g_game.input.prev[VK_UP] = true;
    g_game.updatePlayer(Cfg::DT);
    CHECK(g_game.player.y == g_game.groundY, "holding jump after landing must not trigger unwanted extra jump");
    resetScene();
    g_game.kills = 9;
    g_game.player.hp = 50;
    g_game.player.cannonAmmo = 5;
    g_game.spawnEnemy(EK_RIFLEMAN, 600.f);
    g_game.killEnemy(g_game.enemies[0]);
    CHECK(g_game.player.hp == 62 && g_game.player.cannonAmmo == 17, "ten kills grant health and ammo supply");
    g_game.killEnemy(g_game.enemies[0]);
    CHECK(g_game.kills == 10 && g_game.player.hp == 62, "dead enemy cannot grant duplicate supply");
    g_game.enemies.clear();
    g_game.kills = 19;
    g_game.player.hp = 99;
    g_game.player.cannonAmmo = 59;
    g_game.spawnEnemy(EK_RIFLEMAN, 600.f);
    g_game.killEnemy(g_game.enemies[0]);
    CHECK(g_game.player.hp == 100 && g_game.player.cannonAmmo == 60, "supply respects health and ammo caps");
    resetScene();
    g_game.spawnEnemy(EK_KNIFE, g_game.player.x + 30.f);
    for (int i = 0; i < 14; ++i) g_game.updateEnemies(Cfg::DT);
    CHECK(g_game.player.hp <= 90 && g_game.player.hp > 0, "melee hit matters without instantly killing player");
}

static void testNativeDrawingColors()
{
    setfillcolor(RGB(255, 0, 0));
    fillrectangle(0, 0, 2, 2);
    CHECK((GetImageBuffer()[0] & 0xffffff) == 0xff0000, "offline renderer preserves native red fill color");
    setlinecolor(RGB(0, 0, 255));
    line(0, 3, 2, 3);
    CHECK((GetImageBuffer()[3 * getwidth()] & 0xffffff) == 0x0000ff,
          "offline renderer preserves native blue line color");
}

int main()
{
    initgraph(1920, 1080);
    g_game.init();
    g_game.rng.seed(48);
    testSoldierRendering();
    testWeaponRendering();
    testRescueContinuation();
    testAnimationTiming();
    testAircraftDamage();
    testBossCombat();
    testThirdLevelSky();
    testPacingAndSupplies();
    testNativeDrawingColors();
    closegraph();
    printf("Visual/combat checks: %d, failures: %d\n", checks, failures);
    return failures ? 1 : 0;
}
