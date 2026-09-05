#pragma once

#include "raylib.h"
#include <vector>
#include <string>

struct DifficultyData {
    std::string diffName;
    int level;
    float bpm;
    int noteCount;
    int maxCombo;
    float difficultyRating;
};

struct SongData {
    std::string title;
    std::string artist;
    std::string mapper;
    float bpm;        // <- 여기에 bpm 추가
    float length;
    bool cleared;
    std::vector<DifficultyData> difficulties;
};

class SongSelect {
private:
    std::vector<SongData> songs;
    int selectedSongIndex;
    int selectedDiffIndex;
    int previousSongIndex;
    
    float animScrollOffset;
    float targetScrollOffset;
    float bgTransitionAlpha;
    float carouselAnimProgress;

    bool fontLoaded;
    Font suitFont;

    void DrawBackground(int screenWidth, int screenHeight);
    void DrawSongCarousel(int screenWidth, int screenHeight);
    void DrawCurrentSongInfo(int screenWidth, int screenHeight);
    void DrawInputHints(int screenWidth, int screenHeight);

    float GetSongCardY(int index, float centerY) const;
    float GetSongCardScale(int index) const;
    float GetSongCardAlpha(int index) const;

public:
    SongSelect();
    ~SongSelect();

    void Init();
    void LoadSongs();
    void Update();
    void Draw(int screenWidth, int screenHeight);

    bool IsPlaySelected() const;
    bool IsBackSelected() const;

    const SongData& GetCurrentSong() const;
    int GetCurrentDifficultyIndex() const;
};