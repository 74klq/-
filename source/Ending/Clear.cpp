#include "Clear.h"
#include <cstdio>
#include <cmath>
#include <set>
#include <vector>
#include <algorithm>
#include <string>

namespace
{
    Color WithAlpha(Color color, float alpha)
    {
        color.a = static_cast<unsigned char>(std::clamp(alpha, 0.0f, 255.0f));
        return color;
    }

    void DrawTextCentered(Font font, const char* text, float centerX, float y, float fontSize, float spacing, Color color)
    {
        Vector2 size = MeasureTextEx(font, text, fontSize, spacing);
        DrawTextEx(font, text, { centerX - size.x * 0.5f, y }, fontSize, spacing, color);
    }
}

ClearScreen::ClearScreen()
    : m_MainFont{ 0 },
      m_JacketTexture{ 0 },
      m_LoadedJacketPath(""),
      m_AnimTimer(0.0f)
{
}

ClearScreen::~ClearScreen()
{
    Release();
}

void ClearScreen::Init()
{
    m_AnimTimer = 0.0f;

    if (m_MainFont.texture.id == 0)
    {
        std::set<int> cpSet;

        for (int i = 32; i <= 126; ++i)
            cpSet.insert(i);

        std::vector<std::string> texts = {
            "별이 보이지 않는 밤", "비밀 인형극 II", "클리어", "점수", "정확도", "최대 콤보",
            "PERFECT", "GREAT", "GOOD", "MISS", "BREAK", "FAST", "SLOW", "곡 선택으로",
            "RESULT", "CLEAR", "MAX COMBO", "JUDGEMENT", "PLAY COMPLETE", "NEW RECORD",
            "SONG", "ARTIST", "DIFFICULTY", "SCORE", "ACCURACY", "RANK", "F", "D", "C", "B", "A", "S", "STAGE CLEAR",
            "MAX", "300", "200", "100", "50", "Press ESC to Return"
        };

        for (const auto& text : texts)
        {
            const char* p = text.c_str();

            while (*p)
            {
                int c = 0;
                int byteCount = 0;
                unsigned char lead = static_cast<unsigned char>(*p);

                if (lead < 0x80)
                {
                    c = lead;
                    byteCount = 1;
                }
                else if ((lead & 0xE0) == 0xC0)
                {
                    c = lead & 0x1F;
                    byteCount = 2;
                }
                else if ((lead & 0xF0) == 0xE0)
                {
                    c = lead & 0x0F;
                    byteCount = 3;
                }
                else if ((lead & 0xF8) == 0xF0)
                {
                    c = lead & 0x07;
                    byteCount = 4;
                }
                else
                {
                    ++p;
                    continue;
                }

                bool valid = true;

                for (int i = 1; i < byteCount; ++i)
                {
                    unsigned char next = static_cast<unsigned char>(p[i]);

                    if ((next & 0xC0) != 0x80)
                    {
                        valid = false;
                        break;
                    }

                    c = (c << 6) | (next & 0x3F);
                }

                if (valid)
                    cpSet.insert(c);

                p += byteCount;
            }
        }

        std::vector<int> codepoints(cpSet.begin(), cpSet.end());
        m_MainFont = LoadFontEx(
            "fonts/Pretendard-Black.ttf",
            48,
            codepoints.data(),
            static_cast<int>(codepoints.size())
        );

        if (m_MainFont.texture.id != 0)
            SetTextureFilter(m_MainFont.texture, TEXTURE_FILTER_BILINEAR);
    }
}

void ClearScreen::UnloadJacket()
{
    if (m_JacketTexture.id != 0)
    {
        UnloadTexture(m_JacketTexture);
        m_JacketTexture = Texture2D{ 0 };
    }

    m_LoadedJacketPath = "";
}

void ClearScreen::Update(float dt)
{
    m_AnimTimer += std::max(0.0f, dt);
}

void ClearScreen::Draw(const ClearResultData& result, int screenWidth, int screenHeight)
{
    if (screenWidth <= 0 || screenHeight <= 0)
        return;

    if (m_LoadedJacketPath != result.jacketPath)
    {
        UnloadJacket();

        if (!result.jacketPath.empty() && FileExists(result.jacketPath.c_str()))
        {
            m_JacketTexture = LoadTexture(result.jacketPath.c_str());

            if (m_JacketTexture.id != 0)
                SetTextureFilter(m_JacketTexture, TEXTURE_FILTER_BILINEAR);
        }

        m_LoadedJacketPath = result.jacketPath;
    }

    const float baseScale = std::min(static_cast<float>(screenWidth) / 1280.0f, static_cast<float>(screenHeight) / 720.0f);
    const float scale = std::max(0.55f, baseScale);
    const float sw = static_cast<float>(screenWidth);
    const float sh = static_cast<float>(screenHeight);
    Font font = m_MainFont.texture.id != 0 ? m_MainFont : GetFontDefault();

    DrawRectangleGradientV(0, 0, screenWidth, screenHeight, Color{ 15, 15, 20, 255 }, Color{ 5, 5, 8, 255 });

    if (m_JacketTexture.id != 0)
    {
        Rectangle src = { 0, 0, static_cast<float>(m_JacketTexture.width), static_cast<float>(m_JacketTexture.height) };
        Rectangle dest = { 0, 0, sw, sh };
        DrawTexturePro(m_JacketTexture, src, dest, { 0, 0 }, 0.0f, WHITE);
        DrawRectangle(0, 0, screenWidth, screenHeight, Color{ 0, 0, 0, 190 });
    }
    else
    {
        DrawRectangle(0, 0, screenWidth, screenHeight, Color{ 10, 10, 15, 240 });
    }

    DrawRectangle(0, 0, screenWidth, static_cast<int>(75.0f * scale), Color{ 10, 10, 15, 230 });
    std::string titleStr = result.title.empty() ? "Unknown Track" : result.title;
    std::string artistStr = result.artist.empty() ? "Unknown Artist" : result.artist;
    std::string diffStr = result.difficultyName.empty() ? "Normal" : result.difficultyName;
    std::string headerText = titleStr + " - " + artistStr + " [" + diffStr + "]";
    DrawTextEx(font, headerText.c_str(), { 30.0f * scale, 22.0f * scale }, 22.0f * scale, 1.0f, RAYWHITE);

    float leftX = 60.0f * scale;
    float leftY = 120.0f * scale;

    Color rankColor = RAYWHITE;
    std::string rankStr = result.grade.empty() ? "S" : result.grade;
    if (rankStr == "S" || rankStr == "SS" || rankStr == "SSS") rankColor = Color{ 0, 240, 255, 255 };
    else if (rankStr == "A") rankColor = Color{ 50, 255, 100, 255 };
    else if (rankStr == "B") rankColor = Color{ 100, 150, 255, 255 };
    else if (rankStr == "C") rankColor = Color{ 255, 150, 50, 255 };
    else rankColor = Color{ 255, 50, 50, 255 };

    DrawTextEx(font, rankStr.c_str(), { leftX + 20.0f * scale, leftY }, 150.0f * scale, 2.0f, rankColor);

    float statsY = leftY + 175.0f * scale;
    DrawTextEx(font, "TOTAL SCORE", { leftX, statsY }, 13.0f * scale, 1.0f, Color{ 160, 160, 180, 255 });
    char scoreBuf[64];
    std::snprintf(scoreBuf, sizeof(scoreBuf), "%07d / %07d", result.score, 1000000);
    DrawTextEx(font, scoreBuf, { leftX, statsY + 20.0f * scale }, 30.0f * scale, 1.0f, RAYWHITE);

    DrawTextEx(font, "ACCURACY", { leftX, statsY + 75.0f * scale }, 13.0f * scale, 1.0f, Color{ 160, 160, 180, 255 });
    char accBuf[64];
    std::snprintf(accBuf, sizeof(accBuf), "%.2f%%", result.accuracy);
    DrawTextEx(font, accBuf, { leftX, statsY + 95.0f * scale }, 30.0f * scale, 1.0f, Color{ 0, 240, 255, 255 });

    float rightX = sw - 450.0f * scale;
    float rightY = 130.0f * scale;
    float rowH = 48.0f * scale;

    struct JudgeEntry {
        const char* label;
        int count;
        Color color;
    };

    JudgeEntry entries[6] = {
        { "MAX", result.perfectCount, Color{ 0, 220, 255, 255 } },
        { "300", result.greatCount, Color{ 255, 215, 0, 255 } },
        { "200", result.goodCount, Color{ 50, 255, 100, 255 } },
        { "100", result.breakCount, Color{ 100, 150, 255, 255 } },
        { "50",  result.fastCount, Color{ 255, 150, 50, 255 } },
        { "Miss", result.missCount, Color{ 255, 50, 50, 255 } }
    };

    for (int i = 0; i < 6; ++i) {
        float y = rightY + i * rowH;
        DrawTextEx(font, entries[i].label, { rightX, y }, 22.0f * scale, 1.0f, entries[i].color);
        
        char countBuf[32];
        std::snprintf(countBuf, sizeof(countBuf), "%d", entries[i].count);
        Vector2 sz = MeasureTextEx(font, countBuf, 22.0f * scale, 1.0f);
        DrawTextEx(font, countBuf, { rightX + 350.0f * scale - sz.x, y }, 22.0f * scale, 1.0f, RAYWHITE);
    }

    float botY = sh - 80.0f * scale;
    char comboBuf[64];
    std::snprintf(comboBuf, sizeof(comboBuf), "MAX COMBO : %dx", result.maxCombo);
    DrawTextEx(font, comboBuf, { leftX, botY }, 22.0f * scale, 1.0f, Color{ 255, 200, 0, 255 });

    const char* prompt = "Press ESC to Return";
    float alphaPulse = 0.5f + 0.5f * std::sin(m_AnimTimer * 4.0f);
    Color promptColor = WithAlpha(Color{ 255, 255, 255, 255 }, alphaPulse * 255.0f);
    Vector2 promptSz = MeasureTextEx(font, prompt, 18.0f * scale, 1.0f);
    DrawTextEx(font, prompt, { (sw - promptSz.x) * 0.5f, botY }, 18.0f * scale, 1.0f, promptColor);
}

void ClearScreen::Release()
{
    UnloadJacket();

    if (m_MainFont.texture.id != 0)
    {
        UnloadFont(m_MainFont);
        m_MainFont = Font{ 0 };
    }
}