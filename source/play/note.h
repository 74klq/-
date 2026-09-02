#ifndef NOTE_H
#define NOTE_H

#include "raylib.h"

class Note {
public:
    float x;
    float y;
    float speed;
    bool active;

    Note(float startX, float startY, float moveSpeed);
    void Update();
    void Draw(Texture2D noteTex);
};

#endif