#include "main_Menu.h"
#include "Menu_background.h"
#include "slow_screen.h"
#include "curtain_vfx.h"
#include "../music_execute/Lobby_Music/lobby_bgm.h"
#include "../Skin/Button/UI_Button.h"
#include <raylib.h>
#include <cstdlib>
#include <algorithm>
#include <cmath>
#include <cstdio>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

struct RadialParticle {
    float angle;
    float distance;
    float speed;
    float size;
    float alpha;
    float life;
    float maxLife;
    Color color;
};

struct BurstParticle {
    Vector2 position;
    Vector2 velocity;
    float size;
    float alpha;
    float life;
    float maxLife;
    Color color;
};

struct MenuTrailParticle {
    Vector2 position;
    Vector2 velocity;
    float size;
    float alpha;
    float life;
    float maxLife;
    Color color;
};

static const int MAX_RADIAL_PARTICLES = 50; // 파티클 수 대폭 축소하여 깔끔함 유지를 목표
static const int MAX_BURST = 40;
static const int MAX_TRAIL = 40;
static const int RADIAL_RAYS = 24;
static const int EQUALIZER_BARS = 24;

static RadialParticle radialParticles[MAX_RADIAL_PARTICLES];
static BurstParticle bursts[MAX_BURST];
static MenuTrailParticle trails[MAX_TRAIL];
static float eqBarHeights[EQUALIZER_BARS];
static float eqBarTargets[EQUALIZER_BARS];

static bool decosInitialized = false;

static Texture2D setaTexture = { 0 };
static Font customFont = { 0 };
static bool isAssetsLoaded = false;
static int activeFrames = 0;
static CurtainVFX curtainVfx;
static Skin::Button::UI_Button skinButton(0, 0, 190.0f, 74.0f, "스킨", "03", 2);

static bool isExitingGame = false;   
static float exitFadeTimer = 0.0f;  

static float logoScale = 0.0f;
static float logoAlpha = 0.0f;
static float logoRotation = 0.0f;
static float introTimer = 0.0f;
static bool introFinished = false;
static bool introImpactTriggered = false;
static float introFlashAlpha = 0.0f;

static float pulseTimer = 0.0f;
static float ringRotation1 = 0.0f;
static float ringRotation2 = 0.0f;

static bool menuOpen = false;
static float menuOpenAmount = 0.0f;
static float menuOpenVelocity = 0.0f;
static float menuClosePulse = 0.0f;
static float logoHover = 0.0f;
static float logoPress = 0.0f;
static float logoPulse = 0.0f;
static float beatScale = 0.0f;
static float beatImpact = 0.0f;
static float lastBeat = -1.0f;
static float beatTimer = 0.0f;
static float uiTime = 0.0f;
static float backgroundFade = 0.0f;
static int hoveredMenuIndex = -1;
static float menuItemHover[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
static float menuItemSelected[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
static float menuItemVelocity[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
static float menuItemSweep[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
static float menuItemPulse[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

static const float MENU_BPM = 180.0f;

// --- 완전한 블랙 & 화이트 미니멀 컬러 팔레트 ---
static const Color MONO_WHITE       = { 255, 255, 255, 255 };
static const Color MONO_GRAY_LIGHT  = { 200, 200, 200, 255 };
static const Color MONO_GRAY_MID    = { 100, 100, 100, 255 };
static const Color MONO_GRAY_DARK   = { 30,  30,  30,  255 };
static const Color MONO_BLACK_BG    = { 5,   5,   5,   255 };

static Color GetMonoColor(int index) {
    Color palette[3] = { MONO_WHITE, MONO_GRAY_LIGHT, MONO_GRAY_MID };
    return palette[index % 3];
}

static float Clamp01(float value) {
    return std::max(0.0f, std::min(1.0f, value));
}

static float EaseOutCubic(float value) {
    value = Clamp01(value);
    return 1.0f - std::pow(1.0f - value, 3.0f);
}

static float EaseOutBack(float value) {
    value = Clamp01(value);
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    return 1.0f + c3 * std::pow(value - 1.0f, 3.0f) + c1 * std::pow(value - 1.0f, 2.0f);
}

static Color WithAlpha(Color color, float alpha) {
    color.a = (unsigned char)(Clamp01(alpha) * 255.0f);
    return color;
}

static void DrawCenteredText(Font font, const char* text, float x, float y, float size, float spacing, Color color) {
    Vector2 textSize = MeasureTextEx(font, text, size, spacing);
    DrawTextEx(font, text, { x - textSize.x * 0.5f, y - textSize.y * 0.5f }, size, spacing, color);
}

static void SpawnBurst(Vector2 origin, int amount = MAX_BURST) {
    amount = std::min(amount, MAX_BURST);

    for (int i = 0; i < amount; ++i) {
        float angle = ((float)rand() / RAND_MAX) * PI * 2.0f;
        float speed = 60.0f + ((float)rand() / RAND_MAX) * 350.0f;

        bursts[i].position = origin;
        bursts[i].velocity = { std::cos(angle) * speed, std::sin(angle) * speed };
        bursts[i].size = 1.0f + ((float)rand() / RAND_MAX) * 2.5f;
        bursts[i].maxLife = 0.2f + ((float)rand() / RAND_MAX) * 0.4f;
        bursts[i].life = bursts[i].maxLife;
        bursts[i].alpha = 1.0f;
        bursts[i].color = GetMonoColor(rand() % 3);
    }
}

static void SpawnRadialParticles(int screenWidth, int screenHeight) {
    float cx = screenWidth * 0.5f;
    float cy = screenHeight * 0.39f;
    float maxRadius = std::max(screenWidth, screenHeight) * 0.65f;

    for (int i = 0; i < MAX_RADIAL_PARTICLES; ++i) {
        radialParticles[i].angle = ((float)rand() / RAND_MAX) * PI * 2.0f;
        radialParticles[i].distance = 100.0f + ((float)rand() / RAND_MAX) * maxRadius;
        radialParticles[i].speed = 5.0f + ((float)rand() / RAND_MAX) * 35.0f;
        radialParticles[i].size = 0.8f + ((float)rand() / RAND_MAX) * 1.8f;
        radialParticles[i].alpha = 0.1f + ((float)rand() / RAND_MAX) * 0.3f;
        radialParticles[i].maxLife = 4.0f + ((float)rand() / RAND_MAX) * 6.0f;
        radialParticles[i].life = ((float)rand() / RAND_MAX) * radialParticles[i].maxLife;
        radialParticles[i].color = GetMonoColor(i);
    }

    for (int i = 0; i < MAX_TRAIL; ++i) {
        trails[i].position = { cx, cy };
        trails[i].velocity = { 0.0f, 0.0f };
        trails[i].size = 1.0f;
        trails[i].alpha = 0.0f;
        trails[i].maxLife = 0.4f;
        trails[i].life = 0.0f;
        trails[i].color = GetMonoColor(i);
    }

    for (int i = 0; i < EQUALIZER_BARS; ++i) {
        eqBarHeights[i] = 2.0f;
        eqBarTargets[i] = 2.0f;
    }
}

static void UpdateRadialParticles(float dt, int screenWidth, int screenHeight) {
    float maxRadius = std::max(screenWidth, screenHeight) * 0.70f;

    for (int i = 0; i < MAX_RADIAL_PARTICLES; ++i) {
        radialParticles[i].distance += radialParticles[i].speed * dt;
        radialParticles[i].life -= dt;

        if (radialParticles[i].distance > maxRadius || radialParticles[i].life <= 0.0f) {
            radialParticles[i].angle = ((float)rand() / RAND_MAX) * PI * 2.0f;
            radialParticles[i].distance = 100.0f + ((float)rand() / RAND_MAX) * 60.0f;
            radialParticles[i].speed = 5.0f + ((float)rand() / RAND_MAX) * 35.0f;
            radialParticles[i].size = 0.8f + ((float)rand() / RAND_MAX) * 1.8f;
            radialParticles[i].alpha = 0.1f + ((float)rand() / RAND_MAX) * 0.3f;
            radialParticles[i].maxLife = 4.0f + ((float)rand() / RAND_MAX) * 6.0f;
            radialParticles[i].life = radialParticles[i].maxLife;
            radialParticles[i].color = GetMonoColor(rand() % 3);
        }
    }

    for (int i = 0; i < MAX_BURST; ++i) {
        if (bursts[i].life <= 0.0f) continue;

        bursts[i].life -= dt;
        bursts[i].position.x += bursts[i].velocity.x * dt;
        bursts[i].position.y += bursts[i].velocity.y * dt;
        bursts[i].velocity.x *= std::pow(0.08f, dt);
        bursts[i].velocity.y *= std::pow(0.08f, dt);
        bursts[i].alpha = Clamp01(bursts[i].life / bursts[i].maxLife);
    }

    for (int i = 0; i < MAX_TRAIL; ++i) {
        if (trails[i].life <= 0.0f) continue;

        trails[i].life -= dt;
        trails[i].position.x += trails[i].velocity.x * dt;
        trails[i].position.y += trails[i].velocity.y * dt;
        trails[i].velocity.x *= 0.90f;
        trails[i].velocity.y *= 0.90f;
        trails[i].alpha = Clamp01(trails[i].life / trails[i].maxLife);
    }

    for (int i = 0; i < EQUALIZER_BARS; ++i) {
        if (rand() % 12 == 0) {
            float wave = 0.4f + 0.6f * std::sin(uiTime * 2.5f + i * 0.31f);
            eqBarTargets[i] = (4.0f + (float)(rand() % 20)) * std::max(0.1f, wave);
        }

        eqBarHeights[i] += (eqBarTargets[i] - eqBarHeights[i]) * std::min(1.0f, dt * 8.0f);
    }
}

static void SpawnMenuTrail(Vector2 origin, Vector2 velocity) {
    for (int i = 0; i < MAX_TRAIL; ++i) {
        if (trails[i].life > 0.0f) continue;

        float angle = ((float)rand() / RAND_MAX) * PI * 2.0f;
        float spread = 15.0f + ((float)rand() / RAND_MAX) * 40.0f;

        trails[i].position = origin;
        trails[i].velocity = {
            velocity.x + std::cos(angle) * spread,
            velocity.y + std::sin(angle) * spread
        };
        trails[i].size = 1.0f + ((float)rand() / RAND_MAX) * 2.0f;
        trails[i].maxLife = 0.2f + ((float)rand() / RAND_MAX) * 0.35f;
        trails[i].life = trails[i].maxLife;
        trails[i].alpha = 0.6f;
        trails[i].color = GetMonoColor(i);
    }
}

// 배경 방사형 세밀 디자인 (은은하고 깔끔하게)
static void DrawRadialBackground(int width, int height, Vector2 center, float time, float beat) {
    float maxRadius = std::max(width, height) * 0.75f;

    for (int i = 0; i < RADIAL_RAYS; ++i) {
        float angle = (float)i / RADIAL_RAYS * PI * 2.0f + time * 0.02f;
        float startRadius = 160.0f + beat * 20.0f;
        float endRadius = maxRadius * 0.8f;

        Vector2 start = {
            center.x + std::cos(angle) * startRadius,
            center.y + std::sin(angle) * startRadius
        };

        Vector2 end = {
            center.x + std::cos(angle) * endRadius,
            center.y + std::sin(angle) * endRadius
        };

        DrawLineEx(start, end, 1.0f, WithAlpha(MONO_WHITE, 0.03f + beat * 0.04f));
    }

    for (int i = 0; i < 3; ++i) {
        float radius = 170.0f + i * 80.0f + beat * 10.0f;
        float rotation = time * (3.0f + i * 1.0f) * (i % 2 == 0 ? 1.0f : -1.0f);

        DrawRing(
            center,
            radius,
            radius + 1.0f,
            rotation,
            rotation + 45.0f,
            64,
            WithAlpha(MONO_WHITE, 0.08f + beat * 0.08f)
        );
    }

    for (int i = 0; i < MAX_RADIAL_PARTICLES; ++i) {
        float x = center.x + std::cos(radialParticles[i].angle) * radialParticles[i].distance;
        float y = center.y + std::sin(radialParticles[i].angle) * radialParticles[i].distance;
        float lifeRatio = Clamp01(radialParticles[i].life / radialParticles[i].maxLife);
        float alpha = radialParticles[i].alpha * lifeRatio;

        DrawCircleV({ x, y }, radialParticles[i].size, WithAlpha(radialParticles[i].color, alpha * 0.6f));
    }
}

// 지저분한 복잡 장식들을 완전히 제거하고 깔끔한 최상단/하단 가이드라인만 유지
static void DrawMinimalAtmosphere(int width, int height, float alpha) {
    float scale = (float)height / 720.0f;
    float margin = std::max(26.0f * scale, width * 0.025f);
    
    // 세련된 상하단 경계 라인
    DrawLineEx({ margin, height * 0.16f }, { width - margin, height * 0.16f }, 1.0f, WithAlpha(MONO_GRAY_MID, 0.3f * alpha));
    DrawLineEx({ margin, height * 0.88f }, { width - margin, height * 0.88f }, 1.0f, WithAlpha(MONO_GRAY_MID, 0.3f * alpha));

    DrawText("R Y T H M   A I R", (int)margin, (int)(height * 0.91f), (int)std::max(9.0f, 10.0f * scale), WithAlpha(MONO_WHITE, 0.7f * alpha));
    
    const char* subInfo = "MINIMALIST AUDIO VISUAL";
    int subW = MeasureText(subInfo, (int)std::max(9.0f, 9.0f * scale));
    DrawText(subInfo, width - (int)margin - subW, (int)(height * 0.91f), (int)std::max(9.0f, 9.0f * scale), WithAlpha(MONO_GRAY_LIGHT, 0.5f * alpha));
}

static void DrawLogoDecorations(Vector2 center, float radius, float time, float beat, float alpha) {
    float r1 = radius + 20.0f + beat * 12.0f;

    DrawRing(center, r1, r1 + 1.2f, ringRotation1, ringRotation1 + 90.0f, 64, WithAlpha(MONO_WHITE, alpha * (0.3f + beat * 0.2f)));

    float cross = radius + 40.0f;
    DrawLineEx({ center.x - cross, center.y }, { center.x - radius - 6.0f, center.y }, 1.0f, WithAlpha(MONO_WHITE, alpha * 0.4f));
    DrawLineEx({ center.x + radius + 6.0f, center.y }, { center.x + cross, center.y }, 1.0f, WithAlpha(MONO_WHITE, alpha * 0.4f));
    DrawLineEx({ center.x, center.y - cross }, { center.x, center.y - radius - 6.0f }, 1.0f, WithAlpha(MONO_WHITE, alpha * 0.4f));
    DrawLineEx({ center.x, center.y + radius + 6.0f }, { center.x, center.y + cross }, 1.0f, WithAlpha(MONO_WHITE, alpha * 0.4f));
}

// --- 완전히 군더더기 없는 하얀색/검은색 기반의 깔끔한 버튼 UI ---
static void DrawExpandedMenuItem(Font font, const char* label, const char* indexText, Vector2 center, float open, float selected, float hover, float time, int index) {
    float uiScale = (float)GetScreenHeight() / 720.0f;
    float screenW = (float)GetScreenWidth();
    float itemOpen = EaseOutBack(open);

    float spacing = 210.0f * uiScale;
    float startX = screenW * 0.5f - 1.5f * spacing;
    float targetX = startX + index * spacing;
    float targetY = center.y + 240.0f * uiScale;

    float x = center.x + (targetX - center.x) * itemOpen;
    float y = targetY;
    float appear = Clamp01(open * 1.3f - index * 0.055f);
    float width = 185.0f * uiScale;
    float height = 70.0f * uiScale;
    Rectangle box = { x - width * 0.5f, y - height * 0.5f, width, height };
    if (appear <= 0.01f) return;

    // 100% 흑백 기반 색상 결정
    Color panelBg;
    Color borderColor;
    Color textColor;
    Color indexColor;

    if (selected > 0.5f) {
        // 선택 시 반전 (하얀색 배경, 검은색 글씨)
        panelBg = WithAlpha(MONO_WHITE, 0.95f * appear);
        borderColor = WithAlpha(MONO_WHITE, appear);
        textColor = WithAlpha(MONO_BLACK_BG, appear);
        indexColor = WithAlpha(MONO_GRAY_DARK, appear);
    } else if (hover > 0.5f) {
        // 마우스 호버 시 (짙은 어두운 회색 배경, 하얀색 글씨)
        panelBg = WithAlpha(MONO_GRAY_DARK, 0.9f * appear);
        borderColor = WithAlpha(MONO_WHITE, 0.8f * appear);
        textColor = WithAlpha(MONO_WHITE, appear);
        indexColor = WithAlpha(MONO_GRAY_LIGHT, appear);
    } else {
        // 기본 상태 (깔끔한 검은색 패널, 얇은 테두리)
        panelBg = WithAlpha(MONO_BLACK_BG, 0.85f * appear);
        borderColor = WithAlpha(MONO_GRAY_MID, 0.5f * appear);
        textColor = WithAlpha(MONO_GRAY_LIGHT, 0.9f * appear);
        indexColor = WithAlpha(MONO_GRAY_MID, 0.7f * appear);
    }

    // 직선 위주의 명확하고 깔끔한 모서리 사각형
    DrawRectangleRounded(box, 0.12f, 4, panelBg);
    DrawRectangleRoundedLines(box, 0.12f, 4, borderColor);

    // 인덱스 번호 (01, 02...)
    DrawTextEx(font, indexText, { box.x + 12.0f * uiScale, box.y + 10.0f * uiScale }, 10.0f * uiScale, 1.0f * uiScale, indexColor);

    // 메뉴 텍스트
    float labelFontSize = (19.0f + selected * 1.5f) * uiScale;
    Vector2 labelSize = MeasureTextEx(font, label, labelFontSize, 1.0f * uiScale);
    DrawTextEx(font, label, { box.x + (box.width - labelSize.x) * 0.5f, box.y + (box.height - labelSize.y) * 0.5f + 2.0f * uiScale }, labelFontSize, 1.0f * uiScale, textColor);
}

MainMenu::MainMenu() {
    selectedIndex = 0;
    pulseTimer = 0.0f;
    currentState = MenuState::Main;
    SlowScreen::Init();
}

void MainMenu::Update() {
    float dt = std::min(GetFrameTime(), 0.05f);
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    if (isExitingGame) {
        exitFadeTimer += dt * 1.5f;
        if (exitFadeTimer >= 1.0f) {
            CloseWindow();
            std::exit(0);
        }
        return;
    }

    SlowScreen::Update(dt);
    curtainVfx.Update(dt);
    if (curtainVfx.IsActive()) return;

    unsigned long long dspClock = LobbyBgm_GetDSPClock();
    int sampleRate = LobbyBgm_GetSamplingRate();
    if (sampleRate > 0) {
        pulseTimer = (float)dspClock / (float)sampleRate;
    } else {
        pulseTimer += dt;
    }
    uiTime += dt;
    beatTimer += dt;

    if (!decosInitialized && screenW > 0 && screenH > 0) {
        SpawnRadialParticles(screenW, screenH);
        decosInitialized = true;
    }

    if (currentState == MenuState::SongSelect) {
        activeFrames++;

        if (activeFrames < 5) return;

        songSelect.Update();

        if (songSelect.IsPlaySelected()) return;

        if (songSelect.IsBackSelected()) {
            currentState = MenuState::Main;
            activeFrames = 0;
            menuOpen = false;
            menuOpenAmount = 0.0f;
            selectedIndex = 0;
        }

        return;
    }

    LobbyBgm_Init();
    LobbyBgm_Update();

    UpdateRadialParticles(dt, screenW, screenH);

    ringRotation1 += dt * 12.0f;
    ringRotation2 -= dt * 8.0f;

    backgroundFade += (1.0f - backgroundFade) * std::min(1.0f, dt * 4.0f);

    if (introFlashAlpha > 0.0f) {
        introFlashAlpha -= dt * 3.5f;
        if (introFlashAlpha < 0.0f) introFlashAlpha = 0.0f;
    }

    float beatPeriod = 60.0f / MENU_BPM;
    float beatPosition = std::fmod(pulseTimer, beatPeriod) / beatPeriod;
    float beatEnvelope = std::exp(-beatPosition * 8.0f);
    beatScale += (beatEnvelope - beatScale) * std::min(1.0f, dt * 24.0f);
    beatImpact += (beatEnvelope - beatImpact) * std::min(1.0f, dt * 18.0f);

    float beatNumber = std::floor(pulseTimer / beatPeriod);
    if (beatNumber != lastBeat) {
        lastBeat = beatNumber;
        beatScale = 1.0f;
        beatImpact = 1.0f;

        if (introFinished) {
            Vector2 center = { screenW * 0.5f, screenH * 0.39f };
            SpawnMenuTrail(center, { 0.0f, 0.0f });
        }
    }

    if (!introFinished) {
        introTimer += dt;
        int introMs = (int)(introTimer * 1000.0f);

        if (introMs < 800) {
            logoScale = 0.0f;
            logoAlpha = 0.0f;
            logoRotation = 0.0f;
        }
        else if (introMs < 1600) {
            if (!introImpactTriggered) {
                introImpactTriggered = true;
                SpawnBurst({ screenW * 0.5f, screenH * 0.39f }, MAX_BURST);
                introFlashAlpha = 0.35f;
            }

            float t = (float)(introMs - 800) / 800.0f;
            float ease = EaseOutBack(t);
            logoScale = 2.5f + (1.0f - 2.5f) * ease;
            logoAlpha = EaseOutCubic(t);
            logoRotation = -180.0f * (1.0f - EaseOutCubic(t));
        }
        else {
            logoScale = 1.0f;
            logoAlpha = 1.0f;
            logoRotation = 0.0f;
        }

        if (introMs >= 3500) {
            introFinished = true;
            logoScale = 1.0f;
            logoAlpha = 1.0f;
            logoRotation = 0.0f;
        }

        return;
    }

    Vector2 mouse = GetMousePosition();
    
    float parallaxX = ((mouse.x / (float)std::max(1, screenW)) - 0.5f) * 6.0f;
    float parallaxY = ((mouse.y / (float)std::max(1, screenH)) - 0.5f) * 4.0f;

    Vector2 logoCenter = {
        screenW * 0.5f + parallaxX,
        screenH * 0.39f + parallaxY
    };

    float uiScale = (float)screenH / 720.0f;
    float logoBaseRadius = std::min(screenW, screenH) * 0.25f;
    logoBaseRadius = std::max(150.0f * uiScale, std::min(280.0f * uiScale, logoBaseRadius));

    float actualBeatScale = Clamp01(beatImpact) * 0.06f;
    float hoverScale = logoHover * 0.03f;
    float visualScale = logoScale * (1.0f + actualBeatScale + hoverScale);
    float logoRadius = logoBaseRadius * visualScale;

    float distanceToLogo = std::sqrt(
        std::pow(mouse.x - logoCenter.x, 2.0f) +
        std::pow(mouse.y - logoCenter.y, 2.0f)
    );

    bool insideLogo = distanceToLogo <= logoRadius;

    float hoverTarget = insideLogo ? 1.0f : 0.0f;
    logoHover += (hoverTarget - logoHover) * std::min(1.0f, dt * 12.0f);

    if (insideLogo && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        menuOpen = !menuOpen;
        SpawnBurst(logoCenter, 30);

        if (menuOpen) {
            selectedIndex = 0;
            for (int i = 0; i < 4; ++i) {
                menuItemPulse[i] = 1.0f;
            }
        }
    }

    float openTarget = menuOpen ? 1.0f : 0.0f;
    float openForce = (openTarget - menuOpenAmount) * 100.0f;
    menuOpenVelocity += openForce * dt;
    menuOpenVelocity *= 0.77f;
    menuOpenAmount += menuOpenVelocity * dt;
    menuOpenAmount = Clamp01(menuOpenAmount);

    hoveredMenuIndex = -1;

    if (menuOpenAmount > 0.03f) {
        float spacing = 210.0f * uiScale;
        float startX = screenW * 0.5f - 1.5f * spacing;

        float targetX2 = startX + 2 * spacing;
        float targetY2 = logoCenter.y + 240.0f * uiScale;
        float itemOpen = EaseOutBack(menuOpenAmount);
        float x2 = logoCenter.x + (targetX2 - logoCenter.x) * itemOpen;
        float y2 = targetY2;
        float w2 = 185.0f * uiScale;
        float h2 = 70.0f * uiScale;

        skinButton.SetSize(w2, h2); 
        skinButton.SetPosition(x2 - w2 * 0.5f, y2 - h2 * 0.5f);
        skinButton.Update(dt, menuOpen && selectedIndex == 2, hoveredMenuIndex == 2, mouse, IsMouseButtonPressed(MOUSE_BUTTON_LEFT));

        if (skinButton.CheckCollision(mouse)) {
            hoveredMenuIndex = 2;
        }

        for (int i = 0; i < 4; ++i) {
            if (i == 2) continue;

            Vector2 p = {
                startX + i * spacing,
                logoCenter.y + 240.0f * uiScale
            };

            float w = 185.0f * uiScale;
            float h = 70.0f * uiScale;

            Rectangle hit = {
                p.x - w * 0.5f,
                p.y - h * 0.5f,
                w,
                h
            };

            if (CheckCollisionPointRec(mouse, hit)) {
                hoveredMenuIndex = i;
            }
        }
    }

    if (hoveredMenuIndex >= 0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        selectedIndex = hoveredMenuIndex;
        SpawnBurst(mouse, 25);

        if (selectedIndex == 0) {
            curtainVfx.Start([this]() {
                currentState = MenuState::SongSelect;
                songSelect.Init();
                activeFrames = 0;
                menuOpen = false;
                menuOpenAmount = 0.0f;
                LobbyBgm_Stop();
            });
            return;
        }

        if (selectedIndex == 1) {
            curtainVfx.Start([this]() {});
            return;
        }

        if (selectedIndex == 2) {
            skinButton.ToggleExpand();
            return;
        }

        if (selectedIndex == 3) {
            curtainVfx.Start([this]() {
                isExitingGame = true;
            });
            return;
        }
    }

    if (menuOpen) {
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
            selectedIndex = (selectedIndex - 1 + 4) % 4;
        }

        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
            selectedIndex = (selectedIndex + 1) % 4;
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            menuOpen = false;
        }

        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            if (selectedIndex == 0) {
                curtainVfx.Start([this]() {
                    currentState = MenuState::SongSelect;
                    songSelect.Init();
                    activeFrames = 0;
                    menuOpen = false;
                    menuOpenAmount = 0.0f;
                    LobbyBgm_Stop();
                });
                return;
            }

            if (selectedIndex == 1) {
                curtainVfx.Start([this]() {});
                return;
            }

            if (selectedIndex == 2) {
                skinButton.ToggleExpand();
                return;
            }

            if (selectedIndex == 3) {
                curtainVfx.Start([this]() {
                    isExitingGame = true;
                });
                return;
            }
        }
    }
    else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        menuOpen = true;
        selectedIndex = 0;
        SpawnBurst(logoCenter, 30);
    }

    for (int i = 0; i < 4; ++i) {
        float targetSelected = menuOpen && selectedIndex == i ? 1.0f : 0.0f;
        float force = (targetSelected - menuItemSelected[i]) * 115.0f;
        menuItemVelocity[i] += force * dt;
        menuItemVelocity[i] *= 0.78f;
        menuItemSelected[i] += menuItemVelocity[i] * dt;
        menuItemSelected[i] = Clamp01(menuItemSelected[i]);

        float hover = hoveredMenuIndex == i ? 1.0f : 0.0f;
        menuItemHover[i] += (hover - menuItemHover[i]) * std::min(1.0f, dt * 12.0f);
    }
}

void MainMenu::Draw(int screenWidth, int screenHeight) {
    if (currentState == MenuState::SongSelect) {
        songSelect.Draw(screenWidth, screenHeight);
        curtainVfx.Draw(screenWidth, screenHeight);
        SlowScreen::Draw(screenWidth, screenHeight);
        return;
    }

    if (!isAssetsLoaded) {
        setaTexture = LoadTexture("assets/Seta.png");
        if (setaTexture.id != 0) {
            SetTextureFilter(setaTexture, TEXTURE_FILTER_BILINEAR);
            GenTextureMipmaps(&setaTexture);
        }

        int codepoints[512];
        int count = 0;

        for (int i = 32; i <= 126; ++i) {
            codepoints[count++] = i;
        }

        const wchar_t* krText = L"플레이설정스킨나가기메인메뉴시스템온라인기본도형아래개중고르세요RythmAir방향키선택엔터스페이스확인닫기클릭엔터메뉴열기";
        for (int i = 0; krText[i] != L'\0'; ++i) {
            codepoints[count++] = (int)krText[i];
        }

        if (FileExists("fonts/Hakgyoansim_Dunggeunmiso_TTF_B.ttf")) {
            customFont = LoadFontEx("fonts/Hakgyoansim_Dunggeunmiso_TTF_B.ttf", 64, codepoints, count);
        } else if (FileExists("fonts/Pretendard-Black.ttf")) {
            customFont = LoadFontEx("fonts/Pretendard-Black.ttf", 64, codepoints, count);
        } else if (FileExists("assets/font.ttf")) {
            customFont = LoadFontEx("assets/font.ttf", 64, codepoints, count);
        } else if (FileExists("C:/Windows/Fonts/malgun.ttf")) {
            customFont = LoadFontEx("C:/Windows/Fonts/malgun.ttf", 64, codepoints, count);
        } else {
            customFont = GetFontDefault();
        }

        if (customFont.texture.id != 0) SetTextureFilter(customFont.texture, TEXTURE_FILTER_BILINEAR);
        isAssetsLoaded = true;
    }

    // 칠흑 같은 매트한 딥 블랙 배경
    ClearBackground(MONO_BLACK_BG);

    Vector2 mouse = GetMousePosition();
    float parallaxX = ((mouse.x / (float)std::max(1, screenWidth)) - 0.5f) * 6.0f;
    float parallaxY = ((mouse.y / (float)std::max(1, screenHeight)) - 0.5f) * 4.0f;

    Vector2 center = {
        screenWidth * 0.5f + parallaxX,
        screenHeight * 0.39f + parallaxY
    };

    float beat = Clamp01(beatScale);
    float strongBeat = Clamp01(beatImpact);
    float uiScale = (float)screenHeight / 720.0f;

    MenuBackground::Draw(screenWidth, screenHeight);
    DrawRadialBackground(screenWidth, screenHeight, center, uiTime, beat);
    DrawMinimalAtmosphere(screenWidth, screenHeight, introFinished ? 1.0f : Clamp01((introTimer - 0.2f) / 0.8f));

    float logoBaseRadius = std::min(screenWidth, screenHeight) * 0.25f;
    logoBaseRadius = std::max(150.0f * uiScale, std::min(280.0f * uiScale, logoBaseRadius));

    if (!introFinished) {
        float borderProgress = Clamp01((introTimer * 1000.0f) / 800.0f);
        float currentRadius = logoBaseRadius * 1.5f * EaseOutCubic(borderProgress);
        DrawRing(center, currentRadius - 1.0f, currentRadius + 1.0f, 0.0f, 360.0f * borderProgress, 64, WithAlpha(MONO_WHITE, 0.8f));
    }

    float visualScale = logoScale * (1.0f + strongBeat * 0.06f + logoHover * 0.03f);

    DrawLogoDecorations(center, logoBaseRadius * visualScale, uiTime, strongBeat, logoAlpha);

    if (setaTexture.id != 0) {
        Rectangle src = { 0.0f, 0.0f, (float)setaTexture.width, (float)setaTexture.height };

        float maxTexDim = (float)std::max(setaTexture.width, setaTexture.height);
        float targetDiameter = logoBaseRadius * 2.0f * visualScale;
        float textureScale = (maxTexDim > 0.0f) ? (targetDiameter / maxTexDim) : 1.0f;

        float textureWidth = setaTexture.width * textureScale;
        float textureHeight = setaTexture.height * textureScale;

        Rectangle dest = { center.x, center.y, textureWidth, textureHeight };
        Vector2 origin = { dest.width * 0.5f, dest.height * 0.5f };

        DrawTexturePro(
            setaTexture,
            src,
            dest,
            origin,
            logoRotation,
            Color{ 255, 255, 255, (unsigned char)(logoAlpha * 255.0f) }
        );
    }

    if (logoHover > 0.01f && !menuOpen) {
        float ring = logoBaseRadius * 1.1f;
        DrawRing(center, ring, ring + 1.5f * uiScale, ringRotation1, ringRotation1 + 270.0f, 64, WithAlpha(MONO_WHITE, 0.5f + logoHover * 0.5f));
    }

    // 메뉴 확장 시 4개 버튼 출력
    if (menuOpenAmount > 0.01f) {
        const char* labels[4] = { "플레이", "설정", "스킨", "나가기" };
        const char* indices[4] = { "01", "02", "03", "04" };

        for (int i = 0; i < 4; ++i) {
            if (i == 2) {
                skinButton.Draw(customFont, menuOpenAmount, uiTime);
            } else {
                DrawExpandedMenuItem(
                    customFont,
                    labels[i],
                    indices[i],
                    center,
                    menuOpenAmount,
                    menuItemSelected[i],
                    menuItemHover[i],
                    uiTime,
                    i
                );
            }
        }
    }

    float titleAlpha = introFinished ? 1.0f : Clamp01((introTimer - 0.8f) / 0.5f);

    // 깔끔한 타이틀
    DrawCenteredText(
        customFont,
        "Rythm Air",
        screenWidth * 0.5f,
        screenHeight * 0.09f,
        std::min(26.0f * uiScale, screenWidth * 0.021f),
        4.0f * uiScale,
        WithAlpha(MONO_WHITE, titleAlpha * 0.95f)
    );

    DrawCenteredText(
        customFont,
        menuOpen ? "SELECT OPTION" : "PRESS ENTER TO BEGIN",
        screenWidth * 0.5f,
        screenHeight * 0.125f,
        std::min(10.0f * uiScale, screenWidth * 0.008f),
        2.0f * uiScale,
        WithAlpha(MONO_GRAY_LIGHT, titleAlpha * 0.75f)
    );

    if (introFinished) {
        const char* hint = menuOpen
            ? "[LEFT / RIGHT] SELECT     [ENTER / SPACE] CONFIRM     [ESC] CLOSE"
            : "[CLICK / ENTER] OPEN MENU";

        Vector2 hintSize = MeasureTextEx(customFont, hint, 9.0f * uiScale, 1.2f * uiScale);

        DrawTextEx(
            customFont,
            hint,
            { screenWidth * 0.5f - hintSize.x * 0.5f, screenHeight * 0.94f },
            9.0f * uiScale,
            1.2f * uiScale,
            WithAlpha(MONO_GRAY_MID, 0.8f)
        );
    }

    for (int i = 0; i < MAX_TRAIL; ++i) {
        if (trails[i].life <= 0.0f) continue;
        DrawCircleV(trails[i].position, trails[i].size * uiScale, WithAlpha(trails[i].color, trails[i].alpha * 0.5f));
    }

    for (int i = 0; i < MAX_BURST; ++i) {
        if (bursts[i].life <= 0.0f) continue;
        Color color = bursts[i].color;
        color.a = (unsigned char)(bursts[i].alpha * 200.0f);
        DrawCircleV(bursts[i].position, bursts[i].size * uiScale, color);
    }

    if (introFlashAlpha > 0.001f) {
        DrawRectangle(0, 0, screenWidth, screenHeight, WithAlpha(MONO_WHITE, introFlashAlpha * 0.4f));
    }

    curtainVfx.Draw(screenWidth, screenHeight);
    SlowScreen::Draw(screenWidth, screenHeight);

    if (isExitingGame) {
        float alpha = std::max(0.0f, std::min(1.0f, exitFadeTimer));
        DrawRectangle(0, 0, screenWidth, screenHeight, Color{ 0, 0, 0, (unsigned char)(alpha * 255.0f)});
    }
}

bool MainMenu::IsGameStartSelected() const {
    if (activeFrames < 10) return false;
    return currentState == MenuState::SongSelect && songSelect.IsPlaySelected();
}

bool MainMenu::IsExitSelected() const {
    return currentState == MenuState::Main &&
           selectedIndex == 3 &&
           introFinished &&
           (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE));
}

void MainMenu::Reset() {
    isExitingGame = false;
    exitFadeTimer = 0.0f;
    LobbyBgm_Stop();
    selectedIndex = 0;
    pulseTimer = 0.0f;
    currentState = MenuState::Main;
    activeFrames = 0;
    SlowScreen::Init();
    curtainVfx.Init();

    introTimer = 0.0f;
    introFinished = false;
    introImpactTriggered = false;
    introFlashAlpha = 0.0f;

    logoScale = 0.0f;
    logoAlpha = 0.0f;
    logoRotation = 0.0f;

    ringRotation1 = 0.0f;
    ringRotation2 = 0.0f;

    menuOpen = false;
    menuOpenAmount = 0.0f;
    menuOpenVelocity = 0.0f;
    menuClosePulse = 0.0f;
    logoHover = 0.0f;
    logoPress = 0.0f;
    logoPulse = 0.0f;
    beatScale = 0.0f;
    beatImpact = 0.0f;
    lastBeat = -1.0f;
    beatTimer = 0.0f;
    uiTime = 0.0f;
    backgroundFade = 0.0f;
    hoveredMenuIndex = -1;

    for (int i = 0; i < 4; ++i) {
        menuItemHover[i] = 0.0f;
        menuItemSelected[i] = 0.0f;
        menuItemVelocity[i] = 0.0f;
        menuItemSweep[i] = 0.0f;
        menuItemPulse[i] = 0.0f;
    }
}