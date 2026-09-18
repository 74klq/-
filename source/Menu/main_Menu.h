#ifndef MAIN_MENU_H
#define MAIN_MENU_H

#include "raylib.h"
#include "background_rotation.h"
#include "Map_Selection/Selection.h"

enum class MenuState {
    Main,
    SongSelect
};

class MainMenu {
private:
    BackgroundRotation bgRotation;
    int selectedIndex;
    float pulseTimer;
    const int totalOptions = 3;

    MenuState currentState;
    SongSelect songSelect;

public:
     MainMenu();
     void Update();
     void Draw(int screenWidth, int screenHeight);
     bool IsGameStartSelected() const;
     bool IsExitSelected() const;
     void Reset();
};

#endif