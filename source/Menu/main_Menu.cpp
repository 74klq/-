#include "main_Menu.h"
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
static float menuItemHover[3] = { 0.0f, 0.0f, 0.0f };
static float menuItemSelected[3] = { 0.0f, 0.0f, 0.0f };
static float menuItemVelocity[3] = { 0.0f, 0.0f, 0.0f };
static float menuItemSweep[3] = { 0.0f, 0.0f, 0.0f };
static float menuItemPulse[3] = { 0.0f, 0.0f, 0.0f };

static const float MENU_BPM = 180.0f;
static const Color WHITE_SOFT = { 245, 245, 245, 255 };
static const Color WHITE_DIM = { 155, 155, 155, 255 };
static const Color BLACK_DEEP = { 3, 3, 3, 255 };

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
        bursts[i].size = 1.0f + ((float)rand() / RAND_MAX) * 5.0f;
        bursts[i].maxLife = 0.28f + ((float)rand() / RAND_MAX) * 0.62f;
        bursts[i].life = bursts[i].maxLife;
        bursts[i].alpha = 1.0f;
        bursts[i].color = (i % 4 == 0) ? WHITE_SOFT : Color{ 175, 175, 175, 255 };
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
        radialParticles[i].size = 0.5f + ((float)rand() / RAND_MAX) * 2.4f;
        radialParticles[i].alpha = 0.12f + ((float)rand() / RAND_MAX) * 0.48f;
        radialParticles[i].maxLife = 4.0f + ((float)rand() / RAND_MAX) * 7.0f;
        radialParticles[i].life = ((float)rand() / RAND_MAX) * radialParticles[i].maxLife;
    }

    for (int i = 0; i < MAX_TRAIL; ++i) {
        trails[i].position = { cx, cy };
        trails[i].velocity = { 0.0f, 0.0f };
        trails[i].size = 1.0f;
        trails[i].alpha = 0.0f;
        trails[i].maxLife = 0.5f;
        trails[i].life = 0.0f;
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
            radialParticles[i].size = 0.5f + ((float)rand() / RAND_MAX) * 2.4f;
            radialParticles[i].alpha = 0.12f + ((float)rand() / RAND_MAX) * 0.48f;
            radialParticles[i].maxLife = 4.0f + ((float)rand() / RAND_MAX) * 7.0f;
            radialParticles[i].life = radialParticles[i].maxLife;
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
        trails[i].size = 1.0f + ((float)rand() / RAND_MAX) * 2.8f;
        trails[i].maxLife = 0.25f + ((float)rand() / RAND_MAX) * 0.45f;
        trails[i].life = trails[i].maxLife;
        trails[i].alpha = 0.75f;
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

        DrawLineEx(
            start,
            end,
            1.0f + beat * 1.5f,
            Color{ 255, 255, 255, (unsigned char)(5.0f + widthPulse * 10.0f + beat * 12.0f) }
        );
    }

    for (int i = 0; i < 7; ++i) {
        float radius = 155.0f + i * 55.0f + beat * (18.0f - i * 1.5f);
        float rotation = time * (5.0f + i * 1.2f) * (i % 2 == 0 ? 1.0f : -1.0f);
        float arcLength = 38.0f + std::sin(time * 1.8f + i) * 16.0f;

        DrawRing(
            center,
            radius,
            radius + 1.0f + beat,
            rotation,
            rotation + arcLength,
            64,
            Color{ 255, 255, 255, (unsigned char)(8.0f + beat * 15.0f) }
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
            radialParticles[i].size + beat * 0.8f,
            Color{ 255, 255, 255, (unsigned char)(alpha * 120.0f) }
        );
    }

    float halo = 210.0f + beat * 28.0f + std::sin(time * 1.6f) * 8.0f;
    DrawCircleV(center, halo, Color{ 255, 255, 255, (unsigned char)(2.0f + beat * 5.0f) });
    DrawCircleV(center, halo * 0.72f, Color{ 255, 255, 255, (unsigned char)(2.0f + beat * 4.0f) });
}

static void DrawBottomBars(int width, int height, float beat) {
    float barWidth = (float)width / EQUALIZER_BARS;

    for (int i = 0; i < EQUALIZER_BARS; ++i) {
        float centerDistance = std::abs(i - EQUALIZER_BARS * 0.5f) / (EQUALIZER_BARS * 0.5f);
        float h = eqBarHeights[i] * (1.0f - centerDistance * 0.45f) + beat * 14.0f * (1.0f - centerDistance);

        DrawRectangle(
            (int)(i * barWidth),
            height - (int)h,
            (int)std::max(1.0f, barWidth - 2.0f),
            (int)h,
            Color{ 255, 255, 255, (unsigned char)(7.0f + beat * 16.0f) }
        );
    }
}

static void DrawCornerUI(Font font, int width, int height, float alpha, float time) {
    float margin = std::max(24.0f, width * 0.025f);
    float length = 34.0f;
    Color line = WithAlpha(WHITE_SOFT, alpha * 0.34f);

    DrawLineEx({ margin, margin + length }, { margin, margin }, 1.5f, line);
    DrawLineEx({ margin, margin }, { margin + length, margin }, 1.5f, line);
    DrawLineEx({ width - margin - length, margin }, { width - margin, margin }, 1.5f, line);
    DrawLineEx({ width - margin, margin }, { width - margin, margin + length }, 1.5f, line);
    DrawLineEx({ margin, height - margin - length }, { margin, height - margin }, 1.5f, line);
    DrawLineEx({ margin, height - margin }, { margin + length, height - margin }, 1.5f, line);
    DrawLineEx({ width - margin - length, height - margin }, { width - margin, height - margin }, 1.5f, line);
    DrawLineEx({ width - margin, height - margin - length }, { width - margin, height - margin }, 1.5f, line);

    DrawTextEx(font, "SETA", { margin + 3.0f, margin + 42.0f }, 12.0f, 2.5f, WithAlpha(WHITE_SOFT, alpha * 0.45f));

    const char* status = "MAIN MENU";
    Vector2 statusSize = MeasureTextEx(font, status, 11.0f, 2.0f);
    DrawTextEx(font, status, { width - margin - statusSize.x, margin + 42.0f }, 11.0f, 2.0f, WithAlpha(WHITE_SOFT, alpha * 0.4f));

    float pulse = 0.45f + std::sin(time * 3.0f) * 0.25f;
    DrawCircleV({ width - margin - 6.0f, height - margin - 5.0f }, 2.5f, WithAlpha(WHITE_SOFT, pulse * alpha));
}

static void DrawLogoDecorations(Vector2 center, float radius, float time, float beat, float alpha) {
    float r1 = radius + 26.0f + beat * 20.0f;
    float r2 = radius + 48.0f + beat * 30.0f;

    DrawRing(center, r1, r1 + 1.5f, ringRotation1, ringRotation1 + 100.0f, 64, WithAlpha(WHITE_SOFT, alpha * (0.16f + beat * 0.18f)));
    DrawRing(center, r2, r2 + 1.0f, ringRotation2, ringRotation2 + 62.0f, 64, WithAlpha(WHITE_SOFT, alpha * (0.10f + beat * 0.15f)));

    for (int i = 0; i < 12; ++i) {
        float angle = (time * (10.0f + i * 0.2f) + i * 30.0f) * PI / 180.0f;
        float orbit = r2;
        Vector2 p = {
            center.x + std::cos(angle) * orbit,
            center.y + std::sin(angle) * orbit
        };

        float dotPulse = 0.35f + 0.65f * std::sin(time * 3.0f + i * 0.7f);
        DrawCircleV(p, 1.2f + beat * 1.5f, WithAlpha(WHITE_SOFT, alpha * (0.12f + dotPulse * 0.2f)));
    }

    float cross = radius + 67.0f;
    Color crossColor = WithAlpha(WHITE_SOFT, alpha * 0.13f);

    DrawLineEx({ center.x - cross, center.y }, { center.x - radius - 4.0f, center.y }, 1.0f, crossColor);
    DrawLineEx({ center.x + radius + 4.0f, center.y }, { center.x + cross, center.y }, 1.0f, crossColor);
    DrawLineEx({ center.x, center.y - cross }, { center.x, center.y - radius - 4.0f }, 1.0f, crossColor);
    DrawLineEx({ center.x, center.y + radius + 4.0f }, { center.x, center.y + cross }, 1.0f, crossColor);
}

static void DrawExpandedMenuItem(Font font, const char* label, const char* indexText, Vector2 center, float open, float selected, float hover, float time, int index) {
    float directionX = index == 0 ? -1.0f : (index == 1 ? 0.0f : 1.0f);
    float directionY = index == 1 ? 1.0f : -0.18f;

    float distance = index == 1 ? 235.0f : 250.0f;
    float targetX = center.x + directionX * distance;
    float targetY = center.y + directionY * distance;

    float itemOpen = EaseOutBack(open);
    float x = center.x + (targetX - center.x) * itemOpen;
    float y = center.y + (targetY - center.y) * itemOpen;

    float appear = Clamp01(open * 1.18f - index * 0.08f);
    float selectedLift = selected * -8.0f;
    float hoverLift = hover * -5.0f;

    x += std::sin(time * 2.0f + index) * (1.0f + selected * 2.0f);
    y += selectedLift + hoverLift;

    float width = 178.0f + selected * 24.0f + hover * 10.0f;
    float height = 58.0f + selected * 7.0f + hover * 4.0f;

    Rectangle box = {
        x - width * 0.5f,
        y - height * 0.5f,
        width,
        height
    };

    Color fill = selected > 0.5f
        ? Color{ 247, 247, 247, (unsigned char)(245.0f * appear) }
        : Color{ 10, 10, 10, (unsigned char)(185.0f * appear) };

    Color border = selected > 0.5f
        ? Color{ 255, 255, 255, (unsigned char)(255.0f * appear) }
        : Color{ 120, 120, 120, (unsigned char)((95.0f + hover * 80.0f) * appear) };

    if (appear > 0.01f) {
        DrawRectangleRounded(
            { box.x - 5.0f, box.y - 5.0f, box.width + 10.0f, box.height + 10.0f },
            0.18f,
            10,
            Color{ 255, 255, 255, (unsigned char)(8.0f * appear + selected * 12.0f) }
        );

        DrawRectangleRounded(box, 0.18f, 10, fill);
        DrawRectangleRoundedLinesEx(box, 0.18f, 10, selected > 0.5f ? 2.0f : 1.0f, border);

        DrawTextEx(font, indexText, { box.x + 14.0f, box.y + 9.0f }, 10.0f, 1.5f, selected > 0.5f ? Color{ 80, 80, 80, 255 } : Color{ 130, 130, 130, (unsigned char)(180.0f * appear) });

        float textSizeValue = 21.0f + selected * 6.0f + hover * 2.0f;
        Vector2 textSize = MeasureTextEx(font, label, textSizeValue, 1.0f);
        DrawTextEx(
            font,
            label,
            { box.x + (box.width - textSize.x) * 0.5f, box.y + (box.height - textSize.y) * 0.5f + 2.0f },
            textSizeValue,
            1.0f,
            selected > 0.5f ? Color{ 10, 10, 10, (unsigned char)(255.0f * appear) } : Color{ 225, 225, 225, (unsigned char)(220.0f * appear) }
        );

        float lineLength = selected > 0.5f ? 55.0f : 28.0f + hover * 22.0f;
        DrawLineEx(
            { box.x + 14.0f, box.y + box.height - 6.0f },
            { box.x + 14.0f + lineLength, box.y + box.height - 6.0f },
            selected > 0.5f ? 2.0f : 1.0f,
            selected > 0.5f ? Color{ 10, 10, 10, (unsigned char)(230.0f * appear) } : Color{ 180, 180, 180, (unsigned char)(100.0f * appear) }
        );
    }
}

MainMenu::MainMenu() {
    selectedIndex = 0;
    pulseTimer = 0.0f;
    currentState = MenuState::Main;
}

void MainMenu::Update() {
    float dt = std::min(GetFrameTime(), 0.05f);
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    pulseTimer += dt;
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
        float duration = 1.65f;
        float progress = Clamp01(introTimer / duration);

        if (progress < 0.55f) {
            float t = progress / 0.55f;
            float ease = EaseOutBack(t);
            logoScale = 3.0f + (1.0f - 3.0f) * ease;
            logoAlpha = EaseOutCubic(std::min(1.0f, t * 1.7f));
            logoRotation = -360.0f * (1.0f - EaseOutCubic(t));
        }
        else {
            if (!introImpactTriggered) {
                introImpactTriggered = true;
                SpawnBurst({ screenW * 0.5f, screenH * 0.39f }, MAX_BURST);
                introFlashAlpha = 0.48f;
            }

            float t = (progress - 0.55f) / 0.45f;
            float bounce = std::sin(t * PI * 3.0f) * std::exp(-t * 5.0f) * 0.12f;
            logoScale = 1.0f + bounce;
            logoAlpha = 1.0f;
            logoRotation = 0.0f;
        }

        if (progress >= 1.0f) {
            introFinished = true;
            logoScale = 1.0f;
            logoAlpha = 1.0f;
            logoRotation = 0.0f;
        }

        return;
    }

    Vector2 mouse = GetMousePosition();
    Vector2 logoCenter = { screenW * 0.5f, screenH * 0.39f };

    float logoRadius = std::min(screenW, screenH) * 0.22f;
    logoRadius = std::max(135.0f, std::min(250.0f, logoRadius));

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
            for (int i = 0; i < 3; ++i) {
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
        for (int i = 0; i < 3; ++i) {
            float directionX = i == 0 ? -1.0f : (i == 1 ? 0.0f : 1.0f);
            float directionY = i == 1 ? 1.0f : -0.18f;
            float distance = i == 1 ? 235.0f : 250.0f;

            Vector2 p = {
                logoCenter.x + directionX * distance * EaseOutBack(menuOpenAmount),
                logoCenter.y + directionY * distance * EaseOutBack(menuOpenAmount)
            };

            float w = 190.0f;
            float h = 70.0f;

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
            currentState = MenuState::SongSelect;
            songSelect.Init();
            activeFrames = 0;
            menuOpen = false;
            menuOpenAmount = 0.0f;
            return;
        }

        if (selectedIndex == 2) {
            return;
        }
    }

    if (menuOpen) {
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
            selectedIndex = (selectedIndex - 1 + totalOptions) % totalOptions;
            menuItemPulse[selectedIndex] = 1.0f;
        }

        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D) || IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
            selectedIndex = (selectedIndex + 1) % totalOptions;
            menuItemPulse[selectedIndex] = 1.0f;
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            menuOpen = false;
        }

        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            if (selectedIndex == 0) {
                currentState = MenuState::SongSelect;
                songSelect.Init();
                activeFrames = 0;
                menuOpen = false;
                menuOpenAmount = 0.0f;
                return;
            }

            if (selectedIndex == 2) {
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

    for (int i = 0; i < 3; ++i) {
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
        return;
    }

    if (!isAssetsLoaded) {
        setaTexture = LoadTexture("assets/Seta.png");
        if (setaTexture.id != 0) SetTextureFilter(setaTexture, TEXTURE_FILTER_BILINEAR);

        int codepoints[512];
        int count = 0;

        for (int i = 32; i <= 126; ++i) {
            codepoints[count++] = i;
        }

        const wchar_t* krText = L"플레이설정나가기메인메뉴시스템온라인";
        for (int i = 0; krText[i] != L'\0'; ++i) {
            codepoints[count++] = (int)krText[i];
        }

        if (FileExists("fonts/Pretendard-Black.ttf")) {
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

    DrawRectangleGradientV(
        0,
        0,
        screenWidth,
        screenHeight,
        Color{ 8, 8, 8, 255 },
        Color{ 1, 1, 1, 255 }
    );

    DrawRadialBackground(screenWidth, screenHeight, center, uiTime, strongBeat);
    DrawBottomBars(screenWidth, screenHeight, strongBeat);

    float logoBaseRadius = std::min(screenWidth, screenHeight) * 0.205f;
    logoBaseRadius = std::max(125.0f, std::min(235.0f, logoBaseRadius));

    float actualBeatScale = strongBeat * 0.085f;
    float hoverScale = logoHover * 0.035f;
    float pressScale = logoPress * -0.055f;
    float openScale = menuOpenAmount * 0.012f;

    float visualScale = logoScale * (1.0f + actualBeatScale + hoverScale + pressScale + openScale);
    float logoRadius = logoBaseRadius * visualScale;

    DrawLogoDecorations(center, logoBaseRadius * visualScale, uiTime, strongBeat, logoAlpha);

    if (menuOpenAmount > 0.001f) {
        float menuLineAlpha = EaseOutCubic(menuOpenAmount);

        for (int i = 0; i < 3; ++i) {
            float directionX = i == 0 ? -1.0f : (i == 1 ? 0.0f : 1.0f);
            float directionY = i == 1 ? 1.0f : -0.18f;
            float distance = i == 1 ? 235.0f : 250.0f;
            float t = EaseOutBack(menuOpenAmount);

            Vector2 destination = {
                center.x + directionX * distance * t,
                center.y + directionY * distance * t
            };

            DrawLineEx(
                center,
                destination,
                1.0f,
                Color{ 255, 255, 255, (unsigned char)(10.0f * menuLineAlpha) }
            );

            DrawCircleV(
                destination,
                2.0f + menuItemSelected[i] * 2.0f,
                Color{ 255, 255, 255, (unsigned char)(55.0f * menuLineAlpha) }
            );
        }
    }

    if (setaTexture.id != 0) {
        Rectangle src = {
            0.0f,
            0.0f,
            (float)setaTexture.width,
            (float)setaTexture.height
        };

        float textureScale = visualScale * 0.62f;
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

        unsigned char auraAlpha = (unsigned char)(logoAlpha * (14.0f + strongBeat * 32.0f + logoHover * 18.0f));

        DrawTexturePro(
            setaTexture,
            src,
            aura,
            auraOrigin,
            logoRotation,
            Color{ 255, 255, 255, auraAlpha }
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
            ring + 2.0f,
            ringRotation1,
            ringRotation1 + 270.0f,
            64,
            Color{ 255, 255, 255, (unsigned char)(30.0f + logoHover * 70.0f) }
        );
    }

    if (menuOpenAmount > 0.01f) {
        const char* labels[3] = { "플레이", "설정", "나가기" };
        const char* indices[3] = { "01", "02", "03" };

        for (int i = 0; i < 3; ++i) {
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

    float titleAlpha = introFinished ? 1.0f : Clamp01((introTimer - 0.8f) / 0.5f);

    DrawCenteredText(
        customFont,
        "THE LINE",
        screenWidth * 0.5f,
        screenHeight * 0.09f,
        std::min(28.0f, screenWidth * 0.022f),
        5.0f,
        WithAlpha(WHITE_SOFT, titleAlpha * 0.78f)
    );

    DrawCenteredText(
        customFont,
        menuOpen ? "SELECT MODE" : "CLICK THE LOGO",
        screenWidth * 0.5f,
        screenHeight * 0.13f,
        std::min(11.0f, screenWidth * 0.009f),
        2.2f,
        WithAlpha(WHITE_DIM, titleAlpha * 0.58f)
    );

    float bpmWidth = 80.0f;
    DrawRectangleRounded(
        { screenWidth * 0.5f - bpmWidth * 0.5f, screenHeight * 0.17f, bpmWidth, 24.0f },
        0.35f,
        8,
        Color{ 255, 255, 255, 7 }
    );

    char bpmText[32];
    std::snprintf(bpmText, sizeof(bpmText), "%d BPM", (int)MENU_BPM);

    DrawCenteredText(
        customFont,
        bpmText,
        screenWidth * 0.5f,
        screenHeight * 0.17f + 12.0f,
        10.0f,
        1.5f,
        Color{ 185, 185, 185, (unsigned char)(titleAlpha * 115.0f) }
    );

    DrawCornerUI(customFont, screenWidth, screenHeight, titleAlpha, uiTime);

    if (introFinished) {
        const char* hint = menuOpen
            ? "ARROW / WASD  SELECT     ENTER / SPACE  CONFIRM     ESC  CLOSE"
            : "CLICK / ENTER  OPEN MENU";

        Vector2 hintSize = MeasureTextEx(customFont, hint, 10.0f, 1.5f);

        DrawTextEx(
            customFont,
            hint,
            {
                screenWidth * 0.5f - hintSize.x * 0.5f,
                screenHeight * 0.935f
            },
            10.0f,
            1.5f,
            Color{ 145, 145, 145, 105 }
        );
    }

    for (int i = 0; i < MAX_TRAIL; ++i) {
        if (trails[i].life <= 0.0f) continue;

        DrawCircleV(
            trails[i].position,
            trails[i].size,
            Color{ 255, 255, 255, (unsigned char)(trails[i].alpha * 90.0f) }
        );
    }

    for (int i = 0; i < MAX_BURST; ++i) {
        if (bursts[i].life <= 0.0f) continue;

        Color color = bursts[i].color;
        color.a = (unsigned char)(bursts[i].alpha * 210.0f);
        DrawCircleV(bursts[i].position, bursts[i].size, color);
    }

    if (introFlashAlpha > 0.001f) {
        DrawRectangle(
            0,
            0,
            screenWidth,
            screenHeight,
            Color{ 255, 255, 255, (unsigned char)(introFlashAlpha * 255.0f) }
        );
    }
}

bool MainMenu::IsGameStartSelected() const {
    if (activeFrames < 10) return false;
    return currentState == MenuState::SongSelect && songSelect.IsPlaySelected();
}

bool MainMenu::IsExitSelected() const {
    return currentState == MenuState::Main &&
           selectedIndex == 2 &&
           introFinished &&
           (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE));
}

void MainMenu::Reset() {
    selectedIndex = 0;
    pulseTimer = 0.0f;
    currentState = MenuState::Main;
    activeFrames = 0;

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

    for (int i = 0; i < 3; ++i) {
        menuItemHover[i] = 0.0f;
        menuItemSelected[i] = 0.0f;
        menuItemVelocity[i] = 0.0f;
        menuItemSweep[i] = 0.0f;
        menuItemPulse[i] = 0.0f;
    }
}
