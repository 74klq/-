#include "lobby_bgm.h"

static FMOD_SYSTEM* fmodSystem = nullptr;
static FMOD_SOUND* fmodSound = nullptr;
static FMOD_CHANNEL* fmodChannel = nullptr;
static bool isInitialized = false;

void LobbyBgm_Init() {
    if (!isInitialized) {
        FMOD_System_Create(&fmodSystem, FMOD_VERSION);
        FMOD_System_Init(fmodSystem, 512, FMOD_INIT_NORMAL, 0);

        if (FMOD_System_CreateSound(fmodSystem, "music/Lobby_BGM.mp3", FMOD_LOOP_NORMAL, 0, &fmodSound) == FMOD_OK) {
            FMOD_Sound_SetLoopCount(fmodSound, -1);
            FMOD_System_PlaySound(fmodSystem, fmodSound, 0, false, &fmodChannel);
            isInitialized = true;
        }
    } else {
        FMOD_BOOL playing = 0;
        if (fmodChannel) {
            FMOD_Channel_IsPlaying(fmodChannel, &playing);
        }
        if (!playing) {
            FMOD_System_PlaySound(fmodSystem, fmodSound, 0, false, &fmodChannel);
        }
    }
}

void LobbyBgm_Update() {
    if (isInitialized && fmodSystem) {
        FMOD_System_Update(fmodSystem);
    }
}

void LobbyBgm_Stop() {
    if (isInitialized) {
        if (fmodChannel) {
            FMOD_Channel_Stop(fmodChannel);
            fmodChannel = nullptr;
        }
        if (fmodSound) {
            FMOD_Sound_Release(fmodSound);
            fmodSound = nullptr;
        }
        if (fmodSystem) {
            FMOD_System_Close(fmodSystem);
            FMOD_System_Release(fmodSystem);
            fmodSystem = nullptr;
        }
        isInitialized = false;
    }
}

unsigned long long LobbyBgm_GetDSPClock(void) {
    unsigned long long dspclock = 0;
    if (fmodChannel) {
        FMOD_Channel_GetDSPClock(fmodChannel, &dspclock, 0);
    }
    return dspclock;
}

int LobbyBgm_GetSamplingRate(void) {
    int rate = 44100;
    if (fmodSystem) {
        FMOD_System_GetSoftwareFormat(fmodSystem, &rate, 0, 0);
    }
    return rate;
}