#include "Selection.h"
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <vector>
#include <set>
#include <string>

static std::vector<Texture2D> g_jacketTextures;

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

    for (auto& tex : g_jacketTextures) {
        if (tex.id != 0) {
            UnloadTexture(tex);
        }
    }
    g_jacketTextures.clear();
}

void SongSelect::Init() {
    LoadSongs();
    
    for (auto& tex : g_jacketTextures) {
        if (tex.id != 0) UnloadTexture(tex);
    }
    g_jacketTextures.clear();

    std::vector<std::string> jacketPaths = {
        "album_assets/stars.png",
        "album_assets/kaleidoscope.png",
        "album_assets/Timeline.png",
        "album_assets/R.png"
    };

    for (const auto& path : jacketPaths) {
        Texture2D tex = LoadTexture(path.c_str());
        if (tex.id != 0) {
            SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
        }
        g_jacketTextures.push_back(tex);
    }

    if (!fontLoaded) {
        std::set<int> cpSet;
        for (int i = 32; i <= 126; ++i) cpSet.insert(i);
        
        std::vector<std::string> texts = {
            "A Night Without Visible Stars", "Four Beat Sounds", "Mapper_A",
            "별이 보이지 않는 밤", "Plum", "boyangsic", "Kaleidoscope", "Timeline", "R",
            "Cybernetic Dream", "Neo Synth", "Mapper_C",
            "Neon Highway", "Retro Pulse", "Mapper_B",
            "작곡가:", "에디터:", "BPM:", "길이:",
            "곡 선택", "플레이", "뒤로가기",
            "엔터 누르면 시작 가능"
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
        suitFont = LoadFontEx("fonts/Pretendard-Black.ttf", 72, codepoints.data(), (int)codepoints.size());

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
    bgTransitionAlpha = 1.0f;
}

void SongSelect::LoadSongs() {
    songs.clear();

    SongData song1;
    song1.title = "별이 보이지 않는 밤";
    song1.artist = "Plum";
    song1.mapper = "boyangsic";
    song1.bpm = 88.0f;
    song1.length = 152.0f;
    song1.cleared = true;
    song1.difficulties = { {"NORMAL", 7, 140.0f, 430, 890, 6.0f} };
    songs.push_back(song1);

    SongData song2;
    song2.title = "Kaleidoscope";
    song2.artist = "Plum";
    song2.mapper = "boyangsic";
    song2.bpm = 128.0f;
    song2.length = 135.0f;
    song2.cleared = false;
    song2.difficulties = { {"HARD", 10, 128.0f, 650, 1300, 6.0f} };
    songs.push_back(song2);

    SongData song3;
    song3.title = "Timeline";
    song3.artist = "Plum";
    song3.mapper = "boyangsic";
    song3.bpm = 155.0f;
    song3.length = 168.0f;
    song3.cleared = true;
    song3.difficulties = { {"EXPERT", 17, 155.0f, 1300, 2650, 6.5f} };
    songs.push_back(song3);

    SongData song4;
    song4.title = "R";
    song4.artist = "Plum";
    song4.mapper = "boyangsic";
    song4.bpm = 170.0f;
    song4.length = 142.0f;
    song4.cleared = false;
    song4.difficulties = { {"MASTER", 20, 170.0f, 1750, 3500, 6.5f} };
    songs.push_back(song4);
}

void SongSelect::Update() {
    float dt = GetFrameTime();

    float lerpFactor = 1.0f - std::exp(-16.0f * dt);
    animScrollOffset += (targetScrollOffset - animScrollOffset) * lerpFactor;
    
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
            targetScrollOffset = (float)selectedSongIndex; 
            carouselAnimProgress = 0.0f;
            bgTransitionAlpha = 0.0f;
            selectedDiffIndex = 0;
        }
    }
    if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
        if (selectedSongIndex < (int)songs.size() - 1) {
            previousSongIndex = selectedSongIndex;
            selectedSongIndex++;
            targetScrollOffset = (float)selectedSongIndex;
            carouselAnimProgress = 0.0f;
            bgTransitionAlpha = 0.0f;
            selectedDiffIndex = 0;
        }
    }

    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) {
        int newIndex = selectedSongIndex - (int)wheel;
        newIndex = std::clamp(newIndex, 0, (int)songs.size() - 1);
        if (newIndex != selectedSongIndex) {
            previousSongIndex = selectedSongIndex;
            selectedSongIndex = newIndex;
            targetScrollOffset = (float)selectedSongIndex;
            carouselAnimProgress = 0.0f;
            bgTransitionAlpha = 0.0f;
            selectedDiffIndex = 0;
        }
    }
}

float SongSelect::GetSongCardY(int index, float centerY) const {
    float relativePos = (float)index - animScrollOffset;
    return centerY + relativePos * 145.0f;
}

float SongSelect::GetSongCardScale(int index) const {
    float dist = std::abs((float)index - animScrollOffset);
    float scale = 1.0f - dist * 0.18f;
    return std::clamp(scale, 0.72f, 1.0f);
}

float SongSelect::GetSongCardAlpha(int index) const {
    float dist = std::abs((float)index - animScrollOffset);
    float alpha = 1.0f - dist * 0.30f;
    return std::clamp(alpha, 0.25f, 1.0f);
}

void SongSelect::Draw(int screenWidth, int screenHeight) {
    DrawBackground(screenWidth, screenHeight);
    DrawSongCarousel(screenWidth, screenHeight);
    DrawCurrentSongInfo(screenWidth, screenHeight);
    DrawInputHints(screenWidth, screenHeight);
}

void SongSelect::DrawBackground(int screenWidth, int screenHeight) {
    // 1. 기본 배경 레이어
    DrawRectangle(0, 0, screenWidth, screenHeight, (Color){ 12, 14, 20, 255 });

    // 2. Pan, Zoom & Slow Rotation 변수 계산
    float time = (float)GetTime();
    float scale = 1.10f + 0.02f * std::sin(time * 0.3f); // 기본 110% 확대 + 미세 줌
    float rotation = 1.2f * std::sin(time * 0.15f);      // 미세 회전 (-1.2° ~ +1.2°)
    
    // 대각선 이동 (Pan)
    float panX = 18.0f * std::cos(time * 0.25f);
    float panY = 14.0f * std::sin(time * 0.20f);

    float bgW = screenWidth * scale;
    float bgH = screenHeight * scale;

    // 화면 중심 기준 원점 연산
    Vector2 origin = { bgW * 0.5f, bgH * 0.5f };
    Rectangle destRect = {
        (screenWidth * 0.5f) + panX,
        (screenHeight * 0.5f) + panY,
        bgW,
        bgH
    };

    // 불투명도 레벨 조정 (약간 더 진하게)
    float maxBgAlpha = 0.28f;

    // 이전 곡 배경 Fade Out
    if (bgTransitionAlpha < 1.0f && previousSongIndex >= 0 && previousSongIndex < (int)g_jacketTextures.size()) {
        if (g_jacketTextures[previousSongIndex].id != 0) {
            unsigned char pAlpha = (unsigned char)(255.0f * maxBgAlpha * (1.0f - bgTransitionAlpha));
            DrawTexturePro(
                g_jacketTextures[previousSongIndex],
                (Rectangle){ 0, 0, (float)g_jacketTextures[previousSongIndex].width, (float)g_jacketTextures[previousSongIndex].height },
                destRect, origin, rotation,
                (Color){ 255, 255, 255, pAlpha }
            );
        }
    }

    // 현재 곡 배경 Fade In
    if (selectedSongIndex >= 0 && selectedSongIndex < (int)g_jacketTextures.size()) {
        if (g_jacketTextures[selectedSongIndex].id != 0) {
            unsigned char cAlpha = (unsigned char)(255.0f * maxBgAlpha * bgTransitionAlpha);
            DrawTexturePro(
                g_jacketTextures[selectedSongIndex],
                (Rectangle){ 0, 0, (float)g_jacketTextures[selectedSongIndex].width, (float)g_jacketTextures[selectedSongIndex].height },
                destRect, origin, rotation,
                (Color){ 255, 255, 255, cAlpha }
            );
        }
    }

    // 3. 어두운 오버레이 그라데이션
    DrawRectangleGradientV(0, 0, screenWidth, screenHeight, (Color){ 20, 24, 34, 130 }, (Color){ 8, 9, 14, 180 });
    
    // 4. 격자 사선 효과
    for (int i = -screenWidth; i < screenWidth + screenHeight; i += 80) {
        DrawLine(i, 0, i + screenHeight, screenHeight, (Color){ 255, 255, 255, 6 });
    }
}

void SongSelect::DrawSongCarousel(int screenWidth, int screenHeight) {
    float centerX = (float)screenWidth * 0.30f;
    float centerY = (float)screenHeight * 0.48f;

    for (int i = 0; i < (int)songs.size(); ++i) {
        float cardY = GetSongCardY(i, centerY);
        float scale = GetSongCardScale(i);
        float alpha = GetSongCardAlpha(i);

        if (cardY < -160.0f || cardY > (float)screenHeight + 160.0f) continue;

        bool isCurrent = (i == selectedSongIndex);

        float cardWidth = 380.0f * scale;
        float cardHeight = 120.0f * scale;
        float cardX = centerX - cardWidth / 2.0f;

        Color cardBg = isCurrent ? (Color){ 32, 38, 52, 245 } : (Color){ 18, 21, 28, (unsigned char)(210 * alpha) };
        Color borderColor = isCurrent ? (Color){ 255, 255, 255, 255 } : (Color){ 255, 255, 255, (unsigned char)(40 * alpha) };

        DrawRectangleRounded((Rectangle){ cardX + 5.0f, cardY + 5.0f, cardWidth, cardHeight }, 0.12f, 4, (Color){ 0, 0, 0, (unsigned char)(140 * alpha) });

        if (isCurrent) {
            float pulse = 1.0f + 0.012f * std::sin((float)GetTime() * 8.0f);
            float pWidth = cardWidth * pulse;
            float pHeight = cardHeight * pulse;
            float pX = cardX - (pWidth - cardWidth) / 2.0f;
            float pY = cardY - (pHeight - cardHeight) / 2.0f;
            
            DrawRectangleRounded((Rectangle){ pX, pY, pWidth, pHeight }, 0.12f, 4, cardBg);
            DrawRectangleRoundedLines((Rectangle){ pX, pY, pWidth, pHeight }, 0.12f, 4, WHITE);
            DrawRectangleRounded((Rectangle){ pX, pY, 8.0f, pHeight }, 0.25f, 4, WHITE);
        } else {
            DrawRectangleRounded((Rectangle){ cardX, cardY, cardWidth, cardHeight }, 0.12f, 4, cardBg);
            DrawRectangleRoundedLines((Rectangle){ cardX, cardY, cardWidth, cardHeight }, 0.12f, 4, borderColor);
        }

        float jacketSize = cardHeight - 24.0f * scale;
        float jacketX = cardX + 18.0f * scale;
        float jacketY = cardY + 12.0f * scale;
        
        if (i < (int)g_jacketTextures.size() && g_jacketTextures[i].id != 0) {
            DrawTexturePro(
                g_jacketTextures[i],
                (Rectangle){ 0, 0, (float)g_jacketTextures[i].width, (float)g_jacketTextures[i].height },
                (Rectangle){ jacketX, jacketY, jacketSize, jacketSize },
                (Vector2){ 0, 0 }, 0.0f,
                (Color){ 255, 255, 255, (unsigned char)(255 * alpha) }
            );
            DrawRectangleRoundedLines((Rectangle){ jacketX, jacketY, jacketSize, jacketSize }, 0.10f, 4, (Color){ 255, 255, 255, (unsigned char)(100 * alpha) });
        } else {
            DrawRectangleRounded((Rectangle){ jacketX, jacketY, jacketSize, jacketSize }, 0.10f, 4, (Color){ 50, 60, 78, (unsigned char)(255 * alpha) });
        }
        
        Color titleColor = isCurrent ? WHITE : (Color){ 200, 210, 225, (unsigned char)(220 * alpha) };
        Color artistColor = isCurrent ? (Color){ 210, 220, 235, 255 } : (Color){ 140, 150, 170, (unsigned char)(180 * alpha) };

        float textX = jacketX + jacketSize + 16.0f * scale;
        
        if (fontLoaded) {
            DrawTextEx(suitFont, songs[i].title.c_str(), (Vector2){ textX, jacketY + 10.0f * scale }, 20.0f * scale, 1.0f, titleColor);
            DrawTextEx(suitFont, songs[i].artist.c_str(), (Vector2){ textX, jacketY + 44.0f * scale }, 15.0f * scale, 1.0f, artistColor);
        } else {
            DrawText(songs[i].title.c_str(), (int)textX, (int)(jacketY + 10.0f * scale), (int)(18 * scale), titleColor);
            DrawText(songs[i].artist.c_str(), (int)textX, (int)(jacketY + 44.0f * scale), (int)(14 * scale), artistColor);
        }
    }
}

void SongSelect::DrawCurrentSongInfo(int screenWidth, int screenHeight) {
    const SongData& curSong = songs[selectedSongIndex];

    float infoPanelX = (float)screenWidth * 0.52f;
    float infoPanelY = (float)screenHeight * 0.10f;
    float infoPanelW = (float)screenWidth * 0.44f;
    float infoPanelH = (float)screenHeight * 0.78f;

    DrawRectangleRounded((Rectangle){ infoPanelX + 8.0f, infoPanelY + 8.0f, infoPanelW, infoPanelH }, 0.04f, 4, (Color){ 0, 0, 0, 160 });
    DrawRectangleRounded((Rectangle){ infoPanelX, infoPanelY, infoPanelW, infoPanelH }, 0.04f, 4, (Color){ 20, 24, 34, 250 });
    DrawRectangleRoundedLines((Rectangle){ infoPanelX, infoPanelY, infoPanelW, infoPanelH }, 0.04f, 4, (Color){ 255, 255, 255, 45 });

    float animProgressEase = EaseOutCubic(carouselAnimProgress);
    float baseArtSize = 220.0f;
    float currentArtSize = baseArtSize * (0.94f + 0.06f * animProgressEase);
    float margin = 35.0f;
    float artX = infoPanelX + margin;
    float artY = infoPanelY + margin;

    DrawRectangleRounded((Rectangle){ artX + 5.0f, artY + 5.0f, currentArtSize, currentArtSize }, 0.06f, 4, (Color){ 0, 0, 0, 120 });

    if (selectedSongIndex < (int)g_jacketTextures.size() && g_jacketTextures[selectedSongIndex].id != 0) {
        DrawTexturePro(
            g_jacketTextures[selectedSongIndex],
            (Rectangle){ 0, 0, (float)g_jacketTextures[selectedSongIndex].width, (float)g_jacketTextures[selectedSongIndex].height },
            (Rectangle){ artX, artY, currentArtSize, currentArtSize },
            (Vector2){ 0, 0 }, 0.0f, WHITE
        );
    } else {
        DrawRectangleRounded((Rectangle){ artX, artY, currentArtSize, currentArtSize }, 0.06f, 4, (Color){ 35, 43, 58, 255 });
    }
    DrawRectangleRoundedLines((Rectangle){ artX, artY, currentArtSize, currentArtSize }, 0.06f, 4, WHITE);

    float titleX = artX + currentArtSize + 28.0f;
    float titleY = artY + 10.0f;

    if (fontLoaded) {
        DrawTextEx(suitFont, curSong.title.c_str(), (Vector2){ titleX, titleY }, 32.0f, 1.0f, WHITE);
        DrawTextEx(suitFont, curSong.artist.c_str(), (Vector2){ titleX, titleY + 45.0f }, 20.0f, 1.0f, (Color){ 170, 182, 200, 255 });
    } else {
        DrawText(curSong.title.c_str(), (int)titleX, (int)titleY, 28, WHITE);
        DrawText(curSong.artist.c_str(), (int)titleX, (int)(titleY + 40.0f), 18, (Color){ 170, 182, 200, 255 });
    }

    float metaStartY = artY + currentArtSize + 30.0f;
    float panelContentW = infoPanelW - (margin * 2.0f);
    DrawLine((int)artX, (int)(metaStartY - 15.0f), (int)(artX + panelContentW), (int)(metaStartY - 15.0f), (Color){ 255, 255, 255, 40 });

    float rowSpacing = 36.0f;

    auto DrawMetaRow = [&](const char* label, const std::string& value, float yOffset) {
        if (fontLoaded) {
            DrawTextEx(suitFont, label, (Vector2){ artX, metaStartY + yOffset }, 22.0f, 1.0f, (Color){ 160, 175, 195, 255 });
            DrawTextEx(suitFont, value.c_str(), (Vector2){ artX + 130.0f, metaStartY + yOffset }, 24.0f, 1.0f, (Color){ 240, 245, 255, 255 });
        } else {
            DrawText(label, (int)artX, (int)(metaStartY + yOffset), 20, (Color){ 160, 175, 195, 255 });
            DrawText(value.c_str(), (int)(artX + 130.0f), (int)(metaStartY + yOffset), 22, (Color){ 240, 245, 255, 255 });
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

    float btnHeight = 58.0f;
    float btnY = infoPanelY + infoPanelH - margin - btnHeight;
    Rectangle playBtnRect = { artX, btnY, panelContentW, btnHeight };
    
    float playPulse = 1.0f + 0.012f * std::sin((float)GetTime() * 8.0f);
    Rectangle pBtnScaled = { 
        playBtnRect.x - (playBtnRect.width * (playPulse - 1.0f)) / 2.0f, 
        playBtnRect.y - (playBtnRect.height * (playPulse - 1.0f)) / 2.0f, 
        playBtnRect.width * playPulse, 
        playBtnRect.height * playPulse 
    };

    DrawRectangleRounded((Rectangle){ playBtnRect.x + 4.0f, playBtnRect.y + 4.0f, playBtnRect.width, playBtnRect.height }, 0.20f, 4, (Color){ 0, 0, 0, 140 });
    DrawRectangleRounded(pBtnScaled, 0.20f, 4, WHITE);
    DrawRectangleRoundedLines(pBtnScaled, 0.20f, 4, (Color){ 220, 225, 235, 255 });

    const char* playStr = "엔터 누르면 시작 가능";
    float fontSize = 24.0f;
    if (fontLoaded) {
        Vector2 textSize = MeasureTextEx(suitFont, playStr, fontSize, 1.0f);
        float textX = pBtnScaled.x + (pBtnScaled.width - textSize.x) / 2.0f;
        float textY = pBtnScaled.y + (pBtnScaled.height - textSize.y) / 2.0f;
        DrawTextEx(suitFont, playStr, (Vector2){ textX, textY }, fontSize, 1.0f, (Color){ 15, 18, 25, 255 });
    } else {
        int textWidth = MeasureText(playStr, 22);
        int textX = (int)(pBtnScaled.x + (pBtnScaled.width - (float)textWidth) / 2.0f);
        int textY = (int)(pBtnScaled.y + (pBtnScaled.height - 22.0f) / 2.0f);
        DrawText(playStr, textX, textY, 22, (Color){ 15, 18, 25, 255 });
    }
}

void SongSelect::DrawInputHints(int screenWidth, int screenHeight) {
    const char* guideText = "[ W / S ] 곡 선택    |    [ ENTER ] 플레이    |    [ ESC ] 뒤로가기";
    
    DrawRectangle(0, screenHeight - 50, screenWidth, 50, (Color){ 10, 12, 16, 220 });
    DrawLine(0, screenHeight - 50, screenWidth, screenHeight - 50, (Color){ 255, 255, 255, 20 });

    if (fontLoaded) {
        DrawTextEx(suitFont, guideText, (Vector2){ 50.0f, (float)screenHeight - 36.0f }, 16.0f, 1.0f, (Color){ 200, 210, 225, 255 });
    } else {
        DrawText(guideText, 50, screenHeight - 36, 15, (Color){ 200, 210, 225, 255 });
    }
}

bool SongSelect::IsPlaySelected() const {
    return IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
}

bool SongSelect::IsEditorSelected() const {
    return IsKeyPressed(KEY_P);
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