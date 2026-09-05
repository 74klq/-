#pragma once
#include <vector>
#include <string>

struct SaveNoteData {
    int lane;
    float posX;
};

class ChartSave {
public:
    static void SaveToJSON(const char* filename, const std::string& musicPath, const std::vector<SaveNoteData>& notes);
};