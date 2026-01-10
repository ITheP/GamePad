#include "Idle.h"
#include <stdlib.h>
#include <Screen.h>
#include <Benchmark.h>

// Idle effect, drawn when device has been sat around getting bored for a while
// Usually shown same time LED idle effect kicks in

int IdleParticleCount = IDLE_MAX_PARTICLE_COUNT;

// Safe spawn points, not part of any small area. Area will appear will be from top left (point itself) to random extra pixels in x + y direction from it
// x, y, width of random area, height of random area
// Couple of variants for simple particle limit alternatives
IdleSpawnPoint IdleSpawnPoints[] = {
    {1, 20, 10, 30, IDLE_MAX_PARTICLE_COUNT},           // Left of guitar...
    {13, 28, 9, 9, 32},                                 // Special case within the guitar body :)
    {13, 28, 9, 9, 16},
    {60, 15, 50, 10, IDLE_MAX_PARTICLE_COUNT},          // Top right of guitar...
    {60, 48, 50, 10, IDLE_MAX_PARTICLE_COUNT}};          // Top right of guitar...

int IdleSpawnPointCount = sizeof(IdleSpawnPoints) / sizeof(IdleSpawnPoints[0]);

IdleParticle particles[IDLE_MAX_PARTICLE_COUNT];

int currentParticleCount;
unsigned long nextSpawnTime = 0;
unsigned long spawnRate = 500;

// Benchmark IdleBenchmark("Idle");

// Random +/- 0.15 to 0.3
static float randVel()
{
    float mag = 0.15f + (rand() * (1.0f / RAND_MAX)) * 0.15f;
    return (rand() & 1 ? mag : -mag);
}

// Random + 0.15 to 0.3
static float randPosVel()
{
    return 0.15f + (rand() * (1.0f / RAND_MAX)) * 0.15f;
}

void InitIdleEffect()
{
    // InitDisplayBuffer();

    IdleSpawnPoint *sp = &IdleSpawnPoints[rand() % IdleSpawnPointCount];
    // Randomise max particles from 25% to 100% of max count
    int quarterCount = sp->maxParticles / 4;
    IdleParticleCount = quarterCount + (rand() % (quarterCount * 3));

    // Only need to set the position of first particle - others spawn off from this particle
    IdleParticle *p = &particles[0];

    // Find an initial black pixel to start on in that area
    while (true)
    {
        p->x = sp->x + (rand() % (sp->w));
        p->y = sp->y + (rand() % (sp->h));

        if (Display.getPixel((int)p->x, (int)p->y) == 0)
            break;
    }

    for (int i = 0; i < IdleParticleCount; i++)
    {
        p = &particles[i];

        // // Random starting position on a black pixel
        // while (true)
        // {
        //     p.x = rand() % SCREEN_WIDTH;
        //     p.y = rand() % SCREEN_HEIGHT;

        //     // if (getPixelFast((int)p.x, (int)p.y) == 0)
        //     if (Display.getPixel((int)p.x, (int)p.y) == 0)
        //         break;
        // }

        // Random velocity
        p->vx = randVel();
        p->vy = randVel();
        p->lastX = (int)p->x;
        p->lastY = (int)p->y;
    }

    currentParticleCount = 1;
    nextSpawnTime = millis() + spawnRate;
}

// Over engineered bouncing pixel effect.
// Assumes never move > 1 pixel at a time (easier collision detection then)
// Realistically we throttle velocities to make things look nice
// Accounts for more lively pixel bounces (especially at edge of screen where),
// Slightly randomised velocities, and pixels can collide with each other.
// Not optimal fastest possible code but 256 particles tested at < 1ms overhead
// Note that also imperfections can occur with live animations also on screen affecting
// the environment the pixels are in

void RenderIdleEffect(int maxParticles)
{
    // IdleBenchmark.Start("Idle Start");

    // Test to check layout of screen to help work out particle start positions
    // for (int i = 0; i < SCREEN_WIDTH; i += 10)
    //     Display.drawFastVLine(i, 0, SCREEN_HEIGHT, C_WHITE);
    // for (int j = 0; j < SCREEN_HEIGHT; j += 10)
    //     Display.drawFastHLine(0, j, SCREEN_WIDTH, C_WHITE);

    for (int i = 0; i < currentParticleCount; i++)
    {
        IdleParticle &p = particles[i];

        // Erase previous pixel
        Display.writePixel(p.lastX, p.lastY, C_BLACK);

        float tryXf = p.x + p.vx;
        float tryYf = p.y + p.vy;

        int xi_try = (int)tryXf;
        int yi_try = (int)tryYf;

        float newVx = p.vx;
        float newVy = p.vy;

        // Horizontal collision
        if (xi_try < 0 || xi_try >= SCREEN_WIDTH ||
            Display.getPixel(xi_try, p.lastY) == C_WHITE)
        {

            // Horizontal bounces, we also (if any left to add) increase the particle count and add one to the end
            // Comes from same spot as parent particle, but has it's own velocities
            if (currentParticleCount < IdleParticleCount && currentParticleCount <= maxParticles && millis() > nextSpawnTime)
            {
                IdleParticle &newP = particles[currentParticleCount]; // Points to next in line already

                newP.x = p.x;
                newP.y = p.y;
                newP.lastX = p.lastX;
                newP.lastY = p.lastY;

                nextSpawnTime = millis() + spawnRate;

                currentParticleCount++;
            }

            newVx = -newVx;

            // Force a 1‑pixel rebound
            if (tryXf < 1.0f)
                tryXf += 1.0f;
            else if (tryXf >= SCREEN_WIDTH - 1.0f)
                tryXf -= 1.0f;

            tryXf += newVx;
            xi_try = (int)tryXf;
        }

        // Vertical bounces
        if (yi_try < 0 || yi_try >= SCREEN_HEIGHT ||
            Display.getPixel(p.lastX, yi_try) == C_WHITE)
        {
            newVy = -newVy;

            // // Force a 1‑pixel rebound
            if (tryYf < 1.0f)
                tryYf += 1.0f;
            else if (tryYf >= SCREEN_WIDTH - 1.0f)
                tryYf -= 1.0f;

            tryYf += newVy;
            yi_try = (int)tryYf;
        }

        int xi_final = xi_try;
        int yi_final = yi_try;

        // Final new position checks
        if (Display.getPixel(xi_final, yi_final) == C_WHITE)
        {
            // Clash - something white was there already
            // stay where we are but change velocities to try again next time

            if (newVx > 0)
                newVx = -randPosVel();
            else
                newVx = randPosVel();
            if (newVy > 0)
                newVy = -randPosVel();
            else
                newVy = randPosVel();

            tryXf = p.x;
            tryYf = p.y;

            xi_final = p.lastX;
            yi_final = p.lastY;
        }

        // Commit final values
        p.vx = newVx;
        p.vy = newVy;

        p.x = tryXf;
        p.y = tryYf;

        // Even if we are re-writing where we were before, that's ok as it will have been erased earlier
        Display.writePixel(xi_final, yi_final, C_WHITE);

        p.lastX = xi_final;
        p.lastY = yi_final;
    }

    // IdleBenchmark.Snapshot("Idle Stop");
}

void StopIdleEffect()
{
    for (int i = 0; i < currentParticleCount; i++)
        Display.writePixel((int)particles[i].x, (int)particles[i].y, C_BLACK);
}