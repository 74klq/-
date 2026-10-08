#include "raylib.h"
#include "../Menu/main_Menu.h"
#include "../data/main_data.h"
#include "../play/play_scene.h"

int main() {
    SetConfigFlags(FLAG_FULLSCREEN_MODE | FLAG_VSYNC_HINT);
    
    InitWindow(1280, 720, "R.A");
    InitAudioDevice();
    SetTargetFPS(0); 

    int currentWidth = GetScreenWidth();
    int currentHeight = GetScreenHeight();

    GameState currentState = STATE_MENU;
    
    MainMenu mainMenu;
    PlayScene playScene(mainMenu.GetSongSelect()); 
    playScene.Init();

    while (!WindowShouldClose()) {
        currentWidth = GetScreenWidth();
        currentHeight = GetScreenHeight();

        if (currentState == STATE_MENU) {
            mainMenu.Update();

            if (IsKeyPressed(KEY_P)) {
                currentState = STATE_PLAYING;
                playScene.Init();
            }
            else if (mainMenu.IsGameStartSelected()) {
                 int chosenSong = mainMenu.GetSongSelect().GetSelectedSongIndex(); 
                
                currentState = STATE_PLAYING;
                playScene.Init(chosenSong);
                
                BeginDrawing();
                ClearBackground((Color){10, 12, 18, 255});
                mainMenu.Draw(currentWidth, currentHeight);
                EndDrawing();
                continue; 
            } else if (mainMenu.IsExitSelected()) {
                break;
            }
        } 
        else if (currentState == STATE_PLAYING) {
            playScene.Update();

            if (IsKeyPressed(KEY_ESCAPE) || playScene.ShouldGoBackToMenu()) {
                currentState = STATE_MENU;
                mainMenu.Reset();
                playScene.ResetBackToMenuFlag(); 
            }
        }

        BeginDrawing();
        ClearBackground((Color){10, 12, 18, 255});

        if (currentState == STATE_MENU) {
            mainMenu.Draw(currentWidth, currentHeight);
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
