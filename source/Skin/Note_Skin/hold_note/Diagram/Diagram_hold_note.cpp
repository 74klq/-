#include "Diagram_Hold_Note.h"
#include <cmath>

namespace Skin {
namespace Note {
namespace HoldNote {
namespace Diagram {

void DrawHoldNote(float cx, float headY, float tailY, float w, float uiScale, float time, int lane, bool headHit, bool isHolding)
{
    float bodyHeight = headY - tailY;
    if (bodyHeight < 0.0f) bodyHeight = 0.0f;

    float bodyW = w * 0.6f;
    DrawRectangle(static_cast<int>(cx - bodyW * 0.5f), static_cast<int>(tailY), static_cast<int>(bodyW), static_cast<int>(bodyHeight), Color{ 218, 218, 221, 180 });
    DrawRectangle(static_cast<int>(cx - bodyW * 0.2f), static_cast<int>(tailY), static_cast<int>(bodyW * 0.4f), static_cast<int>(bodyHeight), Color{ 255, 255, 255, 220 });

    float h = 27.0f * uiScale;
    DrawRectangleRounded({ cx - w * 0.5f + 4.0f * uiScale, headY - h * 0.5f + 7.0f * uiScale, w, h }, 0.18f, 8, Color{ 0, 0, 0, 245 });
    DrawRectangleRounded({ cx - w * 0.5f, headY - h * 0.5f, w, h }, 0.18f, 8, Color{ 218, 218, 221, 255 });
    DrawRectangleRounded({ cx - w * 0.5f + 2.0f * uiScale, headY - h * 0.5f + 2.0f * uiScale, w - 4.0f * uiScale, h * 0.42f }, 0.16f, 8, Color{ 255, 255, 255, 255 });
    DrawRectangleRoundedLines({ cx - w * 0.5f, headY - h * 0.5f, w, h }, 0.18f, 8, Color{ 255, 255, 255, 240 });
}

}
}
}
}