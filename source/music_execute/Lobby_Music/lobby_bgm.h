#ifndef LOBBY_BGM_H
#define LOBBY_BGM_H

#include <fmod.h>

#ifdef __cplusplus
extern "C" {
#endif

void LobbyBgm_Init(void);
void LobbyBgm_Update(void);
void LobbyBgm_Stop(void);
unsigned long long LobbyBgm_GetDSPClock(void);
int LobbyBgm_GetSamplingRate(void);

#ifdef __cplusplus
}
#endif

#endif