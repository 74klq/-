#ifndef SELECTION_H
#define SELECTION_H

#include "raylib.h"
#include <string>
#include <vector>

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
    float length;
    bool cleared;
    std::vector<DifficultyData> difficulties;
};

class SongSelect {
public:
    SongSelect();
    ~SongSelect();

    void Init();
    void Update();
    void Draw(int screenWidth, int screenHeight);
    
    bool IsPlaySelected() const;
    bool IsBackSelected() const;
    
    const SongData& GetCurrentSong() const;
    int GetCurrentDifficultyIndex() const;

private:
    std::vector<SongData> songs;
    int selectedSongIndex;
    int selectedDiffIndex;

    float animScrollOffset;
    float targetScrollOffset;
    float bgTransitionAlpha;
    int previousSongIndex;
    
    Font suitFont;
    bool fontLoaded;
    
    void LoadSongs();
};

#endif