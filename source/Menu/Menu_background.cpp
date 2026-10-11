#include "Menu_background.h"
#include "cursor_move.h"

Texture2D MenuBackground::texture = { 0 };
bool MenuBackground::isLoaded = false;

void MenuBackground::Init() {
    if (!isLoaded) {
        if (FileExists("assets/lobby.png")) {
            texture = LoadTexture("assets/lobby.png");
            if (texture.id != 0) {
                SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
            }
        }
        isLoaded = true;
    }
}

void MenuBackground::Draw(int screenWidth, int screenHeight) {
    Init();
    if (texture.id != 0) {
        ParallaxTransform t = CursorMove::GetTransform(1.3f, 0.12f);

        Rectangle src = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };

        float scaledWidth = (float)screenWidth * t.scale;
        float scaledHeight = (float)screenHeight * t.scale;

        Rectangle dest = {
            (float)screenWidth * 0.5f + t.offset.x,
            (float)screenHeight * 0.5f + t.offset.y,
            scaledWidth,
            scaledHeight
        };

        Vector2 origin = { scaledWidth * 0.5f, scaledHeight * 0.5f };

        DrawTexturePro(texture, src, dest, origin, t.rotation, Color{ 255, 255, 255, 70 });
    }
}

void MenuBackground::Unload() {
    if (isLoaded && texture.id != 0) {
        UnloadTexture(texture);
        texture = { 0 };
        isLoaded = false;
    }
}