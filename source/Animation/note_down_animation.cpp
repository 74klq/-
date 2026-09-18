#include "note_down_animation.h"
#include <cmath>
#include <vector>

namespace
{
    static float Clamp01(float v)
    {
        if (v < 0.0f) return 0.0f;
        if (v > 1.0f) return 1.0f;
        return v;
    }

    static float EaseOutCubic(float t)
    {
        t = Clamp01(t);
        float u = 1.0f - t;
        return 1.0f - u * u * u;
    }

    static float EaseOutQuint(float t)
    {
        t = Clamp01(t);
        float u = 1.0f - t;
        return 1.0f - u * u * u * u * u;
    }


    static float SmoothStep(float t)
    {
        t = Clamp01(t);
        return t * t * (3.0f - 2.0f * t);
    }

    static float LerpFloat(float a, float b, float t)
    {
        return a + (b - a) * t;
    }


    static Vector2 CubicBezier(Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3, float t)
    {
        float u = 1.0f - t;
        float uu = u * u;
        float uuu = uu * u;
        float tt = t * t;
        float ttt = tt * t;

        return {
            uuu * p0.x + 3.0f * uu * t * p1.x + 3.0f * u * tt * p2.x + ttt * p3.x,
            uuu * p0.y + 3.0f * uu * t * p1.y + 3.0f * u * tt * p2.y + ttt * p3.y
        };
    }

    static Vector2 CubicBezierDerivative(Vector2 p0, Vector2 p1, Vector2 p2, Vector2 p3, float t)
    {
        float u = 1.0f - t;

        return {
            3.0f * u * u * (p1.x - p0.x) + 6.0f * u * t * (p2.x - p1.x) + 3.0f * t * t * (p3.x - p2.x),
            3.0f * u * u * (p1.y - p0.y) + 6.0f * u * t * (p2.y - p1.y) + 3.0f * t * t * (p3.y - p2.y)
        };
    }

    static float Length(Vector2 v)
    {
        return sqrtf(v.x * v.x + v.y * v.y);
    }

    static float SafeAngleDeg(Vector2 v)
    {
        if (fabsf(v.x) < 0.001f && fabsf(v.y) < 0.001f)
        {
            return 0.0f;
        }
        return atan2f(v.y, v.x) * (180.0f / PI);
    }

    static float LaneDirection(float x)
    {
        const float laneCenters[4] = { 450.0f, 530.0f, 610.0f, 690.0f };
        int closestLane = 0;
        float closestDistance = fabsf(x - laneCenters[0]);

        for (int i = 1; i < 4; ++i)
        {
            float d = fabsf(x - laneCenters[i]);
            if (d < closestDistance)
            {
                closestDistance = d;
                closestLane = i;
            }
        }

        return (closestLane % 2 == 0) ? 1.0f : -1.0f;
    }

    static Color ScaleAlpha(Color color, float alpha)
    {
        if (alpha < 0.0f) alpha = 0.0f;
        if (alpha > 1.0f) alpha = 1.0f;
        color.a = static_cast<unsigned char>(static_cast<float>(color.a) * alpha);
        return color;
    }

    static void DrawQuadGlow(Vector2 center, float width, float height, float rotation, Color color, float alpha)
    {
        Rectangle rect{
            center.x,
            center.y,
            width,
            height
        };

        DrawRectanglePro(
            rect,
            { width * 0.5f, height * 0.5f },
            rotation,
            ScaleAlpha(color, alpha)
        );
    }

    static void DrawSegment(Vector2 a, Vector2 b, float thickness, Color color)
    {
        DrawLineEx(a, b, thickness, color);
    }

    static void DrawShard(Vector2 center, Vector2 velocity, float length, float width, Color color, float alpha, float rotationOffset)
    {
        float angle = SafeAngleDeg(velocity) + rotationOffset;
        Rectangle shard{
            center.x,
            center.y,
            length,
            width
        };

        DrawRectanglePro(
            shard,
            { 0.0f, width * 0.5f },
            angle,
            ScaleAlpha(color, alpha)
        );
    }

    static void DrawAccentLine(Vector2 center, float width, float rotation, Color color, float alpha)
    {
        DrawRectanglePro(
            Rectangle{ center.x, center.y, width, 1.5f },
            { width * 0.5f, 0.75f },
            rotation,
            ScaleAlpha(color, alpha)
        );
    }

    struct NoteAnimState
    {
        bool active;
        float targetX;
        float startY;
        float lastY;
        float lastSeenTime;
    };

    static std::vector<NoteAnimState> s_NoteAnimStates;

    static NoteAnimState* FindOrCreateState(float x, float y)
    {
        float now = (float)GetTime();

        for (auto it = s_NoteAnimStates.begin(); it != s_NoteAnimStates.end();)
        {
            if (now - it->lastSeenTime > 0.25f || it->lastY > 760.0f)
            {
                it = s_NoteAnimStates.erase(it);
            }
            else
            {
                ++it;
            }
        }

        NoteAnimState* best = nullptr;
        float bestDistance = 100000.0f;

        for (auto& state : s_NoteAnimStates)
        {
            if (!state.active)
            {
                continue;
            }

            if (fabsf(state.targetX - x) > 2.0f)
            {
                continue;
            }

            if (y < state.lastY - 8.0f)
            {
                continue;
            }

            float distance = fabsf(y - state.lastY);
            if (distance < bestDistance && distance < 95.0f)
            {
                bestDistance = distance;
                best = &state;
            }
        }

        if (best == nullptr)
        {
            NoteAnimState state{};
            state.active = true;
            state.targetX = x;
            state.startY = y;
            state.lastY = y;
            state.lastSeenTime = now;
            s_NoteAnimStates.push_back(state);
            return &s_NoteAnimStates.back();
        }

        best->lastY = y;
        best->lastSeenTime = now;
        return best;
    }
}

void NoteDownAnimation::DrawCoolNote(
    float x,
    float y,
    float noteWidth,
    float noteHeight,
    float timeRemaining,
    float totalLifeTime
)
{
    if (totalLifeTime <= 0.0f)
    {
        return;
    }

    (void)timeRemaining;

    NoteAnimState* state = FindOrCreateState(x, y);

    const float direction = LaneDirection(x);
    const float baseThrowDistance = 360.0f;
    const float throwDistance = baseThrowDistance + noteWidth * 0.9f;

    const float animationTravelDistance = 170.0f;

    float verticalTravel = y - state->startY;
    float throwProgress = Clamp01(verticalTravel / animationTravelDistance);
    float easedThrowProgress = EaseOutQuint(throwProgress);

    Vector2 finalPos{ x, y };
    Vector2 startPos{ x + direction * throwDistance, y };
    Vector2 controlA{ x + direction * (throwDistance * 0.94f), y + 18.0f };
    Vector2 controlB{ x + direction * (throwDistance * 0.22f), y + 2.0f };

    Vector2 center = finalPos;
    float rotation = 0.0f;
    float horizontalVelocity = 0.0f;
    float verticalVelocity = 1.0f;

    if (throwProgress < 1.0f)
    {
        center = CubicBezier(
            startPos,
            controlA,
            controlB,
            finalPos,
            easedThrowProgress
        );

        Vector2 tangent = CubicBezierDerivative(
            startPos,
            controlA,
            controlB,
            finalPos,
            easedThrowProgress
        );

        float tangentLength = Length(tangent);

        if (tangentLength > 0.001f)
        {
            horizontalVelocity = tangent.x / tangentLength;
            verticalVelocity = tangent.y / tangentLength;
            rotation = (tangent.x / 320.0f) * 7.0f;
            if (rotation > 7.0f) rotation = 7.0f;
            if (rotation < -7.0f) rotation = -7.0f;
        }

        center.y = y;
    }
    else
    {
        center = finalPos;
    }

    float visibility = 1.0f;

    float impactProximity = 0.0f;
    if (y > 540.0f)
    {
        impactProximity = SmoothStep((y - 540.0f) / 55.0f);
    }

    float squash = 1.0f;
    float stretch = 1.0f;
    if (throwProgress < 1.0f)
    {
        float speedPhase = sinf(throwProgress * PI);
        stretch = 1.0f + speedPhase * 0.10f;
        squash = 1.0f - speedPhase * 0.045f;
    }
    else
    {
        float settleProgress = impactProximity;
        float settlePhase = sinf(settleProgress * PI);
        squash = 1.0f + settlePhase * 0.025f;
    }

    float drawWidth = noteWidth * squash;
    float drawHeight = noteHeight * stretch;

    float halfWidth = drawWidth * 0.5f;
    float halfHeight = drawHeight * 0.5f;

    float trailStrength = 0.0f;
    if (throwProgress < 1.0f)
    {
        trailStrength = 1.0f - EaseOutCubic(throwProgress);
    }
    trailStrength *= 0.9f;

    Color shadowColor{ 0, 0, 0, 255 };
    Color bodyColor{ 70, 76, 82, 255 };
    Color bodyTopColor{ 105, 113, 121, 255 };
    Color edgeColor{ 155, 169, 178, 255 };
    Color accentColor{ 188, 220, 232, 255 };
    Color detailColor{ 26, 30, 34, 255 };

    if (trailStrength > 0.001f)
    {
        float travelAngle = SafeAngleDeg({ horizontalVelocity, verticalVelocity });
        float trailLength = LerpFloat(16.0f, 72.0f, trailStrength);
        float trailWidth = LerpFloat(2.0f, drawWidth * 0.64f, trailStrength);

        Vector2 trailBack{
            center.x - cosf(travelAngle * PI / 180.0f) * trailLength * 0.50f,
            center.y - sinf(travelAngle * PI / 180.0f) * trailLength * 0.50f
        };

        DrawSegment(
            trailBack,
            center,
            trailWidth,
            ScaleAlpha(Color{ 180, 210, 220, 255 }, trailStrength * 0.10f * visibility)
        );

        DrawSegment(
            { trailBack.x - direction * 8.0f, trailBack.y },
            { center.x - direction * 8.0f, center.y },
            trailWidth * 0.46f,
            ScaleAlpha(Color{ 236, 240, 242, 255 }, trailStrength * 0.06f * visibility)
        );

        float shardBase = trailLength * 0.45f;
        Vector2 shardCenterA{
            center.x - direction * shardBase * 0.50f,
            center.y - 1.5f
        };
        Vector2 shardCenterB{
            center.x - direction * shardBase * 0.38f,
            center.y + 3.0f
        };

        DrawShard(
            shardCenterA,
            { direction, -0.15f },
            trailLength * 0.34f,
            1.6f,
            Color{ 198, 220, 228, 255 },
            trailStrength * 0.42f * visibility,
            direction > 0.0f ? -3.0f : 3.0f
        );

        DrawShard(
            shardCenterB,
            { direction, 0.22f },
            trailLength * 0.24f,
            1.2f,
            Color{ 145, 160, 168, 255 },
            trailStrength * 0.30f * visibility,
            direction > 0.0f ? 4.0f : -4.0f
        );
    }

    if (throwProgress < 0.95f)
    {
        float ghostT = Clamp01(throwProgress * 1.35f);
        float ghostAlpha = (1.0f - ghostT) * 0.12f * visibility;
        Vector2 ghostOffset{
            direction * 22.0f * (1.0f - throwProgress),
            -4.0f * (1.0f - throwProgress)
        };

        DrawQuadGlow(
            { center.x + ghostOffset.x, center.y + ghostOffset.y },
            drawWidth * 0.92f,
            drawHeight * 0.86f,
            rotation * 0.8f,
            Color{ 170, 190, 198, 255 },
            ghostAlpha
        );
    }

    Rectangle shadowRect{
        center.x - halfWidth - 2.0f,
        center.y - halfHeight + 4.0f,
        drawWidth + 4.0f,
        drawHeight + 4.0f
    };

    DrawRectanglePro(
        shadowRect,
        { shadowRect.width * 0.5f, shadowRect.height * 0.5f },
        rotation,
        ScaleAlpha(shadowColor, 0.48f * visibility)
    );

    Rectangle outerRect{
        center.x - halfWidth - 1.5f,
        center.y - halfHeight - 1.5f,
        drawWidth + 3.0f,
        drawHeight + 3.0f
    };

    DrawRectanglePro(
        outerRect,
        { outerRect.width * 0.5f, outerRect.height * 0.5f },
        rotation,
        ScaleAlpha(edgeColor, 0.88f * visibility)
    );

    Rectangle bodyRect{
        center.x - halfWidth,
        center.y - halfHeight,
        drawWidth,
        drawHeight
    };

    DrawRectanglePro(
        bodyRect,
        { bodyRect.width * 0.5f, bodyRect.height * 0.5f },
        rotation,
        ScaleAlpha(bodyColor, visibility)
    );

    Rectangle topRect{
        center.x - halfWidth + 1.5f,
        center.y - halfHeight + 1.5f,
        drawWidth - 3.0f,
        drawHeight * 0.38f
    };

    DrawRectanglePro(
        topRect,
        { topRect.width * 0.5f, topRect.height * 0.5f },
        rotation,
        ScaleAlpha(bodyTopColor, 0.95f * visibility)
    );

    float detailWidth = drawWidth * 0.60f;
    float detailY = center.y;

    DrawRectanglePro(
        Rectangle{ center.x, detailY, detailWidth, 2.0f },
        { detailWidth * 0.5f, 1.0f },
        rotation,
        ScaleAlpha(detailColor, 0.96f * visibility)
    );

    float edgeInset = 4.0f;
    float innerLineWidth = drawWidth - edgeInset * 2.0f;

    DrawAccentLine(
        { center.x, center.y - halfHeight + 4.0f },
        innerLineWidth,
        rotation,
        accentColor,
        0.74f * visibility
    );

    DrawAccentLine(
        { center.x, center.y + halfHeight - 3.5f },
        innerLineWidth * 0.72f,
        rotation,
        Color{ 110, 130, 138, 255 },
        0.50f * visibility
    );

    DrawRectangleLinesEx(
        Rectangle{ center.x - halfWidth, center.y - halfHeight, drawWidth, drawHeight },
        1.0f,
        ScaleAlpha(edgeColor, 0.92f * visibility)
    );

    float cornerTick = drawWidth * 0.11f;
    float tickThickness = 1.0f;
    float tickAlpha = 0.68f * visibility;

    DrawSegment(
        { center.x - halfWidth + cornerTick, center.y - halfHeight + 1.0f },
        { center.x - halfWidth + cornerTick * 0.30f, center.y - halfHeight + 1.0f },
        tickThickness,
        ScaleAlpha(accentColor, tickAlpha)
    );

    DrawSegment(
        { center.x + halfWidth - cornerTick, center.y - halfHeight + 1.0f },
        { center.x + halfWidth - cornerTick * 0.30f, center.y - halfHeight + 1.0f },
        tickThickness,
        ScaleAlpha(accentColor, tickAlpha)
    );

    if (impactProximity > 0.001f)
    {
        float pulse = sinf(impactProximity * PI);
        float pulseWidth = drawWidth + pulse * 10.0f;
        float pulseHeight = drawHeight + pulse * 4.0f;

        DrawRectangleLinesEx(
            Rectangle{
                center.x - pulseWidth * 0.5f,
                center.y - pulseHeight * 0.5f,
                pulseWidth,
                pulseHeight
            },
            1.2f,
            ScaleAlpha(accentColor, pulse * 0.24f)
        );
    }

    if (throwProgress < 1.0f)
    {
        float speedAccent = sinf(throwProgress * PI);
        float accentSpan = drawWidth * (0.18f + speedAccent * 0.12f);
        float accentOffset = drawWidth * 0.28f;

        DrawAccentLine(
            { center.x - direction * accentOffset, center.y },
            accentSpan,
            rotation,
            Color{ 205, 228, 236, 255 },
            speedAccent * 0.46f * visibility
        );
    }
}
