#ifndef NOTE_DOWN_ANIMATION_H
#define NOTE_DOWN_ANIMATION_H

#include "raylib.h"

class NoteDownAnimation {
public:
    static void DrawCoolNote(float x, float y, float noteWidth, float noteHeight, float timeRemaining, float totalLifeTime);
};

#endif