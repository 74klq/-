#include "Selection.h"
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <vector>
#include <set>
#include <string>
#include <fmod.h>

static std::vector<Texture2D> g_jacketTextures;
static FMOD_SYSTEM* g_fmodSystem = nullptr;
static FMOD_SOUND* g_fmodSounds[15] = { nullptr };
static FMOD_CHANNEL* g_fmodChannel = nullptr;
static int g_currentPlayingSongIndex = -1;
static float g_songSwitchTimer = 0.0f;
static bool g_pendingSongSwitch = false;
static int g_pendingSongIndex = -1;

struct SongHighlight {
    unsigned int startMs;
    unsigned int durationMs;
};

static const SongHighlight g_songHighlights[15] = {
    { 95000, 12000 },
    { 99000, 12000 },
    { 73000, 12000 },
    { 164000, 12000 },
    { 76000, 12000 },
    { 99000, 12000 },
    { 22000, 12000 },
    { 76000, 12000 },
    { 118000, 12000 },
    { 511000, 12000 },
    { 170000, 12000 }, // 11번째 곡 (2분 50초)
    { 88000, 12000 },  // 12번째 곡 (1분 28초)
    { 123000, 12000 }, // 13번째 곡 (2분 3초)
    { 95000, 12000 },  // 14번째 곡 (1분 35초)
    { 234000, 12000 }  // 15번째 곡 (3분 54초)
};

static const char* g_musicPaths[15] = {
    "music/A_Night_Without_Visible_Stars.ogg",
    "music/kaleidoscope.ogg",
    "music/SkysCape.wav",
    "music/TheLostAria.ogg",
    "music/Timeline.ogg",
    "music/Terrasphere.ogg",
    "music/N.ogg",
    "music/doll.ogg",
    "music/R.ogg",
    "music/Megamix.ogg",
    "music/Maelstrom.ogg",
    "music/AAAA.ogg",
    "music/magic.ogg",
    "music/Stars_2021.ogg",
    "music/MemoryLocked.ogg"
};

static inline float EaseOutCubic(float x) {
    return 1.0f - std::pow(1.0f - x, 3.0f);
}

static inline float EaseInOutCubic(float x) {
    return x < 0.5f ? 4.0f * x * x * x : 1.0f - std::pow(-2.0f * x + 2.0f, 3.0f) / 2.0f;
}

SongSelect::SongSelect()
    : selectedSongIndex(0), 
      selectedDiffIndex(0), 
      previousSongIndex(0),
      animScrollOffset(0.0f), 
      targetScrollOffset(0.0f), 
      bgTransitionAlpha(1.0f), 
      carouselAnimProgress(1.0f), 
      fontLoaded(false),
      playRequested(false) 
{
}

SongSelect::~SongSelect() {
    if (g_fmodChannel) {
        FMOD_Channel_Stop(g_fmodChannel);
        g_fmodChannel = nullptr;
    }
    for (int i = 0; i < 15; ++i) {
        if (g_fmodSounds[i]) {
            FMOD_Sound_Release(g_fmodSounds[i]);
            g_fmodSounds[i] = nullptr;
        }
    }
    if (g_fmodSystem) {
        FMOD_System_Close(g_fmodSystem);
        FMOD_System_Release(g_fmodSystem);
        g_fmodSystem = nullptr;
    }

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

    if (!g_fmodSystem) {
        FMOD_System_Create(&g_fmodSystem, FMOD_VERSION);
        FMOD_System_Init(g_fmodSystem, 512, FMOD_INIT_NORMAL, 0);
    }

    for (int i = 0; i < 15; ++i) {
        if (!g_fmodSounds[i]) {
            FMOD_System_CreateSound(g_fmodSystem, g_musicPaths[i], FMOD_CREATESTREAM, 0, &g_fmodSounds[i]);
        }
    }
    
    for (auto& tex : g_jacketTextures) {
        if (tex.id != 0) UnloadTexture(tex);
    }
    g_jacketTextures.clear();

    std::vector<std::string> jacketPaths = {
        "music_assets/stars.png",
        "music_assets/kaleidoscope.png",
        "music_assets/SkysCape.png",
        "music_assets/TheLostAria.png",
        "music_assets/Timeline.png",
        "music_assets/Terrasphere.png",
        "music_assets/N.png",
        "music_assets/SecretDoll2.png",
        "music_assets/R.png",
        "music_assets/PlumMegamix.png",
        "music_assets/Maelstrom.png",
        "music_assets/Tempest.png",
        "music_assets/magic.png",
        "music_assets/Stars2021.png",
        "music_assets/MemoryLocked.png"
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
            "A Night Without Visible Stars",
            "Four Beat Sounds", "Mapper_A",
            "별이 보이지 않는 밤",
            "Plum",
            "Seta",
            "Kaleidoscope",
            "Timeline", "R",
            "Cybernetic Dream", 
            "Neo Synth", 
            "Mapper_C",
            "Neon Highway", 
            "Retro Pulse", 
            "Mapper_B",
            "작곡가:", 
            "에디터:", 
            "BPM:", 
            "길이:",
            "곡 선택", 
            "플레이", 
            "뒤로가기",
            "엔터 누르면 시작 가능,", 
            "N", 
            "SkysCape", 
            "Terrasphere",
            "Tempest", 
            "비밀인형극 II",
            "PLUM MEGAMIX",
            "The Lost Aria",
            "Maelstrom",
            "마법의 문구점",
            "Tempest",
            "MemoryLocked",
            "별이 보이지 않는밤 (2021리메이크)",
            "ほしのおとしもの",
            "ああああ"
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
    
    selectedDiffIndex = 0;
    animScrollOffset = (float)selectedSongIndex;
    targetScrollOffset = (float)selectedSongIndex;
    carouselAnimProgress = 1.0f;
    bgTransitionAlpha = 1.0f;
    playRequested = false;
    g_pendingSongSwitch = false;
    g_songSwitchTimer = 0.0f;

    if (g_fmodSystem && selectedSongIndex >= 0 && selectedSongIndex < 15 && g_fmodSounds[selectedSongIndex]) {
        FMOD_System_PlaySound(g_fmodSystem, g_fmodSounds[selectedSongIndex], 0, false, &g_fmodChannel);
        FMOD_Channel_SetPosition(g_fmodChannel, g_songHighlights[selectedSongIndex].startMs, FMOD_TIMEUNIT_MS);
        g_currentPlayingSongIndex = selectedSongIndex;
    }
}

void SongSelect::LoadSongs() {
    songs.clear();

    SongData song1;
    song1.title = "별이 보이지 않는 밤";
    song1.artist = "Plum";
    song1.mapper = "Seta";
    song1.bpm = 88.0f;
    song1.length = 153.0f;
    song1.cleared = true;
    song1.difficulties = { {"NORMAL", 7, 140.0f, 430, 890, 6.0f} };
    song1.osuFileName = "G.osu";
    song1.musicPlayerActive = 1; 
    song1.videoFileName = "";
    songs.push_back(song1);

    SongData song2;
    song2.title = "Kaleidoscope";
    song2.artist = "Plum";
    song2.mapper = "Seta";
    song2.bpm = 128.0f;
    song2.length = 306.0f;
    song2.cleared = false;
    song2.difficulties = { {"HARD", 10, 128.0f, 650, 1300, 6.0f} };
    song2.osuFileName = "Kaleidoscope.osu";  
    song2.musicPlayerActive = 2;
    song2.videoFileName = "";
    songs.push_back(song2);

    SongData song3;
    song3.title = "SkysCape";
    song3.artist = "Plum";
    song3.mapper = "Seta";
    song3.bpm = 138.0f;
    song3.length = 261.0f;
    song3.cleared = false;
    song3.difficulties = { {"HARD", 12, 138.0f, 850, 1700, 6.0f} };
    song3.osuFileName = "SkysCape.osu";          
    song3.musicPlayerActive = 3; 
    song3.videoFileName = "";
    songs.push_back(song3);

    SongData song4;
    song4.title = "The Lost Aria";
    song4.artist = "Plum";
    song4.mapper = "Seta";
    song4.bpm = 140.0f; 
    song4.length = 235.0f;
    song4.cleared = false;
    song4.difficulties = { {"HARD", 14, 140.0f, 1000, 2000, 6.0f} };
    song4.osuFileName = "TheLostAria.osu";          
    song4.musicPlayerActive = 4; 
    song4.videoFileName = "";
    songs.push_back(song4);

    SongData song5;
    song5.title = "Timeline";
    song5.artist = "Plum";
    song5.mapper = "Seta";
    song5.bpm = 155.0f;
    song5.length = 129.0f;
    song5.cleared = true;
    song5.difficulties = { {"EXPERT", 17, 155.0f, 1300, 2650, 6.5f} };
    song5.osuFileName = "Timeline.osu";   
    song5.musicPlayerActive = 5; 
    song5.videoFileName = "";
    songs.push_back(song5);
    
    SongData song6;
    song6.title = "Terrasphere";
    song6.artist = "Plum";
    song6.mapper = "Seta";
    song6.bpm = 160.0f;
    song6.length = 155.0f;
    song6.cleared = false;
    song6.difficulties = { {"EXPERT", 18, 160.0f, 1450, 2900, 6.5f} };
    song6.osuFileName = "Terrasphere.osu";          
    song6.musicPlayerActive = 6; 
    song6.videoFileName = "";
    songs.push_back(song6);

    SongData song7;
    song7.title = "N";
    song7.artist = "Plum";
    song7.mapper = "Seta";
    song7.bpm = 165.0f;
    song7.length = 255.0f;
    song7.cleared = false;
    song7.difficulties = { {"EXPERT", 19, 165.0f, 1600, 3200, 6.5f} };
    song7.osuFileName = "N.osu";          
    song7.musicPlayerActive = 7; 
    song7.videoFileName = "";
    songs.push_back(song7);

    SongData song8;
    song8.title = "비밀인형극 II";
    song8.artist = "Plum";
    song8.mapper = "Seta";
    song8.bpm = 170.0f;
    song8.length = 226.0f;
    song8.cleared = false;
    song8.difficulties = { {"MASTER", 20, 170.0f, 1750, 3500, 6.5f} };
    song8.osuFileName = "doll.osu";          
    song8.musicPlayerActive = 8; 
    song8.videoFileName = "bg_video/doll_video.mpg";
    songs.push_back(song8);

    SongData song9;
    song9.title = "R";
    song9.artist = "Plum";
    song9.mapper = "Seta";
    song9.bpm = 180.0f;
    song9.length = 147.0f;
    song9.cleared = false;
    song9.difficulties = { {"MASTER", 21, 180.0f, 1900, 3800, 6.5f} };
    song9.osuFileName = "R.osu";          
    song9.musicPlayerActive = 9; 
    song9.videoFileName = "";
    songs.push_back(song9);

    SongData song10;
    song10.title = "PLUM MEGAMIX";
    song10.artist = "Plum";
    song10.mapper = "Seta";
    song10.bpm = 175.0f;
    song10.length = 604.0f;
    song10.cleared = false;
    song10.difficulties = { {"HELL", 23, 175.0f, 2500, 5000, 7.0f} };
    song10.osuFileName = "PlumMegamix.osu";          
    song10.musicPlayerActive = 10; 
    song10.videoFileName = "";
    songs.push_back(song10);

    SongData song11;
    song11.title = "Maelstrom";
    song11.artist = "Plum";
    song11.mapper = "Seta";
    song11.bpm = 150.0f;
    song11.length = 120.0f;
    song11.cleared = false;
    song11.difficulties = { {"HARD", 15, 150.0f, 1200, 2400, 6.5f} };
    song11.osuFileName = "Maelstrom.osu";          
    song11.musicPlayerActive = 11;
    song11.videoFileName = "";
    songs.push_back(song11);

    SongData song12;
    song12.title = "ほしのおとしもの";
    song12.artist = "ああああ";
    song12.mapper = "Seta";
    song12.bpm = 150.0f;
    song12.length = 120.0f;
    song12.cleared = false;
    song12.difficulties = { {"HARD", 15, 150.0f, 1200, 2400, 6.5f} };
    song12.osuFileName = "AAAAA.osu";          
    song12.musicPlayerActive = 12;
    song12.videoFileName = "";
    songs.push_back(song12);

    SongData song13;
    song13.title = "마법의 문구점";
    song13.artist = "Plum";
    song13.mapper = "Seta";
    song13.bpm = 150.0f;
    song13.length = 120.0f;
    song13.cleared = false;
    song13.difficulties = { {"HARD", 15, 150.0f, 1200, 2400, 6.5f} };
    song13.osuFileName = "magic.osu";          
    song13.musicPlayerActive = 13;
    song13.videoFileName = "";
    songs.push_back(song13);

    SongData song14;
    song14.title = "별이 보이지 않는밤 (2021리메이크)";
    song14.artist = "Plum";
    song14.mapper = "Seta";
    song14.bpm = 150.0f;
    song14.length = 120.0f;
    song14.cleared = false;
    song14.difficulties = { {"HARD", 15, 150.0f, 1200, 2400, 6.5f} };
    song14.osuFileName = "new_song.osu";          
    song14.musicPlayerActive = 14;
    song14.videoFileName = "";
    songs.push_back(song14);

    SongData song15;
    song15.title = "MemoryLocked";
    song15.artist = "Plum";
    song15.mapper = "Seta";
    song15.bpm = 150.0f;
    song15.length = 120.0f;
    song15.cleared = false;
    song15.difficulties = { {"HARD", 15, 150.0f, 1200, 2400, 6.5f} };
    song15.osuFileName = "new_song.osu";          
    song15.musicPlayerActive = 15;
    song15.videoFileName = "";
    songs.push_back(song15);
}

void SongSelect::Update() {
    SetMouseScale((float)1280 / GetScreenWidth(), (float)720 / GetScreenHeight());

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

    int prevIndex = selectedSongIndex;

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

    if (selectedSongIndex != prevIndex) {
        g_pendingSongSwitch = true;
        g_songSwitchTimer = 0.0f;
        g_pendingSongIndex = selectedSongIndex;
        if (g_fmodChannel) {
            FMOD_Channel_Stop(g_fmodChannel);
            g_fmodChannel = nullptr;
        }
    }

    if (g_pendingSongSwitch) {
        g_songSwitchTimer += dt;
        if (g_songSwitchTimer >= 0.5f) {
            if (g_fmodSystem && g_pendingSongIndex >= 0 && g_pendingSongIndex < 15 && g_fmodSounds[g_pendingSongIndex]) {
                if (g_fmodChannel) {
                    FMOD_Channel_Stop(g_fmodChannel);
                    g_fmodChannel = nullptr;
                }
                FMOD_System_PlaySound(g_fmodSystem, g_fmodSounds[g_pendingSongIndex], 0, false, &g_fmodChannel);
                FMOD_Channel_SetPosition(g_fmodChannel, g_songHighlights[g_pendingSongIndex].startMs, FMOD_TIMEUNIT_MS);
                g_currentPlayingSongIndex = g_pendingSongIndex;
            }
            g_pendingSongSwitch = false;
        }
    }

    if (g_fmodSystem) {
        FMOD_System_Update(g_fmodSystem);
    }

    if (g_fmodChannel && g_currentPlayingSongIndex >= 0 && g_currentPlayingSongIndex < 15) {
        unsigned int pos = 0;
        FMOD_Channel_GetPosition(g_fmodChannel, &pos, FMOD_TIMEUNIT_MS);
        unsigned int startMs = g_songHighlights[g_currentPlayingSongIndex].startMs;
        unsigned int durationMs = g_songHighlights[g_currentPlayingSongIndex].durationMs;
        if (pos >= startMs + durationMs || pos < startMs) {
            FMOD_Channel_SetPosition(g_fmodChannel, startMs, FMOD_TIMEUNIT_MS);
        }
    }

    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
        playRequested = true;
        if (g_fmodChannel) {
            FMOD_Channel_Stop(g_fmodChannel);
            g_fmodChannel = nullptr;
        }
    }
}

float SongSelect::GetSongCardY(int index, float centerY) const {
    float uiScale = (float)GetScreenHeight() / 720.0f;
    float relativePos = (float)index - animScrollOffset;
    return centerY + relativePos * (145.0f * uiScale);
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
    DrawRectangle(0, 0, screenWidth, screenHeight, (Color){ 12, 14, 20, 255 });

    float time = (float)GetTime();
    float scale = 1.10f + 0.02f * std::sin(time * 0.3f);
    float rotation = 1.2f * std::sin(time * 0.15f); 
    
    float panX = 18.0f * std::cos(time * 0.25f);
    float panY = 14.0f * std::sin(time * 0.20f);

    float bgW = screenWidth * scale;
    float bgH = screenHeight * scale;

    Vector2 origin = { bgW * 0.5f, bgH * 0.5f };
    Rectangle destRect = {
        (screenWidth * 0.5f) + panX,
        (screenHeight * 0.5f) + panY,
        bgW,
        bgH
    };

    float maxBgAlpha = 0.28f;

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

    DrawRectangleGradientV(0, 0, screenWidth, screenHeight, (Color){ 20, 24, 34, 130 }, (Color){ 8, 9, 14, 180 });
    
    for (int i = -screenWidth; i < screenWidth + screenHeight; i += 80) {
        DrawLine(i, 0, i + screenHeight, screenHeight, (Color){ 255, 255, 255, 6 });
    }
}

void SongSelect::DrawSongCarousel(int screenWidth, int screenHeight) {
    float uiScale = (float)screenHeight / 720.0f;
    float centerX = (float)screenWidth * 0.30f;
    float centerY = (float)screenHeight * 0.48f;

    for (int i = 0; i < (int)songs.size(); ++i) {
        float cardY = GetSongCardY(i, centerY);
        float scale = GetSongCardScale(i);
        float alpha = GetSongCardAlpha(i);

        if (cardY < -160.0f * uiScale || cardY > (float)screenHeight + 160.0f * uiScale) continue;

        bool isCurrent = (i == selectedSongIndex);

        float cardWidth = 380.0f * scale * uiScale;
        float cardHeight = 120.0f * scale * uiScale;
        float cardX = centerX - cardWidth / 2.0f;

        Color cardBg = isCurrent ? (Color){ 32, 38, 52, 245 } : (Color){ 18, 21, 28, (unsigned char)(210 * alpha) };
        Color borderColor = isCurrent ? (Color){ 255, 255, 255, 255 } : (Color){ 255, 255, 255, (unsigned char)(40 * alpha) };

        DrawRectangleRounded((Rectangle){ cardX + 5.0f * uiScale, cardY + 5.0f * uiScale, cardWidth, cardHeight }, 0.12f, 4, (Color){ 0, 0, 0, (unsigned char)(140 * alpha) });

        if (isCurrent) {
            float pulse = 1.0f + 0.012f * std::sin((float)GetTime() * 8.0f);
            float pWidth = cardWidth * pulse;
            float pHeight = cardHeight * pulse;
            float pX = cardX - (pWidth - cardWidth) / 2.0f;
            float pY = cardY - (pHeight - cardHeight) / 2.0f;
            
            DrawRectangleRounded((Rectangle){ pX, pY, pWidth, pHeight }, 0.12f, 4, cardBg);
            DrawRectangleRoundedLines((Rectangle){ pX, pY, pWidth, pHeight }, 0.12f, 4, WHITE);
            DrawRectangleRounded((Rectangle){ pX, pY, 8.0f * uiScale, pHeight }, 0.25f, 4, WHITE);
        } else {
            DrawRectangleRounded((Rectangle){ cardX, cardY, cardWidth, cardHeight }, 0.12f, 4, cardBg);
            DrawRectangleRoundedLines((Rectangle){ cardX, cardY, cardWidth, cardHeight }, 0.12f, 4, borderColor);
        }

        float jacketSize = cardHeight - 24.0f * scale * uiScale;
        float jacketX = cardX + 18.0f * scale * uiScale;
        float jacketY = cardY + 12.0f * scale * uiScale;
        
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

        float textX = jacketX + jacketSize + 16.0f * scale * uiScale;
        
        if (fontLoaded) {
            DrawTextEx(suitFont, songs[i].title.c_str(), (Vector2){ textX, jacketY + 10.0f * scale * uiScale }, 20.0f * scale * uiScale, 1.0f, titleColor);
            DrawTextEx(suitFont, songs[i].artist.c_str(), (Vector2){ textX, jacketY + 44.0f * scale * uiScale }, 15.0f * scale * uiScale, 1.0f, artistColor);
        } else {
            DrawText(songs[i].title.c_str(), (int)textX, (int)(jacketY + 10.0f * scale * uiScale), (int)(18 * scale * uiScale), titleColor);
            DrawText(songs[i].artist.c_str(), (int)textX, (int)(jacketY + 44.0f * scale * uiScale), (int)(14 * scale * uiScale), artistColor);
        }
    }
}

void SongSelect::DrawCurrentSongInfo(int screenWidth, int screenHeight) {
    float uiScale = (float)screenHeight / 720.0f;
    const SongData& curSong = songs[selectedSongIndex];

    float infoPanelX = (float)screenWidth * 0.52f;
    float infoPanelY = (float)screenHeight * 0.10f;
    float infoPanelW = (float)screenWidth * 0.44f;
    float infoPanelH = (float)screenHeight * 0.78f;

    DrawRectangleRounded((Rectangle){ infoPanelX + 8.0f * uiScale, infoPanelY + 8.0f * uiScale, infoPanelW, infoPanelH }, 0.04f, 4, (Color){ 0, 0, 0, 160 });
    DrawRectangleRounded((Rectangle){ infoPanelX, infoPanelY, infoPanelW, infoPanelH }, 0.04f, 4, (Color){ 20, 24, 34, 250 });
    DrawRectangleRoundedLines((Rectangle){ infoPanelX, infoPanelY, infoPanelW, infoPanelH }, 0.04f, 4, (Color){ 255, 255, 255, 45 });

    float animProgressEase = EaseOutCubic(carouselAnimProgress);
    float baseArtSize = 220.0f * uiScale;
    float currentArtSize = baseArtSize * (0.94f + 0.06f * animProgressEase);
    float margin = 35.0f * uiScale;
    float artX = infoPanelX + margin;
    float artY = infoPanelY + margin;

    DrawRectangleRounded((Rectangle){ artX + 5.0f * uiScale, artY + 5.0f * uiScale, currentArtSize, currentArtSize }, 0.06f, 4, (Color){ 0, 0, 0, 120 });

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

    float titleX = artX + currentArtSize + 28.0f * uiScale;
    float titleY = artY + 10.0f * uiScale;

    if (fontLoaded) {
        DrawTextEx(suitFont, curSong.title.c_str(), (Vector2){ titleX, titleY }, 32.0f * uiScale, 1.0f, WHITE);
        DrawTextEx(suitFont, curSong.artist.c_str(), (Vector2){ titleX, titleY + 45.0f * uiScale }, 20.0f * uiScale, 1.0f, (Color){ 170, 182, 200, 255 });
    } else {
        DrawText(curSong.title.c_str(), (int)titleX, (int)titleY, (int)(28 * uiScale), WHITE);
        DrawText(curSong.artist.c_str(), (int)titleX, (int)(titleY + 40.0f * uiScale), (int)(18 * uiScale), (Color){ 170, 182, 200, 255 });
    }

    float metaStartY = artY + currentArtSize + 30.0f * uiScale;
    float panelContentW = infoPanelW - (margin * 2.0f);
    DrawLine((int)artX, (int)(metaStartY - 15.0f * uiScale), (int)(artX + panelContentW), (int)(metaStartY - 15.0f * uiScale), (Color){ 255, 255, 255, 40 });

    float rowSpacing = 36.0f * uiScale;

    auto DrawMetaRow = [&](const char* label, const std::string& value, float yOffset) {
        if (fontLoaded) {
            DrawTextEx(suitFont, label, (Vector2){ artX, metaStartY + yOffset }, 22.0f * uiScale, 1.0f, (Color){ 160, 175, 195, 255 });
            DrawTextEx(suitFont, value.c_str(), (Vector2){ artX + 130.0f * uiScale, metaStartY + yOffset }, 24.0f * uiScale, 1.0f, (Color){ 240, 245, 255, 255 });
        } else {
            DrawText(label, (int)artX, (int)(metaStartY + yOffset), (int)(20 * uiScale), (Color){ 160, 175, 195, 255 });
            DrawText(value.c_str(), (int)(artX + 130.0f * uiScale), (int)(metaStartY + yOffset), (int)(22 * uiScale), (Color){ 240, 245, 255, 255 });
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

    float btnHeight = 58.0f * uiScale;
    float btnY = infoPanelY + infoPanelH - margin - btnHeight;
    Rectangle playBtnRect = { artX, btnY, panelContentW, btnHeight };
    
    float playPulse = 1.0f + 0.012f * std::sin((float)GetTime() * 8.0f);
    Rectangle pBtnScaled = { 
        playBtnRect.x - (playBtnRect.width * (playPulse - 1.0f)) / 2.0f, 
        playBtnRect.y - (playBtnRect.height * (playPulse - 1.0f)) / 2.0f, 
        playBtnRect.width * playPulse, 
        playBtnRect.height * playPulse 
    };

    DrawRectangleRounded((Rectangle){ playBtnRect.x + 4.0f * uiScale, playBtnRect.y + 4.0f * uiScale, playBtnRect.width, playBtnRect.height }, 0.20f, 4, (Color){ 0, 0, 0, 140 });
    DrawRectangleRounded(pBtnScaled, 0.20f, 4, WHITE);
    DrawRectangleRoundedLines(pBtnScaled, 0.20f, 4, (Color){ 220, 225, 235, 255 });

    const char* playStr = "엔터 누르면 시작 가능";
    float fontSize = 24.0f * uiScale;
    if (fontLoaded) {
        Vector2 textSize = MeasureTextEx(suitFont, playStr, fontSize, 1.0f);
        float textX = pBtnScaled.x + (pBtnScaled.width - textSize.x) / 2.0f;
        float textY = pBtnScaled.y + (pBtnScaled.height - textSize.y) / 2.0f;
        DrawTextEx(suitFont, playStr, (Vector2){ textX, textY }, fontSize, 1.0f, (Color){ 15, 18, 25, 255 });
    } else {
        int textWidth = MeasureText(playStr, (int)(22 * uiScale));
        int textX = (int)(pBtnScaled.x + (pBtnScaled.width - (float)textWidth) / 2.0f);
        int textY = (int)(pBtnScaled.y + (pBtnScaled.height - 22.0f * uiScale) / 2.0f);
        DrawText(playStr, textX, textY, (int)(22 * uiScale), (Color){ 15, 18, 25, 255 });
    }
}

void SongSelect::DrawInputHints(int screenWidth, int screenHeight) {
    float uiScale = (float)screenHeight / 720.0f;
    const char* guideText = "[ W / S ] 곡 선택    |    [ ENTER ] 플레이    |    [ ESC ] 뒤로가기";
    
    DrawRectangle(0, screenHeight - (int)(50 * uiScale), screenWidth, (int)(50 * uiScale), (Color){ 10, 12, 16, 220 });
    DrawLine(0, screenHeight - (int)(50 * uiScale), screenWidth, screenHeight - (int)(50 * uiScale), (Color){ 255, 255, 255, 20 });

    if (fontLoaded) {
        DrawTextEx(suitFont, guideText, (Vector2){ 50.0f * uiScale, (float)screenHeight - (36.0f * uiScale) }, 16.0f * uiScale, 1.0f, (Color){ 200, 210, 225, 255 });
    } else {
        DrawText(guideText, (int)(50 * uiScale), screenHeight - (int)(36 * uiScale), (int)(15 * uiScale), (Color){ 200, 210, 225, 255 });
    }
}

bool SongSelect::IsPlaySelected() const {
    return playRequested;
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

void SongSelect::ResetPlayRequest() {
    playRequested = false;
}