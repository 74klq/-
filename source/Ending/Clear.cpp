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

    void DrawPanel(Rectangle rec, Color fill, Color border, float rounding = 0.08f)
    {
        DrawRectangleRounded(rec, rounding, 12, fill);
        DrawRectangleRoundedLinesEx(rec, rounding, 12, 1.0f, border);
    }

    void DrawLabelValue(Font font, const char* label, const char* value, float x, float y, float width, float scale, Color valueColor = RAYWHITE)
    {
        DrawTextEx(font, label, { x, y }, 13.0f * scale, 1.0f, Color{ 132, 145, 170, 255 });
        Vector2 valueSize = MeasureTextEx(font, value, 18.0f * scale, 0.5f);
        DrawTextEx(font, value, { x + width - valueSize.x, y - 3.0f * scale }, 18.0f * scale, 0.5f, valueColor);
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
            "SONG", "ARTIST", "DIFFICULTY", "SCORE", "ACCURACY", "RANK", "F", "D", "C", "B", "A", "S"
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

    Color bgTop{ 5, 5, 7, 255 };
    Color bgBottom{ 20, 20, 23, 255 };
    DrawRectangleGradientV(0, 0, screenWidth, screenHeight, bgTop, bgBottom);

    for (int i = 0; i < 70; ++i)
    {
        float x = std::fmod(static_cast<float>((i * 173 + 29) % 1280) / 1280.0f * sw + std::sin(m_AnimTimer * 0.18f + i * 0.7f) * 12.0f * scale + sw, sw);
        float y = std::fmod(static_cast<float>((i * 97 + 41) % 720) / 720.0f * sh + std::cos(m_AnimTimer * 0.15f + i * 0.9f) * 8.0f * scale + sh, sh);
        float pulse = 0.45f + 0.35f * std::sin(m_AnimTimer * 1.5f + i);
        DrawCircleV({ x, y }, (i % 4 == 0 ? 2.0f : 1.0f) * scale, WithAlpha(Color{ 225, 230, 240, 255 }, pulse * 90.0f));
    }

    DrawCircleGradient(
        Vector2{ sw * 0.78f, sh * 0.23f },
        260.0f * scale,
        Color{ 210, 215, 225, 16 },
        Color{ 5, 5, 7, 0 }
    );

    const float margin = 24.0f * scale;
    const float panelX = margin;
    const float panelY = margin;
    const float panelW = sw - margin * 2.0f;
    const float panelH = sh - margin * 2.0f;

    DrawPanel(
        { panelX, panelY, panelW, panelH },
        Color{ 10, 10, 12, 249 },
        Color{ 150, 150, 160, 130 },
        0.025f
    );

    const float headerH = 58.0f * scale;
    DrawRectangleRounded(
        { panelX + 1.0f, panelY + 1.0f, panelW - 2.0f, headerH },
        0.04f,
        10,
        Color{ 25, 25, 28, 255 }
    );
    DrawRectangle(
        static_cast<int>(panelX + 1.0f),
        static_cast<int>(panelY + headerH),
        static_cast<int>(panelW - 2.0f),
        static_cast<int>(2.0f * scale),
        Color{ 235, 235, 240, 255 }
    );

    DrawTextEx(font, "RESULT", { panelX + 24.0f * scale, panelY + 13.0f * scale }, 27.0f * scale, 2.0f * scale, RAYWHITE);
    DrawTextEx(font, "PLAY COMPLETE", { panelX + 145.0f * scale, panelY + 23.0f * scale }, 11.0f * scale, 1.6f * scale, Color{ 180, 180, 190, 255 });

    const char* clearText = "CLEAR";
    DrawTextEx(font, clearText, { panelX + panelW - 120.0f * scale, panelY + 20.0f * scale }, 17.0f * scale, 2.0f * scale, Color{ 235, 235, 240, 255 });

    const float contentY = panelY + headerH + 16.0f * scale;
    const float contentH = panelH - headerH - 30.0f * scale;
    const float leftX = panelX + 18.0f * scale;
    const float leftW = std::min(360.0f * scale, panelW * 0.34f);
    const float gap = 16.0f * scale;
    const float rightX = leftX + leftW + gap;
    const float rightW = panelX + panelW - 18.0f * scale - rightX;

    DrawPanel(
        { leftX, contentY, leftW, contentH },
        Color{ 17, 17, 20, 255 },
        Color{ 75, 75, 82, 210 },
        0.035f
    );

    const float innerPad = 16.0f * scale;
    const float artX = leftX + innerPad;
    const float artY = contentY + innerPad;
    const float artW = leftW - innerPad * 2.0f;
    const float artH = std::min(artW, contentH * 0.48f);
    Rectangle artRec{ artX, artY, artW, artH };

    DrawRectangleRounded(
        { artRec.x - 3.0f * scale, artRec.y - 3.0f * scale, artRec.width + 6.0f * scale, artRec.height + 6.0f * scale },
        0.04f,
        10,
        Color{ 245, 245, 250, 30 }
    );

    if (m_JacketTexture.id != 0)
    {
        float textureW = static_cast<float>(m_JacketTexture.width);
        float textureH = static_cast<float>(m_JacketTexture.height);
        float sourceAspect = textureW / std::max(1.0f, textureH);
        float targetAspect = artW / std::max(1.0f, artH);
        Rectangle src{ 0.0f, 0.0f, textureW, textureH };

        if (sourceAspect > targetAspect)
        {
            float cropW = textureH * targetAspect;
            src.x = (textureW - cropW) * 0.5f;
            src.width = cropW;
        }
        else
        {
            float cropH = textureW / targetAspect;
            src.y = (textureH - cropH) * 0.5f;
            src.height = cropH;
        }

        DrawTexturePro(m_JacketTexture, src, artRec, { 0.0f, 0.0f }, 0.0f, WHITE);
        DrawRectangleGradientV(
            static_cast<int>(artX),
            static_cast<int>(artY + artH * 0.55f),
            static_cast<int>(artW),
            static_cast<int>(artH * 0.45f),
            Color{ 5, 5, 7, 0 },
            Color{ 8, 10, 20, 220 }
        );
    }
    else
    {
        DrawRectangleGradientV(
            static_cast<int>(artX),
            static_cast<int>(artY),
            static_cast<int>(artW),
            static_cast<int>(artH),
            Color{ 42, 42, 48, 255 },
            Color{ 15, 15, 18, 255 }
        );

        for (int i = 0; i < 9; ++i)
        {
            float lineX = artX + artW * (static_cast<float>(i) / 8.0f);
            DrawLineEx({ lineX, artY }, { lineX - artH * 0.45f, artY + artH }, 1.0f * scale, Color{ 145, 145, 155, 65 });
        }

        DrawTextCentered(font, "NO JACKET", artX + artW * 0.5f, artY + artH * 0.45f, 14.0f * scale, 2.0f * scale, Color{ 155, 155, 165, 255 });
    }

    DrawRectangleRoundedLinesEx(artRec, 0.04f, 10, 1.0f * scale, Color{ 220, 220, 228, 220 });

    const float infoY = artY + artH + 16.0f * scale;
    DrawTextEx(font, "TRACK", { artX, infoY }, 10.0f * scale, 2.0f * scale, Color{ 185, 185, 195, 255 });

    float titleFontSize = 22.0f * scale;
    std::string title = result.title.empty() ? "Unknown Track" : result.title;
    while (title.size() > 4 && MeasureTextEx(font, title.c_str(), titleFontSize, 0.2f * scale).x > artW)
        title.pop_back();
    if (title != result.title && title.size() >= 3)
        title.replace(title.size() - 3, 3, "...");

    DrawTextEx(font, title.c_str(), { artX, infoY + 19.0f * scale }, titleFontSize, 0.2f * scale, RAYWHITE);
    DrawTextEx(font, result.artist.c_str(), { artX, infoY + 48.0f * scale }, 14.0f * scale, 0.4f * scale, Color{ 175, 175, 184, 255 });

    char diffBuf[128];
    std::snprintf(diffBuf, sizeof(diffBuf), "%s  /  LV.%d", result.difficultyName.c_str(), result.difficultyLevel);
    DrawRectangleRounded(
        { artX, infoY + 76.0f * scale, artW, 30.0f * scale },
        0.12f,
        8,
        Color{ 37, 37, 42, 255 }
    );
    DrawTextEx(font, diffBuf, { artX + 10.0f * scale, infoY + 83.0f * scale }, 12.0f * scale, 0.3f * scale, Color{ 225, 225, 232, 255 });

    const float gradeSize = std::min(118.0f * scale, contentH * 0.22f);
    const float gradeX = leftX + leftW - gradeSize - 15.0f * scale;
    const float gradeY = contentY + contentH - gradeSize - 16.0f * scale;
    const float glow = 0.5f + 0.5f * std::sin(m_AnimTimer * 2.3f);

    Color gradeColor = Color{ 205, 205, 214, 255 };
    if (result.grade == "S" || result.grade == "SS" || result.grade == "SSS")
        gradeColor = Color{ 255, 255, 255, 255 };
    else if (result.grade == "A")
        gradeColor = Color{ 235, 235, 242, 255 };
    else if (result.grade == "B")
        gradeColor = Color{ 210, 210, 220, 255 };
    else if (result.grade == "C")
        gradeColor = Color{ 180, 180, 192, 255 };
    else if (result.grade == "D" || result.grade == "F")
        gradeColor = Color{ 145, 145, 158, 255 };

    const Vector2 rankCenter{ gradeX + gradeSize * 0.5f, gradeY + gradeSize * 0.5f };
    const float orbitRadius = gradeSize * 0.55f;
    const float orbitRotation = m_AnimTimer * 24.0f;
    DrawCircleLinesV(rankCenter, orbitRadius, WithAlpha(Color{ 230, 230, 238, 255 }, 105.0f + glow * 65.0f));
    DrawCircleLinesV(rankCenter, orbitRadius * 0.82f, WithAlpha(Color{ 145, 145, 155, 255 }, 70.0f));
    for (int tick = 0; tick < 48; ++tick)
    {
        const float angle = (static_cast<float>(tick) * 7.5f + orbitRotation) * DEG2RAD;
        const float innerRadius = orbitRadius * (tick % 4 == 0 ? 0.87f : 0.93f);
        const Vector2 from{ rankCenter.x + std::cos(angle) * innerRadius, rankCenter.y + std::sin(angle) * innerRadius };
        const Vector2 to{ rankCenter.x + std::cos(angle) * orbitRadius, rankCenter.y + std::sin(angle) * orbitRadius };
        DrawLineEx(from, to, tick % 4 == 0 ? 1.5f * scale : 0.75f * scale, WithAlpha(Color{ 230, 230, 238, 255 }, tick % 4 == 0 ? 210.0f : 100.0f));
    }
    const char* orbitGrades[] = { "F", "D", "C", "B", "A", "S" };
    for (int i = 0; i < 6; ++i)
    {
        const float angle = (static_cast<float>(i) * 60.0f + orbitRotation - 90.0f) * DEG2RAD;
        const float labelRadius = orbitRadius + 6.0f * scale;
        const float labelX = rankCenter.x + std::cos(angle) * labelRadius;
        const float labelY = rankCenter.y + std::sin(angle) * labelRadius - 6.0f * scale;
        const float labelAlpha = (i == 5) ? 255.0f : 145.0f;
        DrawTextCentered(font, orbitGrades[i], labelX, labelY, 10.0f * scale, 0.0f, WithAlpha(Color{ 245, 245, 250, 255 }, labelAlpha));
    }
    DrawCircleV(
        rankCenter,
        gradeSize * (0.47f + glow * 0.018f),
        WithAlpha(Color{ 245, 245, 250, 255 }, 12.0f + glow * 14.0f)
    );
    DrawPanel(
        { gradeX, gradeY, gradeSize, gradeSize },
        Color{ 25, 25, 29, 255 },
        WithAlpha(gradeColor, 180.0f + glow * 75.0f),
        0.16f
    );

    Vector2 gradeTextSize = MeasureTextEx(font, result.grade.c_str(), 62.0f * scale, 1.0f * scale);
    DrawTextEx(
        font,
        result.grade.c_str(),
        { gradeX + (gradeSize - gradeTextSize.x) * 0.5f, gradeY + (gradeSize - gradeTextSize.y) * 0.43f },
        62.0f * scale,
        1.0f * scale,
        gradeColor
    );
    DrawTextCentered(font, "RANK", gradeX + gradeSize * 0.5f, gradeY + gradeSize * 0.73f, 9.0f * scale, 2.0f * scale, Color{ 180, 180, 190, 255 });

    const float sectionGap = 12.0f * scale;
    const float topCardH = std::max(108.0f * scale, contentH * 0.23f);
    const float cardW = (rightW - sectionGap * 2.0f) / 3.0f;

    Rectangle scoreCard{ rightX, contentY, cardW, topCardH };
    Rectangle accCard{ rightX + cardW + sectionGap, contentY, cardW, topCardH };
    Rectangle comboCard{ rightX + (cardW + sectionGap) * 2.0f, contentY, cardW, topCardH };

    DrawPanel(scoreCard, Color{ 22, 22, 25, 255 }, Color{ 120, 120, 130, 220 }, 0.07f);
    DrawPanel(accCard, Color{ 22, 22, 25, 255 }, Color{ 145, 145, 153, 220 }, 0.07f);
    DrawPanel(comboCard, Color{ 22, 22, 25, 255 }, Color{ 100, 100, 110, 220 }, 0.07f);

    DrawTextEx(font, "TOTAL SCORE", { scoreCard.x + 14.0f * scale, scoreCard.y + 13.0f * scale }, 10.0f * scale, 1.2f * scale, Color{ 195, 195, 204, 255 });
    char scoreBuf[64];
    std::snprintf(scoreBuf, sizeof(scoreBuf), "%d", result.score);
    float scoreFont = 27.0f * scale;
    while (scoreFont > 13.0f * scale && MeasureTextEx(font, scoreBuf, scoreFont, 0.0f).x > scoreCard.width - 24.0f * scale)
        scoreFont -= 1.0f * scale;
    DrawTextEx(font, scoreBuf, { scoreCard.x + 14.0f * scale, scoreCard.y + 38.0f * scale }, scoreFont, 0.0f, RAYWHITE);
    DrawRectangleRounded(
        { scoreCard.x + 14.0f * scale, scoreCard.y + scoreCard.height - 17.0f * scale, scoreCard.width - 28.0f * scale, 3.0f * scale },
        0.5f,
        6,
        Color{ 55, 55, 62, 255 }
    );
    DrawRectangleRounded(
        { scoreCard.x + 14.0f * scale, scoreCard.y + scoreCard.height - 17.0f * scale, (scoreCard.width - 28.0f * scale) * std::clamp(result.score / 1000000.0f, 0.0f, 1.0f), 3.0f * scale },
        0.5f,
        6,
        Color{ 235, 235, 242, 255 }
    );

    DrawTextEx(font, "ACCURACY", { accCard.x + 14.0f * scale, accCard.y + 13.0f * scale }, 10.0f * scale, 1.2f * scale, Color{ 190, 190, 200, 255 });
    char accBuf[64];
    std::snprintf(accBuf, sizeof(accBuf), "%.2f%%", result.accuracy);
    float accFont = 27.0f * scale;
    while (accFont > 13.0f * scale && MeasureTextEx(font, accBuf, accFont, 0.0f).x > accCard.width - 24.0f * scale)
        accFont -= 1.0f * scale;
    DrawTextEx(font, accBuf, { accCard.x + 14.0f * scale, accCard.y + 38.0f * scale }, accFont, 0.0f, RAYWHITE);
    DrawRectangleRounded(
        { accCard.x + 14.0f * scale, accCard.y + accCard.height - 17.0f * scale, accCard.width - 28.0f * scale, 3.0f * scale },
        0.5f,
        6,
        Color{ 55, 55, 62, 255 }
    );
    DrawRectangleRounded(
        { accCard.x + 14.0f * scale, accCard.y + accCard.height - 17.0f * scale, (accCard.width - 28.0f * scale) * std::clamp(result.accuracy / 100.0f, 0.0f, 1.0f), 3.0f * scale },
        0.5f,
        6,
        Color{ 220, 220, 228, 255 }
    );

    DrawTextEx(font, "MAX COMBO", { comboCard.x + 14.0f * scale, comboCard.y + 13.0f * scale }, 10.0f * scale, 1.2f * scale, Color{ 190, 190, 200, 255 });
    char comboBuf[64];
    std::snprintf(comboBuf, sizeof(comboBuf), "%d", result.maxCombo);
    float comboFont = 29.0f * scale;
    while (comboFont > 13.0f * scale && MeasureTextEx(font, comboBuf, comboFont, 0.0f).x > comboCard.width - 24.0f * scale)
        comboFont -= 1.0f * scale;
    DrawTextEx(font, comboBuf, { comboCard.x + 14.0f * scale, comboCard.y + 38.0f * scale }, comboFont, 0.0f, RAYWHITE);
    DrawTextEx(font, "COMBO", { comboCard.x + 14.0f * scale, comboCard.y + comboCard.height - 25.0f * scale }, 9.0f * scale, 1.0f * scale, Color{ 150, 150, 160, 255 });

    const float judgeY = contentY + topCardH + sectionGap;
    const float judgeH = contentH - topCardH - sectionGap - 52.0f * scale;
    DrawPanel(
        { rightX, judgeY, rightW, judgeH },
        Color{ 17, 22, 36, 255 },
        Color{ 70, 70, 78, 220 },
        0.04f
    );

    DrawTextEx(font, "JUDGEMENT", { rightX + 16.0f * scale, judgeY + 13.0f * scale }, 14.0f * scale, 1.4f * scale, RAYWHITE);
    DrawTextEx(font, "HIT DISTRIBUTION", { rightX + rightW - 150.0f * scale, judgeY + 16.0f * scale }, 9.0f * scale, 1.0f * scale, Color{ 150, 150, 160, 255 });

    struct JudgementItem
    {
        const char* name;
        int count;
        Color color;
    };

    JudgementItem judgements[] = {
        { "PERFECT", result.perfectCount, Color{ 255, 255, 255, 255 } },
        { "GREAT", result.greatCount, Color{ 220, 220, 226, 255 } },
        { "GOOD", result.goodCount, Color{ 185, 185, 194, 255 } },
        { "MISS", result.missCount, Color{ 245, 245, 248, 255 } },
        { "BREAK", result.breakCount, Color{ 205, 205, 213, 255 } },
        { "FAST", result.fastCount, Color{ 165, 165, 176, 255 } },
        { "SLOW", result.slowCount, Color{ 135, 135, 148, 255 } }
    };

    int totalJudgements = 0;
    for (const auto& item : judgements)
        totalJudgements += std::max(0, item.count);

    const float gridX = rightX + 16.0f * scale;
    const float gridY = judgeY + 43.0f * scale;
    const float gridGap = 10.0f * scale;
    const float itemW = (rightW - 42.0f * scale) / 2.0f;
    const float itemH = std::max(27.0f * scale, (judgeH - 56.0f * scale) / 4.0f);

    for (int i = 0; i < 7; ++i)
    {
        const int column = i % 2;
        const int row = i / 2;
        const float x = gridX + column * (itemW + gridGap);
        const float y = gridY + row * itemH;
        const float countWidth = 44.0f * scale;

        DrawRectangleRounded(
            { x, y, itemW, itemH - 4.0f * scale },
            0.12f,
            6,
            Color{ 25, 25, 29, 255 }
        );
        DrawRectangleRounded(
            { x, y + 2.0f * scale, 3.0f * scale, itemH - 8.0f * scale },
            0.5f,
            4,
            judgements[i].color
        );

        DrawTextEx(
            font,
            judgements[i].name,
            { x + 11.0f * scale, y + 5.0f * scale },
            11.0f * scale,
            0.6f * scale,
            judgements[i].color
        );

        char countBuf[24];
        std::snprintf(countBuf, sizeof(countBuf), "%d", judgements[i].count);
        Vector2 countSize = MeasureTextEx(font, countBuf, 14.0f * scale, 0.0f);
        DrawTextEx(
            font,
            countBuf,
            { x + itemW - countWidth + (countWidth - countSize.x) * 0.5f, y + 3.0f * scale },
            14.0f * scale,
            0.0f,
            RAYWHITE
        );

        const float barX = x + 11.0f * scale;
        const float barY = y + itemH - 10.0f * scale;
        const float barW = std::max(1.0f, itemW - 22.0f * scale);
        const float ratio = totalJudgements > 0 ? static_cast<float>(std::max(0, judgements[i].count)) / static_cast<float>(totalJudgements) : 0.0f;

        DrawRectangleRounded(
            { barX, barY, barW, 2.5f * scale },
            0.5f,
            5,
            Color{ 52, 52, 60, 255 }
        );

        if (ratio > 0.0f)
        {
            DrawRectangleRounded(
                { barX, barY, std::max(1.0f, barW * ratio), 2.5f * scale },
                0.5f,
                5,
                judgements[i].color
            );
        }
    }

    const float footerY = panelY + panelH - 39.0f * scale;
    DrawRectangle(
        static_cast<int>(rightX),
        static_cast<int>(footerY - 8.0f * scale),
        static_cast<int>(rightW),
        1,
        Color{ 65, 65, 72, 180 }
    );

    DrawTextEx(font, "SETA  /  RHYTHM GAME", { rightX, footerY }, 9.0f * scale, 1.1f * scale, Color{ 135, 135, 145, 255 });
    const char* hint = "곡 선택으로";
    Vector2 hintSize = MeasureTextEx(font, hint, 12.0f * scale, 0.5f * scale);
    DrawRectangleRounded(
        { panelX + panelW - hintSize.x - 48.0f * scale, footerY - 7.0f * scale, hintSize.x + 30.0f * scale, 27.0f * scale },
        0.18f,
        8,
        Color{ 35, 35, 40, 255 }
    );
    DrawRectangleRoundedLinesEx(
        { panelX + panelW - hintSize.x - 48.0f * scale, footerY - 7.0f * scale, hintSize.x + 30.0f * scale, 27.0f * scale },
        0.18f,
        8,
        1.0f * scale,
        Color{ 180, 180, 190, 190 }
    );
    DrawTextEx(
        font,
        hint,
        { panelX + panelW - hintSize.x - 33.0f * scale, footerY },
        12.0f * scale,
        0.5f * scale,
        Color{ 235, 235, 240, 255 }
    );
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
