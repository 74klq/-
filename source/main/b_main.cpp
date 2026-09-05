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
    // 참고: 프로그램 시작 시의 Init()은 최초 1회 빈 상태 방지용입니다.
    playScene.Init();

    while (!WindowShouldClose()) {
        if (currentState == STATE_MENU) {
            mainMenu.Update();

            if (mainMenu.IsGameStartSelected()) {
                currentState = STATE_PLAYING;
                playScene.Init(); // <--- 메인 메뉴에서 넘어올 때 PlayScene(및 내부 곡 선택창)을 리셋/초기화합니다.
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