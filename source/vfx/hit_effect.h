// source/vfx/hit_effect.h
#pragma once
#include "raylib.h"
#include <vector>

struct Particle {
    Vector2 position;
    Vector2 velocity;
    Color color;
    float radius;
    float alpha;
    float life;
};

class HitEffect {
public:
    static void Spawn(Vector2 pos, Color color);
    static void Update();
    static void Draw();

private:
    static std::vector<Particle> s_Particles;
};