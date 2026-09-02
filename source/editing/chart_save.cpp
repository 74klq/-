#include "chart_save.h"
#include <fstream>
#include <filesystem>

void ChartSave::SaveToJSON(const char* filename, const std::vector<SaveNoteData>& notes) {
    std::string dirPath = "map_data";
    if (!std::filesystem::exists(dirPath)) {
        std::filesystem::create_directory(dirPath);
    }

    std::string fullPath = dirPath + "/" + filename;
    std::ofstream file(fullPath);
    if (!file.is_open()) {
        return;
    }

    file << "{\n";
    file << "  \"noteCount\": " << notes.size() << ",\n";
    file << "  \"notes\": [\n";
    
    for (size_t i = 0; i < notes.size(); ++i) {
        file << "    { \"lane\": " << notes[i].lane << ", \"posX\": " << notes[i].posX << " }";
        if (i < notes.size() - 1) {
            file << ",";
        }
        file << "\n";
    }
    
    file << "  ]\n";
    file << "}\n";

    file.close();
}