#include "Load.h"
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <set>
#include <vector>
#include <string>

float LoadingScreen::EaseOutCubic(float x) {
    return 1.0f - std::pow(1.0f - x, 3.0f);
}

float LoadingScreen::EaseInOutQuad(float x) {
    return x < 0.5f ? 2.0f * x * x : 1.0f - std::pow(-2.0f * x + 2.0f, 2.0f) / 2.0f;
}

// ⭐️ 생성자에서 멤버 플래그 초기화
LoadingScreen::LoadingScreen()
    : currentState(LoadingState::IDLE),
      curtainProgress(0.0f),
      currentLoadProgress(0.0f),
      targetLoadProgress(0.0f),
      stateTimer(0.0f),
      fontLoaded(false) { // 추가
      suitFont.texture.id = 0;
}

// ⭐️ 소멸자에서 안전하게 한글 폰트 자원 언로드
LoadingScreen::~LoadingScreen() {
    if (fontLoaded) {
        UnloadFont(suitFont);
    }
}

// ⭐️ Init 할 때 폰트를 확실하게 로드하여 준비해 둡니다.
void LoadingScreen::Init() {
    currentState = LoadingState::IDLE;
    curtainProgress = 0.0f;
    currentLoadProgress = 0.0f;
    targetLoadProgress = 0.0f;
    stateTimer = 0.0f;
    currentSong = {};

    if (!fontLoaded) {
        std::set<int> cpSet;
        for (int i = 32; i <= 126; ++i) cpSet.insert(i);
        
        std::vector<std::string> texts = {
            "리소스 로딩중...",
            "0123456789%()"
        };

        for (const auto& text : texts) {
            const char* p = text.c_str();
            while (*p) {
                int c = 0;
                int byteCount = 0;
                unsigned char lead = *p;
                if (lead < 0x80) { c = lead; byteCount = 1; }
                else if ((lead & 0xE0) == 0xC0) { c = lead & 0x1F; byteCount = 2; }
                else if ((lead & 0xF0) == 0xE0) { c = lead & 0x0F; byteCount = 3; }
                else if ((lead & 0xF8) == 0xF0) { c = lead & 0x07; byteCount = 4; }
                else { byteCount = 1; p++; continue; }
                
                bool valid = true;
                for (int i = 1; i < byteCount; ++i) {
                    if ((p[i] & 0xC0) != 0x80) { valid = false; break; }
                    c = (c << 6) | (p[i] & 0x3F);
                }
                if (valid) cpSet.insert(c);
                p += byteCount;
            }
        }

        std::vector<int> codepoints(cpSet.begin(), cpSet.end());
        suitFont = LoadFontEx("fonts/Pretendard-Black.ttf", 32, codepoints.data(), (int)codepoints.size());

        if (suitFont.texture.id != 0) {
            SetTextureFilter(suitFont.texture, TEXTURE_FILTER_BILINEAR);
            fontLoaded = true;
        }
    }
}

void LoadingScreen::Start(const LoadingSongData& songData) {
    currentSong = songData;
    currentState = LoadingState::CURTAIN_IN;
    curtainProgress = 0.0f;
    currentLoadProgress = 0.0f;
    targetLoadProgress = 0.0f;
    stateTimer = 0.0f;
}

void LoadingScreen::SetTargetProgress(float progress) {
    targetLoadProgress = std::clamp(progress, 0.0f, 1.0f);
}

void LoadingScreen::Update(float dt) {
    switch (currentState) {
    case LoadingState::CURTAIN_IN: {
        curtainProgress += dt * 1.5f;
        if (curtainProgress >= 1.0f) {
            curtainProgress = 1.0f;
            currentState = LoadingState::LOADING;
            stateTimer = 0.0f;
            currentLoadProgress = 0.0f;
        }
        break;
    }
    case LoadingState::LOADING: {
        stateTimer += dt;
        currentLoadProgress = std::clamp(stateTimer / 5.0f, 0.0f, 1.0f);
        if (stateTimer >= 5.0f) {
            currentLoadProgress = 1.0f;
            currentState = LoadingState::CURTAIN_OUT;
        }
        break;
    }
    case LoadingState::CURTAIN_OUT: {
        curtainProgress -= dt * 1.5f;
        if (curtainProgress <= 0.0f) {
            curtainProgress = 0.0f;
            currentState = LoadingState::COMPLETE;
        }
        break;
    }
    default:
        break;
    }
}

// ⭐️ 외부 인자 제거 완료
void LoadingScreen::Draw() {
    if (currentState == LoadingState::IDLE) return;

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();

    if (currentState == LoadingState::LOADING) {
        DrawRectangle(0, 0, screenWidth, screenHeight, BLACK);
        if (currentSong.jacketTexture.id != 0) {
            float bgOffsetX = std::sin(stateTimer * 0.3f) * 15.0f;
            float bgOffsetY = std::cos(stateTimer * 0.25f) * 15.0f;
            DrawTexturePro(
                currentSong.jacketTexture,
                (Rectangle){ 0, 0, (float)currentSong.jacketTexture.width, (float)currentSong.jacketTexture.height },
                (Rectangle){ -20.0f + bgOffsetX, -20.0f + bgOffsetY, (float)screenWidth + 40.0f, (float)screenHeight + 40.0f },
                (Vector2){ 0, 0 }, 0.0f, (Color){ 255, 255, 255, 35 }
            );
        }
        
        DrawJacket();
        DrawProgressBar(); // ⭐️ 인자 없이 호출
    }
    else if (currentState == LoadingState::CURTAIN_IN || currentState == LoadingState::CURTAIN_OUT) {
        DrawCurtain();
    }
}

void LoadingScreen::DrawCurtain() {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    float easedProgress = EaseInOutQuad(curtainProgress);
    float curtainWidth = (screenWidth * 0.5f) * easedProgress;

    DrawRectangle(0, 0, (int)curtainWidth, screenHeight, BLACK);
    DrawRectangle(screenWidth - (int)curtainWidth, 0, (int)curtainWidth, screenHeight, BLACK);
}

void LoadingScreen::DrawJacket() {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    float uiScale = (float)screenHeight / 720.0f;
    
    float pulseScale = 1.0f - (std::sin(stateTimer * 1.8f) * 0.5f + 0.5f) * 0.08f;
    float baseJacketSize = 280.0f * uiScale;
    float jacketSize = baseJacketSize * pulseScale;
    float jacketX = (screenWidth - jacketSize) * 0.5f;
    float jacketY = (screenHeight - jacketSize) * 0.4f;

    if (currentSong.jacketTexture.id != 0) {
        for (int i = 2; i >= 1; --i) {
            float delayScale = 1.0f - (std::sin((stateTimer - i * 0.12f) * 1.8f) * 0.5f + 0.5f) * 0.08f;
            float aiSize = baseJacketSize * delayScale;
            float aiX = (screenWidth - aiSize) * 0.5f;
            float aiY = (screenHeight - aiSize) * 0.4f;
            
            DrawTexturePro(
                currentSong.jacketTexture,
                (Rectangle){ 0, 0, (float)currentSong.jacketTexture.width, (float)currentSong.jacketTexture.height },
                (Rectangle){ aiX, aiY, aiSize, aiSize },
                (Vector2){ 0, 0 }, 0.0f, (Color){ 255, 255, 255, (unsigned char)(50 / i) }
            );
        }
    }

    DrawRectangleRounded((Rectangle){ jacketX + 6.0f * uiScale, jacketY + 6.0f * uiScale, jacketSize, jacketSize }, 0.06f, 4, (Color){ 0, 0, 0, 180 });

    if (currentSong.jacketTexture.id != 0) {
        DrawTexturePro(
            currentSong.jacketTexture,
            (Rectangle){ 0, 0, (float)currentSong.jacketTexture.width, (float)currentSong.jacketTexture.height },
            (Rectangle){ jacketX, jacketY, jacketSize, jacketSize },
            (Vector2){ 0, 0 }, 0.0f, WHITE
        );
    } else {
        DrawRectangleRounded((Rectangle){ jacketX, jacketY, jacketSize, jacketSize }, 0.06f, 4, (Color){ 30, 35, 45, 255 });
    }
    DrawRectangleRoundedLines((Rectangle){ jacketX, jacketY, jacketSize, jacketSize }, 0.06f, 4, WHITE);
}

void LoadingScreen::DrawProgressBar() {
    Font fontToUse = (fontLoaded) ? suitFont : GetFontDefault();

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    float uiScale = (float)screenHeight / 720.0f;

    float barWidth = 600.0f * uiScale;
    float barHeight = 38.0f * uiScale;
    float barX = (screenWidth - barWidth) * 0.5f;
    float barY = (float)screenHeight * 0.82f;

    DrawRectangleRounded((Rectangle){ barX, barY, barWidth, barHeight }, 0.25f, 4, (Color){ 20, 24, 32, 220 });
    DrawRectangleRoundedLines((Rectangle){ barX, barY, barWidth, barHeight }, 0.25f, 4, (Color){ 255, 255, 255, 80 });

    float fillWidth = (barWidth - 6.0f * uiScale) * currentLoadProgress;
    if (fillWidth > 0.0f) {
        DrawRectangleRounded((Rectangle){ barX + 3.0f * uiScale, barY + 3.0f * uiScale, fillWidth, barHeight - 6.0f * uiScale }, 0.25f, 4, WHITE);
    }

    int percentage = (int)(currentLoadProgress * 100.0f);
    char textBuf[128];
    snprintf(textBuf, sizeof(textBuf), "리소스 로딩중... (%d%%)", percentage);

    float fontSize = 18.0f * uiScale;
    Color textColor = (currentLoadProgress > 0.5f) ? (Color){ 20, 20, 20, 255 } : WHITE;

    if (fontToUse.texture.id != 0) {
        Vector2 textSize = MeasureTextEx(fontToUse, textBuf, fontSize, 1.0f);
        float textX = barX + (barWidth - textSize.x) * 0.5f;
        float textY = barY + (barHeight - textSize.y) * 0.5f;
        DrawTextEx(fontToUse, textBuf, (Vector2){ textX, textY }, fontSize, 1.0f, textColor);
    } else {
        int textWidth = MeasureText(textBuf, (int)fontSize);
        int textX = (int)(barX + (barWidth - (float)textWidth) * 0.5f);
        int textY = (int)(barY + (barHeight - fontSize) * 0.5f);
        DrawText(textBuf, textX, textY, (int)fontSize, textColor);
    }
}

bool LoadingScreen::IsCurtainClosed() const {
    return (currentState == LoadingState::LOADING) || (currentState == LoadingState::CURTAIN_IN && curtainProgress >= 1.0f);
}

bool LoadingScreen::IsComplete() const { return currentState == LoadingState::COMPLETE; }
LoadingState LoadingScreen::GetState() const { return currentState; }