#include "Selection.h"
#include <cmath>
#include <algorithm>

SongSelect::SongSelect()
    : selectedSongIndex(0), selectedDiffIndex(0), 
      animScrollOffset(0.0f), targetScrollOffset(0.0f), 
      bgTransitionAlpha(1.0f), previousSongIndex(0), fontLoaded(false) {}

SongSelect::~SongSelect() {
    if (fontLoaded) {
        UnloadFont(suitFont);
    }
}

void SongSelect::Init() {
    LoadSongs();
    
    if (!fontLoaded) {
        suitFont = LoadFontEx("fonts/SUIT-Light.ttf", 32, 0, 0);
        if (suitFont.texture.id != 0) {
            SetTextureFilter(suitFont.texture, TEXTURE_FILTER_BILINEAR);
            fontLoaded = true;
        }
    }
    
    selectedSongIndex = 0;
    selectedDiffIndex = 0;
    animScrollOffset = 0.0f;
    targetScrollOffset = 0.0f;
}

void SongSelect::LoadSongs() {
    songs.clear();

    SongData song1;
    song1.title = "A Night Without Visible Stars";
    song1.artist = "Four Beat Sounds";
    song1.mapper = "Mapper_A";
    song1.length = 152.0f;
    song1.cleared = true;
    song1.difficulties = {
        {"EASY", 3, 140.0f, 210, 450, 2.5f},
        {"NORMAL", 7, 140.0f, 430, 890, 5.2f},
        {"HARD", 12, 140.0f, 780, 1520, 8.4f},
        {"EXPERT", 16, 140.0f, 1150, 2300, 10.8f}
    };
    songs.push_back(song1);

    SongData song2;
    song2.title = "별이 보이지 않는 밤";
    song2.artist = "Plum";
    song2.mapper = "boyangsic";
    song2.length = 135.0f;
    song2.cleared = false;
    song2.difficulties = {
        {"NORMAL", 5, 128.0f, 320, 640, 4.1f},
        {"HARD", 10, 128.0f, 650, 1300, 7.5f},
        {"EXPERT", 15, 128.0f, 1020, 2100, 10.2f}
    };
    songs.push_back(song2);

    SongData song3;
    song3.title = "Cybernetic Dream";
    song3.artist = "Neo Synth";
    song3.mapper = "Mapper_C";
    song3.length = 168.0f;
    song3.cleared = true;
    song3.difficulties = {
        {"EASY", 2, 155.0f, 180, 390, 1.9f},
        {"NORMAL", 6, 155.0f, 390, 810, 4.8f},
        {"HARD", 11, 155.0f, 720, 1450, 8.0f},
        {"EXPERT", 17, 155.0f, 1300, 2650, 11.5f},
        {"MASTER", 20, 155.0f, 1750, 3500, 13.2f}
    };
    songs.push_back(song3);
}

void SongSelect::Update() {
    float dt = GetFrameTime();

    animScrollOffset += (targetScrollOffset - animScrollOffset) * (dt * 12.0f);

    if (bgTransitionAlpha < 1.0f) {
        bgTransitionAlpha += dt * 4.0f;
        if (bgTransitionAlpha > 1.0f) bgTransitionAlpha = 1.0f;
    }

    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        if (selectedSongIndex > 0) {
            previousSongIndex = selectedSongIndex;
            selectedSongIndex--;
            targetScrollOffset = (float)selectedSongIndex * 95.0f;
            bgTransitionAlpha = 0.0f;
            selectedDiffIndex = std::min(selectedDiffIndex, (int)songs[selectedSongIndex].difficulties.size() - 1);
        }
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        if (selectedSongIndex < (int)songs.size() - 1) {
            previousSongIndex = selectedSongIndex;
            selectedSongIndex++;
            targetScrollOffset = (float)selectedSongIndex * 95.0f;
            bgTransitionAlpha = 0.0f;
            selectedDiffIndex = std::min(selectedDiffIndex, (int)songs[selectedSongIndex].difficulties.size() - 1);
        }
    }

    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        if (selectedDiffIndex > 0) {
            selectedDiffIndex--;
        }
    }
    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        if (selectedDiffIndex < (int)songs[selectedSongIndex].difficulties.size() - 1) {
            selectedDiffIndex++;
        }
    }

    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        selectedSongIndex -= (int)wheel;
        if (selectedSongIndex < 0) selectedSongIndex = 0;
        if (selectedSongIndex >= (int)songs.size()) selectedSongIndex = (int)songs.size() - 1;
        targetScrollOffset = (float)selectedSongIndex * 95.0f;
        selectedDiffIndex = std::min(selectedDiffIndex, (int)songs[selectedSongIndex].difficulties.size() - 1);
    }
}

void SongSelect::Draw(int screenWidth, int screenHeight) {
    DrawRectangle(0, 0, screenWidth, screenHeight, (Color){ 12, 14, 20, 255 });
    DrawRectangleGradientV(0, 0, screenWidth, screenHeight, (Color){ 15, 20, 32, 220 }, (Color){ 6, 8, 14, 250 });

    float listX = 60.0f;
    float listY = 120.0f;
    float listWidth = 440.0f;
    float listHeight = 520.0f;

    DrawRectangleRounded((Rectangle){ listX, listY, listWidth, listHeight }, 0.04f, 4, (Color){ 20, 24, 35, 180 });
    DrawRectangleRoundedLines((Rectangle){ listX, listY, listWidth, listHeight }, 0.04f, 4, (Color){ 255, 255, 255, 25 });

    BeginScissorMode((int)listX, (int)listY, (int)listWidth, (int)listHeight);
    
    float startCardY = listY + 20.0f - animScrollOffset;
    for (size_t i = 0; i < songs.size(); ++i) {
        float cardY = startCardY + i * 95.0f;
        
        if (cardY + 80.0f < listY || cardY > listY + listHeight) continue;

        bool isSelected = (i == (size_t)selectedSongIndex);
        Color cardBg = isSelected ? (Color){ 45, 55, 80, 230 } : (Color){ 28, 33, 45, 140 };
        Color borderColor = isSelected ? (Color){ 100, 160, 255, 255 } : (Color){ 255, 255, 255, 15 };

        Rectangle cardRect = { listX + 15.0f, cardY, listWidth - 30.0f, 80.0f };
        DrawRectangleRounded(cardRect, 0.1f, 4, cardBg);
        DrawRectangleRoundedLines(cardRect, 0.1f, 4, borderColor);

        if (isSelected) {
            DrawRectangleRounded((Rectangle){ listX + 15.0f, cardY, 6.0f, 80.0f }, 0.3f, 4, (Color){ 80, 150, 255, 255 });
        }

        std::string titleText = songs[i].title;
        std::string artistText = songs[i].artist;

        if (fontLoaded) {
            DrawTextEx(suitFont, titleText.c_str(), (Vector2){ listX + 35.0f, cardY + 16.0f }, 20.0f, 1.0f, isSelected ? WHITE : (Color){ 200, 210, 225, 220 });
            DrawTextEx(suitFont, artistText.c_str(), (Vector2){ listX + 35.0f, cardY + 44.0f }, 14.0f, 1.0f, (Color){ 130, 145, 170, 255 });
        } else {
            DrawText(titleText.c_str(), (int)(listX + 35.0f), (int)(cardY + 16.0f), 18, isSelected ? WHITE : (Color){ 200, 210, 225, 220 });
            DrawText(artistText.c_str(), (int)(listX + 35.0f), (int)(cardY + 44.0f), 14, (Color){ 130, 145, 170, 255 });
        }
    }
    EndScissorMode();

    const SongData& currentSong = songs[selectedSongIndex];
    float centerX = 530.0f;
    float centerY = 120.0f;
    float centerWidth = 380.0f;
    float centerHeight = 520.0f;

    DrawRectangleRounded((Rectangle){ centerX, centerY, centerWidth, centerHeight }, 0.04f, 4, (Color){ 20, 24, 35, 180 });
    DrawRectangleRoundedLines((Rectangle){ centerX, centerY, centerWidth, centerHeight }, 0.04f, 4, (Color){ 255, 255, 255, 25 });

    float artSize = 240.0f;
    float artX = centerX + (centerWidth - artSize) / 2.0f;
    float artY = centerY + 25.0f;
    
    DrawRectangleRounded((Rectangle){ artX, artY, artSize, artSize }, 0.05f, 4, (Color){ 32, 40, 58, 255 });
    DrawRectangleRoundedLines((Rectangle){ artX, artY, artSize, artSize }, 0.05f, 4, (Color){ 80, 130, 200, 120 });
    
    if (fontLoaded) {
        DrawTextEx(suitFont, currentSong.title.c_str(), (Vector2){ artX + 20.0f, artY + 90.0f }, 18.0f, 1.0f, WHITE);
        DrawTextEx(suitFont, "SONG JACKET ART", (Vector2){ artX + 45.0f, artY + 125.0f }, 14.0f, 1.0f, (Color){ 140, 160, 190, 200 });
    } else {
        DrawText("SONG JACKET", (int)(artX + 65.0f), (int)(artY + 110.0f), 18, WHITE);
    }

    float metaY = artY + artSize + 20.0f;
    if (fontLoaded) {
        DrawTextEx(suitFont, currentSong.title.c_str(), (Vector2){ centerX + 25.0f, metaY }, 22.0f, 1.0f, WHITE);
        DrawTextEx(suitFont, (std::string("Artist: ") + currentSong.artist).c_str(), (Vector2){ centerX + 25.0f, metaY + 32.0f }, 15.0f, 1.0f, (Color){ 160, 175, 200, 255 });
        DrawTextEx(suitFont, (std::string("Mapper: ") + currentSong.mapper).c_str(), (Vector2){ centerX + 25.0f, metaY + 56.0f }, 14.0f, 1.0f, (Color){ 130, 145, 170, 255 });
        
        char infoBuf[64];
        snprintf(infoBuf, sizeof(infoBuf), "Length: %.0fs   |   BPM: %.0f", currentSong.length, currentSong.difficulties[selectedDiffIndex].bpm);
        DrawTextEx(suitFont, infoBuf, (Vector2){ centerX + 25.0f, metaY + 84.0f }, 13.0f, 1.0f, (Color){ 110, 125, 150, 255 });
    } else {
        DrawText(currentSong.title.c_str(), (int)(centerX + 25.0f), (int)metaY, 20, WHITE);
        DrawText((std::string("Artist: ") + currentSong.artist).c_str(), (int)(centerX + 25.0f), (int)(metaY + 32.0f), 14, (Color){ 160, 175, 200, 255 });
    }

    float rightX = 930.0f;
    float rightY = 120.0f;
    float rightWidth = 300.0f;
    float rightHeight = 520.0f;

    DrawRectangleRounded((Rectangle){ rightX, rightY, rightWidth, rightHeight }, 0.04f, 4, (Color){ 20, 24, 35, 180 });
    DrawRectangleRoundedLines((Rectangle){ rightX, rightY, rightWidth, rightHeight }, 0.04f, 4, (Color){ 255, 255, 255, 25 });

    if (fontLoaded) {
        DrawTextEx(suitFont, "DIFFICULTY", (Vector2){ rightX + 20.0f, rightY + 20.0f }, 16.0f, 1.0f, (Color){ 120, 140, 180, 255 });
    } else {
        DrawText("DIFFICULTY", (int)(rightX + 20.0f), (int)(rightY + 20.0f), 16, (Color){ 120, 140, 180, 255 });
    }

    float diffY = rightY + 50.0f;
    const auto& diffs = currentSong.difficulties;
    for (size_t i = 0; i < diffs.size(); ++i) {
        bool isDiffSelected = (i == (size_t)selectedDiffIndex);
        Rectangle diffRect = { rightX + 20.0f, diffY + i * 45.0f, rightWidth - 40.0f, 38.0f };
        
        Color diffBg = isDiffSelected ? (Color){ 70, 110, 200, 220 } : (Color){ 30, 36, 48, 150 };
        DrawRectangleRounded(diffRect, 0.2f, 4, diffBg);
        
        if (fontLoaded) {
            DrawTextEx(suitFont, diffs[i].diffName.c_str(), (Vector2){ diffRect.x + 15.0f, diffRect.y + 9.0f }, 15.0f, 1.0f, isDiffSelected ? WHITE : (Color){ 170, 185, 210, 255 });
            
            char lvlBuf[16];
            snprintf(lvlBuf, sizeof(lvlBuf), "Lv.%d", diffs[i].level);
            int lvlW = (int)MeasureTextEx(suitFont, lvlBuf, 14.0f, 1.0f).x;
            DrawTextEx(suitFont, lvlBuf, (Vector2){ diffRect.x + diffRect.width - (float)lvlW - 15.0f, diffRect.y + 10.0f }, 14.0f, 1.0f, isDiffSelected ? WHITE : (Color){ 140, 155, 180, 255 });
        } else {
            DrawText(diffs[i].diffName.c_str(), (int)(diffRect.x + 15.0f), (int)(diffRect.y + 10.0f), 14, isDiffSelected ? WHITE : (Color){ 170, 185, 210, 255 });
        }
    }

    float statsY = rightY + 270.0f;
    DrawLine((int)(rightX + 20.0f), (int)statsY, (int)(rightX + rightWidth - 20.0f), (int)statsY, (Color){ 255, 255, 255, 20 });
    
    const DifficultyData& selDiff = diffs[selectedDiffIndex];
    if (fontLoaded) {
        char stat1[64], stat2[64], stat3[64];
        snprintf(stat1, sizeof(stat1), "Note Count: %d", selDiff.noteCount);
        snprintf(stat2, sizeof(stat2), "Max Combo: %d", selDiff.maxCombo);
        snprintf(stat3, sizeof(stat3), "Rating: %.1f*", selDiff.difficultyRating);

        DrawTextEx(suitFont, stat1, (Vector2){ rightX + 25.0f, statsY + 15.0f }, 14.0f, 1.0f, (Color){ 170, 185, 210, 255 });
        DrawTextEx(suitFont, stat2, (Vector2){ rightX + 25.0f, statsY + 38.0f }, 14.0f, 1.0f, (Color){ 170, 185, 210, 255 });
        DrawTextEx(suitFont, stat3, (Vector2){ rightX + 25.0f, statsY + 61.0f }, 14.0f, 1.0f, (Color){ 255, 200, 100, 255 });
    }

    Rectangle playBtnRect = { rightX + 20.0f, rightY + 415.0f, rightWidth - 40.0f, 75.0f };
    DrawRectangleRounded(playBtnRect, 0.25f, 4, (Color){ 50, 140, 255, 255 });
    DrawRectangleRoundedLines(playBtnRect, 0.25f, 4, (Color){ 120, 180, 255, 255 });

    if (fontLoaded) {
        const char* playText = "PLAY GAME";
        int tWidth = (int)MeasureTextEx(suitFont, playText, 20.0f, 1.0f).x;
        DrawTextEx(suitFont, playText, (Vector2){ playBtnRect.x + (playBtnRect.width - (float)tWidth) / 2.0f, playBtnRect.y + 26.0f }, 20.0f, 1.0f, WHITE);
    } else {
        DrawText("PLAY GAME", (int)(playBtnRect.x + 55.0f), (int)(playBtnRect.y + 28.0f), 18, WHITE);
    }

    const char* guide = "[W/S or Up/Down] Select Song   |   [A/D or Left/Right] Select Difficulty   |   [ENTER/SPACE] Play   |   [ESC] Back";
    if (fontLoaded) {
        DrawTextEx(suitFont, guide, (Vector2){ 60.0f, (float)screenHeight - 40.0f }, 13.0f, 1.0f, (Color){ 110, 125, 150, 255 });
    } else {
        DrawText(guide, 60, screenHeight - 40, 13, (Color){ 110, 125, 150, 255 });
    }
}

bool SongSelect::IsPlaySelected() const {
    return IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
}

bool SongSelect::IsBackSelected() const {
    return IsKeyPressed(KEY_ESCAPE);
}

const SongData& SongSelect::GetCurrentSong() const {
    return songs[selectedSongIndex];
}

int SongSelect::GetCurrentDifficultyIndex() const {
    return selectedDiffIndex;
}