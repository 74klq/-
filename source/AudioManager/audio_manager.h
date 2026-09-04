#pragma once
#include <fmod.hpp>
#include <string>

class AudioManager {
public:
    AudioManager();
    ~AudioManager();

    bool init();
    void update();

    void loadSound(const std::string& path, const std::string& key);
    void playSound(const std::string& key);

    unsigned int getMusicPositionMs() const;

    void release();

private:
    FMOD::System* fmodSystem;
    FMOD::Sound* currentSound;
    FMOD::Channel* currentChannel;
};