#include "audio_manager.h"
#include <iostream>

AudioManager::AudioManager() : fmodSystem(nullptr), currentSound(nullptr), currentChannel(nullptr) {}

AudioManager::~AudioManager() {
    release();
}

bool AudioManager::init() {
    FMOD_RESULT result = FMOD::System_Create(&fmodSystem);
    if (result != FMOD_OK) return false;

    result = fmodSystem->init(512, FMOD_INIT_NORMAL, nullptr);
    if (result != FMOD_OK) return false;

    return true;
}

void AudioManager::update() {
    if (fmodSystem) {
        fmodSystem->update();
    }
}

void AudioManager::loadSound(const std::string& path, const std::string& key) {
    fmodSystem->createSound(path.c_str(), FMOD_DEFAULT, nullptr, &currentSound);
}

void AudioManager::playSound(const std::string& key) {
    if (currentSound) {
        fmodSystem->playSound(currentSound, nullptr, false, &currentChannel);
    }
}

unsigned int AudioManager::getMusicPositionMs() const {
    if (!currentChannel) return 0;

    unsigned int ms = 0;
    currentChannel->getPosition(&ms, FMOD_TIMEUNIT_MS);
    return ms;
}

void AudioManager::release() {
    if (currentSound) currentSound->release();
    if (fmodSystem) {
        fmodSystem->close();
        fmodSystem->release();
    }
}