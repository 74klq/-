#ifndef NOTE_H
#define NOTE_H

#include "raylib.h"

class Note {
public:
    float x;
    float y;
    float speed;
    bool active;
    int lane;

    Note(float startX, float startY, float moveSpeed, int noteLane);
    void Update();
    void Draw(); // 인자 제거
};

#endif