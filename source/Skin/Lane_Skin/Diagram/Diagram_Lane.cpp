#include "Diagram_Lane.h"

namespace Skin {
namespace Lane {
namespace Diagram {

void DrawLaneLines(float judgmentLineY)
{
    const int laneCount = 4;
    const float baseWidth = 400.0f;
    const float startX = ((float)GetScreenWidth() - baseWidth) * 0.5f;
    const float laneWidth = baseWidth / 4.0f;

    for (int i = 0; i < laneCount; ++i)
    {
        float x = startX + i * laneWidth;
        DrawRectangle(static_cast<int>(x) + 1, 0, static_cast<int>(laneWidth) - 2, static_cast<int>(judgmentLineY), Color{ 255, 255, 255, 10 });
        DrawRectangle(static_cast<int>(x), 0, 1, static_cast<int>(judgmentLineY), Color{ 255, 255, 255, 50 });
    }
    
    DrawRectangle(static_cast<int>(startX + baseWidth), 0, 1, static_cast<int>(judgmentLineY), Color{ 255, 255, 255, 50 });
}

}
}
}