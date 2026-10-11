#ifndef CURSOR_MOVE_H
#define CURSOR_MOVE_H

#include <raylib.h>

struct ParallaxTransform {
    Vector2 offset;
    float scale;
    float rotation;
};

class CursorMove {
public:
    static ParallaxTransform GetTransform(float depth = 1.0f, float intensity = 0.05f);
};

#endif