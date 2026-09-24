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

    int type;
    int time;
    int endTime;

     Note(float startX, float startY, float moveSpeed, int noteLane, int noteType, int startTime, int endNoteTime);
    void Update();
    void Draw();
};

#endif