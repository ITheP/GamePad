#pragma once

#include <stdlib.h>

#define IDLE_MAX_PARTICLE_COUNT 128             // 256 particles benchmarked at under 1ms overhead with current effect. Screen can get a bit crowded with 256 though.

struct IdleSpawnPoint {
    int x;
    int y;
    int w;
    int h;
    int maxParticles;
};

struct IdleParticle {
    float x;
    float y;
    float vx;
    float vy;
    int lastX;
    int lastY;
};

void InitIdleEffect();
void RenderIdleEffect();
void StopIdleEffect();

void InitDisplayBuffer();
uint8_t getPixelFast(int16_t x, int16_t y);
