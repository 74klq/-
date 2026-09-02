#include "audio_manager.h"
#include <iostream>

AudioManager::AudioManager() : system(nullptr), bgmSound(nullptr), bgmChannel(nullptr) {
}

AudioManager::~AudioManager() {
    Release();
}

void AudioManager::Init() {
    FMOD::System_Create(&system);
    system->init(32, FMOD_INIT_NORMAL, nullptr);
}

void AudioManager::Update() {
    if (system) {
        system->update();
    }
}

void AudioManager::PlayBGM(const std::string& path) {
    if (!system) return;

    if (bgmSound) {
        StopBGM();
    }

    FMOD_RESULT result = system->createSound(path.c_str(), FMOD_LOOP_NORMAL | FMOD_CREATESTREAM, nullptr, &bgmSound);
    if (result == FMOD_OK) {
        system->playSound(bgmSound, nullptr, false, &bgmChannel);
    } else {
        std::cout << "음악을 불러오지 못했습니다: " << path << std::endl;
    }
}

void AudioManager::StopBGM() {
    if (bgmChannel) {
        bgmChannel->stop();
        bgmChannel = nullptr;
    }
    if (bgmSound) {
        bgmSound->release();
        bgmSound = nullptr;
    }
}

void AudioManager::Release() {
    StopBGM();
    if (system) {
        system->close();
        system->release();
        system = nullptr;
    }
}

unsigned int AudioManager::GetMusicPosition() const {
    if (!bgmChannel) return 0;
    
    unsigned int ms = 0;
    bgmChannel->getPosition(&ms, FMOD_TIMEUNIT_MS);
    return ms;
}