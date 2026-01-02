#include "Idle.h"
#include <stdlib.h>
#include <Screen.h>
#include <Benchmark.h>

IdleParticle particles[IdleParticleCount];
//uint8_t *screenBuf = nullptr;
int currentParticleCount;
unsigned long nextSpawnTime = 0;
unsigned long spawnRate = 500;

//Benchmark IdleBenchmark("Idle");

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

// void InitDisplayBuffer()
// {
//     screenBuf = Display.getBuffer();
// }

void InitIdleEffect()
{
    //InitDisplayBuffer();

    for (int i = 0; i < IdleParticleCount; i++)
    {
        IdleParticle &p = particles[i];

        // Random starting position on a black pixel
        while (true)
        {
            p.x = rand() % SCREEN_WIDTH;
            p.y = rand() % SCREEN_HEIGHT;

            // if (getPixelFast((int)p.x, (int)p.y) == 0)
            if (Display.getPixel((int)p.x, (int)p.y) == 0)
                break;
        }

        // Random velocity
        p.vx = randVel();
        p.vy = randVel();
        p.lastX = (int)p.x;
        p.lastY = (int)p.y;
    }

    currentParticleCount = 1;
    nextSpawnTime = millis() + nextSpawnTime;
}

// TODO - INITIAL SPAWN POINT OF PIXEL - MAKE IT FROM A RANDOM POSITION FROM A SET OF 10 USER DEFINED POSITIONS SO WE DONT
// EVER SPAWN A START INSIDE A TINY SPACE ACCIDENTALLY

// Over engineered bouncing pixel effect.
// Assumes never move > 1 pixel at a time (easier collision detection then)
// Realistically we throttle velocities to make things look nice
// Accounts for more lively pixel bounces (especially at edge of screen where),
// Slightly randomised velocities, and pixels can collide with each other.
// Not optimal fastest possible code but 256 particles tested at < 1ms overhead
// Note that also imperfections can occur with live animations also on screen affecting
// the environment the pixels are in

void RenderIdleEffect()
{
    //IdleBenchmark.Start("Idle Start");

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
            if (currentParticleCount < IdleParticleCount && millis() > nextSpawnTime)
            {
                IdleParticle &newP = particles[currentParticleCount];   // Points to next in line already

                newP.x = p.x;
                newP.y = p.y;
                newP.lastX = p.lastX;
                newP.lastY = p.lastY;

                nextSpawnTime = millis() + spawnRate;

                currentParticleCount++;
            }

            newVx = -newVx;

            // Force a 1‑pixel rebound
            // if (p.x < 1.0f)
            //     p.x += 1.0f;
            // else if (p.x >= SCREEN_WIDTH - 1.0f)
            //     p.x -= 1.0f;

            // tryXf = p.x + newVx;
            // xi_try = (int)tryXf;

                        // Force a 1‑pixel rebound
            if (tryXf < 1.0f)
                tryXf += 1.0f;
            else if (tryXf >= SCREEN_WIDTH - 1.0f)
                tryXf -= 1.0f;

            tryXf += newVx;
            xi_try = (int)tryXf;
        }

        // Vertical collision

        if (yi_try < 0 || yi_try >= SCREEN_HEIGHT ||
            Display.getPixel(p.lastX, yi_try) == C_WHITE)
        {
            newVy = -newVy;

            // // Force a 1‑pixel rebound
            // if (p.y < 1.0f)
            //     p.y += 1.0f;
            // else if (p.y >= SCREEN_HEIGHT - 1.0f)
            //     p.y -= 1.0f;

            // tryYf = p.y + newVy;
            // yi_try = (int)tryYf;

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

            tryXf = p.x; // + newVx;
            tryYf = p.y; // + newVy;

            // xi_try = (int)tryXf;
            // yi_try = (int)tryYf;

            xi_final = p.lastX;
            yi_final = p.lastY;
        }

        // Commit final values
        p.vx = newVx;
        p.vy = newVy;

        //p.x += p.vx;
        //p.y += p.vy;
        p.x = tryXf;
        p.y = tryYf;

        // Even if we are re-writing where we were before, that's ok as it will have been erased earlier
        Display.writePixel(xi_final, yi_final, C_WHITE);

        p.lastX = xi_final;
        p.lastY = yi_final;
    }

    //IdleBenchmark.Snapshot("Idle Stop");
}

void StopIdleEffect()
{
    for (int i = 0; i < currentParticleCount; i++) //IdleParticleCount; i++)
        Display.writePixel((int)particles[i].x, (int)particles[i].y, C_BLACK);
}

// Basic early version
// void RenderIdleEffect()
// {
//     //InitDisplayBuffer();
// IdleBenchmark.Start("Idle Start");

//     for (int i = 0; i < IdleParticleCount; i++)
//     {
//         IdleParticle &p = particles[i];

//         // Erase previous pixel
//         Display.writePixel((int)p.x, (int)p.y, 0);

//         float newX = p.x;
//         float newY = p.y;

//         bool moved = false;

//         for (int attempt = 0; attempt < 5; attempt++)
//         {

//             newX = p.x + p.vx;
//             newY = p.y + p.vy;

//             // Bounce off edges
//             if (newX < 0)
//             {
//                 newX = 0;
//                 p.vx = -p.vx;
//             }
//             if (newX >= SCREEN_WIDTH)
//             {
//                 newX = SCREEN_WIDTH - 1;
//                 p.vx = -p.vx;
//             }
//             if (newY < 0)
//             {
//                 newY = 0;
//                 p.vy = -p.vy;
//             }
//             if (newY >= SCREEN_HEIGHT)
//             {
//                 newY = SCREEN_HEIGHT - 1;
//                 p.vy = -p.vy;
//             }

//             // Check if new pixel is black so we can move there, else we loop back and try again in a different direction
//             if (getPixelFast((int)newX, (int)newY) == 0)
//             {
//                 moved = true;
//                 break;
//             }

//             // Otherwise randomise velocity and try again
//             p.vx = randVel();
//             p.vy = randVel();
//         }

//         if (moved)
//         {
//             p.x = newX;
//             p.y = newY;
//         }

//         // Draw new pixel
//         Display.writePixel((int)p.x, (int)p.y, 1);
//     }
// IdleBenchmark.Snapshot("PostCalc");
//     Display.display();
// IdleBenchmark.Snapshot("PostDelay");
// }