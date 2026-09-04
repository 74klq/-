#pragma once

#include <fmod.h>
#include <string>
#include <unordered_map>

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    bool Init();
    void Update();
    bool CreateSound(const std::string& name, const std::string& filePath);
    bool CreateStream(const std::string& name, const std::string& filePath);
    void PlaySoundWithDelay(const std::string& name, unsigned int delayMs);
    void PlayPreview(const std::string& name, unsigned int startMs);
    void StopPreview();

private:
    FMOD_SYSTEM* m_System;
    std::unordered_map<std::string, FMOD_SOUND*> m_Sounds;
    FMOD_CHANNEL* m_PreviewChannel;
};