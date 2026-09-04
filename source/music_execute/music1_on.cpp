#include "../AudioManager/audio_manager.h"
#include "Common_Functions.h"
#include <raylib.h>

namespace MusicExecute {
    class MusicPlayer1 {
    public:
        MusicPlayer1() : m_Channel(nullptr), m_IsPaused(true), m_IsLoaded(false), m_SpeedRatio(1.0f), m_MsgTimer(0.0f) {}
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

        void Play(AudioManager& audioManager, unsigned int delayMs = 0) {
            if (!m_IsLoaded) return;
            audioManager.PlaySoundWithDelay("Music1", delayMs);
        }

        void Update(float dt) {
            if (m_MsgTimer > 0.0f) {
                m_MsgTimer -= dt;
            }
            MusicExecuteUtils::HandleMusicControls(m_Channel, m_IsPaused, m_SpeedRatio, m_MsgTimer);
        }

        void SetActiveChannel(FMOD_CHANNEL* channel) {
            m_Channel = channel;
            if (m_Channel) {
                FMOD_Channel_SetPaused(m_Channel, 1);
                m_IsPaused = true;
            }
        }

        bool IsPaused() const { return m_IsPaused; }
        float GetSpeedRatio() const { return m_SpeedRatio; }
        float GetMsgTimer() const { return m_MsgTimer; }

    private:
        FMOD_CHANNEL* m_Channel;
        bool m_IsPaused;
        bool m_IsLoaded;
        float m_SpeedRatio;
        float m_MsgTimer;
    };
}