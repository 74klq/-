#include "Selection.h"
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <vector>
#include <set>
#include <string>

static inline float EaseOutCubic(float x) {
    return 1.0f - std::pow(1.0f - x, 3.0f);
}

static inline float EaseInOutCubic(float x) {
    return x < 0.5f ? 4.0f * x * x * x : 1.0f - std::pow(-2.0f * x + 2.0f, 3.0f) / 2.0f;
}

SongSelect::SongSelect()
    : selectedSongIndex(0), selectedDiffIndex(0), previousSongIndex(0),
      animScrollOffset(0.0f), targetScrollOffset(0.0f), 
      bgTransitionAlpha(1.0f), carouselAnimProgress(1.0f), fontLoaded(false) {}

SongSelect::~SongSelect() {
    if (fontLoaded) {
        UnloadFont(suitFont);
    }
}

void SongSelect::Init() {
    LoadSongs();
    
    if (!fontLoaded) {
        std::set<int> cpSet;
        for (int i = 32; i <= 126; ++i) cpSet.insert(i);
        
        std::vector<std::string> texts = {
            "A Night Without Visible Stars", "Four Beat Sounds", "Mapper_A",
            "별이 보이지 않는 밤", "Plum", "boyangsic",
            "Cybernetic Dream", "Neo Synth", "Mapper_C",
            "Neon Highway", "Retro Pulse", "Mapper_B",
            "작곡가:", "에디터:", "BPM:", "길이:", "노트 수:", "최대 콤보:", "난이도 레이팅:",
            "곡 이동", "난이도 변경", "플레이", "뒤로가기",
            "DIFFICULTY SELECT (A / D)", "JACKET ART", "PLAY GAME",
            "EASY", "NORMAL", "HARD", "EXPERT", "MASTER"
        };

        for (const auto& text : texts) {
            const char* p = text.c_str();
            while (*p) {
                int c = 0;
                int byteCount = 0;
                unsigned char lead = *p;
                if (lead < 0x80) {
                    c = lead;
                    byteCount = 1;
                } else if ((lead & 0xE0) == 0xC0) {
                    c = lead & 0x1F;
                    byteCount = 2;
                } else if ((lead & 0xF0) == 0xE0) {
                    c = lead & 0x0F;
                    byteCount = 3;
                } else if ((lead & 0xF8) == 0xF0) {
                    c = lead & 0x07;
                    byteCount = 4;
                } else {
                    byteCount = 1;
                    p++;
                    continue;
                }
                
                bool valid = true;
                for (int i = 1; i < byteCount; ++i) {
                    if ((p[i] & 0xC0) != 0x80) { valid = false; break; }
                    c = (c << 6) | (p[i] & 0x3F);
                }
                if (valid) {
                    cpSet.insert(c);
                }
                p += byteCount;
            }
        }

        std::vector<int> codepoints(cpSet.begin(), cpSet.end());
        suitFont = LoadFontEx("fonts/Pretendard-Black.ttf", 68, codepoints.data(), (int)codepoints.size());

        if (suitFont.texture.id != 0) {
            SetTextureFilter(suitFont.texture, TEXTURE_FILTER_BILINEAR);
            fontLoaded = true;
        }
    }
    
    selectedSongIndex = 0;
    selectedDiffIndex = 0;
    animScrollOffset = 0.0f;
    targetScrollOffset = 0.0f;
    carouselAnimProgress = 1.0f;
}

void SongSelect::LoadSongs() {
    songs.clear();

    SongData song1;
    song1.title = "별이 보이지 않는밤";
    song1.artist = "Plum";
    song1.mapper = "boyangsic";
    song1.bpm = 88.0f;
    song1.length = 152.0f;
    song1.cleared = true;
    song1.difficulties = {
        {"EASY", 3, 140.0f, 210, 450, 4.0f},
        {"NORMAL", 7, 140.0f, 430, 890, 6.0f},
        {"HARD", 12, 140.0f, 780, 1520, 6.0f},
        {"EXPERT", 16, 140.0f, 1150, 2300, 6.5f}
    };
    songs.push_back(song1);

    SongData song2;
    song2.title = "Kaleidoscope";
    song2.artist = "Plum";
    song2.mapper = "boyangsic";
    song2.bpm = 128.0f;
    song2.length = 135.0f;
    song2.cleared = false;
    song2.difficulties = {
        {"NORMAL", 5, 128.0f, 320, 640, 4.0f},
        {"HARD", 10, 128.0f, 650, 1300, 6.0f},
        {"EXPERT", 15, 128.0f, 1020, 2100, 6.5f}
    };
    songs.push_back(song2);

    SongData song3;
    song3.title = "Timeline";
    song3.artist = "Plum";
    song3.mapper = "boyangsic";
    song3.bpm = 155.0f;
    song3.length = 168.0f;
    song3.cleared = true;
    song3.difficulties = {
        {"EASY", 2, 155.0f, 180, 390, 4.0f},
        {"NORMAL", 6, 155.0f, 390, 810, 6.0f},
        {"HARD", 11, 155.0f, 720, 1450, 6.0f},
        {"EXPERT", 17, 155.0f, 1300, 2650, 6.5f},
        {"MASTER", 20, 155.0f, 1750, 3500, 6.5f}
    };
    songs.push_back(song3);

    SongData song4;
    song4.title = "R";
    song4.artist = "Plum";
    song4.mapper = "boyangsic";
    song4.bpm = 170.0f;
    song4.length = 142.0f;
    song4.cleared = false;
    song4.difficulties = {
        {"NORMAL", 6, 170.0f, 400, 800, 4.0f},
        {"HARD", 13, 170.0f, 850, 1700, 6.0f},
        {"EXPERT", 18, 170.0f, 1400, 2800, 6.5f}
    };
    songs.push_back(song4);
}

void SongSelect::Update() {
    float dt = GetFrameTime();

    animScrollOffset += (targetScrollOffset - animScrollOffset) * (dt * 16.0f);
    
    if (carouselAnimProgress < 1.0f) {
        carouselAnimProgress += dt * 6.0f;
        if (carouselAnimProgress > 1.0f) carouselAnimProgress = 1.0f;
    }

    if (bgTransitionAlpha < 1.0f) {
        bgTransitionAlpha += dt * 4.0f;
        if (bgTransitionAlpha > 1.0f) bgTransitionAlpha = 1.0f;
    }

    if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
        if (selectedSongIndex > 0) {
            previousSongIndex = selectedSongIndex;
            selectedSongIndex--;
            targetScrollOffset = (float)selectedSongIndex * 1.0f; 
            carouselAnimProgress = 0.0f;
            bgTransitionAlpha = 0.0f;
            selectedDiffIndex = std::min(selectedDiffIndex, (int)songs[selectedSongIndex].difficulties.size() - 1);
        }
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        if (selectedSongIndex < (int)songs.size() - 1) {
            previousSongIndex = selectedSongIndex;
            selectedSongIndex++;
            targetScrollOffset = (float)selectedSongIndex * 1.0f;
            carouselAnimProgress = 0.0f;
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
        int newIndex = selectedSongIndex - (int)wheel;
        newIndex = std::clamp(newIndex, 0, (int)songs.size() - 1);
        if (newIndex != selectedSongIndex) {
            previousSongIndex = selectedSongIndex;
            selectedSongIndex = newIndex;
            targetScrollOffset = (float)selectedSongIndex * 1.0f;
            carouselAnimProgress = 0.0f;
            bgTransitionAlpha = 0.0f;
            selectedDiffIndex = std::min(selectedDiffIndex, (int)songs[selectedSongIndex].difficulties.size() - 1);
        }
    }
}

float SongSelect::GetSongCardY(int index, float centerY) const {
    float relativePos = (float)index - animScrollOffset;
    return centerY + relativePos * 150.0f;
}

float SongSelect::GetSongCardScale(int index) const {
    float dist = std::abs((float)index - animScrollOffset);
    float scale = 1.0f - dist * 0.25f;
    return std::clamp(scale, 0.65f, 1.0f);
}

float SongSelect::GetSongCardAlpha(int index) const {
    float dist = std::abs((float)index - animScrollOffset);
    float alpha = 1.0f - dist * 0.35f;
    return std::clamp(alpha, 0.2f, 1.0f);
}

void SongSelect::Draw(int screenWidth, int screenHeight) {
    DrawBackground(screenWidth, screenHeight);
    DrawSongCarousel(screenWidth, screenHeight);
    DrawCurrentSongInfo(screenWidth, screenHeight);
    DrawInputHints(screenWidth, screenHeight);
}

void SongSelect::DrawBackground(int screenWidth, int screenHeight) {
    DrawRectangle(0, 0, screenWidth, screenHeight, (Color){ 10, 10, 12, 255 });
    DrawRectangleGradientV(0, 0, screenWidth, screenHeight, (Color){ 20, 22, 28, 255 }, (Color){ 8, 8, 10, 255 });
}

void SongSelect::DrawSongCarousel(int screenWidth, int screenHeight) {
    float centerX = (float)screenWidth * 0.32f;
    float centerY = (float)screenHeight * 0.45f;

    for (int i = 0; i < (int)songs.size(); ++i) {
        float cardY = GetSongCardY(i, centerY);
        float scale = GetSongCardScale(i);
        float alpha = GetSongCardAlpha(i);

        if (cardY < -150.0f || cardY > (float)screenHeight + 150.0f) continue;

        bool isCurrent = (i == selectedSongIndex);

        float cardWidth = 340.0f * scale;
        float cardHeight = 110.0f * scale;
        float cardX = centerX - cardWidth / 2.0f;

        Color cardBg = isCurrent ? (Color){ 35, 40, 50, 240 } : (Color){ 18, 20, 25, (unsigned char)(200 * alpha) };
        Color borderColor = isCurrent ? WHITE : (Color){ 255, 255, 255, (unsigned char)(50 * alpha) };

        DrawRectangleRounded((Rectangle){ cardX + 4.0f, cardY + 4.0f, cardWidth, cardHeight }, 0.12f, 4, (Color){ 0, 0, 0, (unsigned char)(120 * alpha) });

        if (isCurrent) {
            float pulse = 1.0f + 0.015f * std::sin((float)GetTime() * 8.0f);
            float pWidth = cardWidth * pulse;
            float pHeight = cardHeight * pulse;
            float pX = cardX - (pWidth - cardWidth) / 2.0f;
            float pY = cardY - (pHeight - cardHeight) / 2.0f;
            DrawRectangleRounded((Rectangle){ pX, pY, pWidth, pHeight }, 0.12f, 4, cardBg);
            DrawRectangleRoundedLines((Rectangle){ pX, pY, pWidth, pHeight }, 0.12f, 4, WHITE);
            DrawRectangleRounded((Rectangle){ pX, pY, 8.0f, pHeight }, 0.3f, 4, WHITE);
        } else {
            DrawRectangleRounded((Rectangle){ cardX, cardY, cardWidth, cardHeight }, 0.12f, 4, cardBg);
            DrawRectangleRoundedLines((Rectangle){ cardX, cardY, cardWidth, cardHeight }, 0.12f, 4, borderColor);
        }

        float jacketSize = cardHeight - 24.0f * scale;
        float jacketX = cardX + 16.0f * scale;
        float jacketY = cardY + 12.0f * scale;
        
        DrawRectangleRounded((Rectangle){ jacketX, jacketY, jacketSize, jacketSize }, 0.08f, 4, (Color){ 60, 70, 85, (unsigned char)(255 * alpha) });
        
        Color titleColor = isCurrent ? WHITE : (Color){ 200, 210, 225, (unsigned char)(220 * alpha) };
        Color artistColor = isCurrent ? (Color){ 220, 220, 220, 255 } : (Color){ 150, 160, 175, (unsigned char)(180 * alpha) };

        float textX = jacketX + jacketSize + 14.0f * scale;
        
        if (fontLoaded) {
            DrawTextEx(suitFont, songs[i].title.c_str(), (Vector2){ textX, jacketY + 8.0f * scale }, 17.0f * scale, 1.0f, titleColor);
            DrawTextEx(suitFont, songs[i].artist.c_str(), (Vector2){ textX, jacketY + 36.0f * scale }, 13.0f * scale, 1.0f, artistColor);
        } else {
            DrawText(songs[i].title.c_str(), (int)textX, (int)(jacketY + 8.0f * scale), (int)(16 * scale), titleColor);
            DrawText(songs[i].artist.c_str(), (int)textX, (int)(jacketY + 36.0f * scale), (int)(12 * scale), artistColor);
        }
    }
}

void SongSelect::DrawCurrentSongInfo(int screenWidth, int screenHeight) {
    const SongData& curSong = songs[selectedSongIndex];
    const DifficultyData& curDiff = curSong.difficulties[selectedDiffIndex];

    float infoPanelX = (float)screenWidth * 0.53f;
    float infoPanelY = (float)screenHeight * 0.12f;
    float infoPanelW = (float)screenWidth * 0.42f;
    float infoPanelH = (float)screenHeight * 0.72f;

    DrawRectangleRounded((Rectangle){ infoPanelX + 6.0f, infoPanelY + 6.0f, infoPanelW, infoPanelH }, 0.03f, 4, (Color){ 0, 0, 0, 150 });
    DrawRectangleRounded((Rectangle){ infoPanelX, infoPanelY, infoPanelW, infoPanelH }, 0.03f, 4, (Color){ 22, 26, 35, 245 });
    DrawRectangleRoundedLines((Rectangle){ infoPanelX, infoPanelY, infoPanelW, infoPanelH }, 0.03f, 4, WHITE);

    float animProgressEase = EaseOutCubic(carouselAnimProgress);
    float baseArtSize = 220.0f;
    float currentArtSize = baseArtSize * (0.92f + 0.08f * animProgressEase);
    float artX = infoPanelX + 30.0f;
    float artY = infoPanelY + 30.0f;

    DrawRectangleRounded((Rectangle){ artX + 4.0f, artY + 4.0f, currentArtSize, currentArtSize }, 0.04f, 4, (Color){ 0, 0, 0, 100 });
    DrawRectangleRounded((Rectangle){ artX, artY, currentArtSize, currentArtSize }, 0.04f, 4, (Color){ 35, 42, 55, 255 });
    DrawRectangleRoundedLines((Rectangle){ artX, artY, currentArtSize, currentArtSize }, 0.04f, 4, WHITE);

    if (fontLoaded) {
        DrawTextEx(suitFont, "JACKET ART", (Vector2){ artX + currentArtSize / 2.0f - 48.0f, artY + currentArtSize / 2.0f - 10.0f }, 15.0f, 1.0f, WHITE);
    }

    float titleX = artX + currentArtSize + 25.0f;
    float titleY = artY + 5.0f;

    if (fontLoaded) {
        DrawTextEx(suitFont, curSong.title.c_str(), (Vector2){ titleX, titleY }, 25.0f, 1.0f, WHITE);
    } else {
        DrawText(curSong.title.c_str(), (int)titleX, (int)titleY, 22, WHITE);
    }

    float metaStartY = titleY + 45.0f;
    float rowSpacing = 28.0f;

    auto DrawMetaRow = [&](const char* label, const std::string& value, float yOffset, Color valColor = WHITE) {
        if (fontLoaded) {
            DrawTextEx(suitFont, label, (Vector2){ titleX, metaStartY + yOffset }, 14.0f, 1.0f, (Color){ 180, 190, 205, 255 });
            DrawTextEx(suitFont, value.c_str(), (Vector2){ titleX + 85.0f, metaStartY + yOffset - 1.0f }, 16.0f, 1.0f, valColor);
        } else {
            DrawText(label, (int)titleX, (int)(metaStartY + yOffset), 13, (Color){ 180, 190, 205, 255 });
            DrawText(value.c_str(), (int)(titleX + 85.0f), (int)(metaStartY + yOffset), 14, valColor);
        }
    };

    DrawMetaRow("작곡가:", curSong.artist, 0.0f);
    DrawMetaRow("에디터:", curSong.mapper, rowSpacing);
    
    char bpmBuf[32], lenBuf[32];
    snprintf(bpmBuf, sizeof(bpmBuf), "%.0f", curSong.bpm);
    int mins = (int)(curSong.length) / 60;
    int secs = (int)(curSong.length) % 60;
    snprintf(lenBuf, sizeof(lenBuf), "%d:%02d (%.0fs)", mins, secs, curSong.length);

    DrawMetaRow("BPM:", bpmBuf, rowSpacing * 2.0f);
    DrawMetaRow("길이:", lenBuf, rowSpacing * 3.0f);

    float diffSectionY = artY + currentArtSize + 25.0f;
    float diffBoxW = infoPanelW - 60.0f;

    if (fontLoaded) {
        DrawTextEx(suitFont, "DIFFICULTY SELECT (A / D)", (Vector2){ artX, diffSectionY }, 13.0f, 1.0f, WHITE);
    }

    diffSectionY += 22.0f;
    for (size_t i = 0; i < curSong.difficulties.size(); ++i) {
        bool isDiffSelected = (int)i == selectedDiffIndex;
        float dx = artX + (i * (diffBoxW / curSong.difficulties.size()));
        float dw = (diffBoxW / curSong.difficulties.size()) - 6.0f;
        
        Rectangle dRect = { dx, diffSectionY, dw, 40.0f };
        
        DrawRectangleRounded((Rectangle){ dx + 2.0f, diffSectionY + 2.0f, dw, 40.0f }, 0.2f, 4, (Color){ 0, 0, 0, 100 });

        if (isDiffSelected) {
            DrawRectangleRounded(dRect, 0.2f, 4, WHITE);
            DrawRectangleRoundedLines(dRect, 0.2f, 4, WHITE);
        } else {
            DrawRectangleRounded(dRect, 0.2f, 4, (Color){ 32, 38, 50, 255 });
            DrawRectangleRoundedLines(dRect, 0.2f, 4, (Color){ 120, 130, 150, 255 });
        }

        if (fontLoaded) {
            char diffLabel[32];
            snprintf(diffLabel, sizeof(diffLabel), "%s Lv.%d", curSong.difficulties[i].diffName.c_str(), curSong.difficulties[i].level);
            int tW = (int)MeasureTextEx(suitFont, diffLabel, 13.0f, 1.0f).x;
            Color textColor = isDiffSelected ? BLACK : WHITE;
            DrawTextEx(suitFont, diffLabel, (Vector2){ dRect.x + (dRect.width - (float)tW) / 2.0f, dRect.y + 11.0f }, 13.0f, 1.0f, textColor);
        }
    }

    float statsY = diffSectionY + 55.0f;
    DrawLine((int)artX, (int)statsY, (int)(artX + diffBoxW), (int)statsY, WHITE);

    char statBuf1[64], statBuf2[64];
    snprintf(statBuf1, sizeof(statBuf1), "노트 수: %d", curDiff.noteCount);
    snprintf(statBuf2, sizeof(statBuf2), "최대 콤보: %d", curDiff.maxCombo);

    if (fontLoaded) {
        DrawTextEx(suitFont, statBuf1, (Vector2){ artX, statsY + 12.0f }, 14.0f, 1.0f, WHITE);
        DrawTextEx(suitFont, statBuf2, (Vector2){ artX + 150.0f, statsY + 12.0f }, 14.0f, 1.0f, WHITE);
        DrawTextEx(suitFont, "난이도:", (Vector2){ artX, statsY + 36.0f }, 14.0f, 1.0f, WHITE);
    }

    float circleStartX = artX + 105.0f;
    float circleY = statsY + 43.0f;
    float circleRadius = 5.5f;
    float circleSpacing = 16.0f;

    for (int c = 0; c < 10; ++c) {
        float cx = circleStartX + (float)c * circleSpacing;
        DrawRing((Vector2){ cx, circleY }, circleRadius - 1.2f, circleRadius, 0.0f, 360.0f, 16, (Color){ 150, 160, 175, 255 });

        float ratingValue = curDiff.difficultyRating;
        if ((float)c < ratingValue) {
            if ((float)c + 0.5f == ratingValue) {
                DrawRectangle((int)(cx - circleRadius), (int)(circleY - circleRadius), (int)circleRadius, (int)(circleRadius * 2.0f), WHITE);
            } else {
                DrawCircle((int)cx, (int)circleY, circleRadius - 0.8f, WHITE);
            }
        }
    }

    Rectangle playBtnRect = { artX, statsY + 72.0f, diffBoxW, 52.0f };
    DrawRectangleRounded((Rectangle){ playBtnRect.x + 3.0f, playBtnRect.y + 3.0f, playBtnRect.width, playBtnRect.height }, 0.25f, 4, (Color){ 0, 0, 0, 120 });
    
    float playPulse = 1.0f + 0.008f * std::sin((float)GetTime() * 10.0f);
    Rectangle pBtnScaled = { 
        playBtnRect.x - (playBtnRect.width * (playPulse - 1.0f)) / 2.0f, 
        playBtnRect.y - (playBtnRect.height * (playPulse - 1.0f)) / 2.0f, 
        playBtnRect.width * playPulse, 
        playBtnRect.height * playPulse 
    };
    
    DrawRectangleRounded(pBtnScaled, 0.25f, 4, WHITE);
    DrawRectangleRoundedLines(pBtnScaled, 0.25f, 4, WHITE);

    if (fontLoaded) {
        const char* playStr = "[ ENTER / SPACE ] PLAY GAME";
        int pW = (int)MeasureTextEx(suitFont, playStr, 17.0f, 1.0f).x;
        DrawTextEx(suitFont, playStr, (Vector2){ pBtnScaled.x + (pBtnScaled.width - (float)pW) / 2.0f, pBtnScaled.y + 17.0f }, 17.0f, 1.0f, BLACK);
    } else {
        DrawText("PLAY GAME", (int)(playBtnRect.x + 120.0f), (int)(playBtnRect.y + 17.0f), 16, BLACK);
    }
}

void SongSelect::DrawInputHints(int screenWidth, int screenHeight) {
    const char* guideText = "[ W / S 또는 UP / DOWN ] 곡 이동    |    [ A / D ] 난이도 변경    |    [ ENTER / SPACE ] 플레이    |    [ ESC ] 뒤로가기";
    
    if (fontLoaded) {
        DrawTextEx(suitFont, guideText, (Vector2){ 60.0f, (float)screenHeight - 38.0f }, 14.0f, 1.0f, WHITE);
    } else {
        DrawText(guideText, 60, screenHeight - 38, 13, WHITE);
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