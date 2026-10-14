#ifndef DIAGRAM_HOLD_NOTE_H
#define DIAGRAM_HOLD_NOTE_H

#include "raylib.h"

namespace Skin {
namespace Note {
namespace HoldNote {
namespace Diagram {

void DrawHoldNote(float cx, float headY, float tailY, float w, float uiScale, float time, int lane, bool headHit, bool isHolding);

}
}
}
}

#endif