#include "raylib.h"
#include "../Menu/main_Menu.h"
#include "../data/main_data.h"
#include "../play/play_scene.h"

int main() {
    const int screenWidth = 1280;
    const int screenHeight = 720;
    
    InitWindow(screenWidth, screenHeight, "R.A");
    InitAudioDevice();
    SetConfigFlags(FLAG_VSYNC_HINT); 
    SetTargetFPS(0); 

    GameState currentState = STATE_MENU;
    
    MainMenu mainMenu;
    PlayScene playScene(mainMenu.GetSongSelect()); 
    playScene.Init();

    while (!WindowShouldClose()) {
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
                mainMenu.Draw(screenWidth, screenHeight);
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
