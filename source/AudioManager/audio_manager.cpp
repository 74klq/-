#include "audio_manager.h"

AudioManager::AudioManager() 
    : m_System(nullptr), m_PreviewChannel(nullptr) {
}

AudioManager::~AudioManager() {
    for (auto& pair : m_Sounds) {
        if (pair.second) FMOD_Sound_Release(pair.second);
    }
    m_Sounds.clear();

    if (m_System) {
        FMOD_System_Close(m_System);
        FMOD_System_Release(m_System);
    }
}

bool AudioManager::Init() {
    FMOD_System_Create(&m_System, FMOD_VERSION);
    FMOD_System_SetDSPBufferSize(m_System, 256, 4);
    FMOD_System_Init(m_System, 512, FMOD_INIT_NORMAL, nullptr);
    return true;
}

void AudioManager::Update() {
    if (m_System) {
        FMOD_System_Update(m_System);
    }
}

bool AudioManager::CreateSound(const std::string& name, const std::string& filePath) {
    FMOD_SOUND* sound = nullptr;
    if (FMOD_System_CreateSound(m_System, filePath.c_str(), FMOD_CREATESAMPLE, 0, &sound) == FMOD_OK) {
        m_Sounds[name] = sound;
        return true;
    }
    return false;
}

bool AudioManager::CreateStream(const std::string& name, const std::string& filePath) {
    FMOD_SOUND* sound = nullptr;
    if (FMOD_System_CreateStream(m_System, filePath.c_str(), FMOD_DEFAULT, 0, &sound) == FMOD_OK) {
        m_Sounds[name] = sound;
        return true;
    }
    return false;
}

void AudioManager::PlaySoundWithDelay(const std::string& name, unsigned int delayMs) {
    auto it = m_Sounds.find(name);
    if (it == m_Sounds.end()) return;

    FMOD_CHANNEL* localChannel = nullptr;
    FMOD_RESULT result = FMOD_System_PlaySound(m_System, it->second, nullptr, true, &localChannel);
    if (result != FMOD_OK || !localChannel) return;

    int sampleRate = 0;
    FMOD_System_GetSoftwareFormat(m_System, &sampleRate, nullptr, nullptr);

    FMOD_CHANNELGROUP* masterGroup = nullptr;
    result = FMOD_System_GetMasterChannelGroup(m_System, &masterGroup);
    if (result != FMOD_OK || !masterGroup) {
        if (localChannel) {
            FMOD_Channel_Stop(localChannel);
        }
        return;
    }

    unsigned long long dspClock = 0;
    FMOD_ChannelGroup_GetDSPClock(masterGroup, &dspClock, nullptr);

    unsigned long long delaySamples = ((unsigned long long)sampleRate * delayMs) / 1000;
    unsigned long long targetClock = dspClock + delaySamples;

    FMOD_Channel_SetDelay(localChannel, targetClock, 0, false);
    FMOD_Channel_SetPaused(localChannel, false); 
}

void AudioManager::PlayPreview(const std::string& name, unsigned int startMs) {
    auto it = m_Sounds.find(name);
    if (it == m_Sounds.end()) return;

    if (m_PreviewChannel) {
        FMOD_BOOL isPlaying = 0;
        FMOD_Channel_IsPlaying(m_PreviewChannel, &isPlaying);
        if (isPlaying) {
            FMOD_Channel_Stop(m_PreviewChannel);
        }
    }

    FMOD_System_PlaySound(m_System, it->second, nullptr, true, &m_PreviewChannel);
    if (m_PreviewChannel) {
        FMOD_Channel_SetPosition(m_PreviewChannel, startMs, FMOD_TIMEUNIT_MS);
        FMOD_Channel_SetPaused(m_PreviewChannel, false);
    }
}

void AudioManager::StopPreview() {
    if (m_PreviewChannel) {
        FMOD_BOOL isPlaying = 0;
        FMOD_Channel_IsPlaying(m_PreviewChannel, &isPlaying);
        if (isPlaying) {
            FMOD_Channel_Stop(m_PreviewChannel);
        }
        m_PreviewChannel = nullptr;
    }
}