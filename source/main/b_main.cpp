#include "raylib.h"
#include "../Menu/main_Menu.h"
#include "../data/main_data.h"
#include "../play/play_scene.h"

int main() {
    const int screenWidth = 1280;
    const int screenHeight = 720;
    
    InitWindow(screenWidth, screenHeight, "The Line");
    InitAudioDevice();
    SetTargetFPS(60);

    GameState currentState = STATE_MENU;
    
    MainMenu mainMenu;
    PlayScene playScene;
    playScene.Init();

    while (!WindowShouldClose()) {
        if (currentState == STATE_MENU) {
            mainMenu.Update();

            if (mainMenu.IsGameStartSelected()) {
                currentState = STATE_PLAYING;
                playScene.Init();
            } else if (mainMenu.IsExitSelected()) {
                break;
            }
        } else if (currentState == STATE_PLAYING) {
            playScene.Update();

            if (IsKeyPressed(KEY_ESCAPE)) {
                currentState = STATE_MENU;
                mainMenu.Reset();
            }
        }

        BeginDrawing();
        ClearBackground((Color){10, 12, 18, 255});

        if (currentState == STATE_MENU) {
            mainMenu.Draw(screenWidth, screenHeight);
        } else if (currentState == STATE_PLAYING) {
            playScene.Draw();
        }

        EndDrawing();
    }

    playScene.Unload();
    CloseAudioDevice();
    CloseWindow();

    return 0;
}