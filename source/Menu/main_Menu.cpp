#include "main_Menu.h"
#include <raylib.h>
#include <cstdlib>
#include <algorithm>
#include <cmath>

#ifndef PI
#define PI 3.14159265358979323846f
#endif

struct WarpStar {
    float angle;
    float distance;
    float speed;
    float length;
    float size;
    Color color;
};

struct TrackNote {
    int lane;
    float progress;
    float speed;
    Color color;
};

struct FloatingParticle {
    Vector2 position;
    Vector2 velocity;
    float size;
    float baseAlpha;
    float pulseSpeed;
    float pulseOffset;
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

static const int MAX_WARP_STARS  = 120;
static const int MAX_TRACK_NOTES  = 16;
static const int MAX_PARTICLES    = 60;
static const int MAX_BURST        = 40;
static const int EQUALIZER_BARS   = 32;

static WarpStar         warpStars[MAX_WARP_STARS];
static TrackNote        trackNotes[MAX_TRACK_NOTES];
static FloatingParticle particles[MAX_PARTICLES];
static BurstParticle    bursts[MAX_BURST];
static float            eqBarHeights[EQUALIZER_BARS];
static float            eqBarTargets[EQUALIZER_BARS];

static bool decosInitialized = false;

static Texture2D setaTexture     = { 0 };
static Font      customFont      = { 0 };
static bool      isAssetsLoaded  = false;
static int       activeFrames    = 0;

static float logoScale        = 0.0f;
static float logoAlpha        = 0.0f;
static float logoRotation     = -360.0f;
static float introTimer       = 0.0f;
static bool  introFinished     = false;

static float shockwaveRadius1 = 0.0f;
static float shockwaveAlpha1  = 0.0f;
static float shockwaveRadius2 = 0.0f;
static float shockwaveAlpha2  = 0.0f;

static float ringRotation1    = 0.0f;
static float ringRotation2    = 0.0f;

static float btnAnimFactor[3] = { 0.0f, 0.0f, 0.0f };
static float btnVelocities[3] = { 0.0f, 0.0f, 0.0f };
static float btnPulse[3]      = { 0.0f, 0.0f, 0.0f };
static float btnSweep[3]      = { 0.0f, 0.0f, 0.0f };

static const Color MONO_KEY_COLORS[4] = {
    { 255, 255, 255, 255 },
    { 200, 200, 200, 255 },
    { 200, 200, 200, 255 },
    { 255, 255, 255, 255 }
};

MainMenu::MainMenu() {
    selectedIndex = 0;
    pulseTimer    = 0.0f;
    currentState  = MenuState::Main;
}

static void SpawnBurst(Vector2 origin) {
    for (int i = 0; i < MAX_BURST; ++i) {
        float angle = ((float)rand() / RAND_MAX) * 2.0f * PI;
        float speed = ((float)rand() / RAND_MAX * 380.0f) + 100.0f;
        bursts[i].position = origin;
        bursts[i].velocity = { std::cos(angle) * speed, std::sin(angle) * speed };
        bursts[i].size     = ((float)rand() / RAND_MAX * 6.0f) + 2.0f;
        bursts[i].maxLife  = ((float)rand() / RAND_MAX * 0.4f) + 0.2f;
        bursts[i].life     = bursts[i].maxLife;
        bursts[i].alpha    = 1.0f;

        unsigned char val = (i % 2 == 0) ? 255 : 180;
        bursts[i].color   = Color{ val, val, val, 255 };
    }
}

static void InitDecorations(int screenWidth, int screenHeight) {
    for (int i = 0; i < MAX_WARP_STARS; ++i) {
        warpStars[i].angle    = ((float)rand() / RAND_MAX) * 2.0f * PI;
        warpStars[i].distance = ((float)rand() / RAND_MAX) * (screenWidth * 0.6f);
        warpStars[i].speed    = ((float)rand() / RAND_MAX * 400.0f) + 250.0f;
        warpStars[i].length   = 2.0f;
        warpStars[i].size     = ((float)rand() / RAND_MAX * 2.0f) + 1.0f;
        
        unsigned char val = (rand() % 2 == 0) ? 255 : 170;
        warpStars[i].color = Color{ val, val, val, 255 };
    }

    for (int i = 0; i < MAX_TRACK_NOTES; ++i) {
        trackNotes[i].lane     = rand() % 4;
        trackNotes[i].progress = ((float)rand() / RAND_MAX);
        trackNotes[i].speed    = ((float)rand() / RAND_MAX * 0.4f) + 0.3f;
        trackNotes[i].color    = MONO_KEY_COLORS[trackNotes[i].lane];
    }

    for (int i = 0; i < MAX_PARTICLES; ++i) {
        particles[i].position    = { (float)(rand() % screenWidth), (float)(rand() % screenHeight) };
        particles[i].velocity    = { ((float)rand() / RAND_MAX - 0.5f) * 20.0f, -((float)rand() / RAND_MAX * 40.0f + 15.0f) };
        particles[i].size        = (float)(rand() % 4 + 2);
        particles[i].baseAlpha   = (float)(rand() % 140 + 30) / 255.0f;
        particles[i].pulseSpeed  = (float)rand() / RAND_MAX * 3.0f + 1.0f;
        particles[i].pulseOffset = (float)rand() / RAND_MAX * 6.28f;
    }

    for (int i = 0; i < MAX_BURST; ++i) bursts[i].life = 0.0f;
    for (int i = 0; i < EQUALIZER_BARS; ++i) eqBarHeights[i] = 0.0f;

    decosInitialized = true;
}

static void UpdateDecorations(float dt, int screenWidth, int screenHeight, float pulseTimer) {
    float maxDist = (float)std::max(screenWidth, screenHeight) * 0.75f;

    for (int i = 0; i < MAX_WARP_STARS; ++i) {
        warpStars[i].distance += warpStars[i].speed * dt;
        warpStars[i].length   = (warpStars[i].distance / maxDist) * 35.0f + 2.0f;

        if (warpStars[i].distance > maxDist) {
            warpStars[i].distance = (float)(rand() % 20);
            warpStars[i].angle    = ((float)rand() / RAND_MAX) * 2.0f * PI;
            warpStars[i].speed    = ((float)rand() / RAND_MAX * 450.0f) + 250.0f;
        }
    }

    for (int i = 0; i < MAX_TRACK_NOTES; ++i) {
        trackNotes[i].progress += trackNotes[i].speed * dt;
        if (trackNotes[i].progress > 1.0f) {
            trackNotes[i].progress = 0.0f;
            trackNotes[i].lane     = rand() % 4;
            trackNotes[i].color    = MONO_KEY_COLORS[trackNotes[i].lane];
        }
    }

    for (int i = 0; i < MAX_PARTICLES; ++i) {
        particles[i].position.x += particles[i].velocity.x * dt;
        particles[i].position.y += particles[i].velocity.y * dt;

        if (particles[i].position.y < -20 || particles[i].position.x < -20 || particles[i].position.x > screenWidth + 20) {
            particles[i].position = { (float)(rand() % screenWidth), (float)(screenHeight + 20) };
        }
    }

    for (int i = 0; i < MAX_BURST; ++i) {
        if (bursts[i].life > 0.0f) {
            bursts[i].life -= dt;
            bursts[i].position.x += bursts[i].velocity.x * dt;
            bursts[i].position.y += bursts[i].velocity.y * dt;
            bursts[i].velocity.x *= 0.91f;
            bursts[i].velocity.y *= 0.91f;
            bursts[i].alpha       = std::max(0.0f, bursts[i].life / bursts[i].maxLife);
        }
    }

    for (int i = 0; i < EQUALIZER_BARS; ++i) {
        if (rand() % 8 == 0) {
            eqBarTargets[i] = ((float)rand() / RAND_MAX * 45.0f + 10.0f) * (0.6f + 0.4f * std::sin(pulseTimer * 8.0f + i));
        }
        eqBarHeights[i] += (eqBarTargets[i] - eqBarHeights[i]) * 12.0f * dt;
    }
}

void MainMenu::Update() {
    float dt = GetFrameTime();
    pulseTimer += dt;

    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    if (!decosInitialized && screenW > 0 && screenH > 0) {
        InitDecorations(screenW, screenH);
    }
    UpdateDecorations(dt, screenW, screenH, pulseTimer);

    ringRotation1 += dt * 30.0f;
    ringRotation2 -= dt * 20.0f;

    if (!introFinished) {
        introTimer += dt;
        float duration = 1.8f;
        float progress = std::min(introTimer / duration, 1.0f);

        if (progress < 0.60f) {
            float t = progress / 0.60f;
            float easeOutBack = 1.0f + 2.5f * std::pow(t - 1.0f, 3.0f) + 2.0f * std::pow(t - 1.0f, 2.0f);
            logoScale    = std::max(0.0f, easeOutBack);
            logoAlpha    = std::min(t * 2.0f, 1.0f);
            logoRotation = (1.0f - t) * -360.0f;
        } else {
            logoScale    = 1.0f;
            logoAlpha    = 1.0f;
            logoRotation = 0.0f;

            float shockProgress1 = (progress - 0.60f) / 0.40f;
            shockwaveRadius1 = shockProgress1 * (float)std::max(screenW, screenH) * 0.85f;
            shockwaveAlpha1  = (1.0f - shockProgress1) * 0.95f;

            float shockProgress2 = std::max(0.0f, shockProgress1 - 0.12f) / 0.88f;
            shockwaveRadius2 = shockProgress2 * (float)std::max(screenW, screenH) * 0.65f;
            shockwaveAlpha2  = (1.0f - shockProgress2) * 0.80f;
        }

        if (progress >= 1.0f) {
            introFinished = true;
        }
        return;
    }

    if (currentState == MenuState::Main) {
        activeFrames = 0;
        int prevIndex = selectedIndex;

        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
            selectedIndex = (selectedIndex - 1 + totalOptions) % totalOptions;
        }
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D) || IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
            selectedIndex = (selectedIndex + 1 + totalOptions) % totalOptions;
        }

        if (prevIndex != selectedIndex) {
            btnPulse[selectedIndex] = 1.0f;
            btnSweep[selectedIndex] = 0.0f;

            float menuCenterY = (float)screenH * 0.81f;
            float spacing     = 220.0f;
            float startX      = (float)screenW * 0.5f - spacing;
            float originX     = startX + selectedIndex * spacing;
            SpawnBurst({ originX, menuCenterY });
        }

        for (int i = 0; i < totalOptions; ++i) {
            float target = (i == selectedIndex) ? 1.0f : 0.0f;
            float force  = (target - btnAnimFactor[i]) * 180.0f;
            btnVelocities[i] += force * dt;
            btnVelocities[i] *= 0.78f;
            btnAnimFactor[i] += btnVelocities[i] * dt;

            if (btnPulse[i] > 0.0f) {
                btnPulse[i] -= dt * 4.5f;
                if (btnPulse[i] < 0.0f) btnPulse[i] = 0.0f;
            }

            if (i == selectedIndex) {
                btnSweep[i] += dt * 2.2f;
                if (btnSweep[i] > 1.0f) btnSweep[i] = 0.0f;
            } else {
                btnSweep[i] = 0.0f;
            }
        }

        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            if (selectedIndex == 0) {
                currentState = MenuState::SongSelect;
                songSelect.Init();
                activeFrames = 0;
                return;
            }
        }
    }
    else if (currentState == MenuState::SongSelect) {
        activeFrames++;
        if (activeFrames < 5) return;

        songSelect.Update();

        if (songSelect.IsPlaySelected()) return;

        if (songSelect.IsBackSelected()) {
            currentState = MenuState::Main;
            activeFrames = 0;
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
        for (int i = 32; i <= 126; i++) codepoints[count++] = i;

        const wchar_t* krText = L"플레이설정나가기";
        for (int i = 0; krText[i] != L'\0'; i++) {
            codepoints[count++] = (int)krText[i];
        }

        if (FileExists("fonts/Pretendard-Black.ttf")) {
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

    Vector2 mousePos = GetMousePosition();
    float tiltX = ((mousePos.x / (float)screenWidth) - 0.5f) * 14.0f;
    float tiltY = ((mousePos.y / (float)screenHeight) - 0.5f) * 10.0f;

    ClearBackground(Color{ 8, 8, 8, 255 });

    Vector2 center = { (float)screenWidth * 0.5f + tiltX, (float)screenHeight * 0.40f + tiltY };

    for (int i = 0; i < MAX_WARP_STARS; ++i) {
        float cosA = std::cos(warpStars[i].angle);
        float sinA = std::sin(warpStars[i].angle);

        Vector2 p1 = { center.x + cosA * warpStars[i].distance, center.y + sinA * warpStars[i].distance };
        Vector2 p2 = { center.x + cosA * (warpStars[i].distance + warpStars[i].length), center.y + sinA * (warpStars[i].distance + warpStars[i].length) };

        float alphaRatio = std::min(warpStars[i].distance / (screenWidth * 0.4f), 1.0f);
        Color wColor = warpStars[i].color;
        wColor.a = (unsigned char)(alphaRatio * 210.0f);

        DrawLineEx(p1, p2, warpStars[i].size * (0.8f + alphaRatio * 1.2f), wColor);
    }

    float trackBottomY = (float)screenHeight;
    float trackTopY    = center.y + 40.0f;
    float topWidth     = 140.0f;
    float bottomWidth  = (float)screenWidth * 0.58f;

    Vector2 trackTopLeft  = { center.x - topWidth * 0.5f, trackTopY };
    Vector2 trackTopRight = { center.x + topWidth * 0.5f, trackTopY };
    Vector2 trackBotLeft  = { center.x - bottomWidth * 0.5f, trackBottomY };
    Vector2 trackBotRight = { center.x + bottomWidth * 0.5f, trackBottomY };

    DrawTriangle(trackTopLeft, trackBotLeft, trackBotRight, Color{ 16, 16, 16, 180 });
    DrawTriangle(trackTopLeft, trackBotRight, trackTopRight, Color{ 16, 16, 16, 180 });

    for (int lane = 0; lane <= 4; ++lane) {
        float t = (float)lane / 4.0f;
        Vector2 pTop = { trackTopLeft.x + (trackTopRight.x - trackTopLeft.x) * t, trackTopY };
        Vector2 pBot = { trackBotLeft.x + (trackBotRight.x - trackBotLeft.x) * t, trackBottomY };
        
        Color lineCol = (lane == 0 || lane == 4) ? Color{ 255, 255, 255, 160 } : Color{ 150, 150, 150, 80 };
        DrawLineEx(pTop, pBot, (lane == 0 || lane == 4) ? 2.0f : 1.0f, lineCol);
    }

    for (int i = 0; i < MAX_TRACK_NOTES; ++i) {
        float p = trackNotes[i].progress;
        float curY = trackTopY + (trackBottomY - trackTopY) * p;
        float laneLeftT  = (float)trackNotes[i].lane / 4.0f;
        float laneRightT = (float)(trackNotes[i].lane + 1) / 4.0f;

        float curTopW = topWidth + (bottomWidth - topWidth) * p;
        float noteX1  = (center.x - curTopW * 0.5f) + curTopW * laneLeftT + 2.0f;
        float noteX2  = (center.x - curTopW * 0.5f) + curTopW * laneRightT - 2.0f;

        float noteHeight = 10.0f + p * 18.0f;
        Color nColor     = trackNotes[i].color;
        nColor.a         = (unsigned char)(p * 210.0f);

        DrawRectangleRec({ noteX1, curY, noteX2 - noteX1, noteHeight }, nColor);
    }

    float eqWidth = (float)screenWidth / EQUALIZER_BARS;
    for (int i = 0; i < EQUALIZER_BARS; ++i) {
        float h = eqBarHeights[i];
        Color barCol = (i % 2 == 0) ? Color{ 230, 230, 230, 100 } : Color{ 140, 140, 140, 100 };
        DrawRectangleRec({ i * eqWidth, (float)screenHeight - h, eqWidth - 2.0f, h }, barCol);
    }

    if (introFinished || introTimer > 0.60f) {
        float beat1 = std::sin(pulseTimer * 4.5f) * 5.0f;
        float beat2 = std::cos(pulseTimer * 3.0f) * 7.0f;

        float r1 = 125.0f + beat1;
        float r2 = 155.0f + beat2;

        DrawRing(center, r1 - 2.0f, r1 + 2.0f, ringRotation1, ringRotation1 + 220.0f, 60, Color{ 255, 255, 255, 150 });
        DrawRing(center, r2 - 2.5f, r2 + 2.5f, ringRotation2, ringRotation2 + 160.0f, 60, Color{ 160, 160, 160, 120 });

        for (int i = 0; i < 8; ++i) {
            float angle = (ringRotation1 * 1.1f) + (i * 45.0f);
            float radAngle = angle * (PI / 180.0f);
            Vector2 orbPos = { center.x + std::cos(radAngle) * r2, center.y + std::sin(radAngle) * r2 };
            
            unsigned char orbVal = (i % 2 == 0) ? 255 : 170;
            DrawCircleV(orbPos, 2.8f, Color{ orbVal, orbVal, orbVal, 255 });
        }
    }

    if (shockwaveAlpha1 > 0.001f) {
        DrawCircleLines((int)center.x, (int)center.y, shockwaveRadius1, Color{ 255, 255, 255, (unsigned char)(shockwaveAlpha1 * 255.0f) });
    }
    if (shockwaveAlpha2 > 0.001f) {
        DrawCircleLines((int)center.x, (int)center.y, shockwaveRadius2, Color{ 180, 180, 180, (unsigned char)(shockwaveAlpha2 * 255.0f) });
    }

    if (setaTexture.id != 0) {
        float beatPulse = introFinished ? (std::sin(pulseTimer * 5.5f) * 0.02f + std::cos(pulseTimer * 11.0f) * 0.008f) : 0.0f;
        float currentScale = logoScale + beatPulse;

        float texW = (float)setaTexture.width * currentScale * 0.28f;
        float texH = (float)setaTexture.height * currentScale * 0.28f;

        Rectangle srcRec  = { 0.0f, 0.0f, (float)setaTexture.width, (float)setaTexture.height };
        Rectangle destRec = { center.x, center.y, texW, texH };
        Vector2 origin    = { texW * 0.5f, texH * 0.5f };

        float auraScale  = currentScale * 1.16f;
        float auraW      = (float)setaTexture.width * auraScale * 0.28f;
        float auraH      = (float)setaTexture.height * auraScale * 0.28f;
        Rectangle auraDest = { center.x, center.y, auraW, auraH };
        Vector2 auraOrigin = { auraW * 0.5f, auraH * 0.5f };

        unsigned char auraAlpha = (unsigned char)(logoAlpha * (80.0f + std::sin(pulseTimer * 4.0f) * 30.0f));
        DrawTexturePro(setaTexture, srcRec, auraDest, auraOrigin, logoRotation, Color{ 255, 255, 255, auraAlpha });

        unsigned char mainAlpha = (unsigned char)(logoAlpha * 255.0f);
        DrawTexturePro(setaTexture, srcRec, destRec, origin, logoRotation, Color{ 255, 255, 255, mainAlpha });
    }

    if (introFinished) {
        const char* menuLabels[3] = { "플레이", "설정", "나가기" };

        float menuCenterY = (float)screenHeight * 0.81f;
        float baseCardW   = 180.0f;
        float baseCardH   = 62.0f;
        float spacing     = 220.0f;
        float startX      = center.x - spacing;

        for (int i = 0; i < totalOptions; ++i) {
            float anim   = btnAnimFactor[i];
            float pShift = btnPulse[i] * 14.0f;
            float optionX = startX + i * spacing;

            float curW = baseCardW + anim * 30.0f + pShift * 0.8f;
            float curH = baseCardH + anim * 12.0f + pShift * 0.4f;

            float elasticY = -anim * 10.0f + (anim > 0.5f ? std::sin(pulseTimer * 6.0f) * 3.0f : 0.0f);
            float optionY  = menuCenterY + elasticY - (pShift * 0.5f);

            Rectangle cardBox = { optionX - curW * 0.5f, optionY - curH * 0.5f, curW, curH };

            if (anim > 0.001f) {
                float outerGlowPad = 7.0f * anim + pShift;
                Rectangle glowBox  = { cardBox.x - outerGlowPad, cardBox.y - outerGlowPad, cardBox.width + outerGlowPad * 2.0f, cardBox.height + outerGlowPad * 2.0f };
                unsigned char glowAlpha = (unsigned char)(anim * 100.0f + pShift * 80.0f);
                DrawRectangleRounded(glowBox, 0.38f, 8, Color{ 255, 255, 255, glowAlpha });
            }

            Color bgColor = (anim > 0.5f) 
                ? Color{ (unsigned char)(240 + anim * 15), (unsigned char)(240 + anim * 15), (unsigned char)(240 + anim * 15), 255 } 
                : Color{ 20, 20, 20, (unsigned char)(200 + anim * 45) };

            DrawRectangleRounded(cardBox, 0.35f, 8, bgColor);

            Color borderColor = (anim > 0.5f) 
                ? Color{ 255, 255, 255, 255 } 
                : Color{ 80, 80, 80, 255 };

            DrawRectangleRoundedLinesEx(cardBox, 0.35f, 8, 1.5f + anim * 1.8f, borderColor);

            if (anim > 0.01f) {
                float lineW = (cardBox.width - 20.0f) * std::min(anim * 1.2f, 1.0f);
                float lineX = cardBox.x + (cardBox.width - lineW) * 0.5f;
                float lineY = cardBox.y + cardBox.height - 4.5f;
                DrawLineEx({ lineX, lineY }, { lineX + lineW, lineY }, 3.0f, Color{ 10, 10, 10, (unsigned char)(anim * 240.0f) });

                float sweepPos = btnSweep[i];
                if (sweepPos > 0.0f && sweepPos < 1.0f) {
                    float sweepX = cardBox.x + cardBox.width * sweepPos;
                    DrawRectangleGradientH((int)sweepX - 20, (int)cardBox.y, 40, (int)cardBox.height,
                        Color{ 255, 255, 255, 0 }, Color{ 255, 255, 255, (unsigned char)(anim * 90.0f) });
                }
            }

            float fontSize   = 25.0f + anim * 9.0f;
            Vector2 textSize = MeasureTextEx(customFont, menuLabels[i], fontSize, 2.0f);
            Vector2 textPos  = { cardBox.x + (cardBox.width - textSize.x) * 0.5f, cardBox.y + (cardBox.height - textSize.y) * 0.5f };

            Color textColor = (anim > 0.5f) 
                ? Color{ 12, 12, 12, 255 } 
                : Color{ 210, 210, 210, 255 };

            if (customFont.texture.id != 0) {
                DrawTextEx(customFont, menuLabels[i], textPos, fontSize, 2.0f, textColor);
            } else {
                DrawText(menuLabels[i], (int)textPos.x, (int)textPos.y, (int)fontSize, textColor);
            }
        }
    }

    for (int i = 0; i < MAX_BURST; ++i) {
        if (bursts[i].life > 0.0f) {
            Color bColor = bursts[i].color;
            bColor.a = (unsigned char)(bursts[i].alpha * 255.0f);
            DrawCircleV(bursts[i].position, bursts[i].size, bColor);
        }
    }
}

bool MainMenu::IsGameStartSelected() const {
    if (activeFrames < 10) return false;
    return (currentState == MenuState::SongSelect && songSelect.IsPlaySelected());
}

bool MainMenu::IsExitSelected() const {
    return (currentState == MenuState::Main && selectedIndex == 2 && introFinished && (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)));
}

void MainMenu::Reset() {
    selectedIndex    = 0;
    pulseTimer       = 0.0f;
    currentState     = MenuState::Main;
    activeFrames     = 0;
    introTimer       = 0.0f;
    introFinished    = false;
    logoScale        = 0.0f;
    logoAlpha        = 0.0f;
    logoRotation     = -360.0f;
    shockwaveRadius1 = 0.0f;
    shockwaveAlpha1  = 0.0f;
    shockwaveRadius2 = 0.0f;
    shockwaveAlpha2  = 0.0f;

    for (int i = 0; i < 3; ++i) {
        btnAnimFactor[i] = 0.0f;
        btnVelocities[i] = 0.0f;
        btnPulse[i]      = 0.0f;
        btnSweep[i]      = 0.0f;
    }
}