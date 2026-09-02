#pragma once
#include <vector>

struct SaveNoteData {
    int lane;
    float posX;
};

class ChartSave {
public:
    static void SaveToJSON(const char* filename, const std::vector<SaveNoteData>& notes);
};