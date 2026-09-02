#ifndef MAIN_MENU_H
#define MAIN_MENU_H

#include "raylib.h"
#include "background_rotation.h"

class MainMenu {
private:
    BackgroundRotation bgRotation;
    int selectedIndex;
    float pulseTimer;
    const int totalOptions = 3;

public:
     MainMenu();
     void Update();
     void Draw(int screenWidth, int screenHeight);
     bool IsGameStartSelected() const;
     bool IsExitSelected() const;
     void Reset();
};

#endif