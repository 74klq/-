#include "note.h"

Note::Note(float startX, float startY, float moveSpeed, int noteLane, int noteType, int startTime, int endNoteTime) {
    x = startX;
    y = startY;
    speed = moveSpeed;
    active = true;
    lane = noteLane;
    
    type = noteType;
    time = startTime;
    endTime = endNoteTime;
}

void Note::Update() {
    y += speed;

    float noteLength = (float)(endTime - time) * (speed / 16.6667f);
    float checkY = y;
    
    if (type == 128) {
        checkY = y - noteLength;
    }

    if (checkY > 900.0f) {
        active = false;
    }
}

void Note::Draw() {
    if (!active) return;

    if (type == 128) {
        float noteLength = (float)(endTime - time) * (speed / 16.6667f);

        DrawRectangle((int)(x - 30.0f), (int)(y - noteLength), 60, (int)noteLength, LIGHTGRAY);
        DrawRectangle((int)(x - 30.0f), (int)(y - 7.5f), 60, 15, WHITE);
        DrawRectangle((int)(x - 30.0f), (int)(y - noteLength - 7.5f), 60, 15, GRAY);
    } 
    else {
        DrawRectangle((int)(x - 30.0f), (int)(y - 7.5f), 60, 15, WHITE);
    }
}
