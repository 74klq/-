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

            // 🎯 곡 선택 화면이나 메뉴 상태일 때 P키를 누르면 무조건 에디터 모드로 강제 실행!
            if (IsKeyPressed(KEY_P)) {
                currentState = STATE_PLAYING;
                playScene.Init(); // 플레이 씬을 초기화하여 인게임 화면으로 진입할 준비를 합니다.
            }
            else if (mainMenu.IsGameStartSelected()) {
                currentState = STATE_PLAYING;
                playScene.Init();
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
