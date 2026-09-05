#pragma once
#include <string>
#include <vector>
#include "raylib.h"

struct SaveNoteData {
    int lane;
    float posX;
};

class ChartSave {
public:
    static void SaveToJSON(const char* filename, const std::string& musicPath, const std::vector<SaveNoteData>& notes);
    static std::vector<std::string> GetSavedChartList();
    static bool LoadFromJSON(const char* filename, std::string& outMusicPath, std::vector<SaveNoteData>& outNotes);

    static void HandleChartInput(const std::string& musicPath, const std::vector<SaveNoteData>& notes, std::string& outMusicPath, std::vector<SaveNoteData>& outNotes, bool& fileLoaded);
    static void DrawChartSystemUI();
    static bool IsPopupOpen();
};