#include <cstdio>
#include <cstdlib>

#define PL_MPEG_IMPLEMENTATION
extern "C" {
    #include "pl_mpeg.h"
}

// TODO: 나중에 인클루드를 .cpp로 한거 고칠것
// TODO: 상속 구조나 백터로 나중에 고쳐야함

// ---------------------------------------

// 혹시라도 나중에 곡 추가하거나 이럴때 눈으로 쳐 보고 switch 복붙하셈 
// -- n달후 나에게 --

// --------------------------------------------------
// 헤더
// --------------------------------------------------
#include "play_scene.h"
#include "note.h"
#include "../Note_Exception_Hadling/NEH_Ghost_Note/Gh_click.h"
#include "../Note_Exception_Hadling/NEH_Fast_Note/Fn_click.h"
#include "../Note_Exception_Hadling/NEH_Slow_Note/Sn_click.h"
#include "../editing/chart_editor.h"
#include "../editing/chart_save.h"
#include "../vfx/hit_effect.h"
#include "../Animation/note_down_animation.h"
#include "../AudioManager/audio_manager.h"
#include "../UI/Loading/Load.h"
#include "../UI/G_I/play_main.h"
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
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
// ----------------------------------------------------------------

// 싱크

// -----------------------------------------------------------------
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

        /**
     * @brief 싱크 맞추는 부분 (제일 중요함)
     * @details DSP 클록으로 프레임 드랍 쳐 와도 딜레이 없음
     *          장치 상관 X
     *          탐라 버그 방지
     * @return ms라서 정확함
     */

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

#define BASE_PLAYFIELD_WIDTH 400.0f
#define PLAYFIELD_X ((float)GetScreenWidth() - BASE_PLAYFIELD_WIDTH) * 0.5f
#define PLAYFIELD_Y 0.0f
#define PLAYFIELD_WIDTH BASE_PLAYFIELD_WIDTH
#define PLAYFIELD_HEIGHT ((float)GetScreenHeight())

static const int LANE_COUNT = 4;
static const float LANE_WIDTH = 68.0f;
#define LANE_AREA_WIDTH ((float)LANE_COUNT * LANE_WIDTH)
#define LANE_START_X (PLAYFIELD_X + (PLAYFIELD_WIDTH - LANE_AREA_WIDTH) / 2.0f)

static inline float GetLaneXCoord(int lane) {
    return LANE_START_X + LANE_WIDTH * ((float)lane + 0.5f);
}

// TODO: 하드코딩 나중에 고칠 예정

// 나도 왜 이렇게 짰는지 기억이 안남

// 참고로 여기 부분 무슨일이 있더라도 절대 건들지마셈

struct MusicPlayerWrapper {
    MusicExecute::MusicPlayer1* p1 = nullptr; // 여기서 = nullptr 지우거나
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
        if (p1) p1->Initialize(am); if (p2) p2->Initialize(am); if (p3) p3->Initialize(am); // 여기서 nullptr 체크없이 부르면 = 터짐 = 좆됨
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

struct BgaVideoPlayer
{
    plm_t* plm = nullptr;
    Texture2D texture = { 0 };
    Image image = { 0 };
    bool loaded = false;
    uint8_t* buffer = nullptr;
    double lastTimeMs = 0.0;
    bool isFrameNew = false;

    static void OnVideoDecode(plm_t* plm, plm_frame_t* frame, void* user)
    {
        BgaVideoPlayer* player = static_cast<BgaVideoPlayer*>(user);
        if (player->buffer)
        {
            plm_frame_to_rgba(frame, player->buffer, plm_get_width(plm) * 4);
            player->isFrameNew = true;
        }
    }

    void Open(const std::string& filepath)
    {
        Close();
        if (filepath.empty()) return;

        plm = plm_create_with_filename(filepath.c_str());
        if (!plm) return;

        plm_set_audio_enabled(plm, 0);
        plm_set_loop(plm, 0);
        plm_set_video_decode_callback(plm, OnVideoDecode, this);

        int width = plm_get_width(plm);
        int height = plm_get_height(plm);

        buffer = static_cast<uint8_t*>(malloc(width * height * 4));

        image = GenImageColor(width, height, BLACK);
        ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
        
        texture = LoadTextureFromImage(image);
        SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
        loaded = true;
        lastTimeMs = 0.0;
        isFrameNew = false;
    }

    void Update(double targetTimeMs)
    {
        if (!loaded || !plm) return;

        double targetTimeSec = targetTimeMs / 1000.0;
        
        plm_seek(plm, targetTimeSec, 0);
        plm_decode(plm, 0.0); 
        
        lastTimeMs = targetTimeMs;

        if (buffer)
        {
            UpdateTextureRec(texture, Rectangle{ 0.0f, 0.0f, static_cast<float>(texture.width), static_cast<float>(texture.height) }, buffer);
            isFrameNew = false;
        }
    }

    void Draw(int screenWidth, int screenHeight)
    {
        if (!loaded || texture.id == 0) return;
        Rectangle srcRec = { 0.0f, 0.0f, static_cast<float>(texture.width), static_cast<float>(texture.height) };
        Rectangle destRec = { 0.0f, 0.0f, static_cast<float>(screenWidth), static_cast<float>(screenHeight) };
        DrawTexturePro(texture, srcRec, destRec, Vector2{ 0.0f, 0.0f }, 0.0f, WHITE);
    }

    void Close()
    {
        if (texture.id != 0)
        {
            UnloadTexture(texture);
            texture = { 0 };
        }
        if (image.data != nullptr)
        {
            UnloadImage(image);
            image = { 0 };
        }
        if (buffer)
        {
            free(buffer);
            buffer = nullptr;
        }
        if (plm)
        {
            plm_destroy(plm);
            plm = nullptr;
        }
        loaded = false;
    }
};

static AudioManager s_AudioManager;
static MusicPlayerWrapper s_MusicPlayer;
static FramedBeatmapClock s_BeatmapClock(true);
static BgaVideoPlayer s_BgaPlayer;

static std::vector<Note> s_Notes;

static PlayMainUI s_PlayMainUI;
static SongInformation s_SongInfo;

static int s_PerfectCount = 0;
static int s_GreatCount = 0;
static int s_GoodCount = 0;
static int s_MissCount = 0;
static int s_MaxCombo = 0;
static int s_Score = 0;
static float s_Accuracy = 100.0f;
static float s_HpRatio = 1.0f;

struct PlayableNote {
    float timeSec;
    int lane;
    int type;
    bool active;
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

static LoadingScreen s_LoadingScreen;
static bool s_IsLoading = false;
static bool s_LoadPrepared = false;
static LoadingSongData s_PendingSongData;
static Texture2D s_LoadedJacketTex = { 0 };

static float Clamp01(float value)
{
    if (value < 0.0f) return 0.0f;
    if (value > 1.0f) return 1.0f;
    return value;
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

    BeginBlendMode(BLEND_ALPHA); 

    if (s_LoadedJacketTex.id != 0)
    {
        Rectangle srcRec = { 0.0f, 0.0f, static_cast<float>(s_LoadedJacketTex.width), static_cast<float>(s_LoadedJacketTex.height) };
        Rectangle destRec = { 0.0f, 0.0f, static_cast<float>(screenW), static_cast<float>(screenH) };
        
        DrawTexturePro(s_LoadedJacketTex, srcRec, destRec, Vector2{ 0.0f, 0.0f }, 0.0f, WHITE);
        DrawRectangle(0, 0, screenW, screenH, Color{ 0, 0, 0, 170 }); 
    }
    else
    {
        DrawRectangleGradientV(0, 0, screenW, screenH, Color{ 17, 17, 19, 255 }, Color{ 1, 1, 2, 255 });
    }

    EndBlendMode();

    DrawRectangleGradientH(0, 0, screenW, screenH, Color{ 0, 0, 0, 115 }, Color{ 0, 0, 0, 115 });

    const float centerX = PLAYFIELD_X + PLAYFIELD_WIDTH * 0.5f;

    for (int i = 0; i < 90; ++i)
    {
        const float fx = fmodf(i * 197.31f + 37.0f, static_cast<float>(screenW));
        const float fy = fmodf(i * 91.73f + 11.0f, static_cast<float>(screenH));
        const float wave = 0.5f + 0.5f * sinf(time * 0.35f + i * 1.37f);
        const unsigned char alpha = static_cast<unsigned char>(8.0f + wave * 24.0f);
        const float radius = (i % 13 == 0) ? 1.4f : 0.65f;

        DrawCircle(static_cast<int>(fx), static_cast<int>(fy), radius, Color{ 230, 230, 235, alpha });
    }

    for (int y = 18; y < screenH; y += 36)
    {
        DrawRectangle(0, y, screenW, 1, Color{ 255, 255, 255, 4 });
    }

    for (int x = 0; x < screenW; x += 80)
    {
        DrawRectangle(x, 0, 1, screenH, Color{ 255, 255, 255, 3 });
    }

    for (int i = 0; i < 5; ++i)
    {
        const float radius = 150.0f + i * 55.0f;
        const float pulse = 0.5f + 0.5f * sinf(time * 0.28f + i);
        DrawCircleLines(static_cast<int>(centerX), screenH / 2, radius, Color{ 255, 255, 255, static_cast<unsigned char>(2 + pulse * 5) });
    }

    DrawRectangleGradientH(0, 0, static_cast<int>(PLAYFIELD_X), screenH, Color{ 0, 0, 0, 0 }, Color{ 0, 0, 0, 135 });
    DrawRectangleGradientH(static_cast<int>(PLAYFIELD_X + PLAYFIELD_WIDTH), 0, screenW - static_cast<int>(PLAYFIELD_X + PLAYFIELD_WIDTH), screenH, Color{ 0, 0, 0, 135 }, Color{ 0, 0, 0, 0 });
    
    int topGlowHeight = static_cast<int>(screenH * 0.2f);
    int bottomGlowHeight = static_cast<int>(screenH * 0.2f);
    
    DrawRectangleGradientV(0, 0, screenW, topGlowHeight, Color{ 0, 0, 0, 105 }, Color{ 0, 0, 0, 0 });
    DrawRectangleGradientV(0, screenH - bottomGlowHeight, screenW, bottomGlowHeight, Color{ 0, 0, 0, 0 }, Color{ 0, 0, 0, 160 });
}

static void DrawMechanicalFrame()
{
    const int left = static_cast<int>(PLAYFIELD_X);
    const int right = static_cast<int>(PLAYFIELD_X + PLAYFIELD_WIDTH);
    const float time = GetTime();
    const float pulse = 0.5f + 0.5f * sinf(time * 2.2f);

    int screenH = GetScreenHeight();
    DrawRectangle(left - 30, 0, 30, screenH, Color{ 5, 5, 5, 250 });
    DrawRectangle(right, 0, 30, screenH, Color{ 5, 5, 5, 250 });

    DrawRectangle(left - 8, 0, 4, screenH, Color{ 73, 73, 73, 180 });
    DrawRectangle(right + 4, 0, 4, screenH, Color{ 73, 73, 73, 180 });

    DrawRectangle(left - 3, 0, 3, screenH, Color{ 235, 235, 235, 210 });
    DrawRectangle(right, 0, 3, screenH, Color{ 235, 235, 235, 210 });

    DrawRectangle(left - 24, 0, 2, screenH, Color{ 43, 43, 43, 230 });
    DrawRectangle(right + 22, 0, 2, screenH, Color{ 43, 43, 43, 230 });


    for (int i = 0; i < 10; ++i)
    {
        float y = 30.0f + i * 70.0f;
        unsigned char a = static_cast<unsigned char>(75.0f + pulse * 35.0f);

        DrawRectangle(left - 24, static_cast<int>(y), 14, 2, Color{ 171, 171, 171, a });
        DrawRectangle(left - 17, static_cast<int>(y) + 5, 7, 1, Color{ 255, 255, 255, 70 });
        DrawRectangle(right + 10, static_cast<int>(y), 14, 2, Color{ 171, 171, 171, a });
        DrawRectangle(right + 10, static_cast<int>(y) + 5, 7, 1, Color{ 255, 255, 255, 70 });
    }

    DrawRectangle(left - 4, 110, 4, 260, Color{ 211, 211, 211, static_cast<unsigned char>(35.0f + pulse * 45.0f) });
    DrawRectangle(right, 110, 4, 260, Color{ 211, 211, 211, static_cast<unsigned char>(35.0f + pulse * 45.0f) });

    DrawTriangle({ static_cast<float>(left) - 8.0f, 390.0f }, { static_cast<float>(left) - 72.0f, 510.0f }, { static_cast<float>(left) - 8.0f, 580.0f }, Color{ 12, 12, 12, 255 });
    DrawTriangleLines({ static_cast<float>(left) - 8.0f, 390.0f }, { static_cast<float>(left) - 72.0f, 510.0f }, { static_cast<float>(left) - 8.0f, 580.0f }, Color{ 136, 136, 136, 170 });

    DrawTriangle({ static_cast<float>(right) + 8.0f, 390.0f }, { static_cast<float>(right) + 72.0f, 510.0f }, { static_cast<float>(right) + 8.0f, 580.0f }, Color{ 12, 12, 12, 255 });
    DrawTriangleLines({ static_cast<float>(right) + 8.0f, 390.0f }, { static_cast<float>(right) + 72.0f, 510.0f }, { static_cast<float>(right) + 8.0f, 580.0f }, Color{ 136, 136, 136, 170 });

    for (int i = 0; i < 6; ++i)
    {
        float y = 410.0f + i * 42.0f;
        DrawRectangle(left - 58, static_cast<int>(y), 32, 1, Color{ 151, 151, 151, 42 });
        DrawRectangle(right + 26, static_cast<int>(y), 32, 1, Color{ 151, 151, 151, 42 });
    }

    DrawRectangle(left - 18, 704, 14, 2, Color{ 226, 226, 226, 130 });
    DrawRectangle(right + 4, 704, 14, 2, Color{ 226, 226, 226, 130 });
}

static void DrawPlayfield()
{
    const float t = GetTime();
    const int px = static_cast<int>(PLAYFIELD_X);
    const int pw = static_cast<int>(PLAYFIELD_WIDTH);

        int screenH = GetScreenHeight();
    DrawRectangle(px - 24, 0, pw + 48, screenH, Color{ 0, 0, 0, 50 });
    DrawRectangle(px, 0, pw, screenH, Color{ 3, 3, 4, 60 });

    DrawRectangleGradientV(px, 0, pw, screenH, Color{ 24, 24, 26, 50 }, Color{ 1, 1, 2, 70 });
    DrawRectangleGradientH(px, 0, 70, screenH, Color{ 0, 0, 0, 80 }, Color{ 0, 0, 0, 0 });
    DrawRectangleGradientH(px + pw - 70, 0, 70, screenH, Color{ 0, 0, 0, 0 }, Color{ 0, 0, 0, 80 });

    for (int y = 8; y < screenH; y += 8)
    {
        unsigned char a = static_cast<unsigned char>(3 + ((y / 8) % 3));
        DrawRectangle(px + 2, y, pw - 4, 1, Color{ 255, 255, 255, a });
    }

    const float scan = fmodf(t * 135.0f, (float)screenH);
    DrawRectangle(px + 2, static_cast<int>(scan), pw - 4, 2, Color{ 255, 255, 255, 15 });
    DrawRectangle(px + 2, static_cast<int>(scan) + 2, pw - 4, 1, Color{ 255, 255, 255, 6 });

    DrawRectangle(px, 0, 3, screenH, Color{ 255, 255, 255, 115 });
    DrawRectangle(px + pw - 3, 0, 3, screenH, Color{ 255, 255, 255, 115 });
    DrawRectangle(px + 8, 0, 1, screenH, Color{ 255, 255, 255, 30 });
    DrawRectangle(px + pw - 9, 0, 1, screenH, Color{ 255, 255, 255, 30 });


       for (int i = 0; i < 4; ++i)
    {
        float cx = GetLaneXCoord(i);
        float pulse = 0.5f + 0.5f * sinf(t * 1.8f + i * 0.9f);
        DrawCircle(static_cast<int>(cx), 120, 48.0f + pulse * 7.0f, Color{ 255, 255, 255, 2 });
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

        DrawRectangle(static_cast<int>(laneX) + 1, 0, static_cast<int>(LANE_WIDTH) - 2, static_cast<int>(judgmentLineY), Color{ 255, 255, 255, static_cast<unsigned char>(2 + i) });

        if (i > 0)
        {
            DrawRectangle(static_cast<int>(laneX) - 4, 0, 8, static_cast<int>(judgmentLineY), Color{ 0, 0, 0, 235 });
            DrawRectangle(static_cast<int>(laneX) - 1, 0, 2, static_cast<int>(judgmentLineY), Color{ 165, 165, 165, 110 });
            DrawRectangle(static_cast<int>(laneX), 0, 1, static_cast<int>(judgmentLineY), Color{ 255, 255, 255, 32 });
        }

        for (int y = 34; y < static_cast<int>(judgmentLineY) - 20; y += 48)
        {
            float pulse = 0.5f + 0.5f * sinf(t * 1.3f + y * 0.02f + i);
            unsigned char a = static_cast<unsigned char>(8 + pulse * 12);
            DrawRectangle(static_cast<int>(laneCenter) - 11, y, 22, 1, Color{ 255, 255, 255, a });
            DrawRectangle(static_cast<int>(laneCenter) - 2, y - 3, 4, 7, Color{ 255, 255, 255, static_cast<unsigned char>(a / 2) });
        }

        DrawRectangle(static_cast<int>(laneCenter) - 1, 24, 2, static_cast<int>(judgmentLineY) - 48, Color{ 255, 255, 255, 8 });

        DrawLine(static_cast<int>(laneX) + 8, 16, static_cast<int>(laneX) + 22, 16, Color{ 255, 255, 255, 48 });
        DrawLine(static_cast<int>(laneX) + 8, 16, static_cast<int>(laneX) + 8, 28, Color{ 255, 255, 255, 48 });
        DrawLine(static_cast<int>(laneX + LANE_WIDTH - 22), 16, static_cast<int>(laneX + LANE_WIDTH - 8), 16, Color{ 255, 255, 255, 48 });
        DrawLine(static_cast<int>(laneX + LANE_WIDTH - 8), 16, static_cast<int>(laneX + LANE_WIDTH - 8), 28, Color{ 255, 255, 255, 48 });
    }

    DrawRectangle(static_cast<int>(LANE_START_X), static_cast<int>(judgmentLineY) - 58, static_cast<int>(LANE_AREA_WIDTH), 1, Color{ 255, 255, 255, 25 });
    DrawRectangle(static_cast<int>(LANE_START_X), static_cast<int>(judgmentLineY) - 34, static_cast<int>(LANE_AREA_WIDTH), 1, Color{ 255, 255, 255, 18 });
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
            DrawRectangleGradientV(static_cast<int>(x) + 3, 40, static_cast<int>(LANE_WIDTH) - 6, static_cast<int>(judgmentLineY) - 42, Color{ 255, 255, 255, 0 }, Color{ 255, 255, 255, static_cast<unsigned char>(22 + pulse * 35) });
            DrawRectangle(static_cast<int>(x) + 4, static_cast<int>(judgmentLineY) - 36, static_cast<int>(LANE_WIDTH) - 8, 5, Color{ 255, 255, 255, static_cast<unsigned char>(100 + pulse * 100) });
            DrawRectangle(static_cast<int>(x) + 8, static_cast<int>(judgmentLineY) - 29, static_cast<int>(LANE_WIDTH) - 16, 2, Color{ 255, 255, 255, 180 });
            DrawCircle(static_cast<int>(cx), static_cast<int>(judgmentLineY), 21.0f + pulse * 7.0f, Color{ 255, 255, 255, static_cast<unsigned char>(8 + pulse * 18) });
        }
        else
        {
            DrawRectangle(static_cast<int>(x) + 10, static_cast<int>(judgmentLineY) - 25, static_cast<int>(LANE_WIDTH) - 20, 1, Color{ 255, 255, 255, 28 });
        }
    }
}

static void DrawNotes(float judgmentLineY)
{
    const float nowMs = s_SongTimer * 1000.0f;
    const float time = GetTime();

    //int monitorRefreshRate = GetMonitorRefreshRate(GetCurrentMonitor());
    //if (monitorRefreshRate <= 0) monitorRefreshRate = 60;

    //float frameTimeSec = 1.0f / static_cast<float>(monitorRefreshRate);
    //float displayLatencySec = frameTimeSec * 2.0f;

    for (const auto& pNote : s_PlayableNotes)
    {
        // if (!pNote.active || pNote.lane < 0 || pNote.lane >= LANE_COUNT)
          //  continue;

          if (!pNote.active)
          continue;

           if (pNote.lane < 0 || pNote.lane >= LANE_COUNT)
            continue;

        float uiScale = (float)GetScreenHeight() / 720.0f; 
        const float diffSec = (pNote.timeSec * 1000.0f - nowMs) / 1000.0f;
        //float displayLatencySec = 0.035f; 
        //const float diffSec = (pNote.timeSec * 1000.0f - nowMs) / 1000.0f;
        //const float diffSec = ((pNote.timeSec * 1000.0f - nowMs) / 1000.0f) - displayLatencySec;
        //const float y = judgmentLineY - diffSec * (s_NoteScrollSpeed * uiScale);
         const float y = judgmentLineY - (diffSec * s_NoteScrollSpeed);

        int screenH = GetScreenHeight();

        if (y < -100.0f || y > (float)screenH + 100.0f)
            continue;

       uiScale = (float)GetScreenHeight() / 720.0f;
        const float cx = GetLaneXCoord(pNote.lane);
        const float w = LANE_WIDTH - 6.0f;
        
        const float h = 36.0f * uiScale; 
        const bool nearHit = fabsf(diffSec) < 0.22f;
        const float pulse = 0.5f + 0.5f * sinf(time * 9.0f + pNote.lane);

        if (nearHit)
        {
            DrawRectangleRounded({ cx - w * 0.5f - 10.0f * uiScale, y - h * 0.5f - 10.0f * uiScale, w + 20.0f * uiScale, h + 20.0f * uiScale }, 0.18f, 8, Color{ 255, 255, 255, static_cast<unsigned char>(8 + pulse * 12) });
        }

        DrawRectangleRounded({ cx - w * 0.5f + 4.0f * uiScale, y - h * 0.5f + 7.0f * uiScale, w, h }, 0.18f, 8, Color{ 0, 0, 0, 245 });
        DrawRectangleRounded({ cx - w * 0.5f, y - h * 0.5f, w, h }, 0.18f, 8, Color{ 218, 218, 221, 255 });
        DrawRectangleRounded({ cx - w * 0.5f + 2.0f * uiScale, y - h * 0.5f + 2.0f * uiScale, w - 4.0f * uiScale, h * 0.42f }, 0.16f, 8, Color{ 255, 255, 255, 255 });
        DrawRectangleRounded({ cx - w * 0.5f + 5.0f * uiScale, y - 2.0f * uiScale, w - 10.0f * uiScale, 4.0f * uiScale }, 0.35f, 8, Color{ 22, 22, 24, 255 });
        DrawRectangleRoundedLines({ cx - w * 0.5f, y - h * 0.5f, w, h }, 0.18f, 8, Color{ 255, 255, 255, 240 });

    }
}

static void DrawJudgmentLine(float judgmentLineY)
{
    const float t = GetTime();
    const float impact = Clamp01(s_JudgmentLinePulse);
    const float pulse = 0.5f + 0.5f * sinf(t * 3.2f);
    const int y = static_cast<int>(judgmentLineY);
    const float left = LANE_START_X - 22.0f;
    const float right = LANE_START_X + LANE_AREA_WIDTH + 22.0f;

    DrawRectangle(static_cast<int>(left) - 14, y - 18, static_cast<int>(right - left) + 28, 36, Color{ 255, 255, 255, static_cast<unsigned char>(7 + impact * 20) });
    DrawRectangle(static_cast<int>(left) - 6, y - 10, static_cast<int>(right - left) + 12, 20, Color{ 255, 255, 255, static_cast<unsigned char>(12 + impact * 30) });

    DrawRectangle(static_cast<int>(left), y - 6, static_cast<int>(right - left), 12, Color{ 4, 4, 5, 245 });
    DrawRectangle(static_cast<int>(left), y - 4, static_cast<int>(right - left), 8, Color{ 110, 110, 112, 230 });
    DrawRectangle(static_cast<int>(left), y - 2, static_cast<int>(right - left), 4, Color{ 238, 238, 240, 255 });
    DrawRectangle(static_cast<int>(left), y - 1, static_cast<int>(right - left), 2, Color{ 255, 255, 255, 255 });

    for (int i = 0; i < LANE_COUNT; ++i)
    {
        const float x = GetLaneXCoord(i);
        const bool hitFlash = impact > 0.01f;
        const float r = 13.0f + impact * 12.0f;

        DrawCircle(static_cast<int>(x), y, r + 9.0f, Color{ 255, 255, 255, static_cast<unsigned char>(4 + impact * 18) });
        DrawCircle(static_cast<int>(x), y, r, Color{ 3, 3, 4, 245 });
        DrawCircleLines(static_cast<int>(x), y, r + 4.0f, Color{ 245, 245, 245, static_cast<unsigned char>(115 + impact * 120) });
        DrawCircleLines(static_cast<int>(x), y, r + 8.0f + pulse * 2.0f, Color{ 255, 255, 255, static_cast<unsigned char>(20 + pulse * 25) });

        DrawLine(static_cast<int>(x) - 26, y, static_cast<int>(x) - 17, y, Color{ 255, 255, 255, 175 });
        DrawLine(static_cast<int>(x) + 17, y, static_cast<int>(x) + 26, y, Color{ 255, 255, 255, 175 });

        if (hitFlash)
        {
            DrawCircle(static_cast<int>(x), y, r * 0.45f, Color{ 255, 255, 255, static_cast<unsigned char>(20 + impact * 80) });
        }
    }

    DrawRectangle(static_cast<int>(left) - 34, y - 1, 24, 2, Color{ 255, 255, 255, static_cast<unsigned char>(80 + pulse * 100) });
    DrawRectangle(static_cast<int>(right) + 10, y - 1, 24, 2, Color{ 255, 255, 255, static_cast<unsigned char>(80 + pulse * 100) });

    const float cx = PLAYFIELD_X + PLAYFIELD_WIDTH * 0.5f;
    const float archY = judgmentLineY + 92.0f;

    DrawRing({ cx, archY }, 86.0f, 100.0f, 205.0f, 335.0f, 64, Color{ 0, 0, 0, 250 });
    DrawRing({ cx, archY }, 100.0f, 104.0f, 205.0f, 335.0f, 64, Color{ 90, 90, 92, 210 });
    DrawRing({ cx, archY }, 108.0f, 112.0f, 208.0f, 332.0f, 64, Color{ 235, 235, 238, 145 });

    DrawRectangle(static_cast<int>(cx - 132), static_cast<int>(judgmentLineY + 54), 264, 3, Color{ 5, 5, 6, 245 });
    DrawRectangle(static_cast<int>(cx - 116), static_cast<int>(judgmentLineY + 57), 232, 2, Color{ 115, 115, 118, 130 });

    for (int i = 0; i < 9; ++i)
    {
        const float x = cx - 104.0f + i * 26.0f;
        const float h = 8.0f + 5.0f * (0.5f + 0.5f * sinf(t * 2.0f + i));
        DrawRectangle(static_cast<int>(x), static_cast<int>(judgmentLineY + 42), 10, static_cast<int>(h), Color{ 190, 190, 193, static_cast<unsigned char>(45 + i * 5) });
    }
}

static void DrawJudgmentText()
{
    if (!s_ShowJudgment || s_SuitFont.texture.id == 0) return;

    float timerProgress = Clamp01(s_JudgmentAnimTimer / 0.3f);
    float appear = 1.0f - timerProgress;
    float ease = EaseOutBack(appear);
    float alpha = Clamp01(s_JudgmentTimer / 0.4f);
    float scale = 0.84f + ease * 0.16f;
    float fontSize = 31.0f * scale;

    std::string rawJudgment = std::string(s_CurrentJudgment);
    Vector2 textSize = MeasureTextEx(s_SuitFont, rawJudgment.c_str(), fontSize, 2.0f);

    float centerX = PLAYFIELD_X + PLAYFIELD_WIDTH * 0.5f;
    float textX = centerX - textSize.x * 0.5f;
    float textY = 282.0f;

    Color mainColor = Color{ 247, 247, 247, 255 };
    Color accentColor = Color{ 189, 189, 189, 190 };

    if (rawJudgment == "GREAT")
    {
        mainColor = Color{ 230, 230, 230, 255 };
        accentColor = Color{ 154, 154, 154, 170 };
    }
    else if (rawJudgment == "GOOD")
    {
        mainColor = Color{ 197, 197, 197, 255 };
        accentColor = Color{ 127, 127, 127, 150 };
    }
    else if (rawJudgment == "MISS")
    {
        mainColor = Color{ 136, 136, 136, 255 };
        accentColor = Color{ 88, 88, 88, 140 };
    }

    mainColor.a = static_cast<unsigned char>(255.0f * alpha);
    accentColor.a = static_cast<unsigned char>(accentColor.a * alpha);

    float lineWidth = 52.0f + ease * 55.0f;

    DrawRectangle(static_cast<int>(centerX - lineWidth), static_cast<int>(textY - 9.0f), static_cast<int>(lineWidth * 2.0f), 1, Color{ accentColor.r, accentColor.g, accentColor.b, static_cast<unsigned char>(accentColor.a * 0.55f) });
    DrawRectangle(static_cast<int>(centerX - lineWidth * 0.55f), static_cast<int>(textY + textSize.y + 7.0f), static_cast<int>(lineWidth * 1.1f), 1, accentColor);

    DrawTextEx(s_SuitFont, rawJudgment.c_str(), { textX + 3.0f, textY + 3.0f }, fontSize, 2.0f, Color{ 0, 0, 0, static_cast<unsigned char>(180.0f * alpha) });
    DrawTextEx(s_SuitFont, rawJudgment.c_str(), { textX, textY }, fontSize, 2.0f, mainColor);

    float smallY = textY + textSize.y + 17.0f;
    const char* status = (rawJudgment == "MISS") ? "OFF BEAT" : "TIMING LOCK";
    Vector2 statusSize = MeasureTextEx(s_SuitFont, status, 9.0f, 1.0f);

    DrawTextEx(s_SuitFont, status, { centerX - statusSize.x * 0.5f, smallY }, 9.0f, 1.0f, Color{ 170, 170, 170, static_cast<unsigned char>(150.0f * alpha) });
}

static void DrawComboHUD()
{
    if (s_Combo <= 0 || s_ComboFont.texture.id == 0 || s_SuitFont.texture.id == 0) return;

    float pulse = (s_ComboAnimTimer > 0.0f) ? Clamp01(s_ComboAnimTimer / 0.2f) : 0.0f;
    float comboScale = 1.0f + EaseOutBack(1.0f - pulse) * 0.08f;
    float comboSize = 73.0f * comboScale;

    std::string comboText = std::to_string(s_Combo);
    Vector2 comboTextSize = MeasureTextEx(s_ComboFont, comboText.c_str(), comboSize, 1.0f);

    float centerX = PLAYFIELD_X + PLAYFIELD_WIDTH * 0.5f;
    float comboX = centerX - comboTextSize.x * 0.5f;
    float comboY = 105.0f - (1.0f - pulse) * 4.0f;
    float glow = 0.5f + 0.5f * sinf(GetTime() * 4.0f);

    DrawRectangle(static_cast<int>(centerX - 108.0f), static_cast<int>(comboY) - 12, 216, 1, Color{ 136, 136, 136, static_cast<unsigned char>(55.0f + glow * 25.0f) });
    DrawRectangle(static_cast<int>(centerX - 70.0f), static_cast<int>(comboY) - 7, 140, 1, Color{ 238, 238, 238, 65 });
    DrawRectangle(static_cast<int>(centerX - 108.0f), static_cast<int>(comboY) + 11, 34, 1, Color{ 193, 193, 193, 120 });
    DrawRectangle(static_cast<int>(centerX + 74.0f), static_cast<int>(comboY) + 11, 34, 1, Color{ 193, 193, 193, 120 });

    DrawTextEx(s_SuitFont, "COMBO", { centerX - 28.0f, comboY - 33.0f }, 16.0f, 2.0f, Color{ 170, 170, 170, 230 });
    DrawTextEx(s_ComboFont, comboText.c_str(), { comboX + 5.0f, comboY + 5.0f }, comboSize, 1.0f, Color{ 108, 108, 108, 70 });
    DrawTextEx(s_ComboFont, comboText.c_str(), { comboX + 2.0f, comboY + 2.0f }, comboSize, 1.0f, Color{ 0, 0, 0, 240 });
    DrawTextEx(s_ComboFont, comboText.c_str(), { comboX, comboY }, comboSize, 1.0f, Color{ 250, 250, 250, 255 });

    float lineY = comboY + comboTextSize.y + 7.0f;

    DrawRectangle(static_cast<int>(centerX - 94.0f), static_cast<int>(lineY), 45, 2, Color{ 171, 171, 171, 120 });
    DrawRectangle(static_cast<int>(centerX - 42.0f), static_cast<int>(lineY), 84, 1, Color{ 246, 246, 246, 100 });
    DrawRectangle(static_cast<int>(centerX + 49.0f), static_cast<int>(lineY), 45, 2, Color{ 171, 171, 171, 120 });
    DrawCircle(static_cast<int>(centerX - 103.0f), static_cast<int>(lineY) + 1, 2.0f, Color{ 211, 211, 211, 150 });
    DrawCircle(static_cast<int>(centerX + 103.0f), static_cast<int>(lineY) + 1, 2.0f, Color{ 211, 211, 211, 150 });

    if (s_ComboAnimTimer > 0.0f)
    {
        float t = 1.0f - Clamp01(s_ComboAnimTimer / 0.2f);
        float burstRadius = 12.0f + t * 48.0f;
        unsigned char burstAlpha = static_cast<unsigned char>((1.0f - t) * 95.0f);

        DrawCircleLines(static_cast<int>(centerX), static_cast<int>(comboY + comboTextSize.y * 0.52f), burstRadius, Color{ 203, 203, 203, burstAlpha });
        DrawCircleLines(static_cast<int>(centerX), static_cast<int>(comboY + comboTextSize.y * 0.52f), burstRadius + 4.0f, Color{ 165, 165, 165, static_cast<unsigned char>(burstAlpha * 0.35f) });
    }
}

static void DrawInputFeedbackFlash(const bool pressedStates[4], float judgmentLineY)
{
    const float time = GetTime();

    for (int i = 0; i < LANE_COUNT; ++i)
    {
        if (!pressedStates[i]) continue;

        float laneX = LANE_START_X + i * LANE_WIDTH;
        float pulse = 0.5f + 0.5f * sinf(time * 13.0f + i * 0.7f);
        unsigned char alpha = static_cast<unsigned char>(45.0f + pulse * 65.0f);

        float uiScale = (float)GetScreenHeight() / 720.0f;
        DrawRectangleGradientV(static_cast<int>(laneX) + 5, 30, static_cast<int>(LANE_WIDTH) - 10, static_cast<int>(judgmentLineY) - 42, Color{ 189, 189, 189, 0 }, Color{ 189, 189, 189, alpha });
        DrawRectangle(static_cast<int>(laneX) + 7, static_cast<int>(judgmentLineY) - 19 * uiScale, static_cast<int>(LANE_WIDTH) - 14, 3, Color{ 234, 234, 234, alpha });
        DrawRectangle(static_cast<int>(laneX) + 13, static_cast<int>(judgmentLineY) + 8 * uiScale, static_cast<int>(LANE_WIDTH) - 26, 2, Color{ 198, 198, 198, static_cast<unsigned char>(alpha * 0.7f) });
        DrawCircleLines(static_cast<int>(laneX + LANE_WIDTH * 0.5f), static_cast<int>(judgmentLineY), (14.0f + pulse * 6.0f) * uiScale, Color{ 208, 208, 208, static_cast<unsigned char>(alpha * 0.65f) });

    }
}

static void DrawInputPanel(const bool pressedStates[4], float judgmentLineY)
{
    int screenH = GetScreenHeight();
    float uiScale = (float)screenH / 720.0f;

    const float panelH = 116.0f * uiScale;
    const float panelY = (float)screenH - panelH;
    const float buttonY = panelY + 28.0f * uiScale;
    const float buttonH = 78.0f * uiScale;
    const float time = GetTime();

    DrawRectangle(static_cast<int>(PLAYFIELD_X), static_cast<int>(judgmentLineY), static_cast<int>(PLAYFIELD_WIDTH), (int)(screenH - judgmentLineY), Color{ 2, 2, 3, 255 });

    for (int i = 0; i < LANE_COUNT; ++i)
    {
        const float x = LANE_START_X + i * LANE_WIDTH;
        const float cx = GetLaneXCoord(i);
        const bool pressed = pressedStates[i];
        const float pulse = 0.5f + 0.5f * sinf(time * 9.0f + i * 0.8f);

        if (pressed)
        {
            DrawRectangleGradientV(static_cast<int>(x), static_cast<int>(judgmentLineY), static_cast<int>(LANE_WIDTH), (int)panelH, Color{ 255, 255, 255, 0 }, Color{ 255, 255, 255, 28 });
            DrawRectangle(static_cast<int>(x) + 4, static_cast<int>(judgmentLineY) + 1, static_cast<int>(LANE_WIDTH) - 8, 3, Color{ 255, 255, 255, static_cast<unsigned char>(110 + pulse * 100) });
        }

        DrawRectangle(static_cast<int>(x) + 2, static_cast<int>(buttonY) + (int)(8.0f * uiScale), static_cast<int>(LANE_WIDTH) - 4, static_cast<int>(buttonH), Color{ 0, 0, 0, 245 });
        DrawRectangleRounded({ x + 1.0f, buttonY, LANE_WIDTH - 2.0f, buttonH }, 0.08f, 8, pressed ? Color{ 245, 245, 247, 255 } : Color{ 82, 82, 86, 255 });
        DrawRectangleRounded({ x + 5.0f, buttonY + 4.0f * uiScale, LANE_WIDTH - 10.0f, buttonH - 8.0f * uiScale }, 0.06f, 8, pressed ? Color{ 128, 128, 132, 255 } : Color{ 13, 13, 15, 255 });
        DrawRectangleGradientV(static_cast<int>(x) + 7, static_cast<int>(buttonY) + (int)(6.0f * uiScale), static_cast<int>(LANE_WIDTH) - 14, static_cast<int>(buttonH) - (int)(12.0f * uiScale), pressed ? Color{ 205, 205, 208, 255 } : Color{ 49, 49, 53, 255 }, pressed ? Color{ 62, 62, 65, 255 } : Color{ 5, 5, 7, 255 });
        DrawRectangle(static_cast<int>(x) + 9, static_cast<int>(buttonY) + (int)(9.0f * uiScale), static_cast<int>(LANE_WIDTH) - 18, 3, pressed ? Color{ 255, 255, 255, static_cast<unsigned char>(205 + pulse * 50) } : Color{ 185, 185, 190, 105 });
        DrawRectangle(static_cast<int>(x) + 9, static_cast<int>(buttonY) + static_cast<int>(buttonH) - (int)(12.0f * uiScale), static_cast<int>(LANE_WIDTH) - 18, 4, Color{ 0, 0, 0, 190 });
        DrawRectangle(static_cast<int>(x) + 10, static_cast<int>(buttonY) + (int)(14.0f * uiScale), 3, (int)(27.0f * uiScale), Color{ 255, 255, 255, static_cast<unsigned char>(pressed ? 100 : 28) });
        DrawRectangle(static_cast<int>(x) + static_cast<int>(LANE_WIDTH) - 13, static_cast<int>(buttonY) + (int)(14.0f * uiScale), 3, (int)(27.0f * uiScale), Color{ 0, 0, 0, 135 });

        if (pressed)
        {
            DrawRectangle(static_cast<int>(x) + 4, static_cast<int>(buttonY) + 3, static_cast<int>(LANE_WIDTH) - 8, 2, Color{ 255, 255, 255, 245 });
            DrawRectangle(static_cast<int>(x) + 4, static_cast<int>(buttonY) + static_cast<int>(buttonH) - 2, static_cast<int>(LANE_WIDTH) - 8, 2, Color{ 255, 255, 255, 125 });
            DrawCircleLines(static_cast<int>(cx), static_cast<int>(buttonY + buttonH * 0.5f), (25.0f + pulse * 4.0f) * uiScale, Color{ 255, 255, 255, static_cast<unsigned char>(35 + pulse * 45) });
        }

        if (s_SuitFont.texture.id != 0)
        {
            const char* key = (i == 0) ? "D" : (i == 1) ? "F" : (i == 2) ? "J" : "K";
            const float fontSize = 31.0f * uiScale;
            Vector2 size = MeasureTextEx(s_SuitFont, key, fontSize, 1.0f);

            DrawTextEx(s_SuitFont, key, { cx - size.x * 0.5f, buttonY + buttonH * 0.5f - size.y * 0.5f }, fontSize, 1.0f, pressed ? Color{ 255, 255, 255, 255 } : Color{ 228, 228, 232, 255 });
        }
    }

    DrawRectangle(static_cast<int>(LANE_START_X), (int)(screenH - 91.0f * uiScale), static_cast<int>(LANE_AREA_WIDTH), 1, Color{ 255, 255, 255, 55 });
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

    const float screenW = static_cast<float>(GetScreenWidth());
    const float screenH = static_cast<float>(GetScreenHeight());

    DrawRectangle(0, 0, static_cast<int>(screenW), static_cast<int>(screenH), Color{ 0, 0, 0, 180 });

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
            DrawRectangle(static_cast<int>(btnX) + 8, static_cast<int>(btnY) + 11, 4, static_cast<int>(btnH) - 22, Color{ 120, 210, 255, 255 });
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
      judgmentLineY((float)GetScreenHeight() - 125.0f)
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
    s_BgaPlayer.Close();
}

void PlayScene::Init(int startSongIndex)
{
    m_State = PlaySceneState::SongSelect;
    m_BackToMenu = false;
    judgmentLineY = (float)GetScreenHeight() - 125.0f;

    s_BgaPlayer.Close();
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

    s_PerfectCount = 0;
    s_GreatCount = 0;
    s_GoodCount = 0;
    s_MissCount = 0;
    s_MaxCombo = 0;
    s_Score = 0;
    s_Accuracy = 100.0f;
    s_HpRatio = 1.0f;

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

    s_LoadingScreen.Init();
    s_IsLoading = false;
    s_LoadPrepared = false;
    if (s_LoadedJacketTex.id != 0)
    {
        UnloadTexture(s_LoadedJacketTex);
        s_LoadedJacketTex = { 0 };
    }

    s_PlayMainUI.Init();

    int codepoints[256];
    for (int i = 0; i < 256; i++) codepoints[i] = i;
    s_ComboFont = LoadFontEx("fonts/Pretendard-Black.ttf", 96, codepoints, 256);
    s_SuitFont = LoadFontEx("fonts/Pretendard-Black.ttf", 96, codepoints, 256);
    
    if (s_ComboFont.texture.id != 0) SetTextureFilter(s_ComboFont.texture, TEXTURE_FILTER_BILINEAR);
    if (s_SuitFont.texture.id != 0) SetTextureFilter(s_SuitFont.texture, TEXTURE_FILTER_BILINEAR);
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
        return;
    }

    s_AudioManager.Update();

    if (s_MusicPlayer.IsValid())
    {
        s_MusicPlayer.Update(GetFrameTime());
    }

    if (s_IsLoading)
    {
        float dt = GetFrameTime();
        s_LoadingScreen.Update(dt);

        if (s_LoadingScreen.IsCurtainClosed() && !s_LoadPrepared)
        {
            int currentSelectedIdx = m_SongSelect.GetSelectedSongIndex(); 
            
            const SongData& curSong = m_SongSelect.GetCurrentSong();
            std::string osuFileName = curSong.osuFileName; 
            s_MusicPlayer.active = curSong.musicPlayerActive;

            s_BgaPlayer.Open(curSong.videoFileName);

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
                    pNote.timeSec = static_cast<float>(saveNote.time) / 1000.0f;
                    pNote.lane = saveNote.lane;

                    if ((saveNote.type & 128) != 0 || saveNote.type == 128) {
                        pNote.type = 1;
                    } else {
                        pNote.type = saveNote.type;
                    }
                    
                    pNote.active = true;
                    s_PlayableNotes.push_back(pNote);
                }

                std::sort(s_PlayableNotes.begin(), s_PlayableNotes.end(), [](const PlayableNote& a, const PlayableNote& b) {
                    return a.timeSec < b.timeSec;
                });
            }
            else
            {
                s_BgaPlayer.Close();
                s_IsLoading = false;
                m_State = PlaySceneState::SongSelect;
                return;
            }

            static const std::vector<std::string> jacketPaths = {
                "music_assets/stars.png",
                "music_assets/kaleidoscope.png",
                "music_assets/SkysCape.png",
                "music_assets/TheLostAria.png",
                "music_assets/Timeline.png",
                "music_assets/Terrasphere.png",
                "music_assets/N.png",
                "music_assets/SecretDoll2.png",
                "music_assets/R.png",
                "music_assets/PlumMegamix.png",
            };

            s_SongInfo.title = curSong.title;
            s_SongInfo.artist = curSong.artist;
            s_SongInfo.jacketPath = (currentSelectedIdx >= 0 && currentSelectedIdx < static_cast<int>(jacketPaths.size())) ? jacketPaths[currentSelectedIdx] : "";
            s_SongInfo.bpm = curSong.bpm;
            s_SongInfo.difficultyName = "HARD";
            s_SongInfo.difficultyLevel = 10;

            s_SongTimer = 0.0f;
            s_IsPaused = false;
            s_PauseSelection = 0;
            s_IgnoreFirstEnter = true; 
            
            m_SongSelect.SetSelectedSongIndex(currentSelectedIdx); 

            s_LoadingScreen.SetTargetProgress(1.0f);
            s_LoadPrepared = true;
            m_State = PlaySceneState::Playing;
        }

        if (s_LoadingScreen.IsComplete())
        {
            if (s_MusicPlayer.IsValid())
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

                // [추가된 코드] 재생될 음악의 실제 총 길이를 구해서 s_SongInfo에 저장합니다.
                if (channel)
                {
                    FMOD_SOUND* currentSound = nullptr;
                    FMOD_Channel_GetCurrentSound(channel, &currentSound);
                    if (currentSound)
                    {
                        unsigned int lengthMs = 0;
                        FMOD_Sound_GetLength(currentSound, &lengthMs, FMOD_TIMEUNIT_MS);
                        s_SongInfo.totalTimeSec = static_cast<float>(lengthMs) / 1000.0f;
                    }
                }
                else
                {
                    s_SongInfo.totalTimeSec = 180.0f; 
                }
            }

            s_IsLoading = false;
            m_State = PlaySceneState::Playing;
            UpdatePlaying();
        }

        return;
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

            if (s_LoadedJacketTex.id != 0)
            {
                UnloadTexture(s_LoadedJacketTex);
                s_LoadedJacketTex = { 0 };
            }

            static const std::vector<std::string> jacketPaths = {
                "music_assets/stars.png",
                "music_assets/kaleidoscope.png",
                "music_assets/SkysCape.png",
                "music_assets/TheLostAria.png",
                "music_assets/Timeline.png",
                "music_assets/Terrasphere.png",
                "music_assets/N.png",
                "music_assets/SecretDoll2.png",
                "music_assets/R.png",
                "music_assets/PlumMegamix.png",
            };

            if (currentSelectedIdx >= 0 && currentSelectedIdx < static_cast<int>(jacketPaths.size()))
            {
                s_LoadedJacketTex = LoadTexture(jacketPaths[currentSelectedIdx].c_str());
                if (s_LoadedJacketTex.id != 0)
                {
                    SetTextureFilter(s_LoadedJacketTex, TEXTURE_FILTER_BILINEAR);
                }
            }

            s_PendingSongData.jacketTexture = s_LoadedJacketTex;
            s_PendingSongData.title = curSong.title;
            s_PendingSongData.artist = curSong.artist;

            s_LoadingScreen.Start(s_PendingSongData);
            s_IsLoading = true;
            s_LoadPrepared = false;
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
        s_ChartEditor.HandleInput();
        return;
    }

    if (s_MusicPlayer.IsValid() && s_MusicPlayer.GetChannelRaw()) 
    {
        if (IsKeyDown(KEY_L) || IsKeyPressed(KEY_L)) 
        {
            FMOD_CHANNEL* channel = s_MusicPlayer.GetChannelRaw();
            float originalFrequency = 44100.0f; 
            FMOD_SOUND* currentSound = nullptr;
            
            FMOD_Channel_GetCurrentSound(channel, &currentSound);
            if (currentSound) 
            {
                FMOD_Sound_GetDefaults(currentSound, &originalFrequency, nullptr);
            }
            
            FMOD_Channel_SetPitch(channel, 1.0f);
            FMOD_Channel_SetFrequency(channel, originalFrequency);
            
            while (GetCharPressed() > 0)
            {
                // 입력 버퍼 비우기
            }
        }
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
                if (s_MusicPlayer.IsValid()) 
                {
                    s_MusicPlayer.Stop();
                }
                
                s_BgaPlayer.Close();
                s_PlayableNotes.clear();
                s_PlayableNotes.shrink_to_fit();
                s_Notes.clear();
                s_Notes.shrink_to_fit();
                s_BeatmapClock.SetChannel(nullptr);
                s_SongTimer = 0.0f;
                s_IsPaused = false;
                s_SongSelectEnterDelay = 0.3f;
                s_Combo = 0; 
                s_LastCombo = 0;
                s_PerfectCount = 0; 
                s_GreatCount = 0; 
                s_GoodCount = 0; 
                s_MissCount = 0;
                s_MaxCombo = 0; 
                s_Score = 0; 
                s_Accuracy = 100.0f; 
                s_HpRatio = 1.0f;
                s_SongInfo.score = 0; 
                s_SongInfo.maxCombo = 0; 
                s_SongInfo.currentCombo = 0;
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
                if (s_MusicPlayer.IsValid()) 
                {
                    s_MusicPlayer.Stop();
                }
                s_BgaPlayer.Close();
                s_PlayableNotes.clear(); 
                s_Notes.clear();
                s_BeatmapClock.SetChannel(nullptr);
                s_SongTimer = 0.0f; 
                s_IsPaused = false; 
                s_SongSelectEnterDelay = 0.3f;
                m_State = PlaySceneState::SongSelect;
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
                s_BgaPlayer.Close();
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

    s_SongTimer = static_cast<float>(s_BeatmapClock.GetCurrentTime() / 1000.0);

    if (s_JudgmentLinePulse > 0.0f) 
    { 
        s_JudgmentLinePulse -= dt * 4.0f; 
        if (s_JudgmentLinePulse < 0.0f) 
        {
            s_JudgmentLinePulse = 0.0f; 
        }
    }
    
    if (s_JudgmentAnimTimer > 0.0f) 
    { 
        s_JudgmentAnimTimer -= dt; 
        if (s_JudgmentAnimTimer < 0.0f) 
        {
            s_JudgmentAnimTimer = 0.0f; 
        }
    }
    
    if (s_ComboAnimTimer > 0.0f) 
    { 
        s_ComboAnimTimer -= dt; 
        if (s_ComboAnimTimer < 0.0f) 
        {
            s_ComboAnimTimer = 0.0f; 
        }
    }

    for (auto& pNote : s_PlayableNotes)
    {
        if (pNote.active)
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
                s_MissCount++; 
                s_HpRatio -= 0.05f; 
                if (s_HpRatio < 0.0f) 
                {
                    s_HpRatio = 0.0f; 
                }
            }
        }
    }

    HitEffect::Update();

    bool lanePressed[4] = { 
        IsKeyPressed(KEY_D), 
        IsKeyPressed(KEY_F), 
        IsKeyPressed(KEY_J), 
        IsKeyPressed(KEY_K) 
    };

    for (int lane = 0; lane < 4; ++lane)
    {
        if (!lanePressed[lane]) 
        {
            continue;
        }

        bool hitHandled = false;
        for (auto& pNote : s_PlayableNotes)
        {
            if (!pNote.active || pNote.lane != lane) 
            {
                continue;
            }

            float timeDiff = s_SongTimer - pNote.timeSec;
            float absDiff = fabsf(timeDiff);

            if (absDiff <= 0.15f)
            {
                pNote.active = false;
                float nX = GetLaneXCoord(pNote.lane);
                HitEffect::Spawn({ nX, judgmentLineY });


                s_ShowJudgment = true; 
                s_JudgmentTimer = 0.4f;
                s_JudgmentLinePulse = 1.0f; 
                s_JudgmentAnimTimer = 0.3f;

                if (absDiff <= 0.07f) 
                { 
                    s_CurrentJudgment = "PERFECT"; 
                    s_Combo++; 
                    s_PerfectCount++; 
                    s_Score += 1000 + s_Combo * 10; 
                    s_HpRatio += 0.01f; 
                }
                else if (absDiff <= 0.12f) 
                { 
                    s_CurrentJudgment = "GREAT"; 
                    s_Combo++; 
                    s_GreatCount++; 
                    s_Score += 700 + s_Combo * 5; 
                    s_HpRatio += 0.005f; 
                }
                else 
                { 
                    s_CurrentJudgment = "GOOD"; 
                    s_Combo++; 
                    s_GoodCount++; 
                    s_Score += 300; 
                }

                if (s_Combo > s_MaxCombo) 
                {
                    s_MaxCombo = s_Combo;
                }
                if (s_HpRatio > 1.0f) 
                {
                    s_HpRatio = 1.0f;
                }

                if (std::string(s_CurrentJudgment) == "PERFECT" && s_Combo != s_LastCombo)
                {
                    s_ComboAnimTimer = 0.2f;
                    s_LastCombo = s_Combo;
                }
                hitHandled = true;
                break;
            }
        }

        if (!hitHandled)
        {
            if (FnClick::IsFastHit(lane, s_SongTimer, s_PlayableNotes)) 
            {
                FnClick::TriggerFast(s_Combo, s_LastCombo, s_ShowJudgment, s_JudgmentTimer, s_CurrentJudgment, s_JudgmentAnimTimer);
            }
            else if (SnClick::IsSlowHit(lane, s_SongTimer, s_PlayableNotes)) 
            {
                SnClick::TriggerSlow(s_Combo, s_LastCombo, s_ShowJudgment, s_JudgmentTimer, s_CurrentJudgment, s_JudgmentAnimTimer);
            }
            else 
            {
                GhClick::TriggerBreak(s_Combo, s_LastCombo, s_ShowJudgment, s_JudgmentTimer, s_CurrentJudgment, s_JudgmentAnimTimer);
            }
        }
    }

    int totalHits = s_PerfectCount + s_GreatCount + s_GoodCount + s_MissCount;
    if (totalHits > 0) 
    {
        s_Accuracy = ((s_PerfectCount * 100.0f + s_GreatCount * 70.0f + s_GoodCount * 30.0f) / (totalHits * 100.0f)) * 100.0f;
    }

    s_SongInfo.playTimeSec = s_SongTimer; 
    s_SongInfo.score = s_Score; 
    s_SongInfo.accuracy = s_Accuracy;
    s_SongInfo.maxCombo = s_MaxCombo; 
    s_SongInfo.currentCombo = s_Combo;
    s_SongInfo.perfectCount = s_PerfectCount; 
    s_SongInfo.greatCount = s_GreatCount;
    s_SongInfo.goodCount = s_GoodCount; 
    s_SongInfo.missCount = s_MissCount; 
    s_SongInfo.hpRatio = s_HpRatio;

    s_PlayMainUI.Update(s_SongInfo);

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
    if (m_State == PlaySceneState::SongSelect)
    {
        m_SongSelect.Draw(GetScreenWidth(), GetScreenHeight());
    }
    else if (m_State == PlaySceneState::Playing)
    {
        DrawPlaying();
    }

    if (s_IsLoading)
    {
        s_LoadingScreen.Draw();
    }
}

void PlayScene::DrawPlaying()
{
    if (s_IsEditorMode)
    {
        s_ChartEditor.Render();
        return;
    }

    bool pressedStates[4] = {
        IsKeyDown(KEY_D),
        IsKeyDown(KEY_F),
        IsKeyDown(KEY_J),
        IsKeyDown(KEY_K)
    };

    DrawBackground();
    DrawPlayfield();
    DrawLanes(pressedStates, judgmentLineY);
    DrawInputPanel(pressedStates, judgmentLineY);
    DrawInputFeedbackFlash(pressedStates, judgmentLineY);
    DrawNotes(judgmentLineY);
    DrawJudgmentLine(judgmentLineY);
    DrawJudgmentText();
    DrawComboHUD();

    HitEffect::Draw();

    s_PlayMainUI.Draw(s_SongInfo, GetScreenWidth(), GetScreenHeight());

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

    s_BgaPlayer.Close();

    if (s_LoadedJacketTex.id != 0)
    {
        UnloadTexture(s_LoadedJacketTex);
        s_LoadedJacketTex = { 0 };
    }

    if (s_ComboFont.texture.id != 0)
    {
        UnloadFont(s_ComboFont);
        s_ComboFont = Font{ 0 };
    }

    if (s_SuitFont.texture.id != 0)
    {
        UnloadFont(s_SuitFont);
        s_SuitFont = Font{ 0 };
    }

    s_ChartEditor.Release();
}