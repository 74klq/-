#include "hit_effect.h"
#include <cmath>
#include <vector>

namespace {
    struct EffectShard {
        Vector2 position;
        Vector2 velocity;
        float life;
        float maxLife;
        float length;
        float width;
        float rotation;
        float rotationSpeed;
        float alpha;
        bool bright;
    };

    struct EffectRing {
        Vector2 position;
        float radius;
        float maxRadius;
        float life;
        float maxLife;
        float width;
    };

    std::vector<EffectShard> s_Shards;
    std::vector<EffectRing> s_Rings;
    Vector2 s_LastHitPosition = { 0.0f, 0.0f };
    float s_FlashLife = 0.0f;
    float s_FlashMaxLife = 0.0f;

    float LerpFloat(float a, float b, float t) {
        return a + (b - a) * t;
    }

    float ClampFloat(float value, float minValue, float maxValue) {
        if (value < minValue) return minValue;
        if (value > maxValue) return maxValue;
        return value;
    }

    Vector2 Polar(float angle, float radius) {
        return { cosf(angle) * radius, sinf(angle) * radius };
    }

    void DrawDiamond(Vector2 center, float size, Color color) {
        Vector2 points[4] = {
            { center.x, center.y - size },
            { center.x + size * 0.55f, center.y },
            { center.x, center.y + size },
            { center.x - size * 0.55f, center.y }
        };
        DrawTriangle(points[0], points[1], points[2], color);
        DrawTriangle(points[0], points[2], points[3], color);
    }
}

std::vector<Particle> HitEffect::s_Particles;

void HitEffect::Spawn(Vector2 pos) {
    s_LastHitPosition = pos;
    s_FlashLife = 0.075f;
    s_FlashMaxLife = 0.075f;

    EffectRing outer;
    outer.position = pos;
    outer.radius = 7.0f;
    outer.maxRadius = 88.0f;
    outer.life = 0.28f;
    outer.maxLife = 0.28f;
    outer.width = 3.0f;
    s_Rings.push_back(outer);

    EffectRing inner;
    inner.position = pos;
    inner.radius = 4.0f;
    inner.maxRadius = 48.0f;
    inner.life = 0.17f;
    inner.maxLife = 0.17f;
    inner.width = 2.0f;
    s_Rings.push_back(inner);

    const int shardCount = 34;
    for (int i = 0; i < shardCount; ++i) {
        float angle = ((float)i / (float)shardCount) * (2.0f * PI);
        float angleJitter = (float)GetRandomValue(-70, 70) * (PI / 1800.0f);
        float speed = (float)GetRandomValue(125, 315);

        EffectShard s;
        s.position = pos;
        s.velocity = Polar(angle + angleJitter, speed);
        s.life = (float)GetRandomValue(18, 36) / 100.0f;
        s.maxLife = s.life;
        s.length = (float)GetRandomValue(9, 22);
        s.width = (float)GetRandomValue(1, 3);
        s.rotation = angle + angleJitter;
        s.rotationSpeed = (float)GetRandomValue(-520, 520) * (PI / 180.0f);
        s.alpha = 1.0f;
        s.bright = (i % 5 == 0);
        s_Shards.push_back(s);
    }

    const int accentCount = 8;
    for (int i = 0; i < accentCount; ++i) {
        float angle = ((float)i / (float)accentCount) * (2.0f * PI) + 0.18f;
        float speed = (float)GetRandomValue(210, 360);

        EffectShard s;
        s.position = pos;
        s.velocity = Polar(angle, speed);
        s.life = (float)GetRandomValue(10, 22) / 100.0f;
        s.maxLife = s.life;
        s.length = (float)GetRandomValue(16, 30);
        s.width = 2.5f;
        s.rotation = angle;
        s.rotationSpeed = (float)GetRandomValue(-260, 260) * (PI / 180.0f);
        s.alpha = 1.0f;
        s.bright = true;
        s_Shards.push_back(s);
    }

    for (int i = 0; i < 24; ++i) {
        Particle p;
        p.position = pos;
        float angle = (float)i * (2.0f * PI / 24.0f);
        float speed = (float)GetRandomValue(70, 170);
        p.velocity = { cosf(angle) * speed, sinf(angle) * speed };
        p.color = WHITE;
        p.radius = (float)GetRandomValue(1, 3);
        p.alpha = 0.8f;
        p.life = 0.22f;
        p.maxLife = 0.22f;
        s_Particles.push_back(p);
    }
}

void HitEffect::Update() {
    float dt = GetFrameTime();

    for (auto it = s_Particles.begin(); it != s_Particles.end();) {
        it->position.x += it->velocity.x * dt;
        it->position.y += it->velocity.y * dt;
        it->velocity.x *= 0.985f;
        it->velocity.y *= 0.985f;
        it->life -= dt;
        it->alpha = ClampFloat(it->life / it->maxLife, 0.0f, 1.0f);

        if (it->life <= 0.0f) {
            it = s_Particles.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = s_Shards.begin(); it != s_Shards.end();) {
        it->position.x += it->velocity.x * dt;
        it->position.y += it->velocity.y * dt;
        it->velocity.x *= powf(0.08f, dt);
        it->velocity.y *= powf(0.08f, dt);
        it->rotation += it->rotationSpeed * dt;
        it->life -= dt;
        it->alpha = ClampFloat(it->life / it->maxLife, 0.0f, 1.0f);

        if (it->life <= 0.0f) {
            it = s_Shards.erase(it);
        } else {
            ++it;
        }
    }

    for (auto it = s_Rings.begin(); it != s_Rings.end();) {
        float t = 1.0f - (it->life / it->maxLife);
        float eased = 1.0f - powf(1.0f - ClampFloat(t, 0.0f, 1.0f), 3.0f);
        it->radius = LerpFloat(7.0f, it->maxRadius, eased);
        it->life -= dt;

        if (it->life <= 0.0f) {
            it = s_Rings.erase(it);
        } else {
            ++it;
        }
    }

    if (s_FlashLife > 0.0f) {
        s_FlashLife -= dt;
        if (s_FlashLife < 0.0f) s_FlashLife = 0.0f;
    }
}

void HitEffect::Draw() {
    for (const auto& ring : s_Rings) {
        float alpha = ClampFloat(ring.life / ring.maxLife, 0.0f, 1.0f);
        float pulse = 1.0f + (1.0f - alpha) * 0.35f;
        Color outer = Fade(Color{ 255, 255, 255, 255 }, alpha * 0.42f);
        Color core = Fade(WHITE, alpha * 0.9f);
        DrawCircleLines((int)ring.position.x, (int)ring.position.y, ring.radius * pulse, outer);
        DrawCircleLines((int)ring.position.x, (int)ring.position.y, ring.radius - ring.width, core);
    }

    for (const auto& p : s_Particles) {
        float tailLength = 5.0f;
        float speed = sqrtf(p.velocity.x * p.velocity.x + p.velocity.y * p.velocity.y);
        if (speed > 1.0f) {
            float nx = p.velocity.x / speed;
            float ny = p.velocity.y / speed;
            Vector2 tail = {
                p.position.x - nx * tailLength,
                p.position.y - ny * tailLength
            };
            DrawLineEx(tail, p.position, p.radius * 0.75f, Fade(p.color, p.alpha * 0.45f));
        }
        DrawCircleV(p.position, p.radius * (0.8f + p.alpha * 0.35f), Fade(p.color, p.alpha));
    }

    for (const auto& s : s_Shards) {
        float alpha = s.alpha;
        float speed = sqrtf(s.velocity.x * s.velocity.x + s.velocity.y * s.velocity.y);
        float stretch = s.length + speed * 0.018f;
        Vector2 dir = { cosf(s.rotation), sinf(s.rotation) };
        Vector2 tail = { s.position.x - dir.x * stretch, s.position.y - dir.y * stretch };

        Color glow = Fade(WHITE, alpha * (s.bright ? 0.95f : 0.58f));
        DrawLineEx(tail, s.position, s.width, glow);

        if (s.bright) {
            DrawLineEx(
                { tail.x + dir.y * 1.5f, tail.y - dir.x * 1.5f },
                { s.position.x + dir.y * 1.5f, s.position.y - dir.x * 1.5f },
                1.0f,
                Fade(WHITE, alpha * 0.38f)
            );
        }
    }

    if (s_FlashLife > 0.0f) {
        float a = ClampFloat(s_FlashLife / s_FlashMaxLife, 0.0f, 1.0f);
        DrawCircleGradient(
            s_LastHitPosition,
            24.0f + (1.0f - a) * 18.0f,
            Fade(WHITE, a * 0.42f),
            Color{ 255, 255, 255, 0 }
        );

        float diamondSize = 7.0f + (1.0f - a) * 4.0f;
        DrawDiamond(s_LastHitPosition, diamondSize, Fade(WHITE, a));
        DrawDiamond(s_LastHitPosition, diamondSize * 0.48f, Fade(Color{ 0, 0, 0, 255 }, a * 0.28f));
    }
}