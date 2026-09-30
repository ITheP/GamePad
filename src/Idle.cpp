#include "Idle.h"
#include <stdlib.h>
#include <Screen.h>
#include <Benchmark.h>
#include <Debug.h>

// Idle effect, drawn when device has been sat around getting bored for a while
// Usually shown same time LED idle effect kicks in

int IdleParticleCount = IDLE_MAX_PARTICLE_COUNT;

// Safe spawn points, not part of any small area. Area will appear will be from top left (point itself) to random extra pixels in x + y direction from it
// x, y, width of random area, height of random area
// Couple of variants for simple particle limit alternatives
IdleSpawnPoint IdleSpawnPoints[] = {
    {1, 20, 10, 30, IDLE_MAX_PARTICLE_COUNT}, // Left of guitar...
    {13, 28, 9, 9, 32},                       // Special case within the guitar body :)
    {13, 28, 9, 9, 16},
    {60, 15, 50, 10, IDLE_MAX_PARTICLE_COUNT},  // Top right of guitar...
    {60, 48, 50, 10, IDLE_MAX_PARTICLE_COUNT}}; // Top right of guitar...

int IdleSpawnPointCount = sizeof(IdleSpawnPoints) / sizeof(IdleSpawnPoints[0]);

IdleParticle particles[IDLE_MAX_PARTICLE_COUNT];


// Similar to bounce effect but with gravity!
// Add this constant near the top with other globals
const float GRAVITY = 0.03f; // Small enough to keep velocities manageable
// Max velocity cap to ensure we never move more than 1 pixel
const float MAX_VELOCITY = 0.95f; // Slightly under 1.0 for safety
const float SWOSH_SPEED = 0.02;
const float RAIN_SPLASH_FACTOR = 0.25;
const float UNORBIT_FORCE = 0.02;
const float SWARM_FOLLOW_FORCE = 0.16;

// Center of screen for UnOrbit effect
const int CENTER_X = SCREEN_WIDTH / 2.0;
const int CENTER_Y = SCREEN_HEIGHT / 2.0;

int currentParticleCount;
unsigned long nextSpawnTime = 0;
unsigned long spawnRate = 500;
int spawnDirection = 1;
// Benchmark IdleBenchmark("Idle");

float swoshTimeV = 0;
float swoshTimeH = 0;

float rnd1to5_a = 0;
float rnd1to5_b = 0;

typedef void (*RenderIdleFunction)();
RenderIdleFunction idleRenderFunctions[] = {
    RenderIdleEffect_Bounce, // Your original bounce effect
    RenderIdleEffect_Gravity, // Your new gravity effect
    RenderIdleEffect_Rain,
    RenderIdleEffect_UnOrbit,
    RenderIdleEffect_GlobalSwoosh,
    RenderIdleEffect_SprinkleSwoosh,
    RenderIdleEffect_GlobalSprinkleSwoosh,
    //RenderIdleEffect_Swarm Not quite right in use
};

const int RenderIdleEffectCount = sizeof(idleRenderFunctions) / sizeof(idleRenderFunctions[0]);

RenderIdleFunction CurrentRenderFunction = nullptr;

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

static float randFloat() {
    return rand() * (1.0f / RAND_MAX);
}

void InitIdleEffect()
{
#ifdef DEBUG_MARKS
    Debug::Mark(1, __LINE__, __FILE__, __func__);
#endif

    // InitDisplayBuffer();

            // Reset swosh time
            swoshTimeV = 0;
            swoshTimeH = 0;
         // Small measure of randomability used by some effects

            rnd1to5_a = (randFloat() * 4.0f) + 1.0f; // 1-5
            rnd1to5_b = (randFloat() * 4.0f) + 1.0f; // 1-5

    IdleSpawnPoint *sp = &IdleSpawnPoints[rand() % IdleSpawnPointCount];
    // Randomise max particles from 25% to 100% of max count
    int quarterCount = sp->maxParticles / 4;
    IdleParticleCount = quarterCount + (rand() % (quarterCount * 3));

    // Only need to set the position of first particle - others spawn off from this particle
    IdleParticle *p = &particles[0];

    // Find an initial black pixel to start on in that area
    // Note a completely white screen will screw things up :)
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
        p->bounces = 0;
    }

    currentParticleCount = 1;
    nextSpawnTime = millis() + spawnRate;
    spawnDirection = 1;

    // Pick an effect to run
    int functionIndex = rand() % RenderIdleEffectCount;
    CurrentRenderFunction = idleRenderFunctions[functionIndex];
}

void RenderIdleEffect()
{
#ifdef DEBUG_MARKS
    Debug::Mark(1, __LINE__, __FILE__, __func__);
#endif

    if (CurrentRenderFunction != nullptr) {
        CurrentRenderFunction();
    }
}

void RenderIdleEffect_SprinkleSwoosh()
{
#ifdef DEBUG_MARKS
    Debug::Mark(1, __LINE__, __FILE__, __func__);
#endif

    // Update swosh time
    swoshTimeV += SWOSH_SPEED * rnd1to5_a;
    swoshTimeH += SWOSH_SPEED * rnd1to5_b;

    for (int i = 0; i < currentParticleCount; i++)
    {
        // Calculate multipliers (0 to 1 range) - each particle gets unique offset
        float sinMultiplier = (sin(swoshTimeV + i) + 1.0f) * 0.5f;
        float cosMultiplier = (cos(swoshTimeH + i) + 1.0f) * 0.5f;

        IdleParticle &p = particles[i];

        // Erase previous pixel
        Display.writePixel(p.lastX, p.lastY, C_BLACK);

        // Apply swosh multipliers to velocities
        float vx_swosh = p.vx * sinMultiplier;
        float vy_swosh = p.vy * cosMultiplier;

        float tryXf = p.x + vx_swosh;
        float tryYf = p.y + vy_swosh;

        int xi_try = (int)tryXf;
        int yi_try = (int)tryYf;

        float newVx = p.vx;
        float newVy = p.vy;

        // Horizontal collision
        if (xi_try < 0 || xi_try >= SCREEN_WIDTH ||
            Display.getPixel(xi_try, p.lastY) == C_WHITE)
        {
            if (millis() > nextSpawnTime)
            {
                if (spawnDirection == 1)
                {
                    if (currentParticleCount < IdleParticleCount)
                    {
                        IdleParticle &newP = particles[currentParticleCount];
                        newP.x = p.x;
                        newP.y = p.y;
                        newP.lastX = p.lastX;
                        newP.lastY = p.lastY;
                        newP.vx = randVel();
                        newP.vy = randVel();
                        nextSpawnTime = millis() + spawnRate;
                        currentParticleCount++;
                    }
                    else
                    {
                        spawnDirection = -1;
                    }
                }

                if (spawnDirection == -1 && i == currentParticleCount - 1)
                {
                    currentParticleCount--;
                    if (currentParticleCount == 0)
                    {
                        InitIdleEffect();
                        return;
                    }
                    continue;
                }
            }

            newVx = -newVx;

            if (tryXf < 1.0f)
                tryXf += 1.0f;
            else if (tryXf >= SCREEN_WIDTH - 1.0f)
                tryXf -= 1.0f;

            tryXf += newVx * sinMultiplier;
            xi_try = (int)tryXf;
        }

        // Vertical collision
        if (yi_try < 0 || yi_try >= SCREEN_HEIGHT ||
            Display.getPixel(p.lastX, yi_try) == C_WHITE)
        {
            newVy = -newVy;

            if (tryYf < 1.0f)
                tryYf += 1.0f;
            else if (tryYf >= SCREEN_HEIGHT - 1.0f)
                tryYf -= 1.0f;

            tryYf += newVy * cosMultiplier;
            yi_try = (int)tryYf;
        }

        int xi_final = xi_try;
        int yi_final = yi_try;

        // Final position check
        if (Display.getPixel(xi_final, yi_final) == C_WHITE)
        {
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

        p.vx = newVx;
        p.vy = newVy;
        p.x = tryXf;
        p.y = tryYf;

        Display.writePixel(xi_final, yi_final, C_WHITE);
        p.lastX = xi_final;
        p.lastY = yi_final;
    }
}

void RenderIdleEffect_GlobalSprinkleSwoosh()
{
#ifdef DEBUG_MARKS
    Debug::Mark(1, __LINE__, __FILE__, __func__);
#endif

    // Update swosh time
    swoshTimeV += SWOSH_SPEED * rnd1to5_a;
    swoshTimeH += SWOSH_SPEED * rnd1to5_b;

    // Calculate global multipliers (0 to 1 range)
    float sinMultiplier = (sin(swoshTimeV) + 1.0f) * 0.5f;
    float cosMultiplier = (cos(swoshTimeH) + 1.0f) * 0.5f;

    for (int i = 0; i < currentParticleCount; i++)
    {
        // Calculate per-particle multipliers (0 to 1 range)
        float sinMultiplier2 = (sin(swoshTimeV + i) + 1.0f) * 0.5f;
        float cosMultiplier2 = (cos(swoshTimeH + i) + 1.0f) * 0.5f;

        IdleParticle &p = particles[i];

        // Erase previous pixel
        Display.writePixel(p.lastX, p.lastY, C_BLACK);

        // Apply combined swosh multipliers to velocities
        float vx_swosh = p.vx * ((sinMultiplier + sinMultiplier2) * 0.5f);
        float vy_swosh = p.vy * ((cosMultiplier + cosMultiplier2) * 0.5f);

        float tryXf = p.x + vx_swosh;
        float tryYf = p.y + vy_swosh;

        int xi_try = (int)tryXf;
        int yi_try = (int)tryYf;

        float newVx = p.vx;
        float newVy = p.vy;

        // Horizontal collision
        if (xi_try < 0 || xi_try >= SCREEN_WIDTH ||
            Display.getPixel(xi_try, p.lastY) == C_WHITE)
        {
            if (millis() > nextSpawnTime)
            {
                if (spawnDirection == 1)
                {
                    if (currentParticleCount < IdleParticleCount)
                    {
                        IdleParticle &newP = particles[currentParticleCount];
                        newP.x = p.x;
                        newP.y = p.y;
                        newP.lastX = p.lastX;
                        newP.lastY = p.lastY;
                        newP.vx = randVel();
                        newP.vy = randVel();
                        nextSpawnTime = millis() + spawnRate;
                        currentParticleCount++;
                    }
                    else
                    {
                        spawnDirection = -1;
                    }
                }

                if (spawnDirection == -1 && i == currentParticleCount - 1)
                {
                    currentParticleCount--;
                    if (currentParticleCount == 0)
                    {
                        InitIdleEffect();
                        return;
                    }
                    continue;
                }
            }

            newVx = -newVx;

            if (tryXf < 1.0f)
                tryXf += 1.0f;
            else if (tryXf >= SCREEN_WIDTH - 1.0f)
                tryXf -= 1.0f;

            tryXf += newVx * sinMultiplier;
            xi_try = (int)tryXf;
        }

        // Vertical collision
        if (yi_try < 0 || yi_try >= SCREEN_HEIGHT ||
            Display.getPixel(p.lastX, yi_try) == C_WHITE)
        {
            newVy = -newVy;

            if (tryYf < 1.0f)
                tryYf += 1.0f;
            else if (tryYf >= SCREEN_HEIGHT - 1.0f)
                tryYf -= 1.0f;

            tryYf += newVy * cosMultiplier;
            yi_try = (int)tryYf;
        }

        int xi_final = xi_try;
        int yi_final = yi_try;

        // Final position check
        if (Display.getPixel(xi_final, yi_final) == C_WHITE)
        {
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

        p.vx = newVx;
        p.vy = newVy;
        p.x = tryXf;
        p.y = tryYf;

        Display.writePixel(xi_final, yi_final, C_WHITE);
        p.lastX = xi_final;
        p.lastY = yi_final;
    }
}

void RenderIdleEffect_Swarm()
{
#ifdef DEBUG_MARKS
    Debug::Mark(1, __LINE__, __FILE__, __func__);
#endif

    // Find the leader (first particle)
    IdleParticle* leader = NULL;
    if (currentParticleCount > 0)
    {
        leader = &particles[0];
    }

    for (int i = 0; i < currentParticleCount; i++)
    {
        IdleParticle &p = particles[i];

        // Erase previous pixel - ALWAYS erase (like other effects)
        Display.writePixel(p.lastX, p.lastY, C_BLACK);

        // If this is the leader, use UnOrbit behavior
        if (i == 0 && leader)
        {
            // Apply UnOrbit behavior to leader
            const float MAX_DIST = sqrt(CENTER_X * CENTER_X + CENTER_Y * CENTER_Y);

            float dx = p.x - CENTER_X;
            float dy = p.y - CENTER_Y;
            float dist = sqrt(dx * dx + dy * dy);

            float dirX = 0.0f, dirY = 0.0f;
            if (dist > 0.001f)
            {
                dirX = dx / dist;
                dirY = dy / dist;
            }

            float distPercent = dist / MAX_DIST;

            float force = 0.0f;
            if (distPercent > 0.65f)
            {
                force = -UNORBIT_FORCE * 1.5f;
                p.outward = false;
            }
            else if (distPercent < 0.35f)
            {
                force = UNORBIT_FORCE * 1.5f;
                p.outward = true;
            }
            else
            {
                force = p.outward ? UNORBIT_FORCE * 0.3f : -UNORBIT_FORCE * 0.3f;
            }

            p.vx += dirX * force;
            p.vy += dirY * force;

            float tangentX = -dirY;
            float tangentY = dirX;
            float orbitSpeed = 0.015f * (1.0f + distPercent);
            p.vx += tangentX * orbitSpeed;
            p.vy += tangentY * orbitSpeed;
        }
        else
        {
            // Follow the leader
            if (leader)
            {
                float dx = leader->x - p.x;
                float dy = leader->y - p.y;
                float dist = sqrt(dx * dx + dy * dy);

                if (dist > 0.5f)
                {
                    float dirX = dx / dist;
                    float dirY = dy / dist;

                    float followStrength = SWARM_FOLLOW_FORCE;
                    if (dist * 0.01f < followStrength)
                        followStrength = dist * 0.01f;

                    p.vx += dirX * followStrength;
                    p.vy += dirY * followStrength;

                    p.vx += ((rand() * (1.0f / RAND_MAX)) - 0.5f) * 0.003f;
                    p.vy += ((rand() * (1.0f / RAND_MAX)) - 0.5f) * 0.003f;

                    if (dist < 1.0f)
                    {
                        p.vx -= dirX * 0.01f;
                        p.vy -= dirY * 0.01f;
                    }
                }
            }
        }

        // Clamp velocities
        if (p.vx > MAX_VELOCITY) p.vx = MAX_VELOCITY;
        if (p.vx < -MAX_VELOCITY) p.vx = -MAX_VELOCITY;
        if (p.vy > MAX_VELOCITY) p.vy = MAX_VELOCITY;
        if (p.vy < -MAX_VELOCITY) p.vy = -MAX_VELOCITY;

        float tryXf = p.x + p.vx;
        float tryYf = p.y + p.vy;

        int xi_try = (int)tryXf;
        int yi_try = (int)tryYf;

        float newVx = p.vx;
        float newVy = p.vy;

        // Horizontal collision - SAME AS OTHER EFFECTS
        if (xi_try < 0 || xi_try >= SCREEN_WIDTH ||
            Display.getPixel(xi_try, p.lastY) == C_WHITE)
        {
            if (millis() > nextSpawnTime)
            {
                if (spawnDirection == 1)
                {
                    if (currentParticleCount < IdleParticleCount)
                    {
                        IdleParticle &newP = particles[currentParticleCount];
                        if (leader)
                        {
                            float angle = (rand() * (1.0f / RAND_MAX)) * 2.0f * PI;
                            float radius = 1.0f + (rand() * (1.0f / RAND_MAX)) * 1.5f;
                            newP.x = leader->x + cos(angle) * radius;
                            newP.y = leader->y + sin(angle) * radius;
                        }
                        else
                        {
                            newP.x = CENTER_X + ((rand() * (1.0f / RAND_MAX)) - 0.5f) * 10.0f;
                            newP.y = CENTER_Y + ((rand() * (1.0f / RAND_MAX)) - 0.5f) * 10.0f;
                        }

                        // Ensure new particle starts on a black pixel
                        if (Display.getPixel((int)newP.x, (int)newP.y) == C_WHITE)
                        {
                            // Find nearest black pixel
                            for (int dy = -2; dy <= 2; dy++)
                            {
                                for (int dx = -2; dx <= 2; dx++)
                                {
                                    int nx = (int)newP.x + dx;
                                    int ny = (int)newP.y + dy;
                                    if (nx >= 0 && nx < SCREEN_WIDTH && ny >= 0 && ny < SCREEN_HEIGHT &&
                                        Display.getPixel(nx, ny) == C_BLACK)
                                    {
                                        newP.x = (float)nx;
                                        newP.y = (float)ny;
                                        break;
                                    }
                                }
                            }
                        }

                        newP.lastX = (int)newP.x;
                        newP.lastY = (int)newP.y;
                        newP.vx = randVel() * 0.3f;
                        newP.vy = randVel() * 0.3f;
                        newP.outward = true;
                        nextSpawnTime = millis() + spawnRate;
                        currentParticleCount++;
                    }
                    else
                    {
                        spawnDirection = -1;
                    }
                }

                if (spawnDirection == -1 && i == currentParticleCount - 1)
                {
                    currentParticleCount--;
                    if (currentParticleCount == 0)
                    {
                        InitIdleEffect();
                        return;
                    }
                    continue;
                }
            }

            newVx = -newVx;

            if (tryXf < 1.0f)
                tryXf += 1.0f;
            else if (tryXf >= SCREEN_WIDTH - 1.0f)
                tryXf -= 1.0f;

            tryXf += newVx;
            xi_try = (int)tryXf;
        }

        // Vertical collision - SAME AS OTHER EFFECTS
        if (yi_try < 0 || yi_try >= SCREEN_HEIGHT ||
            Display.getPixel(p.lastX, yi_try) == C_WHITE)
        {
            newVy = -newVy;

            if (tryYf < 1.0f)
                tryYf += 1.0f;
            else if (tryYf >= SCREEN_HEIGHT - 1.0f)
                tryYf -= 1.0f;

            tryYf += newVy;
            yi_try = (int)tryYf;
        }

        int xi_final = xi_try;
        int yi_final = yi_try;

        // Final position check - SAME AS OTHER EFFECTS
        if (Display.getPixel(xi_final, yi_final) == C_WHITE)
        {
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

        // Commit final values - SAME AS OTHER EFFECTS
        p.vx = newVx;
        p.vy = newVy;
        p.x = tryXf;
        p.y = tryYf;

        // Draw - SAME AS OTHER EFFECTS
        Display.writePixel(xi_final, yi_final, C_WHITE);
        p.lastX = xi_final;
        p.lastY = yi_final;
    }
}

// Over engineered bouncing pixel effect.
// Assumes never move > 1 pixel at a time (easier collision detection then)
// Realistically we throttle velocities to make things look nice
// Accounts for more lively pixel bounces (especially at edge of screen where),
// Slightly randomised velocities, and pixels can collide with each other.
// Not optimal fastest possible code but 256 particles tested at < 1ms overhead
// Note that also imperfections can occur with live animations also on screen affecting
// the environment the pixels are in
void RenderIdleEffect_Bounce()
{
#ifdef DEBUG_MARKS
    Debug::Mark(1, __LINE__, __FILE__, __func__);
#endif

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

            if (millis() > nextSpawnTime)
            {
                // Horizontal bounces,
                // We also (if any left to add) increase the particle count and add one to the end
                // Comes from same spot as parent particle, but has it's own velocities
                // We do this till we reach max particles then we start to de-spawn till none are left and we can start again somewhere else
                if (spawnDirection == 1)
                {

                    if (currentParticleCount < IdleParticleCount)
                    {
                        IdleParticle &newP = particles[currentParticleCount]; // Points to next in line already

                        newP.x = p.x;
                        newP.y = p.y;
                        newP.lastX = p.lastX;
                        newP.lastY = p.lastY;

                        nextSpawnTime = millis() + spawnRate;

                        currentParticleCount++;
                    }
                    else
                    {
                        spawnDirection = -1;
                    }
                }

                // Knock off the last pixel if the time is right
                if (spawnDirection == -1 && i == currentParticleCount - 1)
                {
                    // We haven't drawn the rest of the current pixel yet so we reduce count and skip
                    currentParticleCount--;

                    // Make sure
                    // Display.writePixel(particles[currentParticleCount].lastX, particles[currentParticleCount].lastY, C_BLACK);

                    // If there are no particles left, restart everything from scratch!
                    if (currentParticleCount == 0)
                    {
                        InitIdleEffect();
                        return;
                    }

                    continue; // Skip drawing this particle since we're removing it
                }
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

void RenderIdleEffect_GlobalSwoosh()

{
#ifdef DEBUG_MARKS
    Debug::Mark(1, __LINE__, __FILE__, __func__);
#endif

    // Update swosh time
    swoshTimeV += SWOSH_SPEED * rnd1to5_a;
    swoshTimeH += SWOSH_SPEED * rnd1to5_b;

    // Calculate multipliers (0 to 1 range)
    float sinMultiplier = (sin(swoshTimeV) + 1.0f) * 0.5f;
    float cosMultiplier = (cos(swoshTimeH) + 1.0f) * 0.5f;

    // Serial.printf("Global Swoosh: %f, %f, %f, %f\n", rnd1to5_a, rnd1to5_b, sinMultiplier, cosMultiplier);

    for (int i = 0; i < currentParticleCount; i++)
    {
        IdleParticle &p = particles[i];

        // Erase previous pixel
        Display.writePixel(p.lastX, p.lastY, C_BLACK);

        // Apply swosh multipliers to velocities for this frame's movement
        float vx_swosh = p.vx * sinMultiplier;
        float vy_swosh = p.vy * cosMultiplier;

        float tryXf = p.x + vx_swosh;
        float tryYf = p.y + vy_swosh;

        int xi_try = (int)tryXf;
        int yi_try = (int)tryYf;

        // Keep original velocities for bounce calculation
        float newVx = p.vx;
        float newVy = p.vy;

        // Horizontal collision
        if (xi_try < 0 || xi_try >= SCREEN_WIDTH ||
            Display.getPixel(xi_try, p.lastY) == C_WHITE)
        {
            if (millis() > nextSpawnTime)
            {
                if (spawnDirection == 1)
                {
                    if (currentParticleCount < IdleParticleCount)
                    {
                        IdleParticle &newP = particles[currentParticleCount];

                        newP.x = p.x;
                        newP.y = p.y;
                        newP.lastX = p.lastX;
                        newP.lastY = p.lastY;
                        newP.vx = randVel();
                        newP.vy = randVel();

                        nextSpawnTime = millis() + spawnRate;
                        currentParticleCount++;
                    }
                    else
                    {
                        spawnDirection = -1;
                    }
                }

                if (spawnDirection == -1 && i == currentParticleCount - 1)
                {
                    currentParticleCount--;
                    if (currentParticleCount == 0)
                    {
                        InitIdleEffect();
                        return;
                    }
                    continue;
                }
            }

            // Reverse the base velocity
            newVx = -newVx;

            if (tryXf < 1.0f)
                tryXf += 1.0f;
            else if (tryXf >= SCREEN_WIDTH - 1.0f)
                tryXf -= 1.0f;

            // Apply the swosh multiplier to the bounced velocity
            tryXf += newVx * sinMultiplier;
            xi_try = (int)tryXf;
        }

        // Vertical collision
        if (yi_try < 0 || yi_try >= SCREEN_HEIGHT ||
            Display.getPixel(p.lastX, yi_try) == C_WHITE)
        {
            // Reverse the base velocity
            newVy = -newVy;

            if (tryYf < 1.0f)
                tryYf += 1.0f;
            else if (tryYf >= SCREEN_HEIGHT - 1.0f)
                tryYf -= 1.0f;

            // Apply the swosh multiplier to the bounced velocity
            tryYf += newVy * cosMultiplier;
            yi_try = (int)tryYf;
        }

        int xi_final = xi_try;
        int yi_final = yi_try;

        // Final position check
        if (Display.getPixel(xi_final, yi_final) == C_WHITE)
        {
            // Clash - randomize base velocities
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

        // Store the base velocity back
        p.vx = newVx;
        p.vy = newVy;
        p.x = tryXf;
        p.y = tryYf;

        Display.writePixel(xi_final, yi_final, C_WHITE);
        p.lastX = xi_final;
        p.lastY = yi_final;
    }
}

void RenderIdleEffect_Gravity()
{
#ifdef DEBUG_MARKS
    Debug::Mark(1, __LINE__, __FILE__, __func__);
#endif

    for (int i = 0; i < currentParticleCount; i++)
    {
        IdleParticle &p = particles[i];

        // Erase previous pixel
        Display.writePixel(p.lastX, p.lastY, C_BLACK);

        // Apply gravity to vertical velocity
        p.vy += GRAVITY;

        // Clamp velocities to prevent moving more than 1 pixel per frame
        if (p.vx > MAX_VELOCITY)
            p.vx = MAX_VELOCITY;
        if (p.vx < -MAX_VELOCITY)
            p.vx = -MAX_VELOCITY;
        if (p.vy > MAX_VELOCITY)
            p.vy = MAX_VELOCITY;
        if (p.vy < -MAX_VELOCITY)
            p.vy = -MAX_VELOCITY;

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
            if (millis() > nextSpawnTime)
            {
                if (spawnDirection == 1)
                {
                    if (currentParticleCount < IdleParticleCount)
                    {
                        IdleParticle &newP = particles[currentParticleCount];

                        newP.x = p.x;
                        newP.y = p.y;
                        newP.lastX = p.lastX;
                        newP.lastY = p.lastY;

                        nextSpawnTime = millis() + spawnRate;
                        currentParticleCount++;
                    }
                    else
                    {
                        spawnDirection = -1;
                    }
                }

                if (spawnDirection == -1 && i == currentParticleCount - 1)
                {
                    currentParticleCount--;

                    if (currentParticleCount == 0)
                    {
                        InitIdleEffect();
                        return;
                    }

                    continue;
                }
            }

            newVx = -newVx;

            if (tryXf < 1.0f)
                tryXf += 1.0f;
            else if (tryXf >= SCREEN_WIDTH - 1.0f)
                tryXf -= 1.0f;

            tryXf += newVx;
            xi_try = (int)tryXf;
        }

        // Vertical bounce (including top of screen)
        if (yi_try < 0 || yi_try >= SCREEN_HEIGHT ||
            Display.getPixel(p.lastX, yi_try) == C_WHITE)
        {
            // Check if going off bottom of screen - wrap to top
            if (yi_try >= SCREEN_HEIGHT)
            {
                // Reset to top with random horizontal position
                int newX;
                int attempts = 0;
                do
                {
                    newX = rand() % SCREEN_WIDTH;
                    attempts++;
                } while (Display.getPixel(newX, 0) == C_WHITE && attempts < 100);

                // Reset position to top
                p.x = (float)newX;
                p.y = 0.0f;
                p.lastX = (int)p.x;
                p.lastY = 0;

                // Give it a random downward velocity
                p.vy = 0.1f + (rand() * (1.0f / RAND_MAX)) * 0.2f;
                p.vx = randVel() * 0.5f;

                Display.writePixel((int)p.x, (int)p.y, C_WHITE);
                continue;
            }
            else
            {
                // Normal bounce (top of screen or white pixel)
                newVy = -newVy;

                // Force a 1‑pixel rebound
                if (tryYf < 1.0f)
                    tryYf += 1.0f;
                else if (tryYf >= SCREEN_HEIGHT - 1.0f)
                    tryYf -= 1.0f;

                tryYf += newVy;
                yi_try = (int)tryYf;
            }
        }

        int xi_final = xi_try;
        int yi_final = yi_try;

        // Final new position checks
        if (Display.getPixel(xi_final, yi_final) == C_WHITE)
        {
            // Clash - something white was there already
            if (newVx > 0)
                newVx = -0.1f - (rand() * (1.0f / RAND_MAX)) * 0.2f;
            else
                newVx = 0.1f + (rand() * (1.0f / RAND_MAX)) * 0.2f;

            if (newVy > 0)
                newVy = -0.1f - (rand() * (1.0f / RAND_MAX)) * 0.2f;
            else
                newVy = 0.1f + (rand() * (1.0f / RAND_MAX)) * 0.2f;

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

        Display.writePixel(xi_final, yi_final, C_WHITE);

        p.lastX = xi_final;
        p.lastY = yi_final;
    }
}

void RenderIdleEffect_Rain()
{
#ifdef DEBUG_MARKS
    Debug::Mark(1, __LINE__, __FILE__, __func__);
#endif

    for (int i = 0; i < currentParticleCount; i++)
    {
        IdleParticle &p = particles[i];

        // Erase previous pixel
        Display.writePixel(p.lastX, p.lastY, C_BLACK);

        // Apply gravity
        p.vy += GRAVITY;

        // Clamp velocities
        if (p.vx > MAX_VELOCITY) p.vx = MAX_VELOCITY;
        if (p.vx < -MAX_VELOCITY) p.vx = -MAX_VELOCITY;
        if (p.vy > MAX_VELOCITY) p.vy = MAX_VELOCITY;
        if (p.vy < -MAX_VELOCITY) p.vy = -MAX_VELOCITY;

        float tryXf = p.x + p.vx;
        float tryYf = p.y + p.vy;

        int xi_try = (int)tryXf;
        int yi_try = (int)tryYf;

        float newVx = p.vx;
        float newVy = p.vy;
        
        // Track if this was a downward bounce
        bool wasDownwardBounce = false;

        // Horizontal collision
        if (xi_try < 0 || xi_try >= SCREEN_WIDTH ||
            Display.getPixel(xi_try, p.lastY) == C_WHITE)
        {
            if (millis() > nextSpawnTime)
            {
                if (spawnDirection == 1)
                {
                    if (currentParticleCount < IdleParticleCount)
                    {
                        IdleParticle &newP = particles[currentParticleCount];
                        newP.x = p.x;
                        newP.y = 0;
                        newP.lastX = p.lastX;
                        newP.lastY = 0;
                        newP.vx = randVel();
                        newP.vy = randVel();
                        newP.bounces = 0;  // Initialize bounce counter
                        nextSpawnTime = millis() + spawnRate;
                        currentParticleCount++;
                    }
                    else
                    {
                        spawnDirection = -1;
                    }
                }

                if (spawnDirection == -1 && i == currentParticleCount - 1)
                {
                    currentParticleCount--;
                    if (currentParticleCount == 0)
                    {
                        InitIdleEffect();
                        return;
                    }
                    continue;
                }
            }

            newVx = -newVx;
            //p.bounces++;

            if (tryXf < 1.0f)
                tryXf += 1.0f;
            else if (tryXf >= SCREEN_WIDTH - 1.0f)
                tryXf -= 1.0f;

            tryXf += newVx;
            xi_try = (int)tryXf;
        }

        // Vertical collision (with bottom wrap)
        if (yi_try < 0 || yi_try >= SCREEN_HEIGHT ||
            Display.getPixel(p.lastX, yi_try) == C_WHITE)
        {
            // Wrap at bottom
            if (yi_try >= SCREEN_HEIGHT)
            {
                // Reset to top of screen
                int newX;
                int attempts = 0;
                do
                {
                    newX = rand() % SCREEN_WIDTH;
                    attempts++;
                } while (Display.getPixel(newX, 0) == C_WHITE && attempts < 100);

                p.x = (float)newX;
                p.y = 0.0f;
                p.lastX = (int)p.x;
                p.lastY = 0;
                p.vy = 0.1f + (rand() * (1.0f / RAND_MAX)) * 0.2f;
                p.vx = randVel() * 0.5f;
                p.bounces = 0;  // Reset bounce counter

                Display.writePixel((int)p.x, (int)p.y, C_WHITE);
                continue;
            }
            else
            {
                // Check if this is a downward bounce (particle was moving down)
                // p.vy > 0 means moving downward before the bounce
                if (p.vy > 0)
                {
                    wasDownwardBounce = true;
                    p.bounces++;  // Increment bounce counter
                    
                    // If too many downward bounces, reset to top
                    if (p.bounces > rnd1to5_a) // IDLE_MAX_RAIN_BOUNCES
                    {
                        // Reset to top of screen
                        int newX;
                        int attempts = 0;
                        do
                        {
                            newX = rand() % SCREEN_WIDTH;
                            attempts++;
                        } while (Display.getPixel(newX, 0) == C_WHITE && attempts < 100);

                        p.x = (float)newX;
                        p.y = 0.0f;
                        p.lastX = (int)p.x;
                        p.lastY = 0;
                        p.vy = 0.1f + (rand() * (1.0f / RAND_MAX)) * 0.2f;
                        p.vx = randVel() * 0.5f;
                        p.bounces = 0;  // Reset bounce counter

                        Display.writePixel((int)p.x, (int)p.y, C_WHITE);
                        continue;
                    }
                }

                // Normal bounce (top or white pixel)
                newVy = -(newVy * 0.3f) - 0.15f;
                newVx = randVel() * 2.0f;

                if (tryYf < 1.0f)
                    tryYf += 1.0f;
                else if (tryYf >= SCREEN_HEIGHT - 1.0f)
                    tryYf -= 1.0f;

                tryYf += newVy;
                yi_try = (int)tryYf;
            }
        }

        int xi_final = xi_try;
        int yi_final = yi_try;

        // Final position check
        if (Display.getPixel(xi_final, yi_final) == C_WHITE)
        {
            if (newVx > 0)
                newVx = -0.1f - (rand() * (1.0f / RAND_MAX)) * 0.2f;
            else
                newVx = 0.1f + (rand() * (1.0f / RAND_MAX)) * 0.2f;
            if (newVy > 0)
                newVy = -0.1f - (rand() * (1.0f / RAND_MAX)) * 0.2f;
            else
                newVy = 0.1f + (rand() * (1.0f / RAND_MAX)) * 0.2f;

            tryXf = p.x;
            tryYf = p.y;
            xi_final = p.lastX;
            yi_final = p.lastY;
        }

        p.vx = newVx;
        p.vy = newVy;
        p.x = tryXf;
        p.y = tryYf;

        Display.writePixel(xi_final, yi_final, C_WHITE);
        p.lastX = xi_final;
        p.lastY = yi_final;
    }
}

void RenderIdleEffect_UnOrbit()
{
#ifdef DEBUG_MARKS
    Debug::Mark(1, __LINE__, __FILE__, __func__);
#endif

    const float MAX_DIST = sqrt(CENTER_X * CENTER_X + CENTER_Y * CENTER_Y);

    for (int i = 0; i < currentParticleCount; i++)
    {
        IdleParticle &p = particles[i];

        // Erase previous pixel - ALWAYS erase (like other effects)
        Display.writePixel(p.lastX, p.lastY, C_BLACK);

        // Calculate distance from center
        float dx = p.x - CENTER_X;
        float dy = p.y - CENTER_Y;
        float dist = sqrt(dx * dx + dy * dy);

        // Normalize direction (radial direction from center)
        float dirX = 0.0f, dirY = 0.0f;
        if (dist > 0.001f)
        {
            dirX = dx / dist;
            dirY = dy / dist;
        }

        // Determine if we should push outward or inward
        float distPercent = dist / MAX_DIST;

        float force = 0.0f;
        if (distPercent > 0.65f)
        {
            force = -UNORBIT_FORCE * 1.5f;
            p.outward = false;
        }
        else if (distPercent < 0.35f)
        {
            force = UNORBIT_FORCE * 1.5f;
            p.outward = true;
        }
        else
        {
            force = p.outward ? UNORBIT_FORCE * 0.3f : -UNORBIT_FORCE * 0.3f;
        }

        // Apply radial force
        p.vx += dirX * force;
        p.vy += dirY * force;

        // Add tangential velocity for orbiting
        float tangentX = -dirY;
        float tangentY = dirX;
        float orbitSpeed = 0.015f * (1.0f + distPercent);
        p.vx += tangentX * orbitSpeed;
        p.vy += tangentY * orbitSpeed;

        // Small random perturbation
        p.vx += ((rand() * (1.0f / RAND_MAX)) - 0.5f) * 0.01f;
        p.vy += ((rand() * (1.0f / RAND_MAX)) - 0.5f) * 0.01f;

        // Clamp velocities
        if (p.vx > MAX_VELOCITY) p.vx = MAX_VELOCITY;
        if (p.vx < -MAX_VELOCITY) p.vx = -MAX_VELOCITY;
        if (p.vy > MAX_VELOCITY) p.vy = MAX_VELOCITY;
        if (p.vy < -MAX_VELOCITY) p.vy = -MAX_VELOCITY;

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
            if (millis() > nextSpawnTime)
            {
                if (spawnDirection == 1)
                {
                    if (currentParticleCount < IdleParticleCount)
                    {
                        IdleParticle &newP = particles[currentParticleCount];
                        float angle = (rand() * (1.0f / RAND_MAX)) * 2.0f * PI;
                        float radius = 3.0f + (rand() * (1.0f / RAND_MAX)) * 5.0f;
                        newP.x = CENTER_X + cos(angle) * radius;
                        newP.y = CENTER_Y + sin(angle) * radius;
                        
                        // Ensure new particle starts on a black pixel
                        if (Display.getPixel((int)newP.x, (int)newP.y) == C_WHITE)
                        {
                            // Find nearest black pixel
                            bool found = false;
                            for (int dy = -2; dy <= 2 && !found; dy++)
                            {
                                for (int dx = -2; dx <= 2 && !found; dx++)
                                {
                                    int nx = (int)newP.x + dx;
                                    int ny = (int)newP.y + dy;
                                    if (nx >= 0 && nx < SCREEN_WIDTH && ny >= 0 && ny < SCREEN_HEIGHT &&
                                        Display.getPixel(nx, ny) == C_BLACK)
                                    {
                                        newP.x = (float)nx;
                                        newP.y = (float)ny;
                                        found = true;
                                    }
                                }
                            }
                            if (!found)
                            {
                                newP.x = CENTER_X;
                                newP.y = CENTER_Y;
                            }
                        }
                        newP.lastX = (int)newP.x;
                        newP.lastY = (int)newP.y;
                        newP.vx = randVel() * 0.5f;
                        newP.vy = randVel() * 0.5f;
                        newP.outward = true;
                        nextSpawnTime = millis() + spawnRate;
                        currentParticleCount++;
                    }
                    else
                    {
                        spawnDirection = -1;
                    }
                }

                if (spawnDirection == -1 && i == currentParticleCount - 1)
                {
                    currentParticleCount--;
                    if (currentParticleCount == 0)
                    {
                        InitIdleEffect();
                        return;
                    }
                    continue;
                }
            }

            newVx = -newVx;

            if (tryXf < 1.0f)
                tryXf += 1.0f;
            else if (tryXf >= SCREEN_WIDTH - 1.0f)
                tryXf -= 1.0f;

            tryXf += newVx;
            xi_try = (int)tryXf;

            p.outward = !p.outward;
        }

        // Vertical collision
        if (yi_try < 0 || yi_try >= SCREEN_HEIGHT ||
            Display.getPixel(p.lastX, yi_try) == C_WHITE)
        {
            newVy = -newVy;

            if (tryYf < 1.0f)
                tryYf += 1.0f;
            else if (tryYf >= SCREEN_HEIGHT - 1.0f)
                tryYf -= 1.0f;

            tryYf += newVy;
            yi_try = (int)tryYf;

            p.outward = !p.outward;
        }

        int xi_final = xi_try;
        int yi_final = yi_try;

        // Final position check - SAME AS OTHER EFFECTS
        if (Display.getPixel(xi_final, yi_final) == C_WHITE)
        {
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

        // Commit final values - SAME AS OTHER EFFECTS
        p.vx = newVx;
        p.vy = newVy;
        p.x = tryXf;
        p.y = tryYf;

        // Draw - SAME AS OTHER EFFECTS
        Display.writePixel(xi_final, yi_final, C_WHITE);
        p.lastX = xi_final;
        p.lastY = yi_final;
    }
}

void StopIdleEffect()
{
#ifdef DEBUG_MARKS
    Debug::Mark(1, __LINE__, __FILE__, __func__);
#endif

    for (int i = 0; i < currentParticleCount; i++)
        Display.writePixel((int)particles[i].x, (int)particles[i].y, C_BLACK);
}