#include "../AudioManager/audio_manager.h"
#include "Common_Functions.h"
#include <raylib.h>
#include <string>

namespace MusicExecute {
    class MusicPlayer1 {
    public:
        MusicPlayer1() 
            : m_Channel(nullptr), m_IsPaused(true), m_IsLoaded(false), m_SpeedRatio(1.0f), m_MsgTimer(0.0f), m_InputCooldownTimer(0.0f), m_WaitFrames(0), m_MsgText("") {}
        ~MusicPlayer1() {}

        bool Initialize(AudioManager& audioManager) {
            std::string musicName = "Music1";
            std::string musicPath = "music/A_Night_Without_Visible_Stars.ogg";

            if (!audioManager.CreateStream(musicName, musicPath)) {
                return false;
            }

            m_IsLoaded = true;
            return true;
        }

        void Play(AudioManager& audioManager, int startWaitFrames = 0) {
            if (!m_IsLoaded) return;
            
            FMOD_CHANNEL* channel = audioManager.PlaySoundWithDelay("Music1", 0);
            SetActiveChannel(channel);
            
            m_WaitFrames = startWaitFrames;
            m_InputCooldownTimer = 0.2f; 
        }

        void PlayImmediate() {
            if (!m_Channel) return;
            FMOD_Channel_SetPaused(m_Channel, 0);
            m_IsPaused = false;
        }

        void Stop() {
            if (m_Channel) {
                FMOD_Channel_Stop(m_Channel);
                m_Channel = nullptr;
                m_IsPaused = true;
            }
        }

        void Update(float dt) {
            if (m_MsgTimer > 0.0f) {
                m_MsgTimer -= dt;
            }

            if (m_WaitFrames > 0) {
                m_WaitFrames--;
                return;
            }

            MusicExecuteUtils::HandleMusicControls(m_Channel, m_IsPaused, m_SpeedRatio, m_MsgTimer, m_InputCooldownTimer, m_MsgText);
        }

        void SetActiveChannel(FMOD_CHANNEL* channel) {
            m_Channel = channel;
            if (m_Channel) {
                FMOD_Channel_SetPaused(m_Channel, 1);
                m_IsPaused = true;
            }
        }

        void SetPitch(float pitch) {
            if (m_Channel) {
                FMOD_Channel_SetPitch(m_Channel, pitch);
            }
        }

        bool IsPaused() const { return m_IsPaused; }
        float GetSpeedRatio() const { return m_SpeedRatio; }
        float GetMsgTimer() const { return m_MsgTimer; }
        const std::string& GetMsgText() const { return m_MsgText; }

        float GetTotalDuration() const {
            if (!m_Channel) return 152.0f; 
            FMOD_SOUND* currentSound = nullptr;
            FMOD_Channel_GetCurrentSound(m_Channel, &currentSound);
            if (!currentSound) return 152.0f;

            unsigned int lengthMs = 0;
            FMOD_Sound_GetLength(currentSound, &lengthMs, FMOD_TIMEUNIT_MS);
            if (lengthMs == 0) return 152.0f;
            return (float)lengthMs / 1000.0f;
        }

        unsigned int GetCurrentPositionMs() const {
            if (!m_Channel) return 0;
            unsigned int currentPosMs = 0;
            FMOD_Channel_GetPosition(m_Channel, &currentPosMs, FMOD_TIMEUNIT_MS);
            return currentPosMs;
        }

    private:
        FMOD_CHANNEL* m_Channel;
        bool m_IsPaused;
        bool m_IsLoaded;
        float m_SpeedRatio;
        float m_MsgTimer;
        float m_InputCooldownTimer;
        int m_WaitFrames;
        std::string m_MsgText;
    };
}