#include "play_main.h"
#include <cstdio>
#include <cmath>

static const float PLAYFIELD_X = 400.0f;
static const float PLAYFIELD_WIDTH = 400.0f;

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
    m_MainFont = LoadFont("fonts/Pretendard-Black.ttf");
    m_HealthPulseTimer = 0.0f;
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
    float sideX = PLAYFIELD_X + PLAYFIELD_WIDTH + 20.0f;
    float sideY = 20.0f;
    float sideWidth = static_cast<float>(screenWidth) - sideX - 20.0f;
    float sideHeight = static_cast<float>(screenHeight) - 40.0f;

    if (sideWidth < 200.0f) return;

    DrawSidebarPanel(sideX, sideY, sideWidth, sideHeight);

    float contentX = sideX + 16.0f;
    float contentW = sideWidth - 32.0f;
    float currentY = sideY + 20.0f;

    float jacketSize = contentW > 220.0f ? 220.0f : contentW;
    DrawJacket(songInfo, contentX + (contentW - jacketSize) * 0.5f, currentY, jacketSize);
    currentY += jacketSize + 20.0f;

    DrawSongInfo(songInfo, contentX, currentY, contentW);
    currentY += 100.0f;

    DrawScoreAndAccuracy(songInfo, contentX, currentY, contentW);
    currentY += 105.0f;

    DrawJudgements(songInfo, contentX, currentY, contentW);
    currentY += 125.0f;

    DrawHealthGauge(songInfo, contentX, currentY, contentW, 22.0f);
}

void PlayMainUI::DrawSidebarPanel(float x, float y, float width, float height)
{
    DrawRectangleRounded({ x, y, width, height }, 0.04f, 8, Color{ 12, 14, 20, 225 });
    DrawRectangleRoundedLines({ x, y, width, height }, 0.04f, 8, Color{ 60, 75, 100, 180 });

    DrawRectangleGradientV(static_cast<int>(x + 2), static_cast<int>(y + 2), static_cast<int>(width - 4), 40, Color{ 255, 255, 255, 12 }, Color{ 255, 255, 255, 0 });
}

void PlayMainUI::DrawJacket(const SongInformation& songInfo, float x, float y, float size)
{
    DrawRectangleRounded({ x - 3.0f, y - 3.0f, size + 6.0f, size + 6.0f }, 0.05f, 8, Color{ 0, 0, 0, 200 });

    if (m_JacketTexture.id != 0)
    {
        Rectangle srcRec = { 0.0f, 0.0f, static_cast<float>(m_JacketTexture.width), static_cast<float>(m_JacketTexture.height) };
        Rectangle destRec = { x, y, size, size };
        DrawTexturePro(m_JacketTexture, srcRec, destRec, Vector2{ 0.0f, 0.0f }, 0.0f, WHITE);
    }
    else
    {
        DrawRectangleRounded({ x, y, size, size }, 0.05f, 8, Color{ 30, 34, 45, 255 });
        Font fontToUse = (m_MainFont.texture.id != 0) ? m_MainFont : GetFontDefault();
        const char* noImgText = "NO IMAGE";
        Vector2 textSz = MeasureTextEx(fontToUse, noImgText, 20.0f, 1.0f);
        DrawTextEx(fontToUse, noImgText, { x + (size - textSz.x) * 0.5f, y + (size - textSz.y) * 0.5f }, 20.0f, 1.0f, Color{ 120, 125, 140, 255 });
    }

    DrawRectangleRoundedLines({ x, y, size, size }, 0.05f, 8, Color{ 255, 255, 255, 80 });
}

void PlayMainUI::DrawSongInfo(const SongInformation& songInfo, float x, float y, float width)
{
    Font fontToUse = (m_MainFont.texture.id != 0) ? m_MainFont : GetFontDefault();

    Vector2 titleSz = MeasureTextEx(fontToUse, songInfo.title.c_str(), 22.0f, 1.0f);
    DrawTextEx(fontToUse, songInfo.title.c_str(), { x, y }, 22.0f, 1.0f, Color{ 250, 250, 250, 255 });

    Vector2 artistSz = MeasureTextEx(fontToUse, songInfo.artist.c_str(), 15.0f, 1.0f);
    DrawTextEx(fontToUse, songInfo.artist.c_str(), { x, y + 28.0f }, 15.0f, 1.0f, Color{ 160, 170, 190, 255 });

    DrawRectangle(static_cast<int>(x), static_cast<int>(y + 50.0f), static_cast<int>(width), 1, Color{ 255, 255, 255, 30 });

    char diffBuf[64];
    snprintf(diffBuf, sizeof(diffBuf), "%s Lv.%d", songInfo.difficultyName.c_str(), songInfo.difficultyLevel);
    DrawTextEx(fontToUse, diffBuf, { x, y + 58.0f }, 16.0f, 1.0f, Color{ 220, 180, 80, 255 });

    char bpmBuf[32];
    snprintf(bpmBuf, sizeof(bpmBuf), "BPM %.0f", songInfo.bpm);
    Vector2 bpmSz = MeasureTextEx(fontToUse, bpmBuf, 15.0f, 1.0f);
    DrawTextEx(fontToUse, bpmBuf, { x + width - bpmSz.x, y + 59.0f }, 15.0f, 1.0f, Color{ 140, 150, 170, 255 });
}

void PlayMainUI::DrawScoreAndAccuracy(const SongInformation& songInfo, float x, float y, float width)
{
    Font fontToUse = (m_MainFont.texture.id != 0) ? m_MainFont : GetFontDefault();

    DrawTextEx(fontToUse, "SCORE", { x, y }, 13.0f, 1.0f, Color{ 120, 135, 160, 255 });

    char scoreBuf[32];
    snprintf(scoreBuf, sizeof(scoreBuf), "%07d", songInfo.score);
    DrawTextEx(fontToUse, scoreBuf, { x, y + 16.0f }, 32.0f, 1.5f, Color{ 255, 255, 255, 255 });

    DrawTextEx(fontToUse, "ACCURACY", { x, y + 58.0f }, 13.0f, 1.0f, Color{ 120, 135, 160, 255 });

    char accBuf[32];
    snprintf(accBuf, sizeof(accBuf), "%.2f%%", songInfo.accuracy);
    DrawTextEx(fontToUse, accBuf, { x, y + 74.0f }, 20.0f, 1.0f, Color{ 100, 220, 255, 255 });

    char comboBuf[32];
    snprintf(comboBuf, sizeof(comboBuf), "MAX %d", songInfo.maxCombo);
    Vector2 comboSz = MeasureTextEx(fontToUse, comboBuf, 15.0f, 1.0f);
    DrawTextEx(fontToUse, comboBuf, { x + width - comboSz.x, y + 78.0f }, 15.0f, 1.0f, Color{ 180, 190, 210, 255 });
}

void PlayMainUI::DrawJudgements(const SongInformation& songInfo, float x, float y, float width)
{
    Font fontToUse = (m_MainFont.texture.id != 0) ? m_MainFont : GetFontDefault();

    DrawRectangleRounded({ x, y, width, 115.0f }, 0.06f, 8, Color{ 18, 22, 32, 180 });
    DrawRectangleRoundedLines({ x, y, width, 115.0f }, 0.06f, 8, Color{ 45, 55, 75, 150 });

    struct JudgItem
    {
        const char* label;
        int count;
        Color color;
    };

    JudgItem items[4] = {
        { "PERFECT", songInfo.perfectCount, Color{ 255, 225, 100, 255 } },
        { "GREAT",   songInfo.greatCount,   Color{ 100, 220, 120, 255 } },
        { "GOOD",    songInfo.goodCount,    Color{ 100, 180, 255, 255 } },
        { "MISS",    songInfo.missCount,    Color{ 230, 80, 90, 255 } }
    };

    float itemY = y + 10.0f;
    for (int i = 0; i < 4; ++i)
    {
        DrawTextEx(fontToUse, items[i].label, { x + 12.0f, itemY }, 13.0f, 1.0f, items[i].color);

        char cntBuf[16];
        snprintf(cntBuf, sizeof(cntBuf), "%d", items[i].count);
        Vector2 cntSz = MeasureTextEx(fontToUse, cntBuf, 14.0f, 1.0f);
        DrawTextEx(fontToUse, cntBuf, { x + width - 12.0f - cntSz.x, itemY }, 14.0f, 1.0f, Color{ 230, 230, 240, 255 });

        itemY += 24.0f;
    }
}

void PlayMainUI::DrawHealthGauge(const SongInformation& songInfo, float x, float y, float width, float height)
{
    Font fontToUse = (m_MainFont.texture.id != 0) ? m_MainFont : GetFontDefault();

    DrawTextEx(fontToUse, "HP", { x, y - 16.0f }, 12.0f, 1.0f, Color{ 140, 155, 180, 255 });

    float hpRatio = songInfo.hpRatio;
    if (hpRatio < 0.0f) hpRatio = 0.0f;
    if (hpRatio > 1.0f) hpRatio = 1.0f;

    DrawRectangleRounded({ x, y, width, height }, 0.3f, 8, Color{ 20, 24, 35, 255 });
    DrawRectangleRoundedLines({ x, y, width, height }, 0.3f, 8, Color{ 50, 60, 80, 200 });

    if (hpRatio > 0.0f)
    {
        float fillWidth = (width - 4.0f) * hpRatio;
        if (fillWidth < 6.0f) fillWidth = 6.0f;

        Color hpColor = Color{ 80, 210, 130, 255 };
        if (hpRatio < 0.3f)
        {
            float pulse = 0.5f + 0.5f * sinf(m_HealthPulseTimer * 10.0f);
            hpColor = Color{ 230, static_cast<unsigned char>(60 + pulse * 60), 70, 255 };
        }

        DrawRectangleRounded({ x + 2.0f, y + 2.0f, fillWidth, height - 4.0f }, 0.25f, 8, hpColor);
    }
}