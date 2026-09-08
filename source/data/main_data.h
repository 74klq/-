#pragma once
#include "raylib.h"
#include <vector>

enum GameState {
    STATE_MENU,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_GAMEVOER,
    STATE_RESULT,
    STATE_DEAD
}; // 게임 상태를 열거형으로 정리

struct Note {
    float time;
    int lane;
    bool hit;
    bool active;
}; // 0: 좌클, 1: 우클

// 좆같다 씨발 
// 언제 다해

enum JudgeResult {
    JUDGE_NONE,
    JUDGE_PERFECT,
    JUDGE_GOOD,
    JUDGE_MISS
};

struct PlayerStats {
    int score;
    int combo;
    int maxCombo;
    float health;
    int goodCount;
    int missCount;
}; // 플레이어 게임에서 스탯이랑 그 뭐시깽이 관리하는 구조체