#include "Diagram_Short_Note.h"
#include <cmath>

namespace Skin {
namespace Note {
namespace ShortNote {
namespace Diagram {

void DrawShortNote(float cx, float y, float w, float h, float uiScale, float time, int lane)
{
    float pulse = 0.5f + 0.5f * sinf(time * 9.0f + lane);

    DrawRectangleRounded({ cx - w * 0.5f + 4.0f * uiScale, y - h * 0.5f + 7.0f * uiScale, w, h }, 0.18f, 8, Color{ 0, 0, 0, 245 });
    DrawRectangleRounded({ cx - w * 0.5f, y - h * 0.5f, w, h }, 0.18f, 8, Color{ 218, 218, 221, 255 });
    DrawRectangleRounded({ cx - w * 0.5f + 2.0f * uiScale, y - h * 0.5f + 2.0f * uiScale, w - 4.0f * uiScale, h * 0.42f }, 0.16f, 8, Color{ 255, 255, 255, 255 });
    DrawRectangleRounded({ cx - w * 0.5f + 5.0f * uiScale, y - 2.0f * uiScale, w - 10.0f * uiScale, 4.0f * uiScale }, 0.35f, 8, Color{ 22, 22, 24, 255 });
    DrawRectangleRoundedLines({ cx - w * 0.5f, y - h * 0.5f, w, h }, 0.18f, 8, Color{ 255, 255, 255, 240 });
}

}
}
}
}