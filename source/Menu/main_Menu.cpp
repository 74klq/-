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

static const int MAX_RADIAL_PARTICLES = 150;
static const int MAX_BURST = 70;
static const int MAX_TRAIL = 80;
static const int RADIAL_RAYS = 48;
static const int EQUALIZER_BARS = 36;

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
static const Color PASTE_PINK = { 255, 182, 193, 255 };
static const Color PASTE_YELLOW = { 253, 238, 154, 255 };
static const Color PASTE_BLUE = { 135, 206, 235, 255 };
static const Color PASTE_MINT = { 152, 251, 152, 255 };
static const Color PASTE_PURPLE = { 221, 160, 221, 255 };
static const Color BLACK_DEEP = { 3, 3, 3, 255 };

static Color GetPastelColor(int index) {
    Color palette[5] = { PASTE_PINK, PASTE_YELLOW, PASTE_BLUE, PASTE_MINT, PASTE_PURPLE };
    return palette[index % 5];
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
        float speed = 80.0f + ((float)rand() / RAND_MAX) * 520.0f;

        bursts[i].position = origin;
        bursts[i].velocity = { std::cos(angle) * speed, std::sin(angle) * speed };
        bursts[i].size = 1.5f + ((float)rand() / RAND_MAX) * 5.5f;
        bursts[i].maxLife = 0.28f + ((float)rand() / RAND_MAX) * 0.62f;
        bursts[i].life = bursts[i].maxLife;
        bursts[i].alpha = 1.0f;
        bursts[i].color = GetPastelColor(rand() % 5);
    }
}

static void SpawnRadialParticles(int screenWidth, int screenHeight) {
    float cx = screenWidth * 0.5f;
    float cy = screenHeight * 0.39f;
    float maxRadius = std::max(screenWidth, screenHeight) * 0.68f;

    for (int i = 0; i < MAX_RADIAL_PARTICLES; ++i) {
        radialParticles[i].angle = ((float)rand() / RAND_MAX) * PI * 2.0f;
        radialParticles[i].distance = 120.0f + ((float)rand() / RAND_MAX) * maxRadius;
        radialParticles[i].speed = 8.0f + ((float)rand() / RAND_MAX) * 55.0f;
        radialParticles[i].size = 0.8f + ((float)rand() / RAND_MAX) * 2.8f;
        radialParticles[i].alpha = 0.15f + ((float)rand() / RAND_MAX) * 0.55f;
        radialParticles[i].maxLife = 4.0f + ((float)rand() / RAND_MAX) * 7.0f;
        radialParticles[i].life = ((float)rand() / RAND_MAX) * radialParticles[i].maxLife;
        radialParticles[i].color = GetPastelColor(i);
    }

    for (int i = 0; i < MAX_TRAIL; ++i) {
        trails[i].position = { cx, cy };
        trails[i].velocity = { 0.0f, 0.0f };
        trails[i].size = 1.2f;
        trails[i].alpha = 0.0f;
        trails[i].maxLife = 0.5f;
        trails[i].life = 0.0f;
        trails[i].color = GetPastelColor(i);
    }

    for (int i = 0; i < EQUALIZER_BARS; ++i) {
        eqBarHeights[i] = 2.0f;
        eqBarTargets[i] = 2.0f;
    }
}

static void UpdateRadialParticles(float dt, int screenWidth, int screenHeight) {
    float maxRadius = std::max(screenWidth, screenHeight) * 0.76f;

    for (int i = 0; i < MAX_RADIAL_PARTICLES; ++i) {
        radialParticles[i].distance += radialParticles[i].speed * dt;
        radialParticles[i].life -= dt;

        if (radialParticles[i].distance > maxRadius || radialParticles[i].life <= 0.0f) {
            radialParticles[i].angle = ((float)rand() / RAND_MAX) * PI * 2.0f;
            radialParticles[i].distance = 120.0f + ((float)rand() / RAND_MAX) * 90.0f;
            radialParticles[i].speed = 8.0f + ((float)rand() / RAND_MAX) * 55.0f;
            radialParticles[i].size = 0.8f + ((float)rand() / RAND_MAX) * 2.8f;
            radialParticles[i].alpha = 0.15f + ((float)rand() / RAND_MAX) * 0.55f;
            radialParticles[i].maxLife = 4.0f + ((float)rand() / RAND_MAX) * 7.0f;
            radialParticles[i].life = radialParticles[i].maxLife;
            radialParticles[i].color = GetPastelColor(rand() % 5);
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
        trails[i].velocity.x *= 0.91f;
        trails[i].velocity.y *= 0.91f;
        trails[i].alpha = Clamp01(trails[i].life / trails[i].maxLife);
    }

    for (int i = 0; i < EQUALIZER_BARS; ++i) {
        if (rand() % 10 == 0) {
            float wave = 0.4f + 0.6f * std::sin(uiTime * 3.5f + i * 0.31f);
            eqBarTargets[i] = (5.0f + (float)(rand() % 35)) * std::max(0.12f, wave);
        }

        eqBarHeights[i] += (eqBarTargets[i] - eqBarHeights[i]) * std::min(1.0f, dt * 9.0f);
    }
}

static void SpawnMenuTrail(Vector2 origin, Vector2 velocity) {
    for (int i = 0; i < MAX_TRAIL; ++i) {
        if (trails[i].life > 0.0f) continue;

        float angle = ((float)rand() / RAND_MAX) * PI * 2.0f;
        float spread = 18.0f + ((float)rand() / RAND_MAX) * 55.0f;

        trails[i].position = origin;
        trails[i].velocity = {
            velocity.x + std::cos(angle) * spread,
            velocity.y + std::sin(angle) * spread
        };
        trails[i].size = 1.2f + ((float)rand() / RAND_MAX) * 3.0f;
        trails[i].maxLife = 0.25f + ((float)rand() / RAND_MAX) * 0.45f;
        trails[i].life = trails[i].maxLife;
        trails[i].alpha = 0.8f;
        trails[i].color = GetPastelColor(i);
    }
}

static void DrawRadialBackground(int width, int height, Vector2 center, float time, float beat) {
    float maxRadius = std::max(width, height) * 0.78f;

    for (int i = 0; i < RADIAL_RAYS; ++i) {
        float angle = (float)i / RADIAL_RAYS * PI * 2.0f + time * 0.035f;
        float widthPulse = 0.7f + 0.3f * std::sin(time * 1.7f + i * 0.21f);
        float startRadius = 160.0f + beat * 34.0f;
        float endRadius = maxRadius * (0.82f + widthPulse * 0.12f);

        Vector2 start = {
            center.x + std::cos(angle) * startRadius,
            center.y + std::sin(angle) * startRadius
        };

        Vector2 end = {
            center.x + std::cos(angle) * endRadius,
            center.y + std::sin(angle) * endRadius
        };

        Color rayColor = GetPastelColor(i);
        DrawLineEx(
            start,
            end,
            1.2f + beat * 1.8f,
            WithAlpha(rayColor, 0.1f + widthPulse * 0.12f + beat * 0.15f)
        );
    }

    for (int i = 0; i < 7; ++i) {
        float radius = 155.0f + i * 55.0f + beat * (18.0f - i * 1.5f);
        float rotation = time * (5.0f + i * 1.2f) * (i % 2 == 0 ? 1.0f : -1.0f);
        float arcLength = 38.0f + std::sin(time * 1.8f + i) * 16.0f;

        Color ringColor = GetPastelColor(i);
        DrawRing(
            center,
            radius,
            radius + 1.5f + beat,
            rotation,
            rotation + arcLength,
            64,
            WithAlpha(ringColor, 0.2f + beat * 0.25f)
        );
    }

    for (int i = 0; i < MAX_RADIAL_PARTICLES; ++i) {
        float x = center.x + std::cos(radialParticles[i].angle) * radialParticles[i].distance;
        float y = center.y + std::sin(radialParticles[i].angle) * radialParticles[i].distance;
        float lifeRatio = Clamp01(radialParticles[i].life / radialParticles[i].maxLife);
        float distanceFade = Clamp01(1.0f - radialParticles[i].distance / (maxRadius * 1.05f));
        float alpha = radialParticles[i].alpha * (0.3f + distanceFade * 0.7f) * (0.45f + lifeRatio * 0.55f);

        DrawCircleV(
            { x, y },
            radialParticles[i].size + beat * 0.9f,
            WithAlpha(radialParticles[i].color, alpha * 0.8f)
        );
    }

    float halo = 210.0f + beat * 28.0f + std::sin(time * 1.6f) * 8.0f;
    DrawCircleV(center, halo, WithAlpha(PASTE_PINK, 0.08f + beat * 0.12f));
    DrawCircleV(center, halo * 0.72f, WithAlpha(PASTE_BLUE, 0.08f + beat * 0.12f));
}

static void DrawBottomBars(int width, int height, float beat) {
    float barWidth = (float)width / EQUALIZER_BARS;

    for (int i = 0; i < EQUALIZER_BARS; ++i) {
        float centerDistance = std::abs(i - EQUALIZER_BARS * 0.5f) / (EQUALIZER_BARS * 0.5f);
        float h = eqBarHeights[i] * (1.0f - centerDistance * 0.45f) + beat * 14.0f * (1.0f - centerDistance);

        Color barColor = GetPastelColor(i);
        DrawRectangle(
            (int)(i * barWidth),
            height - (int)h,
            (int)std::max(1.0f, barWidth - 2.0f),
            (int)h,
            WithAlpha(barColor, 0.25f + beat * 0.35f)
        );
    }
}

static void DrawPremiumAtmosphere(int width, int height, float time, float alpha, float beat) {
    float scale = (float)height / 720.0f;
    float margin = std::max(26.0f * scale, width * 0.025f);
    Color faint = WithAlpha(PASTE_BLUE, 0.12f * alpha);
    Color dim = WithAlpha(PASTE_PINK, 0.35f * alpha);
    float gridSpacing = 46.0f * scale;
    float drift = std::fmod(time * 5.0f * scale, gridSpacing);
    for (float x = -gridSpacing + drift; x < width + gridSpacing; x += gridSpacing) {
        DrawLineEx({ x, height * 0.22f }, { x, height * 0.84f }, 1.0f, faint);
    }
    for (float y = height * 0.22f; y < height * 0.84f; y += gridSpacing) {
        DrawLineEx({ width * 0.06f, y }, { width * 0.94f, y }, 1.0f, faint);
    }
    float scanY = std::fmod(time * 85.0f * scale, (float)height);
    DrawRectangle(0, (int)scanY, width, std::max(1, (int)(1.0f * scale)), WithAlpha(PASTE_YELLOW, 0.15f * alpha));
    DrawLineEx({ margin, height * 0.205f }, { width - margin, height * 0.205f }, 1.5f, dim);
    DrawLineEx({ margin, height * 0.855f }, { width - margin, height * 0.855f }, 1.5f, dim);
    float centerX = width * 0.5f;
    for (int side = 0; side < 2; ++side) {
        float originX = side == 0 ? margin + 8.0f * scale : width - margin - 8.0f * scale;
        float direction = side == 0 ? 1.0f : -1.0f;
        for (int i = 0; i < 52; ++i) {
            float phase = time * 2.3f + i * 0.27f + side * 1.7f;
            float envelope = 0.25f + 0.75f * std::abs(std::sin(phase * 0.72f));
            float h = (3.0f + envelope * (10.0f + beat * 15.0f)) * scale;
            float x = originX + direction * i * 3.2f * scale;
            float y = height * 0.5f + std::sin(phase) * 23.0f * scale;
            Color multiColor = GetPastelColor(i);
            DrawLineEx({x, y - h}, {x, y + h}, 1.2f * scale, WithAlpha(multiColor, (0.15f + envelope * 0.25f) * alpha));
        }
    }
    DrawText("R Y T H M   A I R", (int)margin, (int)(height * 0.885f), (int)std::max(9.0f, 10.0f * scale), WithAlpha(PASTE_PINK, 0.8f * alpha));
    const char* build = "INTERACTIVE AUDIO VISUAL";
    int buildW = MeasureText(build, (int)std::max(9.0f, 9.0f * scale));
    DrawText(build, width - (int)margin - buildW, (int)(height * 0.885f), (int)std::max(9.0f, 9.0f * scale), WithAlpha(PASTE_BLUE, 0.8f * alpha));
    float tickW = 32.0f * scale;
    for (int i = 0; i < 5; ++i) {
        float x = centerX + (i - 2) * tickW;
        float h = (i == 2 ? 10.0f : 4.0f) * scale;
        Color tickColor = GetPastelColor(i);
        DrawLineEx({x, height * 0.865f}, {x, height * 0.865f + h}, 1.5f, WithAlpha(tickColor, (i == 2 ? 0.9f : 0.4f) * alpha));
    }
}

static void DrawLogoDecorations(Vector2 center, float radius, float time, float beat, float alpha) {
    float r1 = radius + 26.0f + beat * 20.0f;
    float r2 = radius + 48.0f + beat * 30.0f;

    DrawRing(center, r1, r1 + 2.0f, ringRotation1, ringRotation1 + 100.0f, 64, WithAlpha(PASTE_PINK, alpha * (0.35f + beat * 0.3f)));
    DrawRing(center, r2, r2 + 1.5f, ringRotation2, ringRotation2 + 62.0f, 64, WithAlpha(PASTE_BLUE, alpha * (0.25f + beat * 0.25f)));

    for (int i = 0; i < 12; ++i) {
        float angle = (time * (10.0f + i * 0.2f) + i * 30.0f) * PI / 180.0f;
        float orbit = r2;
        Vector2 p = {
            center.x + std::cos(angle) * orbit,
            center.y + std::sin(angle) * orbit
        };

        float dotPulse = 0.35f + 0.65f * std::sin(time * 3.0f + i * 0.7f);
        Color dotColor = GetPastelColor(i);
        DrawCircleV(p, 1.8f + beat * 1.8f, WithAlpha(dotColor, alpha * (0.3f + dotPulse * 0.4f)));
    }

    float cross = radius + 67.0f;
    DrawLineEx({ center.x - cross, center.y }, { center.x - radius - 4.0f, center.y }, 1.5f, WithAlpha(PASTE_YELLOW, alpha * 0.4f));
    DrawLineEx({ center.x + radius + 4.0f, center.y }, { center.x + cross, center.y }, 1.5f, WithAlpha(PASTE_MINT, alpha * 0.4f));
    DrawLineEx({ center.x, center.y - cross }, { center.x, center.y - radius - 4.0f }, 1.5f, WithAlpha(PASTE_PINK, alpha * 0.4f));
    DrawLineEx({ center.x, center.y + radius + 4.0f }, { center.x, center.y + cross }, 1.5f, WithAlpha(PASTE_PURPLE, alpha * 0.4f));
}

static void DrawExpandedMenuItem(Font font, const char* label, const char* indexText, Vector2 center, float open, float selected, float hover, float time, int index) {
    float uiScale = (float)GetScreenHeight() / 720.0f;
    float screenW = (float)GetScreenWidth();
    float itemOpen = EaseOutBack(open);

    float spacing = 220.0f * uiScale;
    float startX = screenW * 0.5f - 1.5f * spacing;
    float targetX = startX + index * spacing;
    float targetY = center.y + 240.0f * uiScale;

    float x = center.x + (targetX - center.x) * itemOpen;
    float y = targetY + std::sin(time * 1.5f + index * 0.7f) * 1.2f * uiScale;
    float appear = Clamp01(open * 1.3f - index * 0.055f);
    float width = 190.0f * uiScale;
    float height = 74.0f * uiScale;
    Rectangle box = { x - width * 0.5f, y - height * 0.5f, width, height };
    if (appear <= 0.01f) return;

    Color baseColor = GetPastelColor(index);
    Color secondColor = GetPastelColor(index + 2);
    Color thirdColor = GetPastelColor(index + 4);

    Color panel = selected > 0.5f ? Color{ 255, 255, 255, (unsigned char)(245.0f * appear) } 
                                  : WithAlpha(baseColor, 0.65f * appear);

    DrawRectangleRounded(box, 0.5f, 16, panel);
    DrawRectangleRoundedLines(box, 0.5f, 16, WithAlpha(secondColor, appear * 0.85f));
    
    Rectangle innerBox = { box.x + 4, box.y + 4, box.width - 8, box.height - 8 };
    DrawRectangleRoundedLines(innerBox, 0.5f, 16, WithAlpha(thirdColor, appear * 0.4f));

    DrawTextEx(font, indexText, { box.x + 14.0f * uiScale, box.y + 12.0f * uiScale }, 10.0f * uiScale, 1.0f * uiScale, WithAlpha(thirdColor, appear * 0.95f));

    float labelFontSize = (20.0f + selected * 3.0f) * uiScale;
    Vector2 labelSize = MeasureTextEx(font, label, labelFontSize, 1.0f * uiScale);
    Color textColor = selected > 0.5f ? Color{ 60, 50, 60, (unsigned char)(255.0f * appear) } : Color{ 255, 255, 255, (unsigned char)(245.0f * appear) };
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

    ringRotation1 += dt * 18.0f;
    ringRotation2 -= dt * 12.0f;

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
                introFlashAlpha = 0.48f;
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

        if (introMs >= 4000) {
            introFinished = true;
            logoScale = 1.0f;
            logoAlpha = 1.0f;
            logoRotation = 0.0f;
        }

        return;
    }

    Vector2 mouse = GetMousePosition();
    Vector2 logoCenter = {
        screenW * 0.5f,
        screenH * 0.39f
    };

    float logoRadius = std::min(screenW, screenH) * 0.25f;
    logoRadius = std::max(150.0f, std::min(280.0f, logoRadius));

    float distanceToLogo = std::sqrt(
        std::pow(mouse.x - logoCenter.x, 2.0f) +
        std::pow(mouse.y - logoCenter.y, 2.0f)
    );

    bool insideLogo = distanceToLogo <= logoRadius * (1.0f + logoHover * 0.04f);

    float hoverTarget = insideLogo ? 1.0f : 0.0f;
    logoHover += (hoverTarget - logoHover) * std::min(1.0f, dt * 12.0f);
    logoPress += (logoPress - 0.0f) * std::min(1.0f, dt * 16.0f);
    logoPulse += (0.0f - logoPulse) * std::min(1.0f, dt * 5.0f);
    menuClosePulse += (0.0f - menuClosePulse) * std::min(1.0f, dt * 7.0f);

    if (insideLogo && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        menuOpen = !menuOpen;
        logoPress = 1.0f;
        logoPulse = 1.0f;
        menuClosePulse = menuOpen ? 0.0f : 1.0f;

        SpawnBurst(logoCenter, 45);

        if (menuOpen) {
            selectedIndex = 0;
            for (int i = 0; i < 4; ++i) {
                menuItemPulse[i] = 1.0f;
                menuItemSweep[i] = 0.0f;
            }
        }
        else {
            SpawnMenuTrail(logoCenter, { 0.0f, 0.0f });
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
        float uiScale = (float)screenH / 720.0f;
        float spacing = 220.0f * uiScale;
        float startX = screenW * 0.5f - 1.5f * spacing;

        float targetX2 = startX + 2 * spacing;
        float targetY2 = logoCenter.y + 240.0f * uiScale;
        float itemOpen = EaseOutBack(menuOpenAmount);
        float x2 = logoCenter.x + (targetX2 - logoCenter.x) * itemOpen;
        float y2 = targetY2 + std::sin(uiTime * 1.5f + 2 * 0.7f) * 1.2f * uiScale;
        float w2 = 190.0f * uiScale;
        float h2 = 74.0f * uiScale;
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

            float w = 190.0f * uiScale;
            float h = 74.0f * uiScale;

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
        menuItemPulse[selectedIndex] = 1.0f;
        menuItemSweep[selectedIndex] = 0.0f;

        Vector2 clickPos = mouse;
        SpawnBurst(clickPos, 30);

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
            curtainVfx.Start([this]() {
            });
            return;
        }

        if (selectedIndex == 2) {
            skinButton.ToggleExpand();
            return;
        }

        if (selectedIndex == 3) {
            curtainVfx.Start([this]() {
            });
            return;
        }
    }

    if (menuOpen) {
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
            selectedIndex = (selectedIndex - 1 + 4) % 4;
            menuItemPulse[selectedIndex] = 1.0f;
        }

        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
            selectedIndex = (selectedIndex + 1) % 4;
            menuItemPulse[selectedIndex] = 1.0f;
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
                curtainVfx.Start([this]() {
                });
                return;
            }

            if (selectedIndex == 2) {
                skinButton.ToggleExpand();
                return;
            }

            if (selectedIndex == 3) {
                curtainVfx.Start([this]() {
                });
                return;
            }
        }
    }
    else if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        menuOpen = true;
        selectedIndex = 0;
        logoPulse = 1.0f;
        SpawnBurst(logoCenter, 45);
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

        if (menuItemPulse[i] > 0.0f) {
            menuItemPulse[i] -= dt * 5.0f;
            if (menuItemPulse[i] < 0.0f) menuItemPulse[i] = 0.0f;
        }

        if (menuOpen && selectedIndex == i) {
            menuItemSweep[i] += dt * 1.4f;
            if (menuItemSweep[i] > 1.0f) menuItemSweep[i] = 0.0f;
        }
        else {
            menuItemSweep[i] = 0.0f;
        }
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

        const wchar_t* krText = L"플레이설정스킨나가기메인메뉴시스템온라인기본도형";
        for (int i = 0; krText[i] != L'\0'; ++i) {
            codepoints[count++] = (int)krText[i];
        }

        if (FileExists("fonts/Hakgyoansim_Dunggeunmiso_TTF_B.ttf")) {
            customFont = LoadFontEx("fonts/Hakgyoansim_Dunggeunmiso_TTF_B.ttf", 64, codepoints, count);
        }
        else if (FileExists("fonts/Pretendard-Black.ttf")) {
            customFont = LoadFontEx("fonts/Pretendard-Black.ttf", 64, codepoints, count);
        }
        else if (FileExists("assets/font.ttf")) {
            customFont = LoadFontEx("assets/font.ttf", 64, codepoints, count);
        }
        else if (FileExists("C:/Windows/Fonts/malgun.ttf")) {
            customFont = LoadFontEx("C:/Windows/Fonts/malgun.ttf", 64, codepoints, count);
        }
        else {
            customFont = GetFontDefault();
        }

        if (customFont.texture.id != 0) SetTextureFilter(customFont.texture, TEXTURE_FILTER_BILINEAR);
        isAssetsLoaded = true;
    }

    ClearBackground(BLACK_DEEP);

    Vector2 mouse = GetMousePosition();
    float parallaxX = ((mouse.x / (float)std::max(1, screenWidth)) - 0.5f) * 8.0f;
    float parallaxY = ((mouse.y / (float)std::max(1, screenHeight)) - 0.5f) * 6.0f;

    Vector2 center = {
        screenWidth * 0.5f + parallaxX,
        screenHeight * 0.39f + parallaxY
    };

    float beat = Clamp01(beatScale);
    float strongBeat = Clamp01(beatImpact);
    float uiScale = (float)screenHeight / 720.0f;

    MenuBackground::Draw(screenWidth, screenHeight);
    DrawRadialBackground(screenWidth, screenHeight, center, uiTime, beat);
    DrawPremiumAtmosphere(screenWidth, screenHeight, uiTime, introFinished ? 1.0f : Clamp01((introTimer - 0.2f) / 0.8f), beatScale);

    float logoBaseRadius = std::min(screenWidth, screenHeight) * 0.25f;
    logoBaseRadius = std::max(150.0f * uiScale, std::min(280.0f * uiScale, logoBaseRadius));

    if (!introFinished) {
        float borderProgress = Clamp01((introTimer * 1000.0f) / 800.0f);
        float currentRadius = logoBaseRadius * 1.6f * EaseOutCubic(borderProgress);
        DrawRing(center, currentRadius - 1.5f, currentRadius + 1.5f, 0.0f, 360.0f * borderProgress, 64, WithAlpha(PASTE_PINK, 0.85f));
    }

    float actualBeatScale = strongBeat * 0.085f;
    float hoverScale = logoHover * 0.035f;
    float pressScale = logoPress * -0.055f;
    float openScale = menuOpenAmount * 0.012f;

    float visualScale = logoScale * (1.0f + actualBeatScale + hoverScale + pressScale + openScale);
    float logoRadius = logoBaseRadius * visualScale;

    DrawLogoDecorations(center, logoBaseRadius * visualScale, uiTime, strongBeat, logoAlpha);

    if (setaTexture.id != 0) {
        Rectangle src = {
            0.0f,
            0.0f,
            (float)setaTexture.width,
            (float)setaTexture.height
        };

        float maxTexDim = (float)std::max(setaTexture.width, setaTexture.height);
        float targetDiameter = logoBaseRadius * 2.0f * visualScale;
        float textureScale = (maxTexDim > 0.0f) ? (targetDiameter / maxTexDim) : 1.0f;

        float textureWidth = setaTexture.width * textureScale;
        float textureHeight = setaTexture.height * textureScale;

        Rectangle aura = {
            center.x,
            center.y,
            textureWidth * (1.08f + strongBeat * 0.08f),
            textureHeight * (1.08f + strongBeat * 0.08f)
        };

        Rectangle dest = {
            center.x,
            center.y,
            textureWidth,
            textureHeight
        };

        Vector2 auraOrigin = { aura.width * 0.5f, aura.height * 0.5f };
        Vector2 origin = { dest.width * 0.5f, dest.height * 0.5f };

        unsigned char auraAlpha = (unsigned char)(logoAlpha * (20.0f + strongBeat * 40.0f + logoHover * 20.0f));

        DrawTexturePro(
            setaTexture,
            src,
            aura,
            auraOrigin,
            logoRotation,
            WithAlpha(PASTE_PINK, (float)auraAlpha / 255.0f)
        );

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
        float ring = logoBaseRadius * (1.13f + std::sin(uiTime * 4.0f) * 0.012f);
        DrawRing(
            center,
            ring,
            ring + 2.5f * uiScale,
            ringRotation1,
            ringRotation1 + 270.0f,
            64,
            WithAlpha(PASTE_YELLOW, 0.3f + logoHover * 0.7f)
        );
    }

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

    DrawCenteredText(
        customFont,
        "Rythm Air",
        screenWidth * 0.5f,
        screenHeight * 0.09f,
        std::min(28.0f * uiScale, screenWidth * 0.022f),
        5.0f * uiScale,
        WithAlpha(PASTE_PINK, titleAlpha * 0.9f)
    );

    DrawCenteredText(
        customFont,
        menuOpen ? "CHOOSE YOUR DESTINATION" : "PRESS ENTER TO BEGIN",
        screenWidth * 0.5f,
        screenHeight * 0.13f,
        std::min(11.0f * uiScale, screenWidth * 0.009f),
        2.2f * uiScale,
        WithAlpha(PASTE_BLUE, titleAlpha * 0.8f)
    );

    if (introFinished) {
        const char* hint = menuOpen
            ? "ARROW / WASD  SELECT     ENTER / SPACE  CONFIRM     ESC  CLOSE"
            : "CLICK / ENTER  OPEN MENU";

        Vector2 hintSize = MeasureTextEx(customFont, hint, 10.0f * uiScale, 1.5f * uiScale);

        DrawTextEx(
            customFont,
            hint,
            {
                screenWidth * 0.5f - hintSize.x * 0.5f,
                screenHeight * 0.935f
            },
            10.0f * uiScale,
            1.5f * uiScale,
            WithAlpha(PASTE_MINT, 0.85f)
        );
    }

    for (int i = 0; i < MAX_TRAIL; ++i) {
        if (trails[i].life <= 0.0f) continue;

        DrawCircleV(
            trails[i].position,
            trails[i].size * uiScale,
            WithAlpha(trails[i].color, trails[i].alpha * 0.7f)
        );
    }

    for (int i = 0; i < MAX_BURST; ++i) {
        if (bursts[i].life <= 0.0f) continue;

        Color color = bursts[i].color;
        color.a = (unsigned char)(bursts[i].alpha * 230.0f);
        DrawCircleV(bursts[i].position, bursts[i].size * uiScale, color);
    }

    if (introFlashAlpha > 0.001f) {
        DrawRectangle(
            0,
            0,
            screenWidth,
            screenHeight,
            WithAlpha(PASTE_PINK, introFlashAlpha * 0.6f)
        );
    }

    curtainVfx.Draw(screenWidth, screenHeight);
    SlowScreen::Draw(screenWidth, screenHeight);
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