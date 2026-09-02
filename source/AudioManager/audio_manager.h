#pragma once
#include <fmod.hpp>
#include <string>

class AudioManager {
private:
    FMOD::System* system;
    FMOD::Sound* bgmSound;
    FMOD::Channel* bgmChannel;

    AudioManager();
    ~AudioManager();

public:
    static AudioManager& GetInstance() {
        static AudioManager instance;
        return instance;
    }

    void Init();
    void Update();
    void PlayBGM(const std::string& path);
    void StopBGM();
    void Release();

    unsigned int GetMusicPosition() const;
};