#ifndef MENU_BACKGROUND_H
#define MENU_BACKGROUND_H

#include <raylib.h>

class MenuBackground {
public:
    static void Init();
    static void Draw(int screenWidth, int screenHeight);
    static void Unload();

private:
    static Texture2D texture;
    static bool isLoaded;
};

#endif