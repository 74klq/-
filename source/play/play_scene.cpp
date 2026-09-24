#include "play_scene.h"
#include "note.h"
#include "../editing/chart_editor.h"
#include "../editing/chart_save.h"
#include "../vfx/hit_effect.h"
#include "../Animation/note_down_animation.h"
#include "../AudioManager/audio_manager.h"
#include "../music_execute/music1_on.cpp"
#include "../music_execute/music2_on.cpp"
#include "../music_execute/music3_on.cpp"
#include "../music_execute/music4_on.cpp"
#include "../music_execute/music5_on.cpp"
#include "../music_execute/music6_on.cpp"
#include "../music_execute/music7_on.cpp"
#include "../music_execute/music8_on.cpp"
#include "../music_execute/music9_on.cpp"
#include "../music_execute/music10_on.cpp"
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <fmod.h>

class FramedBeatmapClock
{
private:
    FMOD_SYSTEM* system;
    FMOD_CHANNEL* channel;
    double userGlobalOffset;
    bool isFirstFrame;

public:
    FramedBeatmapClock(bool applyOffsets = true, FMOD_CHANNEL* source = nullptr)
        : system(nullptr),
          channel(source),
          userGlobalOffset(0.0),
          isFirstFrame(true)
    {
    }

    ~FramedBeatmapClock() = default;

    void SetSystem(FMOD_SYSTEM* newSystem) { system = newSystem; }
    void SetChannel(FMOD_CHANNEL* newChannel) 
    { 
        channel = newChannel; 
        isFirstFrame = true;
    }
    void SetUserGlobalOffset(double offsetMs) { userGlobalOffset = offsetMs; }
    double GetUserGlobalOffset() const { return userGlobalOffset; }
    void SetUserBeatmapOffset(double offsetMs) {}
    void SetPlatformOffset(double offsetMs) {}
    void UpdatePlatformOffset(bool isWindows = true, bool useExperimentalWasapi = false) {}
    void LoadComplete() {}
    bool IsLoaded() const { return true; }
    double GetTotalAppliedOffset() const { return userGlobalOffset; }
    void ProcessFrame(double deltaTimeMs) {}
    void Start() { isFirstFrame = true; }
    void Stop() {}
    void Reset() { isFirstFrame = true; }
    bool Seek(double positionMs) { return true; }
    double GetElapsedFrameTime() const { return 0.0; }
    bool IsRunning() const { return true; }
    bool IsRewinding() const { return false; }
    double GetRate() const { return 1.0; }
    void SetRate(double newRate) {}
    std::string GetSnapshot() const { return "DSP Sync Active"; }

    double GetCurrentTime() const
    {
        if (!channel || !system) return 0.0;

        FMOD_BOOL isPlaying = false;
        FMOD_Channel_IsPlaying(channel, &isPlaying);
        if (!isPlaying) return 0.0;

        unsigned long long dspClock = 0;
        FMOD_Channel_GetDSPClock(channel, &dspClock, nullptr);

        if (isFirstFrame) {
            unsigned int currentPos = 0;
            FMOD_Channel_GetPosition(channel, &currentPos, FMOD_TIMEUNIT_MS);
            if (currentPos < 50 && dspClock > 100000) { 
                return 0.0; 
            }
            const_cast<FramedBeatmapClock*>(this)->isFirstFrame = false; 
        }

        int sampleRate = 0;
        FMOD_System_GetSoftwareFormat(system, &sampleRate, nullptr, nullptr);

        unsigned int bufferLength = 0;
        int numBuffers = 0;
        FMOD_System_GetDSPBufferSize(system, &bufferLength, &numBuffers);

        if (sampleRate > 0)
        {
            double hardwareLatencyMs = ((double)bufferLength * numBuffers * 1000.0) / sampleRate;
            double calculatedTime = (((double)dspClock / sampleRate) * 1000.0) - hardwareLatencyMs + userGlobalOffset;
            
            if (calculatedTime < 0.0) return 0.0;
            return calculatedTime;
        }

        return 0.0;
    }
};

// 국정원 지하실에 락덥 됬다 프리

static const float PLAYFIELD_X = 400.0f;
static const float PLAYFIELD_Y = 0.0f;
static const float PLAYFIELD_WIDTH = 400.0f;
static const float PLAYFIELD_HEIGHT = 720.0f;

static const int LANE_COUNT = 4;
static const float LANE_WIDTH = 70.0f;
static const float LANE_AREA_WIDTH = LANE_COUNT * LANE_WIDTH;
static const float LANE_START_X =
PLAYFIELD_X + (PLAYFIELD_WIDTH - LANE_AREA_WIDTH) / 2.0f;

static const float LANE_X_COORDS[4] = {
LANE_START_X + LANE_WIDTH * 0.5f,
LANE_START_X + LANE_WIDTH * 1.5f,
LANE_START_X + LANE_WIDTH * 2.5f,
LANE_START_X + LANE_WIDTH * 3.5f
};

struct MusicPlayerWrapper {
    MusicExecute::MusicPlayer1* p1 = nullptr;
    MusicExecute::MusicPlayer2* p2 = nullptr;
    MusicExecute::MusicPlayer3* p3 = nullptr;
    MusicExecute::MusicPlayer4* p4 = nullptr;
    MusicExecute::MusicPlayer5* p5 = nullptr;
    MusicExecute::MusicPlayer6* p6 = nullptr;
    MusicExecute::MusicPlayer7* p7 = nullptr;
    MusicExecute::MusicPlayer8* p8 = nullptr;
    MusicExecute::MusicPlayer9* p9 = nullptr;
    MusicExecute::MusicPlayer10* p10 = nullptr;
    int active = 1;

    void Init(AudioManager& am) {
        if (p1) p1->Initialize(am); if (p2) p2->Initialize(am); if (p3) p3->Initialize(am);
        if (p4) p4->Initialize(am); if (p5) p5->Initialize(am); if (p6) p6->Initialize(am);
        if (p7) p7->Initialize(am); if (p8) p8->Initialize(am); if (p9) p9->Initialize(am);
        if (p10) p10->Initialize(am);
    }

    void Update(float dt) {
        switch (active) {
            case 1: if (p1) p1->Update(dt); break;
            case 2: if (p2) p2->Update(dt); break;
            case 3: if (p3) p3->Update(dt); break;
            case 4: if (p4) p4->Update(dt); break;
            case 5: if (p5) p5->Update(dt); break;
            case 6: if (p6) p6->Update(dt); break;
            case 7: if (p7) p7->Update(dt); break;
            case 8: if (p8) p8->Update(dt); break;
            case 9: if (p9) p9->Update(dt); break;
            case 10: if (p10) p10->Update(dt); break;
        }
    }

    void Stop() {
        if (p1) p1->Stop(); if (p2) p2->Stop(); if (p3) p3->Stop();
        if (p4) p4->Stop(); if (p5) p5->Stop(); if (p6) p6->Stop();
        if (p7) p7->Stop(); if (p8) p8->Stop(); if (p9) p9->Stop();
        if (p10) p10->Stop();
    }

    void Play(AudioManager& am, int i) {
        switch (active) {
            case 1: if (p1) p1->Play(am, i); break;
            case 2: if (p2) p2->Play(am, i); break;
            case 3: if (p3) p3->Play(am, i); break;
            case 4: if (p4) p4->Play(am, i); break;
            case 5: if (p5) p5->Play(am, i); break;
            case 6: if (p6) p6->Play(am, i); break;
            case 7: if (p7) p7->Play(am, i); break;
            case 8: if (p8) p8->Play(am, i); break;
            case 9: if (p9) p9->Play(am, i); break;
            case 10: if (p10) p10->Play(am, i); break;
        }
    }

    void PlayImmediate() {
        switch (active) {
            case 1: if (p1) p1->PlayImmediate(); break;
            case 2: if (p2) p2->PlayImmediate(); break;
            case 3: if (p3) p3->PlayImmediate(); break;
            case 4: if (p4) p4->PlayImmediate(); break;
            case 5: if (p5) p5->PlayImmediate(); break;
            case 6: if (p6) p6->PlayImmediate(); break;
            case 7: if (p7) p7->PlayImmediate(); break;
            case 8: if (p8) p8->PlayImmediate(); break;
            case 9: if (p9) p9->PlayImmediate(); break;
            case 10: if (p10) p10->PlayImmediate(); break;
        }
    }

    void SetPitch(float p) {
        switch (active) {
            case 1: if (p1) p1->SetPitch(p); break;
            case 2: if (p2) p2->SetPitch(p); break;
            case 3: if (p3) p3->SetPitch(p); break;
            case 4: if (p4) p4->SetPitch(p); break;
            case 5: if (p5) p5->SetPitch(p); break;
            case 6: if (p6) p6->SetPitch(p); break;
            case 7: if (p7) p7->SetPitch(p); break;
            case 8: if (p8) p8->SetPitch(p); break;
            case 9: if (p9) p9->SetPitch(p); break;
            case 10: if (p10) p10->SetPitch(p); break;
        }
    }

    FMOD_CHANNEL* GetChannelRaw() const {
        switch (active) {
            case 1: return p1 ? p1->GetChannelRaw() : nullptr;
            case 2: return p2 ? p2->GetChannelRaw() : nullptr;
            case 3: return p3 ? p3->GetChannelRaw() : nullptr;
            case 4: return p4 ? p4->GetChannelRaw() : nullptr;
            case 5: return p5 ? p5->GetChannelRaw() : nullptr;
            case 6: return p6 ? p6->GetChannelRaw() : nullptr;
            case 7: return p7 ? p7->GetChannelRaw() : nullptr;
            case 8: return p8 ? p8->GetChannelRaw() : nullptr;
            case 9: return p9 ? p9->GetChannelRaw() : nullptr;
            case 10: return p10 ? p10->GetChannelRaw() : nullptr;
        }
        return nullptr;
    }

    bool IsValid() const {
        switch (active) {
            case 1: return p1 != nullptr; case 2: return p2 != nullptr; case 3: return p3 != nullptr;
            case 4: return p4 != nullptr; case 5: return p5 != nullptr; case 6: return p6 != nullptr;
            case 7: return p7 != nullptr; case 8: return p8 != nullptr; case 9: return p9 != nullptr;
            case 10: return p10 != nullptr;
        }
        return false;
    }
};

static AudioManager s_AudioManager;
static MusicPlayerWrapper s_MusicPlayer;
static FramedBeatmapClock s_BeatmapClock(true);

static std::vector<Note> s_Notes;

struct PlayableNote {
float timeSec;
float endTimeSec;
int lane;
int type;
bool active;
bool isHolding;
float lastTickTime;
};

static std::vector<PlayableNote> s_PlayableNotes;

static float s_SongTimer = 0.0f;
static float s_SpawnTimer = 0.0f;

static int s_Combo = 0;

static bool s_ShowJudgment = false;
static float s_JudgmentTimer = 0.0f;

static const char* s_CurrentJudgment = "PERFECT";

static bool s_IsEditorMode = false;
static ChartEditor s_ChartEditor;

static Font s_ComboFont = { 0 };
static Font s_SuitFont = { 0 };

static float s_JudgmentLinePulse = 0.0f;
static float s_JudgmentAnimTimer = 0.0f;
static float s_ComboAnimTimer = 0.0f;

static int s_LastCombo = 0;

static float s_NoteScrollSpeed = 200.0f;

static bool s_IsPaused = false;
static int s_PauseSelection = 0; 
static bool s_IgnoreFirstEnter = false;
static float s_SongSelectEnterDelay = 0.0f;

static float Clamp01(float value)
{
if (value < 0.0f) return 0.0f;
if (value > 1.0f) return 1.0f;
return value;
}

[[maybe_unused]] static float EaseOutCubic(float value)
{
value = Clamp01(value);
float inv = 1.0f - value;
return 1.0f - inv * inv * inv;
}

static float EaseOutBack(float value)
{
value = Clamp01(value);

const float c1 = 1.70158f;
const float c3 = c1 + 1.0f;

float x = value - 1.0f;

return 1.0f + c3 * x * x * x + c1 * x * x;

}

static void DrawBackground()
{
    const float time = GetTime();
    const int screenW = GetScreenWidth();
    const int screenH = GetScreenHeight();

    DrawRectangleGradientV(
        0, 0, screenW, screenH,
        Color{ 17, 17, 19, 255 },
        Color{ 1, 1, 2, 255 }
    );

    DrawRectangleGradientH(
        0, 0, screenW, screenH,
        Color{ 0, 0, 0, 115 },
        Color{ 0, 0, 0, 115 }
    );

    const float centerX = PLAYFIELD_X + PLAYFIELD_WIDTH * 0.5f;

    for (int i = 0; i < 90; ++i)
    {
        const float fx = fmodf(i * 197.31f + 37.0f, (float)screenW);
        const float fy = fmodf(i * 91.73f + 11.0f, 590.0f);
        const float wave = 0.5f + 0.5f * sinf(time * 0.35f + i * 1.37f);
        const unsigned char alpha = (unsigned char)(8.0f + wave * 24.0f);
        const float radius = (i % 13 == 0) ? 1.4f : 0.65f;

        DrawCircle(
            (int)fx,
            (int)fy,
            radius,
            Color{ 230, 230, 235, alpha }
        );
    }

    for (int y = 18; y < 600; y += 36)
    {
        DrawRectangle(
            0, y, screenW, 1,
            Color{ 255, 255, 255, 4 }
        );
    }

    for (int x = 0; x < screenW; x += 80)
    {
        DrawRectangle(
            x, 0, 1, 600,
            Color{ 255, 255, 255, 3 }
        );
    }

    for (int i = 0; i < 5; ++i)
    {
        const float radius = 150.0f + i * 55.0f;
        const float pulse = 0.5f + 0.5f * sinf(time * 0.28f + i);

        DrawCircleLines(
            (int)centerX,
            310,
            radius,
            Color{ 255, 255, 255, (unsigned char)(2 + pulse * 5) }
        );
    }

    DrawRectangleGradientH(
        0, 0, (int)PLAYFIELD_X, screenH,
        Color{ 0, 0, 0, 0 },
        Color{ 0, 0, 0, 135 }
    );

    DrawRectangleGradientH(
        (int)(PLAYFIELD_X + PLAYFIELD_WIDTH),
        0,
        screenW - (int)(PLAYFIELD_X + PLAYFIELD_WIDTH),
        screenH,
        Color{ 0, 0, 0, 135 },
        Color{ 0, 0, 0, 0 }
    );

    DrawRectangleGradientV(
        0, 0, screenW, 130,
        Color{ 0, 0, 0, 105 },
        Color{ 0, 0, 0, 0 }
    );

    DrawRectangleGradientV(
        0, 560, screenW, 160,
        Color{ 0, 0, 0, 0 },
        Color{ 0, 0, 0, 160 }
    );
}

static void DrawMechanicalFrame()
{
    const int left = (int)PLAYFIELD_X;
    const int right = (int)(PLAYFIELD_X + PLAYFIELD_WIDTH);
    const float time = GetTime();
    const float pulse = 0.5f + 0.5f * sinf(time * 2.2f);

    DrawRectangle(left - 30, 0, 30, 720, Color{ 5, 5, 5, 250 });
    DrawRectangle(right, 0, 30, 720, Color{ 5, 5, 5, 250 });

    DrawRectangle(left - 8, 0, 4, 720, Color{ 73, 73, 73, 180 });
    DrawRectangle(right + 4, 0, 4, 720, Color{ 73, 73, 73, 180 });

    DrawRectangle(left - 3, 0, 3, 720, Color{ 235, 235, 235, 210 });
    DrawRectangle(right, 0, 3, 720, Color{ 235, 235, 235, 210 });

    DrawRectangle(left - 24, 0, 2, 720, Color{ 43, 43, 43, 230 });
    DrawRectangle(right + 22, 0, 2, 720, Color{ 43, 43, 43, 230 });

    for (int i = 0; i < 10; ++i)
    {
        float y = 30.0f + i * 70.0f;
        unsigned char a = (unsigned char)(75.0f + pulse * 35.0f);

        DrawRectangle(left - 24, (int)y, 14, 2, Color{ 171, 171, 171, a });
        DrawRectangle(left - 17, (int)y + 5, 7, 1, Color{ 255, 255, 255, 70 });
        DrawRectangle(right + 10, (int)y, 14, 2, Color{ 171, 171, 171, a });
        DrawRectangle(right + 10, (int)y + 5, 7, 1, Color{ 255, 255, 255, 70 });
    }

    DrawRectangle(left - 4, 110, 4, 260, Color{ 211, 211, 211, (unsigned char)(35.0f + pulse * 45.0f) });
    DrawRectangle(right, 110, 4, 260, Color{ 211, 211, 211, (unsigned char)(35.0f + pulse * 45.0f) });

    DrawTriangle(
        { (float)left - 8.0f, 390.0f },
        { (float)left - 72.0f, 510.0f },
        { (float)left - 8.0f, 580.0f },
        Color{ 12, 12, 12, 255 }
    );

    DrawTriangleLines(
        { (float)left - 8.0f, 390.0f },
        { (float)left - 72.0f, 510.0f },
        { (float)left - 8.0f, 580.0f },
        Color{ 136, 136, 136, 170 }
    );

    DrawTriangle(
        { (float)right + 8.0f, 390.0f },
        { (float)right + 72.0f, 510.0f },
        { (float)right + 8.0f, 580.0f },
        Color{ 12, 12, 12, 255 }
    );

    DrawTriangleLines(
        { (float)right + 8.0f, 390.0f },
        { (float)right + 72.0f, 510.0f },
        { (float)right + 8.0f, 580.0f },
        Color{ 136, 136, 136, 170 }
    );

    for (int i = 0; i < 6; ++i)
    {
        float y = 410.0f + i * 42.0f;
        DrawRectangle(left - 58, (int)y, 32, 1, Color{ 151, 151, 151, 42 });
        DrawRectangle(right + 26, (int)y, 32, 1, Color{ 151, 151, 151, 42 });
    }

    DrawRectangle(left - 18, 704, 14, 2, Color{ 226, 226, 226, 130 });
    DrawRectangle(right + 4, 704, 14, 2, Color{ 226, 226, 226, 130 });
}

static void DrawPlayfield()
{
    const float t = GetTime();
    const int px = (int)PLAYFIELD_X;
    const int pw = (int)PLAYFIELD_WIDTH;

    DrawRectangle(px - 24, 0, pw + 48, 720, Color{ 0, 0, 0, 180 });
    DrawRectangle(px, 0, pw, 720, Color{ 3, 3, 4, 255 });

    DrawRectangleGradientV(px, 0, pw, 720,
        Color{ 24, 24, 26, 235 },
        Color{ 1, 1, 2, 255 });

    DrawRectangleGradientH(px, 0, 70, 720,
        Color{ 0, 0, 0, 220 },
        Color{ 0, 0, 0, 0 });
    DrawRectangleGradientH(px + pw - 70, 0, 70, 720,
        Color{ 0, 0, 0, 0 },
        Color{ 0, 0, 0, 220 });

    for (int y = 8; y < 720; y += 8)
    {
        unsigned char a = (unsigned char)(3 + ((y / 8) % 3));
        DrawRectangle(px + 2, y, pw - 4, 1, Color{ 255, 255, 255, a });
    }

    const float scan = fmodf(t * 135.0f, 720.0f);
    DrawRectangle(px + 2, (int)scan, pw - 4, 2, Color{ 255, 255, 255, 15 });
    DrawRectangle(px + 2, (int)scan + 2, pw - 4, 1, Color{ 255, 255, 255, 6 });

    DrawRectangle(px, 0, 3, 720, Color{ 255, 255, 255, 115 });
    DrawRectangle(px + pw - 3, 0, 3, 720, Color{ 255, 255, 255, 115 });
    DrawRectangle(px + 8, 0, 1, 720, Color{ 255, 255, 255, 30 });
    DrawRectangle(px + pw - 9, 0, 1, 720, Color{ 255, 255, 255, 30 });

    for (int i = 0; i < 4; ++i)
    {
        float cx = LANE_X_COORDS[i];
        float pulse = 0.5f + 0.5f * sinf(t * 1.8f + i * 0.9f);
        DrawCircle((int)cx, 120, 48.0f + pulse * 7.0f, Color{ 255, 255, 255, 2 });
    }

    DrawRectangle(px, 0, pw, 3, Color{ 255, 255, 255, 230 });
    DrawRectangle(px, 717, pw, 3, Color{ 12, 12, 14, 255 });

    DrawMechanicalFrame();
}

static void DrawLaneDecorations(float judgmentLineY)
{
    const float t = GetTime();

    for (int i = 0; i < LANE_COUNT; ++i)
    {
        const float laneX = LANE_START_X + i * LANE_WIDTH;
        const float laneCenter = laneX + LANE_WIDTH * 0.5f;

        DrawRectangle(
            (int)laneX + 1,
            0,
            (int)LANE_WIDTH - 2,
            (int)judgmentLineY,
            Color{ 255, 255, 255, (unsigned char)(2 + i) }
        );

        if (i > 0)
        {
            DrawRectangle((int)laneX - 4, 0, 8, (int)judgmentLineY, Color{ 0, 0, 0, 235 });
            DrawRectangle((int)laneX - 1, 0, 2, (int)judgmentLineY, Color{ 165, 165, 165, 110 });
            DrawRectangle((int)laneX, 0, 1, (int)judgmentLineY, Color{ 255, 255, 255, 32 });
        }

        for (int y = 34; y < (int)judgmentLineY - 20; y += 48)
        {
            float pulse = 0.5f + 0.5f * sinf(t * 1.3f + y * 0.02f + i);
            unsigned char a = (unsigned char)(8 + pulse * 12);
            DrawRectangle((int)laneCenter - 11, y, 22, 1, Color{ 255, 255, 255, a });
            DrawRectangle((int)laneCenter - 2, y - 3, 4, 7, Color{ 255, 255, 255, (unsigned char)(a / 2) });
        }

        DrawRectangle((int)laneCenter - 1, 24, 2, (int)judgmentLineY - 48, Color{ 255, 255, 255, 8 });

        DrawLine((int)laneX + 8, 16, (int)laneX + 22, 16, Color{ 255, 255, 255, 48 });
        DrawLine((int)laneX + 8, 16, (int)laneX + 8, 28, Color{ 255, 255, 255, 48 });
        DrawLine((int)(laneX + LANE_WIDTH - 22), 16, (int)(laneX + LANE_WIDTH - 8), 16, Color{ 255, 255, 255, 48 });
        DrawLine((int)(laneX + LANE_WIDTH - 8), 16, (int)(laneX + LANE_WIDTH - 8), 28, Color{ 255, 255, 255, 48 });
    }

    DrawRectangle((int)LANE_START_X, (int)judgmentLineY - 58, (int)LANE_AREA_WIDTH, 1, Color{ 255, 255, 255, 25 });
    DrawRectangle((int)LANE_START_X, (int)judgmentLineY - 34, (int)LANE_AREA_WIDTH, 1, Color{ 255, 255, 255, 18 });
}

static void DrawLanes(const bool pressedStates[4], float judgmentLineY)
{
    DrawLaneDecorations(judgmentLineY);
    const float t = GetTime();

    for (int i = 0; i < LANE_COUNT; ++i)
    {
        const float x = LANE_START_X + i * LANE_WIDTH;
        const float cx = x + LANE_WIDTH * 0.5f;
        const bool pressed = pressedStates[i];
        const float pulse = 0.5f + 0.5f * sinf(t * 8.0f + i * 0.8f);

        if (pressed)
        {
            DrawRectangleGradientV(
                (int)x + 3, 40, (int)LANE_WIDTH - 6, (int)judgmentLineY - 42,
                Color{ 255, 255, 255, 0 },
                Color{ 255, 255, 255, (unsigned char)(22 + pulse * 35) });

            DrawRectangle((int)x + 4, (int)judgmentLineY - 36,
                (int)LANE_WIDTH - 8, 5, Color{ 255, 255, 255, (unsigned char)(100 + pulse * 100) });

            DrawRectangle((int)x + 8, (int)judgmentLineY - 29,
                (int)LANE_WIDTH - 16, 2, Color{ 255, 255, 255, 180 });

            DrawCircle((int)cx, (int)judgmentLineY, 21.0f + pulse * 7.0f,
                Color{ 255, 255, 255, (unsigned char)(8 + pulse * 18) });
        }
        else
        {
            DrawRectangle((int)x + 10, (int)judgmentLineY - 25,
                (int)LANE_WIDTH - 20, 1, Color{ 255, 255, 255, 28 });
        }
    }
}

static void DrawNotes(float judgmentLineY)
{
    const float nowMs = s_SongTimer * 1000.0f;
    const float time = GetTime();

    for (const auto& pNote : s_PlayableNotes)
    {
        if (!pNote.active || pNote.lane < 0 || pNote.lane >= LANE_COUNT)
            continue;

        const float diffSec = (pNote.timeSec * 1000.0f - nowMs) / 1000.0f;
        const float y = judgmentLineY - diffSec * s_NoteScrollSpeed;

        if (pNote.type == 128)
        {
            const float endDiffSec = (pNote.endTimeSec * 1000.0f - nowMs) / 1000.0f;
            const float endY = judgmentLineY - endDiffSec * s_NoteScrollSpeed;

            if (y < -100.0f && endY > 760.0f)
                continue;

            const float cx = LANE_X_COORDS[pNote.lane];
            const float w = LANE_WIDTH - 6.0f;
            const float h = 24.0f;
            const float noteLength = endY - y;

            DrawRectangleRounded(
                { cx - w * 0.5f + 4.0f, endY - h * 0.5f + 7.0f, w, noteLength + h },
                0.18f, 8, Color{ 0, 0, 0, 245 });

            DrawRectangleRounded(
                { cx - w * 0.5f, endY - h * 0.5f, w, noteLength + h },
                0.18f, 8, Color{ 160, 160, 165, 255 });

            DrawRectangleRounded(
                { cx - w * 0.5f + 2.0f, endY - h * 0.5f + 2.0f, w - 4.0f, (noteLength + h) * 0.95f },
                0.16f, 8, Color{ 210, 210, 215, 255 });

            DrawRectangleRounded(
                { cx - w * 0.5f, y - h * 0.5f, w, h },
                0.18f, 8, Color{ 255, 255, 255, 255 });

            DrawRectangleRounded(
                { cx - w * 0.5f, endY - h * 0.5f, w, h },
                0.18f, 8, Color{ 100, 100, 105, 255 });
        }
        else
        {
            if (y < -100.0f || y > 760.0f)
                continue;

            const float cx = LANE_X_COORDS[pNote.lane];
            const float w = LANE_WIDTH - 6.0f;
            const float h = 24.0f;
            const bool nearHit = fabsf(diffSec) < 0.22f;
            const float pulse = 0.5f + 0.5f * sinf(time * 9.0f + pNote.lane);

            if (nearHit)
            {
                DrawRectangleRounded(
                    { cx - w * 0.5f - 10.0f, y - h * 0.5f - 10.0f, w + 20.0f, h + 20.0f },
                    0.18f, 8, Color{ 255, 255, 255, (unsigned char)(8 + pulse * 12) });
            }

            DrawRectangleRounded(
                { cx - w * 0.5f + 4.0f, y - h * 0.5f + 7.0f, w, h },
                0.18f, 8, Color{ 0, 0, 0, 245 });

            DrawRectangleRounded(
                { cx - w * 0.5f, y - h * 0.5f, w, h },
                0.18f, 8, Color{ 218, 218, 221, 255 });

            DrawRectangleRounded(
                { cx - w * 0.5f + 2.0f, y - h * 0.5f + 2.0f, w - 4.0f, h * 0.42f },
                0.16f, 8, Color{ 255, 255, 255, 255 });

            DrawRectangleRounded(
                { cx - w * 0.5f + 5.0f, y - 2.0f, w - 10.0f, 4.0f },
                0.35f, 8, Color{ 22, 22, 24, 255 });

            DrawRectangleRoundedLines(
                { cx - w * 0.5f, y - h * 0.5f, w, h },
                0.18f, 8, Color{ 255, 255, 255, 240 });

            DrawRectangle((int)(cx - w * 0.5f + 10.0f), (int)(y - h * 0.5f + 2.0f),
                (int)(w - 20.0f), 1, Color{ 255, 255, 255, 150 });

            DrawRectangle((int)(cx - 18), (int)(y + h * 0.5f + 5), 36, 2,
                Color{ 0, 0, 0, 100 });
        }
    }
}

static void DrawJudgmentLine(float judgmentLineY)
{
    const float t = GetTime();
    const float impact = Clamp01(s_JudgmentLinePulse);
    const float pulse = 0.5f + 0.5f * sinf(t * 3.2f);
    const int y = (int)judgmentLineY;
    const float left = LANE_START_X - 22.0f;
    const float right = LANE_START_X + LANE_AREA_WIDTH + 22.0f;

    DrawRectangle((int)left - 14, y - 18, (int)(right - left) + 28, 36,
        Color{ 255, 255, 255, (unsigned char)(7 + impact * 20) });
    DrawRectangle((int)left - 6, y - 10, (int)(right - left) + 12, 20,
        Color{ 255, 255, 255, (unsigned char)(12 + impact * 30) });

    DrawRectangle((int)left, y - 6, (int)(right - left), 12, Color{ 4, 4, 5, 245 });
    DrawRectangle((int)left, y - 4, (int)(right - left), 8, Color{ 110, 110, 112, 230 });
    DrawRectangle((int)left, y - 2, (int)(right - left), 4, Color{ 238, 238, 240, 255 });
    DrawRectangle((int)left, y - 1, (int)(right - left), 2, Color{ 255, 255, 255, 255 });

    for (int i = 0; i < LANE_COUNT; ++i)
    {
        const float x = LANE_X_COORDS[i];
        const bool hitFlash = impact > 0.01f;
        const float r = 13.0f + impact * 12.0f;

        DrawCircle((int)x, y, r + 9.0f, Color{ 255, 255, 255, (unsigned char)(4 + impact * 18) });
        DrawCircle((int)x, y, r, Color{ 3, 3, 4, 245 });
        DrawCircleLines((int)x, y, r + 4.0f, Color{ 245, 245, 245, (unsigned char)(115 + impact * 120) });
        DrawCircleLines((int)x, y, r + 8.0f + pulse * 2.0f, Color{ 255, 255, 255, (unsigned char)(20 + pulse * 25) });

        DrawLine((int)x - 26, y, (int)x - 17, y, Color{ 255, 255, 255, 175 });
        DrawLine((int)x + 17, y, (int)x + 26, y, Color{ 255, 255, 255, 175 });

        if (hitFlash)
        {
            DrawCircle((int)x, y, r * 0.45f, Color{ 255, 255, 255, (unsigned char)(20 + impact * 80) });
        }
    }

    DrawRectangle((int)left - 34, y - 1, 24, 2, Color{ 255, 255, 255, (unsigned char)(80 + pulse * 100) });
    DrawRectangle((int)right + 10, y - 1, 24, 2, Color{ 255, 255, 255, (unsigned char)(80 + pulse * 100) });

    const float cx = PLAYFIELD_X + PLAYFIELD_WIDTH * 0.5f;
    const float archY = judgmentLineY + 92.0f;

    DrawRing({ cx, archY }, 86.0f, 100.0f, 205.0f, 335.0f, 64, Color{ 0, 0, 0, 250 });
    DrawRing({ cx, archY }, 100.0f, 104.0f, 205.0f, 335.0f, 64, Color{ 90, 90, 92, 210 });
    DrawRing({ cx, archY }, 108.0f, 112.0f, 208.0f, 332.0f, 64, Color{ 235, 235, 238, 145 });

    DrawRectangle((int)(cx - 132), (int)(judgmentLineY + 54), 264, 3, Color{ 5, 5, 6, 245 });
    DrawRectangle((int)(cx - 116), (int)(judgmentLineY + 57), 232, 2, Color{ 115, 115, 118, 130 });

    for (int i = 0; i < 9; ++i)
    {
        const float x = cx - 104.0f + i * 26.0f;
        const float h = 8.0f + 5.0f * (0.5f + 0.5f * sinf(t * 2.0f + i));
        DrawRectangle((int)x, (int)(judgmentLineY + 42), 10, (int)h,
            Color{ 190, 190, 193, (unsigned char)(45 + i * 5) });
    }
}

static void DrawJudgmentText()
{
if (!s_ShowJudgment ||
s_SuitFont.texture.id == 0)
{
return;
}

float timerProgress =
    Clamp01(
        s_JudgmentAnimTimer /
        0.3f
    );

float appear =
    1.0f -
    timerProgress;

float ease =
    EaseOutBack(appear);

float alpha =
    Clamp01(
        s_JudgmentTimer /
        0.4f
    );

float scale =
    0.84f +
    ease * 0.16f;

float fontSize =
    31.0f *
    scale;

std::string rawJudgment =
    std::string(s_CurrentJudgment);

Vector2 textSize =
    MeasureTextEx(
        s_SuitFont,
        rawJudgment.c_str(),
        fontSize,
        2.0f
    );

float centerX =
    PLAYFIELD_X +
    PLAYFIELD_WIDTH * 0.5f;

float textX =
    centerX -
    textSize.x * 0.5f;

float textY = 282.0f;

Color mainColor =
    Color{ 247, 247, 247, 255 };

Color accentColor =
    Color{ 189, 189, 189, 190 };

if (rawJudgment == "GREAT")
{
    mainColor =
        Color{ 230, 230, 230, 255 };

    accentColor =
        Color{ 154, 154, 154, 170 };
}
else if (rawJudgment == "GOOD")
{
    mainColor =
        Color{ 197, 197, 197, 255 };

    accentColor =
        Color{ 127, 127, 127, 150 };
}
else if (rawJudgment == "MISS")
{
    mainColor =
        Color{ 136, 136, 136, 255 };

    accentColor =
        Color{ 88, 88, 88, 140 };
}

mainColor.a =
    (unsigned char)(255.0f * alpha);

accentColor.a =
    (unsigned char)(accentColor.a * alpha);

float lineWidth =
    52.0f +
    ease * 55.0f;

DrawRectangle(
    (int)(centerX - lineWidth),
    (int)(textY - 9.0f),
    (int)(lineWidth * 2.0f),
    1,
    Color{
        accentColor.r,
        accentColor.g,
        accentColor.b,
        (unsigned char)(accentColor.a * 0.55f)
    }
);

DrawRectangle(
    (int)(centerX - lineWidth * 0.55f),
    (int)(textY + textSize.y + 7.0f),
    (int)(lineWidth * 1.1f),
    1,
    accentColor
);

DrawTextEx(
    s_SuitFont,
    rawJudgment.c_str(),
    {
        textX + 3.0f,
        textY + 3.0f
    },
    fontSize,
    2.0f,
    Color{
        0,
        0,
        0,
        (unsigned char)(180.0f * alpha)
    }
);

DrawTextEx(
    s_SuitFont,
    rawJudgment.c_str(),
    {
        textX,
        textY
    },
    fontSize,
    2.0f,
    mainColor
);

float smallY =
    textY +
    textSize.y +
    17.0f;

const char* status =
    rawJudgment == "MISS"
        ? "OFF BEAT"
        : "TIMING LOCK";

Vector2 statusSize =
    MeasureTextEx(
        s_SuitFont,
        status,
        9.0f,
        1.0f
    );

DrawTextEx(
    s_SuitFont,
    status,
    {
        centerX - statusSize.x * 0.5f,
        smallY
    },
    9.0f,
    1.0f,
    Color{ 170, 170, 170,
        (unsigned char)(150.0f * alpha)
    }
);

}

static void DrawComboHUD()
{
if (s_Combo <= 0 ||
s_ComboFont.texture.id == 0 ||
s_SuitFont.texture.id == 0)
{
return;
}

float pulse =
    s_ComboAnimTimer > 0.0f
        ? Clamp01(s_ComboAnimTimer / 0.2f)
        : 0.0f;

float comboScale =
    1.0f +
    EaseOutBack(1.0f - pulse) *
    0.08f;

float comboSize =
    73.0f *
    comboScale;

std::string comboText =
    std::to_string(s_Combo);

Vector2 comboTextSize =
    MeasureTextEx(
        s_ComboFont,
        comboText.c_str(),
        comboSize,
        1.0f
    );

float centerX =
    PLAYFIELD_X +
    PLAYFIELD_WIDTH * 0.5f;

float comboX =
    centerX -
    comboTextSize.x * 0.5f;

float comboY =
    105.0f -
    (1.0f - pulse) * 4.0f;

float glow =
    0.5f +
    0.5f *
    sinf(GetTime() * 4.0f);

DrawRectangle(
    (int)(centerX - 108.0f),
    (int)comboY - 12,
    216,
    1,
    Color{ 136, 136, 136,
        (unsigned char)(55.0f + glow * 25.0f)
    }
);

DrawRectangle(
    (int)(centerX - 70.0f),
    (int)comboY - 7,
    140,
    1,
    Color{ 238, 238, 238, 65 }
);

DrawRectangle(
    (int)(centerX - 108.0f),
    (int)comboY + 11,
    34,
    1,
    Color{ 193, 193, 193, 120 }
);

DrawRectangle(
    (int)(centerX + 74.0f),
    (int)comboY + 11,
    34,
    1,
    Color{ 193, 193, 193, 120 }
);

DrawTextEx(
    s_SuitFont,
    "COMBO",
    {
        centerX - 28.0f,
        comboY - 33.0f
    },
    16.0f,
    2.0f,
    Color{ 170, 170, 170, 230 }
);

DrawTextEx(
    s_ComboFont,
    comboText.c_str(),
    {
        comboX + 5.0f,
        comboY + 5.0f
    },
    comboSize,
    1.0f,
    Color{ 108, 108, 108, 70 }
);

DrawTextEx(
    s_ComboFont,
    comboText.c_str(),
    {
        comboX + 2.0f,
        comboY + 2.0f
    },
    comboSize,
    1.0f,
    Color{ 0, 0, 0, 240 }
);

DrawTextEx(
    s_ComboFont,
    comboText.c_str(),
    {
        comboX,
        comboY
    },
    comboSize,
    1.0f,
    Color{ 250, 250, 250, 255 }
);

float lineY =
    comboY +
    comboTextSize.y +
    7.0f;

DrawRectangle(
    (int)(centerX - 94.0f),
    (int)lineY,
    45,
    2,
    Color{ 171, 171, 171, 120 }
);

DrawRectangle(
    (int)(centerX - 42.0f),
    (int)lineY,
    84,
    1,
    Color{ 246, 246, 246, 100 }
);

DrawRectangle(
    (int)(centerX + 49.0f),
    (int)lineY,
    45,
    2,
    Color{ 171, 171, 171, 120 }
);

DrawCircle(
    (int)(centerX - 103.0f),
    (int)lineY + 1,
    2.0f,
    Color{ 211, 211, 211, 150 }
);

DrawCircle(
    (int)(centerX + 103.0f),
    (int)lineY + 1,
    2.0f,
    Color{ 211, 211, 211, 150 }
);

if (s_ComboAnimTimer > 0.0f)
{
    float t =
        1.0f -
        Clamp01(
            s_ComboAnimTimer /
            0.2f
        );

    float burstRadius =
        12.0f +
        t * 48.0f;

    unsigned char burstAlpha =
        (unsigned char)(
            (1.0f - t) *
            95.0f
        );

    DrawCircleLines(
        (int)centerX,
        (int)(comboY + comboTextSize.y * 0.52f),
        burstRadius,
        Color{ 203, 203, 203,
            burstAlpha
        }
    );

    DrawCircleLines(
        (int)centerX,
        (int)(comboY + comboTextSize.y * 0.52f),
        burstRadius + 4.0f,
        Color{ 165, 165, 165,
            (unsigned char)(burstAlpha * 0.35f)
        }
    );
}

}

static void DrawInputFeedbackFlash(
const bool pressedStates[4],
float judgmentLineY
)
{
const float time = GetTime();

for (int i = 0; i < LANE_COUNT; ++i)
{
    if (!pressedStates[i])
        continue;

    float laneX =
        LANE_START_X +
        i * LANE_WIDTH;

    float pulse =
        0.5f +
        0.5f *
        sinf(
            time * 13.0f +
            i * 0.7f
        );

    unsigned char alpha =
        (unsigned char)(
            45.0f +
            pulse * 65.0f
        );

    DrawRectangleGradientV(
        (int)laneX + 5,
        30,
        (int)LANE_WIDTH - 10,
        (int)judgmentLineY - 42,
        Color{ 189, 189, 189, 0 },
        Color{ 189, 189, 189, alpha }
    );

    DrawRectangle(
        (int)laneX + 7,
        (int)judgmentLineY - 19,
        (int)LANE_WIDTH - 14,
        3,
        Color{ 234, 234, 234, alpha }
    );

    DrawRectangle(
        (int)laneX + 13,
        (int)judgmentLineY + 8,
        (int)LANE_WIDTH - 26,
        2,
        Color{ 198, 198, 198,
            (unsigned char)(alpha * 0.7f)
        }
    );

    DrawCircleLines(
        (int)(laneX + LANE_WIDTH * 0.5f),
        (int)judgmentLineY,
        14.0f + pulse * 6.0f,
        Color{ 208, 208, 208,
            (unsigned char)(alpha * 0.65f)
        }
    );
}

}

static void DrawInputPanel(const bool pressedStates[4], float judgmentLineY)
{
    const float panelY = 604.0f;
    const float buttonY = 632.0f;
    const float buttonH = 78.0f;
    const float time = GetTime();

    DrawRectangle(
        (int)PLAYFIELD_X,
        (int)panelY,
        (int)PLAYFIELD_WIDTH,
        116,
        Color{ 2, 2, 3, 255 }
    );

    DrawRectangleGradientV(
        (int)PLAYFIELD_X,
        (int)panelY,
        (int)PLAYFIELD_WIDTH,
        34,
        Color{ 26, 26, 29, 255 },
        Color{ 5, 5, 7, 255 }
    );

    DrawRectangle(
        (int)PLAYFIELD_X,
        (int)panelY,
        (int)PLAYFIELD_WIDTH,
        2,
        Color{ 255, 255, 255, 235 }
    );

    DrawRectangle(
        (int)PLAYFIELD_X + 8,
        (int)panelY + 28,
        (int)PLAYFIELD_WIDTH - 16,
        1,
        Color{ 255, 255, 255, 28 }
    );

    for (int i = 0; i < LANE_COUNT; ++i)
    {
        const float x = LANE_START_X + i * LANE_WIDTH;
        const float cx = LANE_X_COORDS[i];
        const bool pressed = pressedStates[i];
        const float pulse = 0.5f + 0.5f * sinf(time * 9.0f + i * 0.8f);

        if (pressed)
        {
            DrawRectangleGradientV(
                (int)x,
                (int)judgmentLineY,
                (int)LANE_WIDTH,
                116,
                Color{ 255, 255, 255, 0 },
                Color{ 255, 255, 255, 28 }
            );

            DrawRectangle(
                (int)x + 4,
                (int)judgmentLineY + 1,
                (int)LANE_WIDTH - 8,
                3,
                Color{ 255, 255, 255, (unsigned char)(110 + pulse * 100) }
            );
        }

        DrawRectangle(
            (int)x + 2,
            (int)buttonY + 8,
            (int)LANE_WIDTH - 4,
            (int)buttonH,
            Color{ 0, 0, 0, 245 }
        );

        DrawRectangleRounded(
            { x + 1.0f, buttonY, LANE_WIDTH - 2.0f, buttonH },
            0.08f,
            8,
            pressed ? Color{ 245, 245, 247, 255 } : Color{ 82, 82, 86, 255 }
        );

        DrawRectangleRounded(
            { x + 5.0f, buttonY + 4.0f, LANE_WIDTH - 10.0f, buttonH - 8.0f },
            0.06f,
            8,
            pressed ? Color{ 128, 128, 132, 255 } : Color{ 13, 13, 15, 255 }
        );

        DrawRectangleGradientV(
            (int)x + 7,
            (int)buttonY + 6,
            (int)LANE_WIDTH - 14,
            (int)buttonH - 12,
            pressed ? Color{ 205, 205, 208, 255 } : Color{ 49, 49, 53, 255 },
            pressed ? Color{ 62, 62, 65, 255 } : Color{ 5, 5, 7, 255 }
        );

        DrawRectangle(
            (int)x + 9,
            (int)buttonY + 9,
            (int)LANE_WIDTH - 18,
            3,
            pressed
                ? Color{ 255, 255, 255, (unsigned char)(205 + pulse * 50) }
                : Color{ 185, 185, 190, 105 }
        );

        DrawRectangle(
            (int)x + 9,
            (int)buttonY + (int)buttonH - 12,
            (int)LANE_WIDTH - 18,
            4,
            Color{ 0, 0, 0, 190 }
        );

        DrawRectangle(
            (int)x + 10,
            (int)buttonY + 14,
            3,
            27,
            Color{ 255, 255, 255, (unsigned char)(pressed ? 100 : 28) }
        );

        DrawRectangle(
            (int)x + (int)LANE_WIDTH - 13,
            (int)buttonY + 14,
            3,
            27,
            Color{ 0, 0, 0, 135 }
        );

        if (pressed)
        {
            DrawRectangle(
                (int)x + 4,
                (int)buttonY + 3,
                (int)LANE_WIDTH - 8,
                2,
                Color{ 255, 255, 255, 245 }
            );

            DrawRectangle(
                (int)x + 4,
                (int)buttonY + (int)buttonH - 2,
                (int)LANE_WIDTH - 8,
                2,
                Color{ 255, 255, 255, 125 }
            );

            DrawCircleLines(
                (int)cx,
                (int)buttonY + buttonH * 0.5f,
                25.0f + pulse * 4.0f,
                Color{ 255, 255, 255, (unsigned char)(35 + pulse * 45) }
            );
        }

        if (s_SuitFont.texture.id != 0)
        {
            const char* key =
                i == 0 ? "D" :
                i == 1 ? "F" :
                i == 2 ? "J" : "K";

            const float fontSize = 31.0f;
            Vector2 size = MeasureTextEx(s_SuitFont, key, fontSize, 1.0f);

            DrawTextEx(
                s_SuitFont,
                key,
                { cx - size.x * 0.5f, buttonY + buttonH * 0.5f - size.y * 0.5f },
                fontSize,
                1.0f,
                pressed ? Color{ 255, 255, 255, 255 } : Color{ 228, 228, 232, 255 }
            );
        }
    }

    DrawRectangle(
        (int)LANE_START_X,
        629,
        (int)LANE_AREA_WIDTH,
        1,
        Color{ 255, 255, 255, 55 }
    );
}

[[maybe_unused]] static void DrawPlaySceneSideMarkers()
{
const float time = GetTime();

const float left =
    PLAYFIELD_X - 42.0f;

const float right =
    PLAYFIELD_X +
    PLAYFIELD_WIDTH +
    42.0f;

float pulse =
    0.5f +
    0.5f *
    sinf(time * 2.5f);

DrawRectangleRounded(
    Rectangle{
        left - 24.0f,
        88.0f,
        30.0f,
        58.0f
    },
    0.16f,
    8,
    Color{ 10, 14, 17, 240 }
);

DrawRectangleRounded(
    Rectangle{
        right - 6.0f,
        88.0f,
        30.0f,
        58.0f
    },
    0.16f,
    8,
    Color{ 10, 14, 17, 240 }
);

DrawRectangleRoundedLines(
    Rectangle{
        left - 24.0f,
        88.0f,
        30.0f,
        58.0f
    },
    0.16f,
    8,
    Color{
        90,
        135,
        145,
        (unsigned char)(100.0f + pulse * 30.0f)
    }
);

DrawRectangleRoundedLines(
    Rectangle{
        right - 6.0f,
        88.0f,
        30.0f,
        58.0f
    },
    0.16f,
    8,
    Color{
        90,
        135,
        145,
        (unsigned char)(100.0f + pulse * 30.0f)
    }
);

DrawRectangle(
    left - 12.0f,
    112.0f,
    13,
    2,
    Color{ 190, 235, 242, 180 }
);

DrawRectangle(
    right - 1.0f,
    112.0f,
    13,
    2,
    Color{ 190, 235, 242, 180 }
);

DrawCircle(
    (int)(left - 6.0f),
    113,
    3.0f,
    Color{ 110, 205, 220, 180 }
);

DrawCircle(
    (int)(right + 6.0f),
    113,
    3.0f,
    Color{ 110, 205, 220, 180 }
);

DrawRectangle(
    left - 5.0f,
    169.0f,
    8,
    2,
    Color{ 120, 170, 180, 120 }
);

DrawRectangle(
    right - 3.0f,
    169.0f,
    8,
    2,
    Color{ 120, 170, 180, 120 }
);

for (int i = 0; i < 3; ++i)
{
    float y =
        205.0f +
        i * 20.0f;

    DrawRectangle(
        (int)(left - 22.0f),
        (int)y,
        14,
        1,
        Color{ 90, 130, 140, 55 }
    );

    DrawRectangle(
        (int)(right + 8.0f),
        (int)y,
        14,
        1,
        Color{ 90, 130, 140, 55 }
    );
}

}

[[maybe_unused]] static void DrawUpperTechnicalHUD()
{
const float time = GetTime();

float left =
    PLAYFIELD_X - 17.0f;

float right =
    PLAYFIELD_X +
    PLAYFIELD_WIDTH +
    17.0f;

float pulse =
    0.5f +
    0.5f *
    sinf(time * 1.8f);

DrawRectangle(
    (int)left,
    18,
    (int)(right - left),
    2,
    Color{
        145,
        185,
        192,
        (unsigned char)(65.0f + pulse * 20.0f)
    }
);

DrawRectangle(
    (int)left,
    22,
    48,
    1,
    Color{ 210, 240, 245, 150 }
);

DrawRectangle(
    (int)(right - 48.0f),
    22,
    48,
    1,
    Color{ 210, 240, 245, 150 }
);

DrawRectangle(
    (int)left,
    34,
    100,
    25,
    Color{ 8, 12, 15, 235 }
);

DrawRectangleLines(
    (int)left,
    34,
    100,
    25,
    Color{ 95, 135, 145, 125 }
);

DrawRectangle(
    (int)left + 5,
    39,
    2,
    14,
    Color{ 100, 205, 220, 150 }
);

DrawTextEx(
    s_SuitFont,
    "INPUT MATRIX",
    {
        left + 13.0f,
        40.0f
    },
    9.0f,
    1.0f,
    Color{ 165, 200, 208, 225 }
);

DrawRectangle(
    (int)(right - 100.0f),
    34,
    100,
    25,
    Color{ 8, 12, 15, 235 }
);

DrawRectangleLines(
    (int)(right - 100.0f),
    34,
    100,
    25,
    Color{ 95, 135, 145, 125 }
);

DrawRectangle(
    (int)(right - 7.0f),
    39,
    2,
    14,
    Color{ 100, 205, 220, 150 }
);

DrawTextEx(
    s_SuitFont,
    "SYNC / 4L",
    {
        right - 84.0f,
        40.0f
    },
    9.0f,
    1.0f,
    Color{ 165, 200, 208, 225 }
);

for (int i = 0; i < 8; ++i)
{
    float x =
        PLAYFIELD_X +
        8.0f +
        i * 43.0f;

    float level =
        0.35f +
        0.65f *
        (0.5f +
         0.5f *
         sinf(
             time * 1.7f +
             i * 0.7f
         ));

    DrawRectangle(
        (int)x,
        66,
        25,
        2,
        Color{ 65, 90, 98, 90 }
    );

    DrawRectangle(
        (int)x,
        66,
        (int)(25.0f * level),
        2,
        Color{
            95,
            175,
            190,
            (unsigned char)(45.0f + level * 55.0f)
        }
    );

    DrawRectangle(
        (int)(x + 28.0f),
        66,
        5,
        2,
        Color{ 150, 200, 210, 100 }
    );
}

DrawTextEx(
    s_SuitFont,
    "LIVE",
    {
        PLAYFIELD_X + 8.0f,
        78.0f
    },
    8.0f,
    1.0f,
    Color{ 100, 170, 182, 145 }
);

DrawTextEx(
    s_SuitFont,
    "004 LANES",
    {
        PLAYFIELD_X + PLAYFIELD_WIDTH - 72.0f,
        78.0f
    },
    8.0f,
    1.0f,
    Color{ 100, 170, 182, 145 }
);

}

[[maybe_unused]] static void DrawTopStatusStrip()
{
const float time = GetTime();

float centerX =
    PLAYFIELD_X +
    PLAYFIELD_WIDTH * 0.5f;

float pulse =
    0.5f +
    0.5f *
    sinf(time * 2.0f);

DrawRectangle(
    (int)(centerX - 72.0f),
    8,
    144,
    1,
    Color{
        120,
        180,
        192,
        (unsigned char)(45.0f + pulse * 20.0f)
    }
);

DrawTextEx(
    s_SuitFont,
    "THE LINE",
    {
        centerX - 29.0f,
        27.0f
    },
    10.0f,
    1.2f,
    Color{ 170, 205, 212, 210 }
);

DrawRectangle(
    (int)(centerX - 18.0f),
    43,
    36,
    1,
    Color{ 125, 190, 200, 90 }
);

}

static void DrawPauseUI()
{
    if (!s_IsPaused) return;

    std::vector<std::string> texts = {
        "A Night Without Visible Stars", "Four Beat Sounds", "Mapper_A",
        "별이 보이지 않는 밤", "Plum", "boyangsic", "Kaleidoscope", "Timeline", "R",
        "Cybernetic Dream", "Neo Synth", "Mapper_C",
        "Neon Highway", "Retro Pulse", "Mapper_B",
        "작곡가:", "에디터:", "BPM:", "길이:",
        "곡 선택", "플레이", "뒤로가기",
        "엔터 누르면 시작 가능",
        "계속하기", "나가기"
    };

    const float screenW = (float)GetScreenWidth();
    const float screenH = (float)GetScreenHeight();

    DrawRectangle(0, 0, (int)screenW, (int)screenH, Color{ 0, 0, 0, 180 });

    const float panelW = 340.0f;
    const float panelH = 220.0f;
    const float panelX = (screenW - panelW) * 0.5f;
    const float panelY = (screenH - panelH) * 0.5f;

    DrawRectangleRounded({ panelX, panelY, panelW, panelH }, 0.1f, 8, Color{ 15, 18, 24, 245 });
    DrawRectangleRoundedLines({ panelX, panelY, panelW, panelH }, 0.1f, 8, Color{ 110, 185, 225, 210 });

    Font fontToUse = (s_SuitFont.texture.id != 0) ? s_SuitFont : GetFontDefault();

    const char* title = "PAUSED";
    Vector2 titleSize = MeasureTextEx(fontToUse, title, 28.0f, 1.5f);
    DrawTextEx(fontToUse, title, { (screenW - titleSize.x) * 0.5f, panelY + 22.0f }, 28.0f, 1.5f, Color{ 245, 245, 250, 255 });

    const float btnW = 240.0f;
    const float btnH = 46.0f;

    for (int i = 0; i < 2; ++i)
    {
        float btnX = (screenW - btnW) * 0.5f;
        float btnY = panelY + 80.0f + i * 62.0f;
        bool isSelected = (s_PauseSelection == i);

        Color btnBg = isSelected ? Color{ 45, 85, 125, 230 } : Color{ 28, 32, 42, 210 };
        Color btnBorder = isSelected ? Color{ 120, 210, 255, 255 } : Color{ 70, 75, 85, 180 };
        Color textColor = isSelected ? Color{ 255, 255, 255, 255 } : Color{ 175, 180, 190, 255 };

        DrawRectangleRounded({ btnX, btnY, btnW, btnH }, 0.15f, 8, btnBg);
        DrawRectangleRoundedLines({ btnX, btnY, btnW, btnH }, 0.15f, 8, btnBorder);

        if (isSelected)
        {
            DrawRectangle((int)btnX + 8, (int)btnY + 11, 4, (int)btnH - 22, Color{ 120, 210, 255, 255 });
        }

        std::string optText = (i == 0) ? texts[23] : texts[24];
        Vector2 optSize = MeasureTextEx(fontToUse, optText.c_str(), 20.0f, 1.0f);
        DrawTextEx(fontToUse, optText.c_str(), { (screenW - optSize.x) * 0.5f, btnY + (btnH - optSize.y) * 0.5f }, 20.0f, 1.0f, textColor);
    }
}

PlayScene::PlayScene(SongSelect& sharedSongSelect)
: m_State(PlaySceneState::SongSelect),
  m_SongSelect(sharedSongSelect), 
  m_BackToMenu(false),
  judgmentLineY(595.0f)
{
    s_MusicPlayer.p1 = new MusicExecute::MusicPlayer1();
    s_MusicPlayer.p2 = new MusicExecute::MusicPlayer2();
    s_MusicPlayer.p3 = new MusicExecute::MusicPlayer3();
    s_MusicPlayer.p4 = new MusicExecute::MusicPlayer4();
    s_MusicPlayer.p5 = new MusicExecute::MusicPlayer5();
    s_MusicPlayer.p6 = new MusicExecute::MusicPlayer6();
    s_MusicPlayer.p7 = new MusicExecute::MusicPlayer7();
    s_MusicPlayer.p8 = new MusicExecute::MusicPlayer8();
    s_MusicPlayer.p9 = new MusicExecute::MusicPlayer9();
    s_MusicPlayer.p10 = new MusicExecute::MusicPlayer10();
}

PlayScene::~PlayScene()
{

delete s_MusicPlayer.p1; 
delete s_MusicPlayer.p2;  
delete s_MusicPlayer.p3;
delete s_MusicPlayer.p4;  
delete s_MusicPlayer.p5;  
delete s_MusicPlayer.p6;
delete s_MusicPlayer.p7;  
delete s_MusicPlayer.p8;  
delete s_MusicPlayer.p9;
delete s_MusicPlayer.p10;

/*if (s_MusicPlayer.p1)
{
delete s_MusicPlayer.p1;
s_MusicPlayer.p1 = nullptr;
}
if (s_MusicPlayer.p2)
{
delete s_MusicPlayer.p2;
s_MusicPlayer.p2 = nullptr;
}
if (s_MusicPlayer.p3)
{
delete s_MusicPlayer.p3;
s_MusicPlayer.p3 = nullptr; */
}

void PlayScene::Init(int startSongIndex)
{
    m_State = PlaySceneState::SongSelect;
    m_BackToMenu = false;
    judgmentLineY = 595.0f;

    s_BeatmapClock = FramedBeatmapClock(true);
    s_BeatmapClock.LoadComplete();

    s_AudioManager.Init();

    if (s_MusicPlayer.IsValid())
    {
        s_MusicPlayer.Init(s_AudioManager);
    }

    m_SongSelect.SetSelectedSongIndex(startSongIndex); 

    m_SongSelect.ResetPlayRequest(); 

    s_Notes.clear();
    s_PlayableNotes.clear();

    s_SongTimer = 0.0f;
    s_SpawnTimer = 0.0f;
    s_Combo = 0;

    s_ShowJudgment = false;
    s_JudgmentTimer = 0.0f;
    s_IsEditorMode = false;
    s_IsPaused = false;
    s_PauseSelection = 0;
    
    s_IgnoreFirstEnter = true; 
    s_SongSelectEnterDelay = 0.1f;

    s_JudgmentLinePulse = 0.0f;
    s_JudgmentAnimTimer = 0.0f;
    s_ComboAnimTimer = 0.0f;
    s_LastCombo = 0;
    s_NoteScrollSpeed = 200.0f;

    s_ComboFont = LoadFont("fonts/Pretendard-Black.ttf");
    s_SuitFont = LoadFont("fonts/Pretendard-Black.ttf");
}


void PlayScene::Update()
{
    if (s_SongSelectEnterDelay > 0.0f)
    {
        s_SongSelectEnterDelay -= GetFrameTime();
        if (s_SongSelectEnterDelay < 0.0f) s_SongSelectEnterDelay = 0.0f;
    }

    if (IsKeyPressed(KEY_P))
    {
        s_IsEditorMode = true;
        m_State = PlaySceneState::Playing;
        s_ChartEditor.Init();
        return;
    }

    s_AudioManager.Update();

    if (s_MusicPlayer.IsValid())
    {
        s_MusicPlayer.Update(GetFrameTime());
    }

    if (m_State == PlaySceneState::SongSelect)
    {
        m_SongSelect.Update();

        if (m_SongSelect.IsBackSelected())
        {
            m_BackToMenu = true;
            return;
        }
        else if (m_SongSelect.IsEditorSelected())
        {
            s_IsEditorMode = true;
            m_State = PlaySceneState::Playing;
            s_ChartEditor.Init();
            return;
        }
        else if (m_SongSelect.IsPlaySelected() && s_SongSelectEnterDelay <= 0.0f)
        {
            m_SongSelect.ResetPlayRequest(); 
            
            int currentSelectedIdx = m_SongSelect.GetSelectedSongIndex(); 
            
            const SongData& curSong = m_SongSelect.GetCurrentSong();
            std::string osuFileName = curSong.osuFileName; 
            s_MusicPlayer.active = curSong.musicPlayerActive;

            std::string outAudioFile, outTitle, outArtist, outCreator, outVersion;
            float outHP = 5.0f, outOD = 5.0f, outCS = 4.0f, outAR = 5.0f;
            float outSliderMultiplier = 1.4f, outSliderTickRate = 1.0f;
            std::vector<SaveNoteData> loadedNotes;

            if (ChartSave::LoadFromOsu(
                    osuFileName.c_str(), outAudioFile, outTitle, outArtist, outCreator, outVersion,
                    outHP, outOD, outCS, outAR, outSliderMultiplier, outSliderTickRate, loadedNotes))
            {
                if (outAR <= 0.0f) outAR = 5.0f;
                s_NoteScrollSpeed = outAR * 50.0f;

                s_Notes.clear();
                s_PlayableNotes.clear();

                for (size_t i = 0; i < loadedNotes.size(); ++i)
                {
                    const auto& saveNote = loadedNotes[i];
                    PlayableNote pNote;
                    pNote.timeSec = (float)saveNote.time / 1000.0f;
                    pNote.endTimeSec = (float)saveNote.endTime / 1000.0f;
                    pNote.lane = saveNote.lane;
                    pNote.type = saveNote.type;
                    pNote.active = true;
                    pNote.isHolding = false;
                    pNote.lastTickTime = 0.0f;
                    s_PlayableNotes.push_back(pNote);
                }
            }
            else
            {
                m_State = PlaySceneState::SongSelect;
                return;
            }

            if (!outAudioFile.empty() && s_MusicPlayer.IsValid())
            {
                s_MusicPlayer.Stop();
                s_BeatmapClock.SetChannel(nullptr);
                s_MusicPlayer.Play(s_AudioManager, 0);
                s_MusicPlayer.PlayImmediate();
                s_MusicPlayer.SetPitch(1.0f);

                FMOD_SYSTEM* system = s_AudioManager.GetSystemRaw();
                FMOD_CHANNEL* channel = s_MusicPlayer.GetChannelRaw();
                s_BeatmapClock.SetSystem(system);
                s_BeatmapClock.SetChannel(channel);
                s_BeatmapClock.LoadComplete();
                s_BeatmapClock.Start();
            }

            s_SongTimer = 0.0f;
            s_IsPaused = false;
            s_PauseSelection = 0;
            s_IgnoreFirstEnter = true; 
            
            m_SongSelect.SetSelectedSongIndex(currentSelectedIdx); 

            m_State = PlaySceneState::Playing;
            return; 
        }
    }
    else if (m_State == PlaySceneState::Playing)
    {
        UpdatePlaying();
    }
}


void PlayScene::UpdatePlaying()
{
    if (s_IsEditorMode)
    {
        if (IsKeyPressed(KEY_P))
        {
            s_IsEditorMode = false;
            m_State = PlaySceneState::SongSelect;
            m_BackToMenu = true;
            if (s_MusicPlayer.IsValid()) s_MusicPlayer.Stop();
            return;
        }

        if (IsKeyPressed(KEY_ESCAPE))
        {
            s_IsEditorMode = false;
            s_SongSelectEnterDelay = 0.3f;
            m_State = PlaySceneState::SongSelect;
            return;
        }

        s_ChartEditor.HandleInput();
        return;
    }

    if (s_IgnoreFirstEnter)
    {
        s_IgnoreFirstEnter = false;
        return;
    }

    if (IsKeyPressed(KEY_ENTER))
    {
        if (!s_IsPaused)
        {
            s_IsPaused = true;
            s_PauseSelection = 0;
            if (s_MusicPlayer.IsValid() && s_MusicPlayer.GetChannelRaw())
            {
                FMOD_Channel_SetPaused(s_MusicPlayer.GetChannelRaw(), true);
            }
            return;
        }
        else
        {
            if (s_PauseSelection == 0) 
            {
                s_IsPaused = false;
                if (s_MusicPlayer.IsValid() && s_MusicPlayer.GetChannelRaw())
                {
                    FMOD_Channel_SetPaused(s_MusicPlayer.GetChannelRaw(), false);
                }
            }
            else if (s_PauseSelection == 1) 
            {
                if (s_MusicPlayer.IsValid()) s_MusicPlayer.Stop();
                s_PlayableNotes.clear();
                s_PlayableNotes.shrink_to_fit();
                s_Notes.clear();
                s_Notes.shrink_to_fit();
                s_BeatmapClock.SetChannel(nullptr);
                s_SongTimer = 0.0f;
                s_IsPaused = false;
                s_SongSelectEnterDelay = 0.3f;
                m_State = PlaySceneState::SongSelect;
            }
            return;
        }
    }

    if (s_IsPaused)
    {
        if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))
        {
            s_PauseSelection = (s_PauseSelection - 1 + 2) % 2;
        }
        if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S))
        {
            s_PauseSelection = (s_PauseSelection + 1) % 2;
        }

        if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_Z))
        {
            if (s_PauseSelection == 0) 
            {
                s_IsPaused = false;
                if (s_MusicPlayer.IsValid() && s_MusicPlayer.GetChannelRaw())
                {
                    FMOD_Channel_SetPaused(s_MusicPlayer.GetChannelRaw(), false);
                }
            }
            else if (s_PauseSelection == 1) 
            {
                if (s_MusicPlayer.IsValid()) s_MusicPlayer.Stop();
                s_PlayableNotes.clear();
                s_PlayableNotes.shrink_to_fit();
                s_Notes.clear();
                s_Notes.shrink_to_fit();
                s_BeatmapClock.SetChannel(nullptr);
                s_SongTimer = 0.0f;
                s_IsPaused = false;
                s_SongSelectEnterDelay = 0.3f;
                m_State = PlaySceneState::SongSelect;
            }
        }

        const float screenW = (float)GetScreenWidth();
        const float screenH = (float)GetScreenHeight();
        const float panelY = (screenH - 220.0f) * 0.5f;
        const float btnW = 240.0f;
        const float btnH = 46.0f;
        Vector2 mousePos = GetMousePosition();

        for (int i = 0; i < 2; ++i)
        {
            float btnX = (screenW - btnW) * 0.5f;
            float btnY = panelY + 80.0f + i * 62.0f;

            if (CheckCollisionPointRec(mousePos, { btnX, btnY, btnW, btnH }))
            {
                s_PauseSelection = i;
                if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
                {
                    if (i == 0) 
                    {
                        s_IsPaused = false;
                        if (s_MusicPlayer.IsValid() && s_MusicPlayer.GetChannelRaw())
                        {
                            FMOD_Channel_SetPaused(s_MusicPlayer.GetChannelRaw(), false);
                        }
                    }
                    else if (i == 1) 
                    {
                        if (s_MusicPlayer.IsValid()) s_MusicPlayer.Stop();
                        s_PlayableNotes.clear();
                        s_PlayableNotes.shrink_to_fit();
                        s_Notes.clear();
                        s_Notes.shrink_to_fit();
                        s_BeatmapClock.SetChannel(nullptr);
                        s_SongTimer = 0.0f;
                        s_IsPaused = false;
                        s_SongSelectEnterDelay = 0.3f;
                        m_State = PlaySceneState::SongSelect;
                    }
                }
            }
        }

        return; 
    }

    if (s_SongTimer > 1.0f && s_MusicPlayer.IsValid())
    {
        FMOD_CHANNEL* ch = s_MusicPlayer.GetChannelRaw();
        if (ch)
        {
            FMOD_BOOL isPlaying = false;
            FMOD_Channel_IsPlaying(ch, &isPlaying);
            if (!isPlaying)
            {
                s_MusicPlayer.Stop();
                s_PlayableNotes.clear();
                s_SongTimer = 0.0f;
                s_BeatmapClock.SetChannel(nullptr);
                s_SongSelectEnterDelay = 0.3f;
                m_State = PlaySceneState::SongSelect; 
                return;
            }
        }
    }

    float dt = GetFrameTime();

    if (IsKeyPressed(KEY_LEFT_BRACKET))
    {
        s_BeatmapClock.SetUserGlobalOffset(s_BeatmapClock.GetUserGlobalOffset() - 5.0);
    }
    if (IsKeyPressed(KEY_RIGHT_BRACKET))
    {
        s_BeatmapClock.SetUserGlobalOffset(s_BeatmapClock.GetUserGlobalOffset() + 5.0);
    }

    s_SongTimer = static_cast<float>(s_BeatmapClock.GetCurrentTime() / 1000.0);

    if (s_JudgmentLinePulse > 0.0f)
    {
        s_JudgmentLinePulse -= dt * 4.0f;
        if (s_JudgmentLinePulse < 0.0f) s_JudgmentLinePulse = 0.0f;
    }

    if (s_JudgmentAnimTimer > 0.0f)
    {
        s_JudgmentAnimTimer -= dt;
        if (s_JudgmentAnimTimer < 0.0f) s_JudgmentAnimTimer = 0.0f;
    }

    if (s_ComboAnimTimer > 0.0f)
    {
        s_ComboAnimTimer -= dt;
        if (s_ComboAnimTimer < 0.0f) s_ComboAnimTimer = 0.0f;
    }

    for (auto& pNote : s_PlayableNotes)
    {
        if (pNote.active)
        {
            if (pNote.type == 128)
            {
                if (!pNote.isHolding)
                {
                    float timeDiff = s_SongTimer - pNote.timeSec;
                    if (timeDiff > 0.3f)
                    {
                        pNote.active = false;
                        s_Combo = 0;
                        s_LastCombo = 0;
                        s_ShowJudgment = true;
                        s_JudgmentTimer = 0.3f;
                        s_CurrentJudgment = "MISS";
                        s_JudgmentAnimTimer = 0.3f;
                    }
                }
                else
                {
                    float timeDiff = s_SongTimer - pNote.endTimeSec;
                    if (timeDiff > 0.3f)
                    {
                        pNote.active = false;
                        pNote.isHolding = false;
                        s_Combo = 0;
                        s_LastCombo = 0;
                        s_ShowJudgment = true;
                        s_JudgmentTimer = 0.3f;
                        s_CurrentJudgment = "MISS";
                        s_JudgmentAnimTimer = 0.3f;
                    }
                }
            }
            else
            {
                float timeDiff = s_SongTimer - pNote.timeSec;
                if (timeDiff > 0.3f)
                {
                    pNote.active = false;
                    s_Combo = 0;
                    s_LastCombo = 0;
                    s_ShowJudgment = true;
                    s_JudgmentTimer = 0.3f;
                    s_CurrentJudgment = "MISS";
                    s_JudgmentAnimTimer = 0.3f;
                }
            }
        }
    }

    HitEffect::Update();

    bool lanePressed[4] = { IsKeyPressed(KEY_D), IsKeyPressed(KEY_F), IsKeyPressed(KEY_J), IsKeyPressed(KEY_K) };
    bool laneDown[4] = { IsKeyDown(KEY_D), IsKeyDown(KEY_F), IsKeyDown(KEY_J), IsKeyDown(KEY_K) };
    bool laneReleased[4] = { IsKeyReleased(KEY_D), IsKeyReleased(KEY_F), IsKeyReleased(KEY_J), IsKeyReleased(KEY_K) };

    for (int lane = 0; lane < 4; ++lane)
    {
        for (auto& pNote : s_PlayableNotes)
        {
            if (!pNote.active || pNote.lane != lane) continue;

            if (pNote.type == 128)
            {
                if (!pNote.isHolding && lanePressed[lane])
                {
                    float timeDiff = s_SongTimer - pNote.timeSec;
                    float absDiff = fabsf(timeDiff);

                    if (absDiff <= 0.15f)
                    {
                        pNote.isHolding = true;
                        pNote.lastTickTime = s_SongTimer;
                        float nX = LANE_X_COORDS[pNote.lane];
                        HitEffect::Spawn({ nX, judgmentLineY });

                        s_ShowJudgment = true;
                        s_JudgmentTimer = 0.4f;
                        s_JudgmentLinePulse = 1.0f;
                        s_JudgmentAnimTimer = 0.3f;

                        if (absDiff <= 0.05f) { s_CurrentJudgment = "PERFECT"; s_Combo++; }
                        else if (absDiff <= 0.10f) { s_CurrentJudgment = "GREAT"; s_Combo = 0; s_LastCombo = 0; }
                        else { s_CurrentJudgment = "GOOD"; s_Combo = 0; s_LastCombo = 0; }

                        if (std::string(s_CurrentJudgment) == "PERFECT" && s_Combo != s_LastCombo)
                        {
                            s_ComboAnimTimer = 0.2f;
                            s_LastCombo = s_Combo;
                        }
                        break;
                    }
                }
                else if (pNote.isHolding)
                {
                    if (laneReleased[lane] || !laneDown[lane])
                    {
                        pNote.active = false;
                        pNote.isHolding = false;
                        s_Combo = 0;
                        s_LastCombo = 0;
                        s_ShowJudgment = true;
                        s_JudgmentTimer = 0.3f;
                        s_CurrentJudgment = "MISS";
                        s_JudgmentAnimTimer = 0.3f;
                        break;
                    }

                    if (s_SongTimer >= pNote.endTimeSec)
                    {
                        pNote.active = false;
                        pNote.isHolding = false;
                        float nX = LANE_X_COORDS[pNote.lane];
                        HitEffect::Spawn({ nX, judgmentLineY });

                        s_ShowJudgment = true;
                        s_JudgmentTimer = 0.4f;
                        s_JudgmentLinePulse = 1.0f;
                        s_JudgmentAnimTimer = 0.3f;
                        s_CurrentJudgment = "PERFECT";
                        s_Combo++;
                        s_ComboAnimTimer = 0.2f;
                        s_LastCombo = s_Combo;
                        break;
                    }

                    if (s_SongTimer - pNote.lastTickTime >= 0.100f)
                    {
                        s_Combo++;
                        s_LastCombo = s_Combo;
                        s_ComboAnimTimer = 0.1f;
                        pNote.lastTickTime += 0.100f;
                    }
                }
            }
            else
            {
                if (lanePressed[lane])
                {
                    float timeDiff = s_SongTimer - pNote.timeSec;
                    float absDiff = fabsf(timeDiff);

                    if (absDiff <= 0.15f)
                    {
                        pNote.active = false;
                        float nX = LANE_X_COORDS[pNote.lane];
                        HitEffect::Spawn({ nX, judgmentLineY });

                        s_ShowJudgment = true;
                        s_JudgmentTimer = 0.4f;
                        s_JudgmentLinePulse = 1.0f;
                        s_JudgmentAnimTimer = 0.3f;

                        if (absDiff <= 0.05f) { s_CurrentJudgment = "PERFECT"; s_Combo++; }
                        else if (absDiff <= 0.10f) { s_CurrentJudgment = "GREAT"; s_Combo = 0; s_LastCombo = 0; }
                        else { s_CurrentJudgment = "GOOD"; s_Combo = 0; s_LastCombo = 0; }

                        if (std::string(s_CurrentJudgment) == "PERFECT" && s_Combo != s_LastCombo)
                        {
                            s_ComboAnimTimer = 0.2f;
                            s_LastCombo = s_Combo;
                        }
                        break;
                    }
                }
            }
        }
    }

    if (s_ShowJudgment)
    {
        s_JudgmentTimer -= dt;
        if (s_JudgmentTimer <= 0.0f)
        {
            s_ShowJudgment = false;
        }
    }
}

void PlayScene::Draw()
{
if (
m_State ==
PlaySceneState::SongSelect
)
{
m_SongSelect.Draw(
GetScreenWidth(),
GetScreenHeight()
);
}
else if (
m_State ==
PlaySceneState::Playing
)
{
DrawPlaying();
}
}

void PlayScene::DrawPlaying()
{
if (s_IsEditorMode)
{
s_ChartEditor.Render();

    return;
}

bool isDPressed =
    IsKeyDown(KEY_D);

bool isFPressed =
    IsKeyDown(KEY_F);

bool isJPressed =
    IsKeyDown(KEY_J);

bool isKPressed =
    IsKeyDown(KEY_K);

bool pressedStates[4] = {
    isDPressed,
    isFPressed,
    isJPressed,
    isKPressed
};

DrawBackground();

DrawPlayfield();

DrawLanes(
    pressedStates,
    judgmentLineY
);

DrawInputPanel(
    pressedStates,
    judgmentLineY
);

DrawInputFeedbackFlash(
    pressedStates,
    judgmentLineY
);

DrawNotes(
    judgmentLineY
);

DrawJudgmentLine(
    judgmentLineY
);

DrawJudgmentText();
DrawComboHUD();

HitEffect::Draw();

if (s_IsPaused)
{
    DrawPauseUI();
}

}

void PlayScene::Unload()
{
if (s_MusicPlayer.IsValid())
{
s_MusicPlayer.Stop();
}

if (s_ComboFont.texture.id != 0)
{
    UnloadFont(
        s_ComboFont
    );

    s_ComboFont =
        (Font){ 0 };
}

if (s_SuitFont.texture.id != 0)
{
    UnloadFont(
        s_SuitFont
    );

    s_SuitFont =
        (Font){ 0 };
}

s_ChartEditor.Release();
}