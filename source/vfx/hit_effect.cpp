// source/vfx/hit_effect.cpp
#include "hit_effect.h"
#include <cmath>

std::vector<Particle> HitEffect::s_Particles;

void HitEffect::Spawn(Vector2 pos, Color color) {
    int count = 24;
    for (int i = 0; i < count; ++i) {
        float angle = (float)i * (360.0f / count) * (PI / 180.0f);
        float speed = (float)GetRandomValue(80, 220);
        
        Particle p;
        p.position = pos;
        p.velocity = { cosf(angle) * speed, sinf(angle) * speed };
        p.color = color;
        p.radius = (float)GetRandomValue(3, 7);
        p.alpha = 1.0f;
        p.life = 0.4f;
        s_Particles.push_back(p);
    }
}

void HitEffect::Update() {
    float dt = GetFrameTime();
    for (auto it = s_Particles.begin(); it != s_Particles.end();) {
        it->position.x += it->velocity.x * dt;
        it->position.y += it->velocity.y * dt;
        it->life -= dt;
        it->alpha = it->life / 0.4f;
        if (it->life <= 0.0f) {
            it = s_Particles.erase(it);
        } else {
            ++it;
        }
    }
}

void HitEffect::Draw() {
    for (const auto& p : s_Particles) {
        DrawCircleV(p.position, p.radius, Fade(p.color, p.alpha));
    }
}