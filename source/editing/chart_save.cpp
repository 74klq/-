#include "chart_save.h"
#include <fstream>
#include <sstream>
#include <filesystem>

static bool s_IsSaveOpen = false;
static bool s_IsLoadOpen = false;
static std::string s_InputString = "";
static Font s_Font = { 0 };
static bool s_FontLoaded = false;
static int s_KeyCooldown = 0;

bool ChartSave::IsPopupOpen() {
    return s_IsSaveOpen || s_IsLoadOpen;
}

void ChartSave::SaveToJSON(const char* filename, const std::string& musicPath, const std::vector<SaveNoteData>& notes) {
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
    file << "  \"musicPath\": \"" << musicPath << "\",\n";
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

std::vector<std::string> ChartSave::GetSavedChartList() {
    std::vector<std::string> fileList;
    std::string dirPath = "map_data";
    if (std::filesystem::exists(dirPath)) {
        for (const auto& entry : std::filesystem::directory_iterator(dirPath)) {
            if (entry.path().extension() == ".json") {
                fileList.push_back(entry.path().filename().string());
            }
        }
    }
    return fileList;
}

bool ChartSave::LoadFromJSON(const char* filename, std::string& outMusicPath, std::vector<SaveNoteData>& outNotes) {
    std::string fullPath = std::string("map_data/") + filename;
    std::ifstream file(fullPath);
    if (!file.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();

    outNotes.clear();
    
    size_t musicPos = content.find("\"musicPath\"");
    if (musicPos != std::string::npos) {
        size_t start = content.find("\"", musicPos + 11);
        if (start != std::string::npos) {
            size_t end = content.find("\"", start + 1);
            if (end != std::string::npos) {
                outMusicPath = content.substr(start + 1, end - start - 1);
            }
        }
    }

    size_t notesPos = content.find("\"notes\"");
    if (notesPos != std::string::npos) {
        size_t current = notesPos;
        while (true) {
            size_t lanePos = content.find("\"lane\"", current);
            if (lanePos == std::string::npos) break;

            size_t colonLane = content.find(":", lanePos);
            int lane = std::stoi(content.substr(colonLane + 1));

            size_t posXPos = content.find("\"posX\"", lanePos);
            size_t colonPosX = content.find(":", posXPos);
            float posX = std::stof(content.substr(colonPosX + 1));

            outNotes.push_back({lane, posX});
            current = posXPos + 1;
        }
    }

    return true;
}

void ChartSave::HandleChartInput(const std::string& musicPath, const std::vector<SaveNoteData>& notes, std::string& outMusicPath, std::vector<SaveNoteData>& outNotes, bool& fileLoaded) {
    if (!s_FontLoaded) {
        if (FileExists("fonts/Pretendard-Black.ttf")) {
            std::vector<std::string> targetTexts = {
                "저장할 이름:", "여기에 저장할 이름을 입력하세요."
            };

            std::vector<int> codepointsList;
            for (int i = 0; i < 95; i++) {
                codepointsList.push_back(32 + i);
            }

            for (const auto& text : targetTexts) {
                for (size_t i = 0; i < text.length(); ) {
                    unsigned int codepoint = 0;
                    int utf8Size = 0;
                    unsigned char c = text[i];
                    
                    if (c < 0x80) {
                        codepoint = c;
                        utf8Size = 1;
                    } else if ((c & 0xE0) == 0xC0) {
                        codepoint = ((c & 0x1F) << 6) | (text[i + 1] & 0x3F);
                        utf8Size = 2;
                    } else if ((c & 0xF0) == 0xE0) {
                        codepoint = ((c & 0x0F) << 12) | ((text[i + 1] & 0x3F) << 6) | (text[i + 2] & 0x3F);
                        utf8Size = 3;
                    } else if ((c & 0xF8) == 0xF0) {
                        codepoint = ((c & 0x07) << 18) | ((text[i + 1] & 0x3F) << 12) | ((text[i + 2] & 0x3F) << 6) | (text[i + 3] & 0x3F);
                        utf8Size = 4;
                    } else {
                        utf8Size = 1;
                    }

                    if (codepoint != 0) {
                        bool exists = false;
                        for (int cp : codepointsList) {
                            if (cp == (int)codepoint) {
                                exists = true;
                                break;
                            }
                        }
                        if (!exists) {
                            codepointsList.push_back((int)codepoint);
                        }
                    }
                    i += utf8Size;
                }
            }

            std::vector<std::string> charts = GetSavedChartList();
            for (const auto& chart : charts) {
                for (size_t i = 0; i < chart.length(); ) {
                    unsigned int codepoint = 0;
                    int utf8Size = 0;
                    unsigned char c = chart[i];
                    
                    if (c < 0x80) {
                        codepoint = c;
                        utf8Size = 1;
                    } else if ((c & 0xE0) == 0xC0) {
                        codepoint = ((c & 0x1F) << 6) | (chart[i + 1] & 0x3F);
                        utf8Size = 2;
                    } else if ((c & 0xF0) == 0xE0) {
                        codepoint = ((c & 0x0F) << 12) | ((chart[i + 1] & 0x3F) << 6) | (chart[i + 2] & 0x3F);
                        utf8Size = 3;
                    } else if ((c & 0xF8) == 0xF0) {
                        codepoint = ((c & 0x07) << 18) | ((chart[i + 1] & 0x3F) << 12) | ((chart[i + 2] & 0x3F) << 6) | (chart[i + 3] & 0x3F);
                        utf8Size = 4;
                    } else {
                        utf8Size = 1;
                    }

                    if (codepoint != 0) {
                        bool exists = false;
                        for (int cp : codepointsList) {
                            if (cp == (int)codepoint) {
                                exists = true;
                                break;
                            }
                        }
                        if (!exists) {
                            codepointsList.push_back((int)codepoint);
                        }
                    }
                    i += utf8Size;
                }
            }

            for (int i = 0; i < 26; i++) {
                int upper = 65 + i;
                int lower = 97 + i;
                bool hasU = false, hasL = false;
                for (int cp : codepointsList) {
                    if (cp == upper) hasU = true;
                    if (cp == lower) hasL = true;
                }
                if (!hasU) codepointsList.push_back(upper);
                if (!hasL) codepointsList.push_back(lower);
            }

            s_Font = LoadFontEx("fonts/Pretendard-Black.ttf", 32, codepointsList.data(), codepointsList.size());
        }
        s_FontLoaded = true;
    }

    if (s_KeyCooldown > 0) {
        s_KeyCooldown--;
    }

    if (!s_IsLoadOpen && IsKeyPressed(KEY_B) && s_KeyCooldown == 0) {
        s_IsSaveOpen = !s_IsSaveOpen;
        s_InputString.clear();
        s_KeyCooldown = 10;
    }

    if (!s_IsSaveOpen && IsKeyPressed(KEY_TWO) && s_KeyCooldown == 0) {
        s_IsLoadOpen = !s_IsLoadOpen;
        s_KeyCooldown = 10;
    }

    if (s_IsSaveOpen) {
        int key = GetCharPressed();
        while (key > 0) {
            if ((key >= 32) && (key <= 125)) {
                s_InputString.push_back((char)key);
            }
            key = GetCharPressed();
        }

        if (IsKeyPressed(KEY_BACKSPACE)) {
            if (!s_InputString.empty()) {
                s_InputString.pop_back();
            }
        }

        if (IsKeyPressed(KEY_ENTER)) {
            if (!s_InputString.empty()) {
                std::string filename = s_InputString;
                if (filename.find(".json") == std::string::npos) {
                    filename += ".json";
                }
                SaveToJSON(filename.c_str(), musicPath, notes);
                s_IsSaveOpen = false;
                s_InputString.clear();
                s_KeyCooldown = 10;
            }
        }

        if (IsKeyPressed(KEY_ESCAPE)) {
            s_IsSaveOpen = false;
            s_KeyCooldown = 10;
        }
        return;
    }

    if (s_IsLoadOpen) {
        if (IsKeyPressed(KEY_ESCAPE)) {
            s_IsLoadOpen = false;
            s_KeyCooldown = 10;
            return;
        }

        std::vector<std::string> charts = GetSavedChartList();
        int screenWidth = GetScreenWidth();
        int screenHeight = GetScreenHeight();
        Vector2 mousePos = GetMousePosition();

        float boxWidth = 400.f;
        float boxHeight = 50.f + charts.size() * 45.f;
        if (boxHeight < 100.f) boxHeight = 100.f;
        if (boxHeight > 500.f) boxHeight = 500.f;

        Rectangle box = { (float)screenWidth / 2.f - boxWidth / 2.f, (float)screenHeight / 2.f - boxHeight / 2.f, boxWidth, boxHeight };

        float startY = box.y + 25.f;
        for (size_t i = 0; i < charts.size(); ++i) {
            Rectangle itemBox = { box.x + 20.f, startY + (float)(i * 45.f), boxWidth - 40.f, 35.f };
            if (CheckCollisionPointRec(mousePos, itemBox)) {
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                    if (LoadFromJSON(charts[i].c_str(), outMusicPath, outNotes)) {
                        fileLoaded = true;
                        s_IsLoadOpen = false;
                        s_KeyCooldown = 10;
                    }
                }
            }
        }
        return;
    }
}

void ChartSave::DrawChartSystemUI() {
    if (!s_IsSaveOpen && !s_IsLoadOpen) return;

    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    Vector2 mousePos = GetMousePosition();

    DrawRectangle(0, 0, screenWidth, screenHeight, Fade(BLACK, 0.6f));

    if (s_IsSaveOpen) {
        Rectangle box = { (float)screenWidth / 2.f - 250.f, (float)screenHeight / 2.f - 50.f, 500.f, 100.f };
        DrawRectangleRounded(box, 0.3f, 8, RAYWHITE);
        DrawRectangleRoundedLines(box, 0.3f, 8, LIGHTGRAY);

        DrawTextEx(s_Font, "저장할 이름:", Vector2{ box.x + 25.f, box.y + 35.f }, 20, 1, DARKGRAY);

        std::string displayText;
        Color textColor = DARKGRAY;
        if (s_InputString.empty()) {
            displayText = "여기에 저장할 이름을 입력하세요.";
            textColor = LIGHTGRAY;
        } else {
            displayText = s_InputString + "|";
        }

        Vector2 labelSize = MeasureTextEx(s_Font, "저장할 이름: ", 20, 1);
        DrawTextEx(s_Font, displayText.c_str(), Vector2{ box.x + 25.f + labelSize.x, box.y + 35.f }, 20, 1, textColor);
    }

    if (s_IsLoadOpen) {
        std::vector<std::string> charts = GetSavedChartList();

        float boxWidth = 400.f;
        float boxHeight = 50.f + charts.size() * 45.f;
        if (boxHeight < 100.f) boxHeight = 100.f;
        if (boxHeight > 500.f) boxHeight = 500.f;

        Rectangle box = { (float)screenWidth / 2.f - boxWidth / 2.f, (float)screenHeight / 2.f - boxHeight / 2.f, boxWidth, boxHeight };
        DrawRectangleRounded(box, 0.15f, 8, RAYWHITE);
        DrawRectangleRoundedLines(box, 0.15f, 8, LIGHTGRAY);

        float startY = box.y + 25.f;
        for (size_t i = 0; i < charts.size(); ++i) {
            Rectangle itemBox = { box.x + 20.f, startY + (float)(i * 45.f), boxWidth - 40.f, 35.f };
            bool isHovered = CheckCollisionPointRec(mousePos, itemBox);

            Color itemBg = isHovered ? Color{ 230, 240, 255, 255 } : WHITE;
            Rectangle drawItemBox = itemBox;

            if (isHovered) {
                drawItemBox.x -= 3.f;
                drawItemBox.y -= 1.5f;
                drawItemBox.width += 6.f;
                drawItemBox.height += 3.f;
            }

            DrawRectangleRounded(drawItemBox, 0.3f, 4, itemBg);
            DrawRectangleRoundedLines(drawItemBox, 0.3f, 4, LIGHTGRAY);

            DrawTextEx(s_Font, charts[i].c_str(), Vector2{ drawItemBox.x + 15.f, drawItemBox.y + 7.f }, 20, 1, DARKGRAY);
        }
    }
}