#include "Diagram_Judgment.h"
#include <cmath>

extern float GetLaneXCoord(int lane);

namespace Skin {
namespace Judgment {
namespace Diagram {

static float Clamp01(float value)
{
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
}

void DrawJudgmentLine(float judgmentLineY, float judgmentLinePulse)
{
    const float t = GetTime();
    const float impact = Clamp01(judgmentLinePulse);
    const float pulse = 0.5f + 0.5f * sinf(t * 3.2f);
    const int y = static_cast<int>(judgmentLineY);

    float screenWidth = static_cast<float>(GetScreenWidth());
    float playfieldWidth = 400.0f;
    float playfieldX = (screenWidth - playfieldWidth) * 0.5f;
    int laneCount = 4;
    float laneStartX = playfieldX;
    float laneAreaWidth = playfieldWidth;

    const float left = laneStartX - 22.0f;
    const float right = laneStartX + laneAreaWidth + 22.0f;

    DrawRectangle(static_cast<int>(left) - 14, y - 18, static_cast<int>(right - left) + 28, 36, Color{ 255, 255, 255, static_cast<unsigned char>(7 + impact * 20) });
    DrawRectangle(static_cast<int>(left) - 6, y - 10, static_cast<int>(right - left) + 12, 20, Color{ 255, 255, 255, static_cast<unsigned char>(12 + impact * 30) });

    DrawRectangle(static_cast<int>(left), y - 6, static_cast<int>(right - left), 12, Color{ 4, 4, 5, 245 });
    DrawRectangle(static_cast<int>(left), y - 4, static_cast<int>(right - left), 8, Color{ 110, 110, 112, 230 });
    DrawRectangle(static_cast<int>(left), y - 2, static_cast<int>(right - left), 4, Color{ 238, 238, 240, 255 });
    DrawRectangle(static_cast<int>(left), y - 1, static_cast<int>(right - left), 2, Color{ 255, 255, 255, 255 });

    for (int i = 0; i < laneCount; ++i)
    {
        const float x = GetLaneXCoord(i);
        const bool hitFlash = impact > 0.01f;
        const float r = 13.0f + impact * 12.0f;

        DrawCircle(static_cast<int>(x), y, r + 9.0f, Color{ 255, 255, 255, static_cast<unsigned char>(4 + impact * 18) });
        DrawCircle(static_cast<int>(x), y, r, Color{ 3, 3, 4, 245 });
        DrawCircleLines(static_cast<int>(x), y, r + 4.0f, Color{ 245, 245, 245, static_cast<unsigned char>(115 + impact * 120) });
        DrawCircleLines(static_cast<int>(x), y, r + 8.0f + pulse * 2.0f, Color{ 255, 255, 255, static_cast<unsigned char>(20 + pulse * 25) });

        DrawLine(static_cast<int>(x) - 26, y, static_cast<int>(x) - 17, y, Color{ 255, 255, 255, 175 });
        DrawLine(static_cast<int>(x) + 17, y, static_cast<int>(x) + 26, y, Color{ 255, 255, 255, 175 });

        if (hitFlash)
        {
            DrawCircle(static_cast<int>(x), y, r * 0.45f, Color{ 255, 255, 255, static_cast<unsigned char>(20 + impact * 80) });
        }
    }

    DrawRectangle(static_cast<int>(left) - 34, y - 1, 24, 2, Color{ 255, 255, 255, static_cast<unsigned char>(80 + pulse * 100) });
    DrawRectangle(static_cast<int>(right) + 10, y - 1, 24, 2, Color{ 255, 255, 255, static_cast<unsigned char>(80 + pulse * 100) });

    const float cx = playfieldX + playfieldWidth * 0.5f;
    const float archY = judgmentLineY + 92.0f;

    DrawRing({ cx, archY }, 86.0f, 100.0f, 205.0f, 335.0f, 64, Color{ 0, 0, 0, 250 });
    DrawRing({ cx, archY }, 100.0f, 104.0f, 205.0f, 335.0f, 64, Color{ 90, 90, 92, 210 });
    DrawRing({ cx, archY }, 108.0f, 112.0f, 208.0f, 332.0f, 64, Color{ 235, 235, 238, 145 });

    DrawRectangle(static_cast<int>(cx - 132), static_cast<int>(judgmentLineY + 54), 264, 3, Color{ 5, 5, 6, 245 });
    DrawRectangle(static_cast<int>(cx - 116), static_cast<int>(judgmentLineY + 57), 232, 2, Color{ 115, 115, 118, 130 });

    for (int i = 0; i < 9; ++i)
    {
        const float x = cx - 104.0f + i * 26.0f;
        const float h = 8.0f + 5.0f * (0.5f + 0.5f * sinf(t * 2.0f + i));
        DrawRectangle(static_cast<int>(x), static_cast<int>(judgmentLineY + 42), 10, static_cast<int>(h), Color{ 190, 190, 193, static_cast<unsigned char>(45 + i * 5) });
    }
}

}
}
}