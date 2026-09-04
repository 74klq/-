#include "note.h"

Note::Note(float startX, float startY, float moveSpeed, int noteLane) {
    x = startX;
    y = startY;
    speed = moveSpeed;
    active = true;
    lane = noteLane;
}

void Note::Update() {
    y += speed;

    if (y > 900.0f) {
        active = false;
    }
}

void Note::Draw() { // 인자 제거
    if (active) {
        DrawRectangle((int)(x - 30.0f), (int)(y - 7.5f), 60, 15, WHITE);
    }
}