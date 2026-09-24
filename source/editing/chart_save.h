#pragma once
#include <string>
#include <vector>
#include "raylib.h"

struct SaveNoteData
{
    int lane;
    int time;
    int type;
    int endTime;
};

class ChartSave
{
public:
static bool SaveToOsu(const char* filename,
                      const std::string& audioFile,
                      const std::string& title,
                      const std::string& artist,
                      const std::string& creator,
                      const std::string& version,
                      float hp,
                      float od,
                      float cs,
                      float ar,
                      float sliderMultiplier,
                      float sliderTickRate,
                      float bpm,
                      const std::vector<SaveNoteData>& notes);


    static bool LoadFromOsu(const char* filename,
                            std::string& outAudioFile,
                            std::string& outTitle,
                            std::string& outArtist,
                            std::string& outCreator,
                            std::string& outVersion,
                            float& outHP,
                            float& outOD,
                            float& outCS,
                            float& outAR,
                            float& outSliderMultiplier,
                            float& outSliderTickRate,
                            std::vector<SaveNoteData>& outNotes);

    static bool LoadFromOsu(const char* filename,
                            std::string& outAudioFile,
                            std::string& outTitle,
                            std::string& outArtist,
                            std::string& outCreator,
                            std::string& outVersion,
                            float& outHP,
                            float& outOD,
                            float& outCS,
                            float& outAR,
                            float& outSliderMultiplier,
                            float& outSliderTickRate,
                            float& outBpm,
                            float& outOffset,
                            std::vector<SaveNoteData>& outNotes);

    static std::vector<std::string> GetSavedChartList();

    static void HandleChartInput(const std::string& audioFile,
                                 const std::string& title,
                                 const std::string& artist,
                                 const std::string& creator,
                                 const std::string& version,
                                 float hp,
                                 float od,
                                 float cs,
                                 float ar,
                                 float sliderMultiplier,
                                 float sliderTickRate,
                                 const std::vector<SaveNoteData>& notes,
                                 std::string& outAudioFile,
                                 std::string& outTitle,
                                 std::string& outArtist,
                                 std::string& outCreator,
                                 std::string& outVersion,
                                 float& outHP,
                                 float& outOD,
                                 float& outCS,
                                 float& outAR,
                                 float& outSliderMultiplier,
                                 float& outSliderTickRate,
                                 float& outBpm,
                                 float& outOffset,
                                 float& scrollSpeed,
                                 std::vector<SaveNoteData>& outNotes,
                                 bool& fileLoaded);

    static void DrawChartSystemUI();

    static bool IsPopupOpen();
};