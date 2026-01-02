#pragma once

#include <stdlib.h>

static const int IdleParticleCount = 256;       // 256 particles benchmarked at under 1ms overhead with current effect

struct IdleSpawnPoint {
    int x;
    int y;
    int w;
    int h;
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
