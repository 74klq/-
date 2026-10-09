#include "play_main.h"
#include <cstdio>
#include <cmath>
#include <set>
#include <vector>

#define BASE_PLAYFIELD_WIDTH 400.0f
#define PLAYFIELD_X ((float)GetScreenWidth() - BASE_PLAYFIELD_WIDTH) * 0.5f
#define PLAYFIELD_WIDTH BASE_PLAYFIELD_WIDTH

PlayMainUI::PlayMainUI()
    : m_MainFont{ 0 },
      m_JacketTexture{ 0 },
      m_LoadedJacketPath(""),
      m_HealthPulseTimer(0.0f)
{
}

PlayMainUI::~PlayMainUI()
{
    UnloadJacket();
    if (m_MainFont.texture.id != 0)
    {
        UnloadFont(m_MainFont);
        m_MainFont = Font{ 0 };
    }
}

void PlayMainUI::Init()
{
    m_HealthPulseTimer = 0.0f;

    if (m_MainFont.texture.id == 0)
    {
        std::set<int> cpSet;

        for (int i = 32; i <= 126; ++i) cpSet.insert(i);

        std::vector<std::string> texts = {
            "별이 보이지 않는 밤", "비밀 인형극 II"
        };

        for (const auto& text : texts) {
            const char* p = text.c_str();
            while (*p) {
                int c = 0; int byteCount = 0;
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
        m_MainFont = LoadFontEx("fonts/Pretendard-Black.ttf", 32, codepoints.data(), static_cast<int>(codepoints.size()));

        if (m_MainFont.texture.id != 0) {
            SetTextureFilter(m_MainFont.texture, TEXTURE_FILTER_BILINEAR);
       }
   }
}

void PlayMainUI::UnloadJacket()
{
    if (m_JacketTexture.id != 0)
    {
        UnloadTexture(m_JacketTexture);
        m_JacketTexture = Texture2D{ 0 };
    }
    m_LoadedJacketPath = "";
}

void PlayMainUI::Update(const SongInformation& songInfo)
{
    m_HealthPulseTimer += GetFrameTime();

    if (m_LoadedJacketPath != songInfo.jacketPath)
    {
        UnloadJacket();
        if (!songInfo.jacketPath.empty() && FileExists(songInfo.jacketPath.c_str()))
        {
            m_JacketTexture = LoadTexture(songInfo.jacketPath.c_str());
            if (m_JacketTexture.id != 0)
            {
                SetTextureFilter(m_JacketTexture, TEXTURE_FILTER_BILINEAR);
            }
        }
        m_LoadedJacketPath = songInfo.jacketPath;
    }
}

void PlayMainUI::Draw(const SongInformation& songInfo, int screenWidth, int screenHeight)
{
    float uiScale = static_cast<float>(screenHeight) / 720.0f;
    float maxSideWidth = 260.0f * uiScale;

    float leftW = (PLAYFIELD_X - 40.0f) > maxSideWidth ? maxSideWidth : (PLAYFIELD_X - 40.0f);
    float leftX = PLAYFIELD_X - 20.0f - leftW;
    float leftY = (static_cast<float>(screenHeight) - (680.0f * uiScale)) * 0.5f;
    float leftH = 680.0f * uiScale;

    float rightX = PLAYFIELD_X + PLAYFIELD_WIDTH + 20.0f;
    float rightW = (static_cast<float>(screenWidth) - rightX - 20.0f) > maxSideWidth ? maxSideWidth : (static_cast<float>(screenWidth) - rightX - 20.0f);
    float rightY = (static_cast<float>(screenHeight) - (680.0f * uiScale)) * 0.5f;
    float rightH = 680.0f * uiScale;

    if (rightW < 100.0f) return;

    DrawSidebarPanel(leftX, leftY, leftW, leftH);
    DrawSidebarPanel(rightX, rightY, rightW, rightH);

    float contentLeftX = leftX + (16.0f * uiScale);
    float contentLeftW = leftW - (32.0f * uiScale);
    float currentLeftY = leftY + (20.0f * uiScale);

    float jacketSize = contentLeftW > (180.0f * uiScale) ? (180.0f * uiScale) : contentLeftW;
    DrawJacket(songInfo, contentLeftX + (contentLeftW - jacketSize) * 0.5f, currentLeftY, jacketSize);
    currentLeftY += jacketSize + (25.0f * uiScale);

    DrawSongInfo(songInfo, contentLeftX, currentLeftY, contentLeftW);
    currentLeftY += 110.0f * uiScale;

    DrawHealthGauge(songInfo, contentLeftX, currentLeftY, contentLeftW, 10.0f * uiScale);

    float contentRightX = rightX + (16.0f * uiScale);
    float contentRightW = rightW - (32.0f * uiScale);
    float currentRightY = rightY + (20.0f * uiScale);

    DrawScoreAndAccuracy(songInfo, contentRightX, currentRightY, contentRightW);
    currentRightY += 210.0f * uiScale;

    DrawJudgements(songInfo, contentRightX, currentRightY, contentRightW);

    const bool isMapActive = !songInfo.jacketPath.empty() && !m_LoadedJacketPath.empty();
    float curTimeSec = isMapActive ? songInfo.playTimeSec : 0.0f;
    if (curTimeSec < 0.0f) curTimeSec = 0.0f;

    float laneAreaWidth = 4.0f * 68.0f;
    float laneStartX = PLAYFIELD_X + (PLAYFIELD_WIDTH - laneAreaWidth) * 0.5f;
    float topY = 42.0f * uiScale;
    float barHeight = 12.0f * uiScale;

    float maxTimeSec = (songInfo.totalTimeSec > 0.0f) ? songInfo.totalTimeSec : 180.0f;
    int curSecTotal = static_cast<int>(curTimeSec);
    int curMin = curSecTotal / 60;
    int curSec = curSecTotal % 60;

    int totalSecTotal = static_cast<int>(maxTimeSec);
    int totalMin = totalSecTotal / 60;
    int totalSec = totalSecTotal % 60;

    char playfieldTimerStr[32];
    snprintf(playfieldTimerStr, sizeof(playfieldTimerStr), "%02d:%02d / %02d:%02d", curMin, curSec, totalMin, totalSec);

    Font fontToUse = (m_MainFont.texture.id != 0) ? m_MainFont : GetFontDefault();
    Vector2 pfTmSz = MeasureTextEx(fontToUse, playfieldTimerStr, 15.0f * uiScale, 1.0f);
    DrawTextEx(fontToUse, playfieldTimerStr, { laneStartX + (laneAreaWidth - pfTmSz.x) * 0.5f, topY - (22.0f * uiScale) }, 15.0f * uiScale, 1.0f, WHITE);

    DrawRectangleRounded({ laneStartX - (2.0f * uiScale), topY - (2.0f * uiScale), laneAreaWidth + (4.0f * uiScale), barHeight + (4.0f * uiScale) }, 0.2f, 4, Color{ 20, 20, 25, 220 });
    DrawRectangleRoundedLines({ laneStartX - (2.0f * uiScale), topY - (2.0f * uiScale), laneAreaWidth + (4.0f * uiScale), barHeight + (4.0f * uiScale) }, 0.2f, 4, Color{ 255, 255, 255, 120 });
    DrawRectangle(static_cast<int>(laneStartX), static_cast<int>(topY), static_cast<int>(laneAreaWidth), static_cast<int>(barHeight), Color{ 40, 40, 50, 255 });

    float progressRatio = curTimeSec / maxTimeSec;
    if (progressRatio > 1.0f) progressRatio = 1.0f;

    if (progressRatio > 0.0f)
    {
        DrawRectangle(static_cast<int>(laneStartX), static_cast<int>(topY), static_cast<int>(laneAreaWidth * progressRatio), static_cast<int>(barHeight), Color{ 220, 220, 255, 220 });
    }

    float arrowX = laneStartX + progressRatio * laneAreaWidth;
    Vector2 p1 = { arrowX, topY + barHeight + (10.0f * uiScale) };
    Vector2 p2 = { arrowX - (7.0f * uiScale), topY + barHeight + (2.0f * uiScale) };
    Vector2 p3 = { arrowX + (7.0f * uiScale), topY + barHeight + (2.0f * uiScale) };
    DrawTriangle(p1, p2, p3, WHITE);
    DrawTriangleLines(p1, p2, p3, Color{ 180, 180, 180, 255 });
}

void PlayMainUI::DrawSidebarPanel(float x, float y, float width, float height)
{
    (void)x;
    (void)y;
    (void)width;
    (void)height;
}

void PlayMainUI::DrawJacket(const SongInformation& songInfo, float x, float y, float size)
{
    Rectangle destRec = { x, y, size, size };
    float roundness = 0.12f;
    int segments = 16;

    if (m_JacketTexture.id != 0)
    {
        Rectangle srcRec = { 0.0f, 0.0f, static_cast<float>(m_JacketTexture.width), static_cast<float>(m_JacketTexture.height) };
        DrawTexturePro(m_JacketTexture, srcRec, destRec, Vector2{ 0.0f, 0.0f }, 0.0f, WHITE);

        float r = size * roundness;
        Color bgCol = Color{ 15, 15, 15, 255 };
        for (int i = 0; i < static_cast<int>(r); ++i)
        {
            for (int j = 0; j < static_cast<int>(r); ++j)
            {
                if ((r - i) * (r - i) + (r - j) * (r - j) > r * r)
                {
                    DrawPixel(static_cast<int>(x) + i, static_cast<int>(y) + j, bgCol);
                    DrawPixel(static_cast<int>(x + size - 1) - i, static_cast<int>(y) + j, bgCol);
                    DrawPixel(static_cast<int>(x) + i, static_cast<int>(y + size - 1) - j, bgCol);
                    DrawPixel(static_cast<int>(x + size - 1) - i, static_cast<int>(y + size - 1) - j, bgCol);
                }
            }
        }
    }
    else
    {
        DrawRectangleRounded(destRec, roundness, segments, Color{ 40, 40, 40, 255 });
        Font fontToUse = (m_MainFont.texture.id != 0) ? m_MainFont : GetFontDefault();
        const char* noImgText = "NO IMAGE";
        Vector2 textSz = MeasureTextEx(fontToUse, noImgText, 20.0f, 1.0f);
        DrawTextEx(fontToUse, noImgText, { x + (size - textSz.x) * 0.5f, y + (size - textSz.y) * 0.5f }, 20.0f, 1.0f, Color{ 160, 160, 160, 255 });
    }

    DrawRectangleRoundedLines(destRec, roundness, segments, Color{ 255, 255, 255, 60 });
}

void PlayMainUI::DrawSongInfo(const SongInformation& songInfo, float x, float y, float width)
{
    Font fontToUse = (m_MainFont.texture.id != 0) ? m_MainFont : GetFontDefault();
    float uiScale = static_cast<float>(GetScreenHeight()) / 720.0f;

    DrawTextEx(fontToUse, songInfo.title.c_str(), { x, y }, 28.0f * uiScale, 1.0f, WHITE);
    DrawTextEx(fontToUse, songInfo.artist.c_str(), { x, y + (36.0f * uiScale) }, 18.0f * uiScale, 1.0f, Color{ 200, 200, 200, 255 });

    char diffBuf[64];
    snprintf(diffBuf, sizeof(diffBuf), "%s LV.%d", songInfo.difficultyName.c_str(), songInfo.difficultyLevel);
    DrawTextEx(fontToUse, diffBuf, { x, y + (68.0f * uiScale) }, 18.0f * uiScale, 1.0f, WHITE);

    char bpmBuf[32];
    snprintf(bpmBuf, sizeof(bpmBuf), "BPM %.0f", songInfo.bpm);
    Vector2 bpmSz = MeasureTextEx(fontToUse, bpmBuf, 18.0f * uiScale, 1.0f);
    DrawTextEx(fontToUse, bpmBuf, { x + width - bpmSz.x, y + (68.0f * uiScale) }, 18.0f * uiScale, 1.0f, WHITE);
}

void PlayMainUI::DrawScoreAndAccuracy(const SongInformation& songInfo, float x, float y, float width)
{
    Font fontToUse = (m_MainFont.texture.id != 0) ? m_MainFont : GetFontDefault();
    float uiScale = static_cast<float>(GetScreenHeight()) / 720.0f;
    const bool isMapActive = !songInfo.jacketPath.empty() && !m_LoadedJacketPath.empty();

    const char* highLabel = "HIGH SCORE";
    Vector2 highLblSz = MeasureTextEx(fontToUse, highLabel, 14.0f * uiScale, 1.0f);
    DrawTextEx(fontToUse, highLabel, { x + width - highLblSz.x, y }, 14.0f * uiScale, 1.0f, Color{ 180, 180, 180, 255 });

    const char* highVal = "0";
    Vector2 highValSz = MeasureTextEx(fontToUse, highVal, 20.0f * uiScale, 1.0f);
    DrawTextEx(fontToUse, highVal, { x + width - highValSz.x, y + (18.0f * uiScale) }, 20.0f * uiScale, 1.0f, WHITE);

    float currentY = y + (52.0f * uiScale);
    const char* curLabel = "SCORE";
    Vector2 curLblSz = MeasureTextEx(fontToUse, curLabel, 16.0f * uiScale, 1.0f);
    DrawTextEx(fontToUse, curLabel, { x + width - curLblSz.x, currentY }, 16.0f * uiScale, 1.0f, Color{ 180, 180, 180, 255 });

    int sc = 0;
    if (isMapActive)
    {
        sc = songInfo.score;
        int hitScore = (songInfo.perfectCount + songInfo.greatCount + songInfo.goodCount) * 10;
        if (sc < hitScore) sc = hitScore;
        if (sc < 0) sc = 0;
        if (sc > 99999999) sc = 99999999;
    }

    char scoreBuf[32];
    if (sc >= 1000000) snprintf(scoreBuf, sizeof(scoreBuf), "%d,%03d,%03d", sc / 1000000, (sc / 1000) % 1000, sc % 1000);
    else if (sc >= 1000) snprintf(scoreBuf, sizeof(scoreBuf), "%d,%03d", sc / 1000, sc % 1000);
    else snprintf(scoreBuf, sizeof(scoreBuf), "%d", sc);

    Vector2 scSz = MeasureTextEx(fontToUse, scoreBuf, 52.0f * uiScale, 1.5f);
    DrawTextEx(fontToUse, scoreBuf, { x + width - scSz.x, currentY + (22.0f * uiScale) }, 52.0f * uiScale, 1.5f, WHITE);

    currentY += 88.0f * uiScale;

    DrawTextEx(fontToUse, "ACCURACY", { x, currentY }, 15.0f * uiScale, 1.0f, Color{ 180, 180, 180, 255 });
    char accBuf[32];
    float accuracy = isMapActive ? songInfo.accuracy : 0.0f;
    snprintf(accBuf, sizeof(accBuf), "%.2f%%", accuracy);
    DrawTextEx(fontToUse, accBuf, { x, currentY + (20.0f * uiScale) }, 26.0f * uiScale, 1.0f, WHITE);

    const char* comboLbl = "COMBO";
    Vector2 comboLblSz = MeasureTextEx(fontToUse, comboLbl, 15.0f * uiScale, 1.0f);
    DrawTextEx(fontToUse, comboLbl, { x + width - comboLblSz.x, currentY }, 15.0f * uiScale, 1.0f, Color{ 180, 180, 180, 255 });

    int currentCombo = 0;
    if (isMapActive)
    {
        currentCombo = songInfo.maxCombo;
        if (currentCombo < 0) currentCombo = 0;
    }

    char comboBuf[32];
    snprintf(comboBuf, sizeof(comboBuf), "%d", currentCombo);
    Vector2 comboValSz = MeasureTextEx(fontToUse, comboBuf, 26.0f * uiScale, 1.0f);
    DrawTextEx(fontToUse, comboBuf, { x + width - comboValSz.x, currentY + (20.0f * uiScale) }, 26.0f * uiScale, 1.0f, WHITE);
}

void PlayMainUI::DrawJudgements(const SongInformation& songInfo, float x, float y, float width)
{
    Font fontToUse = (m_MainFont.texture.id != 0) ? m_MainFont : GetFontDefault();
    float uiScale = static_cast<float>(GetScreenHeight()) / 720.0f;
    const bool isMapActive = !songInfo.jacketPath.empty() && !m_LoadedJacketPath.empty();

    struct JudgItem { const char* label; int count; };
    JudgItem items[4] = {
        { "PERFECT", isMapActive ? songInfo.perfectCount : 0 },
        { "GREAT",   isMapActive ? songInfo.greatCount : 0 },
        { "GOOD",    isMapActive ? songInfo.goodCount : 0 },
        { "MISS",    isMapActive ? songInfo.missCount : 0 }
    };

    float itemY = y;
    for (int i = 0; i < 4; ++i)
    {
        DrawTextEx(fontToUse, items[i].label, { x, itemY }, 18.0f * uiScale, 1.0f, WHITE);

        char cntBuf[16];
        snprintf(cntBuf, sizeof(cntBuf), "%d", items[i].count);
        Vector2 cntSz = MeasureTextEx(fontToUse, cntBuf, 22.0f * uiScale, 1.0f);

        DrawTextEx(fontToUse, cntBuf, { x + width - cntSz.x, itemY }, 22.0f * uiScale, 1.0f, WHITE);
        itemY += 30.0f * uiScale;
    }
}

void PlayMainUI::DrawHealthGauge(const SongInformation& songInfo, float x, float y, float width, float height)
{
    Font fontToUse = (m_MainFont.texture.id != 0) ? m_MainFont : GetFontDefault();
    float uiScale = static_cast<float>(GetScreenHeight()) / 720.0f;
    const bool isMapActive = !songInfo.jacketPath.empty() && !m_LoadedJacketPath.empty();

    float hpRatio = isMapActive ? songInfo.hpRatio : 0.0f;
    if (hpRatio < 0.0f) hpRatio = 0.0f;
    if (hpRatio > 1.0f) hpRatio = 1.0f;

    int hpPercent = static_cast<int>(hpRatio * 100.0f);

    DrawTextEx(fontToUse, "HP", { x, y }, 15.0f * uiScale, 1.0f, Color{ 180, 180, 180, 255 });

    char percentBuf[16];
    snprintf(percentBuf, sizeof(percentBuf), "%d%%", hpPercent);
    Vector2 pSz = MeasureTextEx(fontToUse, percentBuf, 15.0f * uiScale, 1.0f);
    DrawTextEx(fontToUse, percentBuf, { x + width - pSz.x, y }, 15.0f * uiScale, 1.0f, WHITE);

    float barY = y + (24.0f * uiScale);
    DrawRectangle(static_cast<int>(x), static_cast<int>(barY), static_cast<int>(width), static_cast<int>(height), Color{ 255, 255, 255, 30 });

    if (hpRatio > 0.0f)
    {
        float fillWidth = width * hpRatio;
        DrawRectangle(static_cast<int>(x), static_cast<int>(barY), static_cast<int>(fillWidth), static_cast<int>(height), WHITE);
    }
}
