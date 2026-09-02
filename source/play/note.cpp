#include "note.h"

Note::Note(float startX, float startY, float moveSpeed) {
    x = startX;
    y = startY;
    speed = moveSpeed;
    active = true;
}

void Note::Update() {
    x -= speed;

    if (x < -100.0f) {
        active = false;
    }
}

void Note::Draw(Texture2D noteTex) {
    if (active) {
        float drawX = x - (noteTex.width / 2.0f);
        float drawY = y - (noteTex.height / 2.0f);

        DrawTexture(noteTex, (int)drawX, (int)drawY, WHITE);
    }
}