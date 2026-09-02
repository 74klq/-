#include "main_Menu.h"
#include <cstdlib>
#include <algorithm>
#include <cmath>

static Texture2D playTexture = { 0 };
static Texture2D settingTexture = { 0 };
static Texture2D exitTexture = { 0 };
static Texture2D menuUiTextures[4] = { { 0 }, { 0 }, { 0 }, { 0 } };
static bool isAssetsLoaded = false;

MainMenu::MainMenu() {
    selectedIndex = 0;
    pulseTimer = 0.0f;
}

void MainMenu::Update() {
    bgRotation.Update();

    float dt = GetFrameTime();
    pulseTimer += dt * 4.0f;

    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        selectedIndex = (selectedIndex - 1 + totalOptions) % totalOptions;
    }
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        selectedIndex = (selectedIndex + 1 + totalOptions) % totalOptions;
    }
}

void MainMenu::Draw(int screenWidth, int screenHeight) {
    if (!isAssetsLoaded) {
        playTexture = LoadTexture("assets/play.png");
        settingTexture = LoadTexture("assets/setting.png");
        exitTexture = LoadTexture("assets/exit.png");

        menuUiTextures[0] = LoadTexture("assets/MenuUI2.png");
        menuUiTextures[1] = LoadTexture("assets/MenuUI3.png");
        menuUiTextures[2] = LoadTexture("assets/MenuUI4.png");
        menuUiTextures[3] = LoadTexture("assets/MenuUI5.png");

        if (playTexture.id != 0) SetTextureFilter(playTexture, TEXTURE_FILTER_POINT);
        if (settingTexture.id != 0) SetTextureFilter(settingTexture, TEXTURE_FILTER_POINT);
        if (exitTexture.id != 0) SetTextureFilter(exitTexture, TEXTURE_FILTER_POINT);
        
        for (int i = 0; i < 4; ++i) {
            if (menuUiTextures[i].id != 0) SetTextureFilter(menuUiTextures[i], TEXTURE_FILTER_BILINEAR);
        }

        isAssetsLoaded = true;
    }

    bgRotation.Draw(screenWidth, screenHeight);

    int uiTexMap[6] = { 0, 0, 0, 3, 2, 1 };
    int currentBgIdx = bgRotation.GetCurrentIndex();
    int currentCreditIdx = uiTexMap[currentBgIdx];
    Texture2D currentTex = menuUiTextures[currentCreditIdx];

    float pulseFactor = std::sin(pulseTimer * 0.8f) * 0.012f;

    if (currentTex.id != 0) {
        float baseScale = std::min((float)screenWidth / (float)currentTex.width, (float)screenHeight / (float)currentTex.height) * 0.58f;
        
        float alphaMultiplier = 1.0f;
        if (bgRotation.IsTransitioning()) {
            float t = bgRotation.GetTransitionProgress();
            float easeT = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
            alphaMultiplier = 1.0f - easeT;
        }

        float uiWidth = (float)currentTex.width * (baseScale + pulseFactor);
        float uiHeight = (float)currentTex.height * (baseScale + pulseFactor);
        float uiX = ((float)screenWidth - uiWidth) / 2.0f;
        float uiY = ((float)screenHeight - uiHeight) / 2.0f;

        Rectangle srcRec = { 0.0f, 0.0f, (float)currentTex.width, (float)currentTex.height };
        Vector2 origin = { 0.0f, 0.0f };

        float glowScale = baseScale + pulseFactor + 0.018f;
        float glowWidth = (float)currentTex.width * glowScale;
        float glowHeight = (float)currentTex.height * glowScale;
        float glowX = ((float)screenWidth - glowWidth) / 2.0f;
        float glowY = ((float)screenHeight - glowHeight) / 2.0f;
        Rectangle glowRect = { glowX, glowY, glowWidth, glowHeight };
        
        unsigned char glowAlpha = (unsigned char)(alphaMultiplier * 90.0f);
        Color glowTint = { 140, 170, 255, glowAlpha }; 
        DrawTexturePro(currentTex, srcRec, glowRect, origin, 0.0f, glowTint);

        Rectangle destRec = { uiX, uiY, uiWidth, uiHeight };
        unsigned char uiAlpha = (unsigned char)(alphaMultiplier * 245.0f);
        Color uiTint = { 255, 255, 255, uiAlpha }; 
        DrawTexturePro(currentTex, srcRec, destRec, origin, 0.0f, uiTint);
    }

    if (bgRotation.IsTransitioning()) {
        int nextBgIdx = bgRotation.GetNextIndex();
        int nextCreditIdx = uiTexMap[nextBgIdx];
        Texture2D nextTex = menuUiTextures[nextCreditIdx];

        if (nextTex.id != 0) {
            float baseScale = std::min((float)screenWidth / (float)nextTex.width, (float)screenHeight / (float)nextTex.height) * 0.58f;
            float t = bgRotation.GetTransitionProgress();
            float easeT = t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);

            float uiWidth = (float)nextTex.width * (baseScale + pulseFactor);
            float uiHeight = (float)nextTex.height * (baseScale + pulseFactor);
            float uiX = ((float)screenWidth - uiWidth) / 2.0f;
            float uiY = ((float)screenHeight - uiHeight) / 2.0f;

            Rectangle srcRec = { 0.0f, 0.0f, (float)nextTex.width, (float)nextTex.height };
            Vector2 origin = { 0.0f, 0.0f };
            Rectangle destRec = { uiX, uiY, uiWidth, uiHeight };

            unsigned char uiAlpha = (unsigned char)(easeT * 245.0f);
            Color uiTint = { 255, 255, 255, uiAlpha }; 
            DrawTexturePro(nextTex, srcRec, destRec, origin, 0.0f, uiTint);
        }
    }

    int baseTabWidth = 180;
    int baseTabHeight = 55;
    int totalWidth = baseTabWidth * 3 + 80;
    int startX = (screenWidth - totalWidth) / 2;
    int startY = screenHeight - 120;

    Texture2D textures[3] = { playTexture, settingTexture, exitTexture };

    for (int i = 0; i < totalOptions; ++i) {
        int currentX = startX + i * (baseTabWidth + 40);
        bool isSelected = (i == selectedIndex);

        Texture2D currentTexBtn = textures[i];
        if (currentTexBtn.id != 0) {
            Rectangle srcRec = { 0.0f, 0.0f, (float)currentTexBtn.width, (float)currentTexBtn.height };
            
            float breath = isSelected ? (std::sin(pulseTimer * 1.5f) * 3.5f) : 0.0f;
            
            float targetW = ((float)currentTexBtn.width * 0.45f) + breath;
            float targetH = ((float)currentTexBtn.height * 0.45f) + (breath * 0.8f);

            float drawX = (float)currentX + ((float)baseTabWidth - targetW) / 2.0f;
            float drawY = (float)startY + ((float)baseTabHeight - targetH) / 2.0f;

            Rectangle destRec = { drawX, drawY, targetW, targetH };
            Vector2 origin = { 0.0f, 0.0f };
            
            Color tint = isSelected ? WHITE : (Color){200, 210, 225, 255};

            DrawTexturePro(currentTexBtn, srcRec, destRec, origin, 0.0f, tint);
        }
    }

    const char* guideText = "dev: four beat / C++ / FMOD/ Raylib";
    int guideWidth = MeasureText(guideText, 14);
    DrawText(guideText, (screenWidth - guideWidth) / 2, screenHeight - 40, 14, (Color){110, 125, 150, 255});
}

bool MainMenu::IsGameStartSelected() const {
    return (selectedIndex == 0 && IsKeyPressed(KEY_ENTER));
}

bool MainMenu::IsExitSelected() const {
    return (selectedIndex == 2 && IsKeyPressed(KEY_ENTER));
}

void MainMenu::Reset() {
    selectedIndex = 0;
    pulseTimer = 0.0f;
}