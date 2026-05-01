#include <GL/glut.h>
#include <cmath>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>
#include <algorithm>

// ============= CAR PROPERTIES =============
struct CarState
{
    float posX = 0.0f, posY = 0.0f, posZ = 0.0f;
    float speed = 0.0f;
    float rotation = 0.0f;
    float wheelRotation = 0.0f;
    float steeringAngle = 0.0f;
    float acceleration = 0.0f;
} car;

// ============= CAR PHYSICS =============
const float MAX_SPEED = 4.5f;
const float FRICTION = 0.985f;
const float BRAKE_POWER = 0.85f;
const float ACCEL_FORWARD = 0.012f;
const float ACCEL_REVERSE = 0.008f;

// ============= CAMERA PROPERTIES =============
struct Camera
{
    float x = 0.0f, y = 5.0f, z = 15.0f;
    float angleX = 0.0f, angleY = 0.0f;
    int mode = 1; // 1=Follow, 2=Top, 3=FirstPerson, 4=Free
} camera;

// ============= ENVIRONMENT =============
const float ROAD_WIDTH = 20.0f;
const float ROAD_LENGTH = 1000.0f;

std::vector<float> treePositions;
std::vector<float> coinPositions;
std::vector<float> obstaclePositions;
std::vector<int> obstacleTypes;

// ============= GAME STATE =============
struct GameState
{
    int score = 0;
    int coinsCollected = 0;
    float time = 0.0f;
    bool isOver = false;
    bool isWon = false;
    bool showInstructions = true;
} game;

// ============= LIGHTING & ATMOSPHERE ============
bool lightingEnabled = true;
bool nightMode = false;
bool fogEnabled = false;

// Automatic day/night cycle (toggle every 15 seconds)
float dayNightTimer = 0.0f;
const float DAY_NIGHT_DURATION = 15.0f; // seconds to toggle day/night

// ============= SEASONS & WEATHER =============
enum Season
{
    SUMMER,
    RAINY_SEASON,
    AUTUMN,
    LATE_AUTUMN,
    WINTER,
    SPRING,
    SEASON_COUNT
};
Season currentSeason = SUMMER;
float seasonTimer = 0.0f;
// Per your request: you can change duration; default kept
const float SEASON_DURATION = 30.0f;

bool isRaining = false;
bool isSnowing = false;
std::vector<float> rainDrops;
std::vector<float> snowFlakes;
std::vector<float> fallingLeaves;
std::vector<float> groundLeaves;
std::vector<float> groundSnow;
float snowAccumulationLevel = 0.0f;

// ============= COLORS =============
struct SeasonColors
{
    float sky[3];
    float grass[3];
    float tree[3];
} seasonColors[SEASON_COUNT] = {
    // SUMMER
    {{0.53f, 0.81f, 0.98f}, {0.1f, 0.5f, 0.1f}, {0.0f, 0.6f, 0.0f}},
    // RAINY_SEASON (darker sky)
    {{0.35f, 0.45f, 0.55f}, {0.12f, 0.45f, 0.12f}, {0.05f, 0.45f, 0.06f}},
    // AUTUMN
    {{0.7f, 0.7f, 0.9f}, {0.65f, 0.45f, 0.15f}, {0.85f, 0.45f, 0.0f}},
    // LATE_AUTUMN (even grayer sky, more bare)
    {{0.65f, 0.62f, 0.78f}, {0.55f, 0.38f, 0.12f}, {0.75f, 0.38f, 0.05f}},
    // WINTER
    {{0.85f, 0.85f, 1.0f}, {0.95f, 0.95f, 1.0f}, {0.7f, 0.7f, 0.75f}},
    // SPRING
    {{0.6f, 0.85f, 1.0f}, {0.15f, 0.7f, 0.15f}, {0.0f, 0.85f, 0.1f}}};

// other colors
float carBodyColor[3] = {0.9f, 0.1f, 0.1f};
float wheelColor[3] = {0.1f, 0.1f, 0.1f};
const float skyColorNight[3] = {0.05f, 0.05f, 0.2f};

// ============= IMPROVED DISTRIBUTION FUNCTIONS =============

bool isPositionValid(float x, float z, const std::vector<float>& existingPositions, float minDistance) {
    for (size_t i = 0; i < existingPositions.size(); i += 2) {
        float dx = x - existingPositions[i];
        float dz = z - existingPositions[i + 1];
        if (dx * dx + dz * dz < minDistance * minDistance) {
            return false;
        }
    }
    return true;
}

bool isRoadPositionValid(float x, float z, float roadHalfWidth, float margin) {
    return fabs(x) > roadHalfWidth + margin;
}

void initializeEnvironment()
{
    srand(static_cast<unsigned int>(time(0)));

    // Clear all existing objects
    treePositions.clear();
    coinPositions.clear();
    obstaclePositions.clear();
    obstacleTypes.clear();

    // ============= TREE DISTRIBUTION =============
    const float TREE_ROAD_MARGIN = 3.0f;
    const float MIN_TREE_DISTANCE = 4.0f;
    const int TREES_PER_SECTION = 3;
    
    // Roadside trees - more dense near road
    for (float z = -ROAD_LENGTH/2; z <= ROAD_LENGTH/2; z += 8.0f) {
        // Left side trees
        for (int i = 0; i < TREES_PER_SECTION; i++) {
            float x = -(ROAD_WIDTH/2 + TREE_ROAD_MARGIN + (rand() % 150) * 0.1f);
            float treeZ = z + (rand() % 100 - 50) * 0.1f;
            
            if (isPositionValid(x, treeZ, treePositions, MIN_TREE_DISTANCE)) {
                treePositions.push_back(x);
                treePositions.push_back(treeZ);
            }
        }
        
        // Right side trees
        for (int i = 0; i < TREES_PER_SECTION; i++) {
            float x = ROAD_WIDTH/2 + TREE_ROAD_MARGIN + (rand() % 150) * 0.1f;
            float treeZ = z + (rand() % 100 - 50) * 0.1f;
            
            if (isPositionValid(x, treeZ, treePositions, MIN_TREE_DISTANCE)) {
                treePositions.push_back(x);
                treePositions.push_back(treeZ);
            }
        }
    }
    
    // Background trees - sparser distribution
    for (int i = 0; i < 200; i++) {
        int attempts = 0;
        bool validPosition = false;
        float x, z;
        
        while (!validPosition && attempts < 20) {
            x = (rand() % 1200 - 600) * 0.1f;
            z = (rand() % static_cast<int>(ROAD_LENGTH * 0.8f)) - ROAD_LENGTH * 0.4f;
            
            // Must be outside road area with good margin
            if (fabs(x) > ROAD_WIDTH/2 + 8.0f) {
                if (isPositionValid(x, z, treePositions, MIN_TREE_DISTANCE + 2.0f)) {
                    validPosition = true;
                }
            }
            attempts++;
        }
        
        if (validPosition) {
            treePositions.push_back(x);
            treePositions.push_back(z);
        }
    }

    // ============= COIN DISTRIBUTION =============
    const float COIN_ROAD_MARGIN = 1.5f;
    const float MIN_COIN_DISTANCE = 3.0f;
    const int COIN_COUNT = 50;
    
    for (int i = 0; i < COIN_COUNT; i++) {
        int attempts = 0;
        bool validPosition = false;
        float x, z;
        
        while (!validPosition && attempts < 30) {
            // Coins should be on or very near the road
            x = (rand() % static_cast<int>(ROAD_WIDTH - COIN_ROAD_MARGIN * 2)) - 
                (ROAD_WIDTH/2 - COIN_ROAD_MARGIN);
            z = (rand() % static_cast<int>(ROAD_LENGTH - 200)) - (ROAD_LENGTH/2 - 100);
            
            // Avoid placing coins too close to road edges
            if (fabs(x) < ROAD_WIDTH/2 - 1.0f) {
                if (isPositionValid(x, z, coinPositions, MIN_COIN_DISTANCE)) {
                    validPosition = true;
                }
            }
            attempts++;
        }
        
        if (validPosition) {
            coinPositions.push_back(x);
            coinPositions.push_back(z);
        }
    }

    // ============= OBSTACLE DISTRIBUTION =============
    const float OBSTACLE_ROAD_MARGIN = 2.0f;
    const float MIN_OBSTACLE_DISTANCE = 5.0f;
    const int OBSTACLE_COUNT = 30;
    
    // Define obstacle zones (avoid clustering)
    struct ObstacleZone {
        float minZ, maxZ;
        int maxObstacles;
    };
    
    std::vector<ObstacleZone> zones = {
        {-ROAD_LENGTH/2, -ROAD_LENGTH/4, 8},
        {-ROAD_LENGTH/4, 0, 8},
        {0, ROAD_LENGTH/4, 7},
        {ROAD_LENGTH/4, ROAD_LENGTH/2, 7}
    };
    
    for (const auto& zone : zones) {
        int obstaclesInZone = 0;
        
        for (int i = 0; i < zone.maxObstacles * 3; i++) { // Give up to 3 attempts per slot
            if (obstaclesInZone >= zone.maxObstacles) break;
            
            int attempts = 0;
            bool validPosition = false;
            float x, z;
            
            while (!validPosition && attempts < 15) {
                x = (rand() % static_cast<int>(ROAD_WIDTH - OBSTACLE_ROAD_MARGIN * 2)) - 
                    (ROAD_WIDTH/2 - OBSTACLE_ROAD_MARGIN);
                z = zone.minZ + (rand() % static_cast<int>(zone.maxZ - zone.minZ));
                
                // Avoid center lane for obstacles
                if (fabs(x) > 2.0f) {
                    if (isPositionValid(x, z, obstaclePositions, MIN_OBSTACLE_DISTANCE) &&
                        isPositionValid(x, z, coinPositions, 4.0f)) {
                        validPosition = true;
                    }
                }
                attempts++;
            }
            
            if (validPosition) {
                obstaclePositions.push_back(x);
                obstaclePositions.push_back(z);
                obstacleTypes.push_back(rand() % 6);
                obstaclesInZone++;
            }
        }
    }
    
    // Fill remaining obstacles if any zone didn't use all slots
    int currentObstacleCount = obstaclePositions.size() / 2;
    while (currentObstacleCount < OBSTACLE_COUNT) {
        int attempts = 0;
        bool validPosition = false;
        float x, z;
        
        while (!validPosition && attempts < 20) {
            x = (rand() % static_cast<int>(ROAD_WIDTH - OBSTACLE_ROAD_MARGIN * 2)) - 
                (ROAD_WIDTH/2 - OBSTACLE_ROAD_MARGIN);
            z = (rand() % static_cast<int>(ROAD_LENGTH - 200)) - (ROAD_LENGTH/2 - 100);
            
            if (fabs(x) > 2.0f) {
                if (isPositionValid(x, z, obstaclePositions, MIN_OBSTACLE_DISTANCE) &&
                    isPositionValid(x, z, coinPositions, 4.0f)) {
                    validPosition = true;
                }
            }
            attempts++;
        }
        
        if (validPosition) {
            obstaclePositions.push_back(x);
            obstaclePositions.push_back(z);
            obstacleTypes.push_back(rand() % 6);
            currentObstacleCount++;
        } else {
            break; // Can't find valid position, stop trying
        }
    }

    printf("Environment initialized:\n");
    printf("- Trees: %zu\n", treePositions.size() / 2);
    printf("- Coins: %zu\n", coinPositions.size() / 2);
    printf("- Obstacles: %zu\n", obstaclePositions.size() / 2);
}

void initWeatherEffects()
{
    rainDrops.clear();
    snowFlakes.clear();

    if (isRaining)
    {
        for (int i = 0; i < 800; i++)
        {
            rainDrops.push_back((rand() % 300 - 150) * 1.0f);
            rainDrops.push_back((rand() % 60) + 5.0f);
            rainDrops.push_back((rand() % 300 - 150) * 1.0f);
            rainDrops.push_back((rand() % 100) * 0.01f); // velocity variation
        }
    }

    if (isSnowing)
    {
        for (int i = 0; i < 500; i++)
        {
            snowFlakes.push_back((rand() % 300 - 150) * 1.0f);
            snowFlakes.push_back((rand() % 40) + 5.0f);
            snowFlakes.push_back((rand() % 300 - 150) * 1.0f);
            snowFlakes.push_back((rand() % 100 - 50) * 0.005f); // x drift
            snowFlakes.push_back((rand() % 100 - 50) * 0.005f); // z drift
        }
    }
}

// ============= DRAWING FUNCTIONS =============
// drawSkybox unchanged logic except uses extended seasons (already wired by colors)
void drawSkybox()
{
    glDisable(GL_LIGHTING);
    glPushMatrix();

    // Sky colors: make them paler by blending slightly toward white
    float topR, topG, topB;
    float botR, botG, botB;

    if (nightMode)
    {
        // Night remains dark but a bit softer
        topR = skyColorNight[0] * 0.95f + 0.05f;
        topG = skyColorNight[1] * 0.95f + 0.05f;
        topB = skyColorNight[2] * 0.95f + 0.05f;

        botR = topR * 0.65f + 0.1f;
        botG = topG * 0.65f + 0.1f;
        botB = topB * 0.65f + 0.1f;
    }
    else
    {
        float *sky = seasonColors[currentSeason].sky;
        // Blend a bit toward white to make it paler
        topR = sky[0] * 0.82f + 0.18f;
        topG = sky[1] * 0.82f + 0.18f;
        topB = sky[2] * 0.82f + 0.18f;

        botR = (sky[0] * 0.65f + 0.25f);
        botG = (sky[1] * 0.65f + 0.25f);
        botB = (sky[2] * 0.7f + 0.2f);
    }

    // Draw top (ceiling)
    glBegin(GL_QUADS);
    glColor3f(topR, topG, topB);
    glVertex3f(-200, 100, -200);
    glVertex3f(200, 100, -200);
    glVertex3f(200, 100, 200);
    glVertex3f(-200, 100, 200);
    glEnd();

    // Draw side walls with softer gradient
    auto DrawWall = [&](float x1, float y1, float z1,
                        float x2, float y2, float z2,
                        float x3, float y3, float z3,
                        float x4, float y4, float z4)
    {
        glBegin(GL_QUADS);
        glColor3f(botR, botG, botB); // bottom (slightly brighter than before)
        glVertex3f(x1, y1, z1);
        glVertex3f(x2, y2, z2);
        glColor3f(topR, topG, topB); // top
        glVertex3f(x3, y3, z3);
        glVertex3f(x4, y4, z4);
        glEnd();
    };

    // North wall
    DrawWall(-200, -20, -200, 200, -20, -200, 200, 100, -200, -200, 100, -200);
    // South wall
    DrawWall(-200, -20, 200, 200, -20, 200, 200, 100, 200, -200, 100, 200);
    // East wall
    DrawWall(200, -20, -200, 200, -20, 200, 200, 100, 200, 200, 100, -200);
    // West wall
    DrawWall(-200, -20, -200, -200, -20, 200, -200, 100, 200, -200, 100, -200);

    // ----- Summer Clouds: much lighter & subtler -----
    if (currentSeason == SUMMER)
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_LIGHTING);

        unsigned int seed = 42424243u;
        auto nextF = [&](float a = 0.0f, float b = 1.0f) -> float
        {
            seed ^= seed << 13;
            seed ^= seed >> 17;
            seed ^= seed << 5;
            return a + ((seed % 10000) / 10000.0f) * (b - a);
        };

        // Reduced density for airy clouds
        const int clusterCount = 12;   // fewer clusters
        const int puffPerCluster = 4;  // fewer puffs per cluster
        const float cloudYMin = 68.0f; // slightly higher, further back
        const float cloudYMax = 86.0f;

        for (int c = 0; c < clusterCount; ++c)
        {
            float cx = nextF(-150.0f, 150.0f);
            float cz = nextF(-150.0f, 150.0f);
            float cy = nextF(cloudYMin, cloudYMax);
            float clusterScale = nextF(6.0f, 14.0f);

            for (int p = 0; p < puffPerCluster; ++p)
            {
                float ox = nextF(-clusterScale * 0.6f, clusterScale * 0.6f);
                float oz = nextF(-clusterScale * 0.6f, clusterScale * 0.6f);
                float oy = nextF(-clusterScale * 0.12f, clusterScale * 0.12f);
                float puffScale = nextF(clusterScale * 0.5f, clusterScale * 0.9f);

                // MUCH lighter alpha and near-white shade
                float alpha = nextF(0.18f, 0.36f);     // subtle
                float whiteShade = nextF(0.94f, 1.0f); // very white

                glColor4f(whiteShade, whiteShade, whiteShade, alpha);

                glPushMatrix();
                glTranslatef(cx + ox, cy + oy, cz + oz);
                glScalef(puffScale * 0.9f, puffScale * 0.35f, puffScale * 0.9f);
                glutSolidSphere(1.0f, 10, 8);
                glPopMatrix();
            }

            // fewer edge puffs
            if (nextF() < 0.35f)
            {
                int wall = static_cast<int>(nextF(0.0f, 4.0f));
                float wy = nextF(22.0f, 72.0f);
                float tx = 0.0f, tz = 0.0f;
                switch (wall)
                {
                case 0:
                    tx = nextF(-150.0f, 150.0f);
                    tz = -200.0f + 1.0f;
                    break;
                case 1:
                    tx = nextF(-150.0f, 150.0f);
                    tz = 200.0f - 1.0f;
                    break;
                case 2:
                    tx = 200.0f - 1.0f;
                    tz = nextF(-150.0f, 150.0f);
                    break;
                default:
                    tx = -200.0f + 1.0f;
                    tz = nextF(-150.0f, 150.0f);
                    break;
                }
                for (int p = 0; p < 2; ++p)
                {
                    float ox = nextF(-5.0f, 5.0f);
                    float oz = nextF(-5.0f, 5.0f);
                    float puffScale = nextF(5.0f, 9.0f);
                    float alpha = nextF(0.16f, 0.32f);
                    float whiteShade = nextF(0.94f, 1.0f);

                    glColor4f(whiteShade, whiteShade, whiteShade, alpha);
                    glPushMatrix();
                    glTranslatef(tx + ox, wy + nextF(-1.5f, 1.5f), tz + oz);
                    glScalef(puffScale * 0.9f, puffScale * 0.35f, puffScale * 0.9f);
                    glutSolidSphere(1.0f, 10, 8);
                    glPopMatrix();
                }
            }
        }

        glDisable(GL_BLEND);
    }

    glPopMatrix();
    if (lightingEnabled)
        glEnable(GL_LIGHTING);
}

void drawSunMoon()
{
    // Draw sun/moon in front of the car; draw stars at night (not in winter).
    glDisable(GL_LIGHTING);

    float rad = car.rotation * 3.14159265f / 180.0f;
    float distanceAhead = 80.0f; // tweak if you want closer/farther
    float sx = car.posX + sin(rad) * distanceAhead;
    float sz = car.posZ + cos(rad) * distanceAhead;
    float sy = 35.0f; // height above scene

    // --- SUN (day only) ---
    if (!nightMode)
    {
        float glowR = 1.0f, glowG = 0.95f, glowB = 0.7f;
        if (isRaining)
        {
            glowR = 0.72f;
            glowG = 0.78f;
            glowB = 0.86f;
        }
        else if (isSnowing || currentSeason == WINTER)
        {
            glowR = 0.94f;
            glowG = 0.97f;
            glowB = 1.0f;
        }
        else if (currentSeason == AUTUMN || currentSeason == LATE_AUTUMN)
        {
            glowR = 1.0f;
            glowG = 0.82f;
            glowB = 0.52f;
        }

        glPushMatrix();
        glTranslatef(sx, sy, sz);

        // soft glow
        glColor4f(glowR, glowG, glowB, 0.35f);
        glutSolidSphere(6.0f, 20, 20);

        // sun core
        glColor3f(1.0f, 0.98f, 0.85f);
        glutSolidSphere(3.8f, 24, 24);
        glPopMatrix();
    }

    // --- MOON (night only) ---
    if (nightMode)
    {
        glPushMatrix();
        glTranslatef(sx - 8.0f, sy + 4.0f, sz - 6.0f);

        // moon glow
        glColor4f(0.95f, 0.95f, 0.9f, 0.28f);
        glutSolidSphere(4.8f, 20, 20);

        // moon core
        glColor3f(0.95f, 0.95f, 0.86f);
        glutSolidSphere(2.6f, 22, 22);
        glPopMatrix();
    }

    // --- STARS (night, but NOT in winter) ---
    if (nightMode && currentSeason != WINTER)
    {
        glColor3f(1.0f, 1.0f, 1.0f);
        glPointSize(2.0f);
        glBegin(GL_POINTS);

        unsigned int seed = 123456789u;
        for (int i = 0; i < 400; ++i)
        {
            seed ^= seed << 13;
            seed ^= seed >> 17;
            seed ^= seed << 5;
            float sxOff = ((seed % 1000) / 1000.0f) * 600.0f - 300.0f; // -300..300

            seed ^= seed << 13;
            seed ^= seed >> 17;
            seed ^= seed << 5;
            float syOff = ((seed % 1000) / 1000.0f) * 80.0f + 10.0f; // 10..90

            seed ^= seed << 13;
            seed ^= seed >> 17;
            seed ^= seed << 5;
            float szOff = ((seed % 1000) / 1000.0f) * 400.0f - 200.0f; // -200..200

            glVertex3f(car.posX + sxOff, syOff, car.posZ + szOff);
        }
        glEnd();
    }

    if (lightingEnabled)
        glEnable(GL_LIGHTING);
}

void updateWeatherEffects()
{
    if (isRaining)
    {
        for (size_t i = 0; i < rainDrops.size(); i += 4)
        {
            float velocity = 0.6f + rainDrops[i + 3];
            rainDrops[i + 1] -= velocity;

            if (rainDrops[i + 1] < -1.0f)
            {
                rainDrops[i + 1] = 60.0f;
                rainDrops[i] = car.posX + (rand() % 200 - 100);
                rainDrops[i + 2] = car.posZ + (rand() % 200 - 100);
            }
        }
    }

    if (isSnowing)
    {
        for (size_t i = 0; i < snowFlakes.size(); i += 5)
        {
            snowFlakes[i + 1] -= 0.12f;
            snowFlakes[i] += snowFlakes[i + 3];
            snowFlakes[i + 2] += snowFlakes[i + 4];

            // Gentle drift
            snowFlakes[i + 3] += (rand() % 100 - 50) * 0.0001f;
            snowFlakes[i + 4] += (rand() % 100 - 50) * 0.0001f;

            // Keep drift in bounds
            snowFlakes[i + 3] = std::max(-0.05f, std::min(0.05f, snowFlakes[i + 3]));
            snowFlakes[i + 4] = std::max(-0.05f, std::min(0.05f, snowFlakes[i + 4]));

            if (snowFlakes[i + 1] < -1.0f)
            {
                snowFlakes[i + 1] = 40.0f;
                snowFlakes[i] = car.posX + (rand() % 200 - 100);
                snowFlakes[i + 2] = car.posZ + (rand() % 200 - 100);

                // Add to ground snow
                if (groundSnow.size() < 300)
                {
                    groundSnow.push_back(snowFlakes[i]);
                    groundSnow.push_back(-0.88f + snowAccumulationLevel);
                    groundSnow.push_back(snowFlakes[i + 2]);
                    groundSnow.push_back((rand() % 40 + 10) * 0.01f);
                }
            }
        }
    }
}

void drawWeatherEffects()
{
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);

    if (isRaining)
    {
        glColor4f(0.7f, 0.7f, 0.9f, 0.5f);
        glLineWidth(1.5f);
        glBegin(GL_LINES);
        for (size_t i = 0; i < rainDrops.size(); i += 4)
        {
            glVertex3f(rainDrops[i], rainDrops[i + 1], rainDrops[i + 2]);
            glVertex3f(rainDrops[i], rainDrops[i + 1] - 2.5f, rainDrops[i + 2]);
        }
        glEnd();
    }

    if (isSnowing)
    {
        glColor4f(1.0f, 1.0f, 1.0f, 0.9f);
        glPointSize(3.0f);
        glBegin(GL_POINTS);
        for (size_t i = 0; i < snowFlakes.size(); i += 5)
        {
            glVertex3f(snowFlakes[i], snowFlakes[i + 1], snowFlakes[i + 2]);
        }
        glEnd();

        // Ground snow
        if (snowAccumulationLevel > 0.0f)
        {
            glColor4f(1.0f, 1.0f, 1.0f, 0.8f);
            for (size_t i = 0; i < groundSnow.size(); i += 4)
            {
                glPushMatrix();
                glTranslatef(groundSnow[i], groundSnow[i + 1], groundSnow[i + 2]);
                glutSolidSphere(groundSnow[i + 3], 6, 6);
                glPopMatrix();
            }
        }
    }

    if (lightingEnabled)
        glEnable(GL_LIGHTING);
}

void updateFallingLeaves(float dt)
{
    if (currentSeason == AUTUMN || currentSeason == LATE_AUTUMN)
    {
        static float leafTimer = 0.0f;
        leafTimer += dt;

        // heavier in LATE_AUTUMN: spawn more frequently and more leaves
        float spawnInterval = (currentSeason == LATE_AUTUMN) ? 0.08f : 0.15f;
        int spawnCount = (currentSeason == LATE_AUTUMN) ? 12 : 5;

        if (leafTimer > spawnInterval && fallingLeaves.size() < 150 * 7)
        {
            for (int i = 0; i < spawnCount; i++)
            {
                int treeIdx = (rand() % (treePositions.size() / 2)) * 2;
                float tx = treePositions[treeIdx];
                float tz = treePositions[treeIdx + 1];

                fallingLeaves.push_back(tx + (rand() % 60 - 30) * 0.05f);
                fallingLeaves.push_back(3.5f + (rand() % 50) * 0.05f);
                fallingLeaves.push_back(tz + (rand() % 60 - 30) * 0.05f);
                fallingLeaves.push_back((rand() % 80 - 40) * 0.01f);
                // fall speed slightly faster in LATE_AUTUMN
                fallingLeaves.push_back((currentSeason == LATE_AUTUMN) ? -0.12f - (rand() % 40) * 0.001f : -0.08f - (rand() % 40) * 0.001f);
                fallingLeaves.push_back((rand() % 80 - 40) * 0.01f);
                fallingLeaves.push_back(rand() % 360);
            }
            leafTimer = 0.0f;
        }

        for (size_t i = 0; i < fallingLeaves.size(); i += 7)
        {
            fallingLeaves[i] += fallingLeaves[i + 3];
            fallingLeaves[i + 1] += fallingLeaves[i + 4];
            fallingLeaves[i + 2] += fallingLeaves[i + 5];
            fallingLeaves[i + 6] += 8.0f;

            fallingLeaves[i + 3] += (rand() % 100 - 50) * 0.00015f;
            fallingLeaves[i + 5] += (rand() % 100 - 50) * 0.00015f;

            if (fallingLeaves[i + 1] <= -0.85f)
            {
                if (groundLeaves.size() < 500 * 4)
                {
                    groundLeaves.push_back(fallingLeaves[i]);
                    groundLeaves.push_back(-0.88f);
                    groundLeaves.push_back(fallingLeaves[i + 2]);
                    groundLeaves.push_back(rand() % 360);
                }
                fallingLeaves.erase(fallingLeaves.begin() + i, fallingLeaves.begin() + i + 7);
                i -= 7;
            }
        }
    }
    else
    {
        fallingLeaves.clear();
        if (currentSeason != AUTUMN && currentSeason != LATE_AUTUMN)
            groundLeaves.clear();
    }
}

void drawFallingLeaves()
{
    if (currentSeason == AUTUMN || currentSeason == LATE_AUTUMN)
    {
        glDisable(GL_LIGHTING);

        float leafColors[][3] = {
            {0.9f, 0.5f, 0.1f},
            {0.8f, 0.3f, 0.0f},
            {0.95f, 0.7f, 0.2f},
            {0.7f, 0.4f, 0.1f}};

        for (size_t i = 0; i < fallingLeaves.size(); i += 7)
        {
            glColor3fv(leafColors[(i / 7) % 4]);
            glPushMatrix();
            glTranslatef(fallingLeaves[i], fallingLeaves[i + 1], fallingLeaves[i + 2]);
            glRotatef(fallingLeaves[i + 6], 0.0f, 0.0f, 1.0f);
            glRotatef(45.0f, 1.0f, 0.0f, 0.0f);
            glScalef(0.35f, 0.08f, 0.35f);
            glutSolidCube(0.6f);
            glPopMatrix();
        }

        for (size_t i = 0; i < groundLeaves.size(); i += 4)
        {
            glColor3fv(leafColors[(i / 4) % 4]);
            glPushMatrix();
            glTranslatef(groundLeaves[i], groundLeaves[i + 1], groundLeaves[i + 2]);
            glRotatef(groundLeaves[i + 3], 0.0f, 1.0f, 0.0f);
            glScalef(0.4f, 0.05f, 0.4f);
            glutSolidCube(0.6f);
            glPopMatrix();
        }

        if (lightingEnabled)
            glEnable(GL_LIGHTING);
    }
}

void drawTree(float x, float z)
{
    // Trunk
    glColor3f(0.35f, 0.25f, 0.15f);
    glPushMatrix();
    glTranslatef(x, -0.4f, z);

    // Draw trunk as tapered
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= 8; i++)
    {
        float angle = (i / 8.0f) * 2.0f * 3.14159f;
        float topR = 0.25f;
        float botR = 0.35f;

        glVertex3f(cos(angle) * topR, 1.8f, sin(angle) * topR);
        glVertex3f(cos(angle) * botR, 0.0f, sin(angle) * botR);
    }
    glEnd();
    glPopMatrix();

    // Leaves/branches based on season
    glPushMatrix();
    glTranslatef(x, 1.0f, z);

    switch (currentSeason)
    {
    case SPRING:
        // Bright green with pink blossoms
        glColor3f(0.2f, 0.85f, 0.3f);
        glutSolidSphere(1.3f, 12, 12);

        glColor4f(1.0f, 0.75f, 0.8f, 0.8f);
        for (int i = 0; i < 8; i++)
        {
            glPushMatrix();
            glTranslatef((rand() % 80 - 40) * 0.02f,
                         (rand() % 80 - 40) * 0.02f,
                         (rand() % 80 - 40) * 0.02f);
            glutSolidSphere(0.15f, 6, 6);
            glPopMatrix();
        }
        break;

    case SUMMER:
        glColor3f(0.05f, 0.6f, 0.15f);
        glutSolidSphere(1.4f, 14, 14);
        glColor3f(0.0f, 0.5f, 0.1f);
        glutSolidSphere(1.2f, 12, 12);
        break;

    case RAINY_SEASON:
        // Lush but darker/warmer toned leaves, and some droplet highlights
        glColor3f(0.06f, 0.5f, 0.12f);
        glutSolidSphere(1.25f, 12, 12);
        glColor3f(0.02f, 0.45f, 0.09f);
        glutSolidSphere(1.0f, 10, 10);
        break;

    case AUTUMN:
        glColor3f(0.85f, 0.5f, 0.15f);
        glutSolidSphere(1.2f, 12, 12);
        glColor3f(0.9f, 0.65f, 0.25f);
        glutSolidSphere(1.0f, 10, 10);
        glColor3f(0.75f, 0.35f, 0.05f);
        glutSolidSphere(0.8f, 8, 8);
        break;

    case LATE_AUTUMN:
        glColor3f(0.75f, 0.35f, 0.05f);
        glutSolidSphere(0.9f, 10, 10);
        glColor3f(0.3f, 0.25f, 0.2f);
        for (int i = 0; i < 8; i++) {
            glPushMatrix();
            glRotatef(i * 45.0f, 0.0f, 1.0f, 0.0f);
            glRotatef(30.0f, 0.0f, 0.0f, 1.0f);
            glTranslatef(0.0f, 0.3f, 0.0f);
            glScalef(0.05f, 0.8f, 0.05f);
            glutSolidCube(1.0f);
            glPopMatrix();
        }
        break;

    case WINTER:
        // Bare branches + some snow on top
        glColor3f(0.3f, 0.25f, 0.2f);
        for (int i = 0; i < 10; i++)
        {
            glPushMatrix();
            glRotatef(i * 36.0f, 0.0f, 1.0f, 0.0f);
            glRotatef(35.0f, 0.0f, 0.0f, 1.0f);
            glTranslatef(0.0f, 0.4f, 0.0f);
            glScalef(0.06f, 1.0f, 0.06f);
            glutSolidCube(1.0f);
            glPopMatrix();
        }

        glColor3f(0.98f, 0.98f, 1.0f);
        glutSolidSphere(0.7f, 10, 10);
        break;
    }
    glPopMatrix();
}

void drawCoin(float x, float z)
{
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 0.84f, 0.0f);
    glPushMatrix();
    glTranslatef(x, 0.5f, z);
    glRotatef(90.0f, 1.0f, 0.0f, 0.0f);
    glutSolidTorus(0.1f, 0.3f, 10, 10);
    glPopMatrix();
    if (lightingEnabled)
        glEnable(GL_LIGHTING);
}

void drawObstacle(float x, float z, int type)
{
    glPushMatrix();
    glTranslatef(x, 0.0f, z);

    switch (type)
    {
    case 0: // Rock
        glColor3f(0.5f, 0.5f, 0.5f);
        glutSolidSphere(0.9f, 10, 10);
        glColor3f(0.4f, 0.4f, 0.4f);
        glutSolidSphere(0.7f, 8, 8);
        break;

    case 1: // Traffic Cone
        glColor3f(1.0f, 0.5f, 0.0f);
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        glutSolidCone(0.6f, 1.4f, 12, 10);

        glColor3f(1.0f, 1.0f, 1.0f);
        glTranslatef(0.0f, 0.0f, 0.5f);
        glutSolidTorus(0.1f, 0.65f, 8, 12);
        break;

    case 2: // Barrel
        glColor3f(0.85f, 0.25f, 0.25f);
        glScalef(0.8f, 1.2f, 0.8f);
        glutSolidCube(1.2f);

        glColor3f(0.3f, 0.3f, 0.3f);
        glScalef(1.25f, 0.83f, 1.25f);
        glTranslatef(0.0f, 0.4f, 0.0f);
        glutSolidTorus(0.05f, 0.55f, 8, 16);
        break;

    case 3: // Tree Stump
        glColor3f(0.4f, 0.3f, 0.2f);
        glScalef(1.0f, 0.5f, 1.0f);
        glutSolidCube(1.4f);

        glColor3f(0.5f, 0.4f, 0.3f);
        glTranslatef(0.0f, 0.5f, 0.0f);
        glutSolidCube(1.2f);
        break;

    case 4: // Puddle
        if (currentSeason == SPRING || currentSeason == AUTUMN || currentSeason == LATE_AUTUMN || currentSeason == RAINY_SEASON)
        {
            glDisable(GL_LIGHTING);
                // Realistic mud/kada color - brownish with some variation
                if (currentSeason == SPRING) {
                    // Spring mud - darker and wetter looking
                    glColor4f(0.45f, 0.35f, 0.15f, 0.7f); // Dark brown mud
                } else {
                    // Autumn mud - lighter and drier looking
                    glColor4f(0.55f, 0.42f, 0.22f, 0.65f); // Light brown mud
                }
                glScalef(2.5f, 0.08f, 2.5f);
                glutSolidCube(1.2f);
               
                // Add some texture/variation to make it look more like real mud
                glColor4f(0.35f, 0.25f, 0.1f, 0.4f); // Darker spots
                glPushMatrix();
                glTranslatef(0.3f, 0.05f, 0.2f);
                glutSolidSphere(0.3f, 6, 6);
                glPopMatrix();
               
                glPushMatrix();
                glTranslatef(-0.2f, 0.05f, -0.3f);
                glutSolidSphere(0.25f, 6, 6);
                glPopMatrix();
               
                if (lightingEnabled) glEnable(GL_LIGHTING);
        }
        break;

    case 5: // Barrier
        glColor3f(1.0f, 0.9f, 0.0f);
        glScalef(2.2f, 0.35f, 0.15f);
        glutSolidCube(1.2f);

        glColor3f(0.2f, 0.2f, 0.2f);
        glScalef(0.45f, 2.86f, 0.83f);
        for (int i = -4; i <= 4; i += 4)
        {
            glPushMatrix();
            glTranslatef(i * 0.1f, 0.0f, 0.0f);
            glutSolidCube(0.2f);
            glPopMatrix();
        }
        break;
    }
    glPopMatrix();
}

void drawWheel()
{
    glColor3fv(wheelColor);
    glutSolidTorus(0.2f, 0.7f, 20, 20);

    glColor3f(0.3f, 0.3f, 0.3f);
    for (int i = 0; i < 5; i++)
    {
        glPushMatrix();
        glRotatef(i * 72.0f + car.wheelRotation, 0.0f, 0.0f, 1.0f);
        glScalef(1.5f, 0.1f, 0.1f);
        glutSolidCube(0.5f);
        glPopMatrix();
    }
}

void drawCar()
{
    // ========== CHASSIS ==========
    // Lower body (main chassis)
    glColor3fv(carBodyColor);
    glPushMatrix();
    glTranslatef(0.0f, 0.4f, 0.0f);
    glScalef(3.8f, 0.5f, 1.6f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Upper body
    glColor3f(carBodyColor[0] * 0.95f, carBodyColor[1] * 0.95f, carBodyColor[2] * 0.95f);
    glPushMatrix();
    glTranslatef(-0.2f, 0.9f, 0.0f);
    glScalef(2.2f, 0.5f, 1.5f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Hood
    glColor3fv(carBodyColor);
    glPushMatrix();
    glTranslatef(1.5f, 0.55f, 0.0f);
    glScalef(1.2f, 0.3f, 1.5f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Front bumper
    glColor3f(carBodyColor[0] * 0.8f, carBodyColor[1] * 0.8f, carBodyColor[2] * 0.8f);
    glPushMatrix();
    glTranslatef(2.3f, 0.25f, 0.0f);
    glScalef(0.3f, 0.4f, 1.7f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Rear bumper
    glPushMatrix();
    glTranslatef(-2.1f, 0.25f, 0.0f);
    glScalef(0.3f, 0.4f, 1.7f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // ========== ROOF & WINDOWS ==========
    // Windshield
    glEnable(GL_BLEND);
    glColor4f(0.15f, 0.35f, 0.55f, 0.6f);
    glPushMatrix();
    glTranslatef(0.8f, 1.0f, 0.0f);
    glRotatef(25.0f, 0.0f, 0.0f, 1.0f);
    glScalef(0.8f, 0.5f, 1.4f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Rear window
    glPushMatrix();
    glTranslatef(-0.9f, 1.0f, 0.0f);
    glRotatef(-25.0f, 0.0f, 0.0f, 1.0f);
    glScalef(0.7f, 0.5f, 1.4f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Side windows
    glPushMatrix();
    glTranslatef(0.2f, 1.05f, 0.85f);
    glScalef(1.8f, 0.45f, 0.05f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.2f, 1.05f, -0.85f);
    glScalef(1.8f, 0.45f, 0.05f);
    glutSolidCube(1.0f);
    glPopMatrix();
    glDisable(GL_BLEND);

    // ========== SPOILER ==========
    glColor3f(0.1f, 0.1f, 0.1f);
    // Spoiler supports
    glPushMatrix();
    glTranslatef(-1.8f, 0.9f, 0.6f);
    glScalef(0.1f, 0.4f, 0.1f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-1.8f, 0.9f, -0.6f);
    glScalef(0.1f, 0.4f, 0.1f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Spoiler wing
    glPushMatrix();
    glTranslatef(-1.8f, 1.15f, 0.0f);
    glScalef(0.15f, 0.08f, 1.4f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // ========== SIDE SKIRTS ==========
    glColor3f(0.15f, 0.15f, 0.15f);
    glPushMatrix();
    glTranslatef(0.2f, 0.05f, 1.0f);
    glScalef(3.0f, 0.15f, 0.15f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.2f, 0.05f, -1.0f);
    glScalef(3.0f, 0.15f, 0.15f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // ========== HEADLIGHTS ==========
    glDisable(GL_LIGHTING);

    // Headlight glow
    glColor4f(1.0f, 1.0f, 0.85f, 0.5f);
    glPushMatrix();
    glTranslatef(2.35f, 0.45f, 0.55f);
    glutSolidSphere(0.22f, 10, 10);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(2.35f, 0.45f, -0.55f);
    glutSolidSphere(0.22f, 10, 10);
    glPopMatrix();

    // Headlight lens
    glColor3f(1.0f, 1.0f, 0.95f);
    glPushMatrix();
    glTranslatef(2.32f, 0.45f, 0.55f);
    glScalef(0.08f, 0.2f, 0.3f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(2.32f, 0.45f, -0.55f);
    glScalef(0.08f, 0.2f, 0.3f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // ========== TAILLIGHTS ==========
    glColor3f(0.9f, 0.0f, 0.0f);
    glPushMatrix();
    glTranslatef(-2.15f, 0.45f, 0.65f);
    glScalef(0.08f, 0.18f, 0.25f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-2.15f, 0.45f, -0.65f);
    glScalef(0.08f, 0.18f, 0.25f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Inner taillight detail
    glColor3f(0.6f, 0.0f, 0.0f);
    glPushMatrix();
    glTranslatef(-2.13f, 0.45f, 0.65f);
    glScalef(0.05f, 0.12f, 0.18f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-2.13f, 0.45f, -0.65f);
    glScalef(0.05f, 0.12f, 0.18f);
    glutSolidCube(1.0f);
    glPopMatrix();

    if (lightingEnabled)
        glEnable(GL_LIGHTING);

    // ========== WHEELS - CORRECTED POSITIONS ==========
    // Front Right Wheel - RAISED (Y position changed from -0.5f to -0.1f)
    glPushMatrix();
    glTranslatef(1.3f, -0.1f, 1.0f);
    glRotatef(car.steeringAngle, 0.0f, 1.0f, 0.0f);
    glRotatef(car.wheelRotation, 0.0f, 0.0f, 1.0f);
    drawWheel();
    glPopMatrix();

    // Front Left Wheel - RAISED (Y position changed from -0.5f to -0.1f)
    glPushMatrix();
    glTranslatef(1.3f, -0.1f, -1.0f);
    glRotatef(car.steeringAngle, 0.0f, 1.0f, 0.0f);
    glRotatef(car.wheelRotation, 0.0f, 0.0f, 1.0f);
    drawWheel();
    glPopMatrix();

    // Rear Right Wheel - RAISED (Y position changed from -0.5f to -0.1f)
    glPushMatrix();
    glTranslatef(-1.3f, -0.1f, 1.0f);
    glRotatef(car.wheelRotation, 0.0f, 0.0f, 1.0f);
    drawWheel();
    glPopMatrix();

    // Rear Left Wheel - RAISED (Y position changed from -0.5f to -0.1f)
    glPushMatrix();
    glTranslatef(-1.3f, -0.1f, -1.0f);
    glRotatef(car.wheelRotation, 0.0f, 0.0f, 1.0f);
    drawWheel();
    glPopMatrix();

    // ========== EXHAUST PIPES ==========
    glColor3f(0.15f, 0.15f, 0.15f);
    glPushMatrix();
    glTranslatef(-2.05f, 0.05f, 0.6f);
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    glutSolidCone(0.1f, 0.25f, 10, 10);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-2.05f, 0.05f, -0.6f);
    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);
    glutSolidCone(0.1f, 0.25f, 10, 10);
    glPopMatrix();

    // ========== MIRRORS ==========
    glColor3f(0.1f, 0.1f, 0.1f);
    // Left mirror
    glPushMatrix();
    glTranslatef(0.5f, 1.0f, 1.0f);
    glScalef(0.15f, 0.15f, 0.3f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Right mirror
    glPushMatrix();
    glTranslatef(0.5f, 1.0f, -1.0f);
    glScalef(0.15f, 0.15f, 0.3f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // ========== FRONT GRILLE ==========
    glColor3f(0.1f, 0.1f, 0.1f);
    glPushMatrix();
    glTranslatef(2.25f, 0.45f, 0.0f);
    glScalef(0.1f, 0.25f, 1.2f);
    glutSolidCube(1.0f);
    glPopMatrix();
}

void drawRoad()
{
    // Main road surface
    if (currentSeason == WINTER && snowAccumulationLevel > 0.05f)
    {
        glColor3f(0.75f, 0.75f, 0.85f);
    }
    else
    {
        glColor3f(0.25f, 0.25f, 0.28f);
    }

    glPushMatrix();
    glTranslatef(0.0f, -0.95f, 0.0f);
    glScalef(ROAD_WIDTH, 0.1f, 2000.0f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Road edges/curbs
    glColor3f(0.35f, 0.35f, 0.35f);
    glPushMatrix();
    glTranslatef(-ROAD_WIDTH / 2 - 0.2f, -0.9f, 0.0f);
    glScalef(0.4f, 0.15f, ROAD_LENGTH);
    glutSolidCube(1.0f);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(ROAD_WIDTH / 2 + 0.2f, -0.9f, 0.0f);
    glScalef(0.4f, 0.15f, ROAD_LENGTH);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Road markings (lane lines)
    if (!(currentSeason == WINTER && snowAccumulationLevel > 0.1f))
    {
        glColor3f(0.95f, 0.95f, 0.0f);
        for (float z = -ROAD_LENGTH / 2; z < ROAD_LENGTH / 2; z += 12.0f)
        {
            glPushMatrix();
            glTranslatef(0.0f, -0.89f, z);
            glScalef(0.15f, 0.02f, 2.5f);
            glutSolidCube(1.0f);
            glPopMatrix();
        }

        // Side lines
        glColor3f(0.95f, 0.95f, 0.95f);
        glPushMatrix();
        glTranslatef(-ROAD_WIDTH / 2 + 0.3f, -0.89f, 0.0f);
        glScalef(0.12f, 0.02f, ROAD_LENGTH);
        glutSolidCube(1.0f);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(ROAD_WIDTH / 2 - 0.3f, -0.89f, 0.0f);
        glScalef(0.12f, 0.02f, ROAD_LENGTH);
        glutSolidCube(1.0f);
        glPopMatrix();
    }

    // Ground/Grass
    glColor3fv(seasonColors[currentSeason].grass);
    glPushMatrix();
    glTranslatef(0.0f, -1.05f, 0.0f);
    glScalef(300.0f, 0.1f, 2000.0f);
    glutSolidCube(1.0f);
    glPopMatrix();
}

void drawEnvironment()
{
    for (size_t i = 0; i < treePositions.size(); i += 2)
    {
        float dist = sqrt(pow(treePositions[i] - car.posX, 2) +
                          pow(treePositions[i + 1] - car.posZ, 2));
        if (dist < 100.0f)
        {
            drawTree(treePositions[i], treePositions[i + 1]);
        }
    }

    for (size_t i = 0; i < coinPositions.size(); i += 2)
    {
        drawCoin(coinPositions[i], coinPositions[i + 1]);
    }

    for (size_t i = 0; i < obstaclePositions.size(); i += 2)
    {
        int typeIdx = i / 2;
        if (typeIdx < obstacleTypes.size())
        {
            drawObstacle(obstaclePositions[i], obstaclePositions[i + 1],
                         obstacleTypes[typeIdx]);
        }
    }

    drawFallingLeaves();
}

void drawHUD()
{
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    gluOrtho2D(0, 1200, 0, 800);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    // Semi-transparent panel for HUD
    glEnable(GL_BLEND);
    glColor4f(0.0f, 0.0f, 0.0f, 0.5f);
    glBegin(GL_QUADS);
    glVertex2f(10, 720);
    glVertex2f(320, 720);
    glVertex2f(320, 450);
    glVertex2f(10, 450);
    glEnd();

    glColor3f(1.0f, 1.0f, 1.0f);

    // HUD Info
    std::string hudText[] = {
        "Score: " + std::to_string(game.score),
        "Coins: " + std::to_string(game.coinsCollected) + "/50",
        "Time: " + std::to_string(static_cast<int>(game.time)) + "s",
        "Speed: " + std::to_string(static_cast<int>(fabs(car.speed) * 60)) + " km/h"};

    int yPos = 690;
    for (const auto &text : hudText)
    {
        glRasterPos2f(25, yPos);
        for (char c : text)
        {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
        }
        yPos -= 35;
    }

    // Season info
    glRasterPos2f(25, 540);
    std::string seasonText = "Season: ";
    const char *seasonNames[] = {"SUMMER", "RAINY", "AUTUMN", "LATE AUTUMN", "WINTER", "SPRING"};
    seasonText += seasonNames[currentSeason];
    seasonText += " (" + std::to_string(static_cast<int>(SEASON_DURATION - seasonTimer)) + "s)";
    for (char c : seasonText)
    {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }

    // Weather
    glRasterPos2f(25, 505);
    std::string weatherText = "Weather: ";
    if (isRaining)
        weatherText += "RAINING";
    else if (isSnowing)
        weatherText += "SNOWING";
    else
        weatherText += "CLEAR";
    for (char c : weatherText)
    {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }

    // Camera mode
    glRasterPos2f(25, 470);
    const char *cameraModes[] = {"", "Follow", "Top-Down", "First-Person", "Free"};
    std::string cameraText = "Camera: " + std::string(cameraModes[camera.mode]);
    for (char c : cameraText)
    {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
    }

    // Day/Night timer display
    glRasterPos2f(25, 435);
    std::string dayNightText = "Time to toggle Day/Night: " + std::to_string(static_cast<int>(DAY_NIGHT_DURATION - dayNightTimer)) + "s";
    for (char c : dayNightText)
    {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
    }

    // Instructions hint
    glRasterPos2f(20, 25);
    std::string hintText = "Press 'I' for controls";
    for (char c : hintText)
    {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
    }

    // Instructions panel
    if (game.showInstructions)
    {
        glColor4f(0.0f, 0.0f, 0.0f, 0.85f);
        glBegin(GL_QUADS);
        glVertex2f(300, 80);
        glVertex2f(900, 80);
        glVertex2f(900, 650);
        glVertex2f(300, 650);
        glEnd();

        glColor3f(0.2f, 0.8f, 1.0f);
        glBegin(GL_LINE_LOOP);
        glVertex2f(300, 80);
        glVertex2f(900, 80);
        glVertex2f(900, 650);
        glVertex2f(300, 650);
        glEnd();

        glColor3f(1.0f, 1.0f, 0.0f);
        glRasterPos2f(380, 615);
        std::string title = "ENHANCED CAR DRIVING SIMULATOR";
        for (char c : title)
        {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
        }

        glColor3f(1.0f, 1.0f, 1.0f);
        std::vector<std::string> instructions = {
            "DRIVING:",
            "  W/Up Arrow - Accelerate",
            "  S/Down Arrow - Brake/Reverse",
            "  A/D/Left/Right Arrow - Steer",
            "  SPACE - Emergency brake",
            "",
            "CAMERA (C to cycle):",
            "  Follow, Top-Down, First-Person, Free",
            "  Arrow keys in Free mode to look around",
            "",
            "ENVIRONMENT:",
            "  L - Toggle lighting    N - Day/Night (also auto toggles every 15s)",
            "  F - Toggle fog         T - Next season",
            "  R - Random car color",
            "",
            "GOAL: Collect all 50 coins!",
            "Avoid obstacles or it's GAME OVER!"};

        yPos = 570;
        for (const auto &line : instructions)
        {
            glRasterPos2f(330, yPos);
            for (char c : line)
            {
                glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, c);
            }
            yPos -= 30;
        }
    }

    // Game Over
    if (game.isOver)
    {
        glColor4f(0.8f, 0.0f, 0.0f, 0.9f);
        glBegin(GL_QUADS);
        glVertex2f(350, 250);
        glVertex2f(850, 250);
        glVertex2f(850, 500);
        glVertex2f(350, 500);
        glEnd();

        glColor3f(1.0f, 1.0f, 1.0f);
        glRasterPos2f(500, 430);
        std::string gameOverText = "GAME OVER!";
        for (char c : gameOverText)
        {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, c);
        }

        glRasterPos2f(480, 370);
        std::string scoreText = "Score: " + std::to_string(game.score);
        for (char c : scoreText)
        {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
        }

        glRasterPos2f(450, 300);
        std::string restartText = "Press 'R' to Restart";
        for (char c : restartText)
        {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
        }
    }

    // Victory
    if (game.isWon)
    {
        glColor4f(0.0f, 0.8f, 0.0f, 0.9f);
        glBegin(GL_QUADS);
        glVertex2f(350, 250);
        glVertex2f(850, 250);
        glVertex2f(850, 500);
        glVertex2f(350, 500);
        glEnd();

        glColor3f(1.0f, 1.0f, 0.0f);
        glRasterPos2f(510, 430);
        std::string winText = "VICTORY!";
        for (char c : winText)
        {
            glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, c);
        }

        glColor3f(1.0f, 1.0f, 1.0f);
        glRasterPos2f(440, 370);
        std::string scoreText = "Final Score: " + std::to_string(game.score);
        for (char c : scoreText)
        {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
        }

        glRasterPos2f(430, 300);
        std::string restartText = "Press 'R' to Play Again";
        for (char c : restartText)
        {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, c);
        }
    }

    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_DEPTH_TEST);
}

// ============= GAME LOGIC =============
void checkCollisions()
{
    // Coin collection
    for (size_t i = 0; i < coinPositions.size(); i += 2)
    {
        float dx = car.posX - coinPositions[i];
        float dz = car.posZ - coinPositions[i + 1];
        float distance = sqrt(dx * dx + dz * dz);

        if (distance < 1.8f)
        {
            game.score += 10;
            game.coinsCollected++;

            // Respawn coin
            coinPositions[i] = (rand() % static_cast<int>(ROAD_WIDTH - 4)) - (ROAD_WIDTH / 2 - 2);
            coinPositions[i + 1] = (rand() % static_cast<int>(ROAD_LENGTH - 200)) - (ROAD_LENGTH / 2 - 100);
            break;
        }
    }

    // Obstacle collision
    if (!game.isOver)
    {
        for (size_t i = 0; i < obstaclePositions.size(); i += 2)
        {
            float dx = car.posX - obstaclePositions[i];
            float dz = car.posZ - obstaclePositions[i + 1];
            float distance = sqrt(dx * dx + dz * dz);

            if (distance < 1.5f)
            {
                game.isOver = true;
                car.speed = 0.0f;
                car.acceleration = 0.0f;
                break;
            }
        }
    }

    // Win condition
    if (game.coinsCollected >= 50 && !game.isWon)
    {
        game.isWon = true;
        game.score += static_cast<int>(1000 - game.time * 0.5f);
        if (game.score < 0)
            game.score = 0;
    }
}

void updateSeason(float dt)
{
    seasonTimer += dt;
    if (seasonTimer >= SEASON_DURATION)
    {
        seasonTimer = 0.0f;
        currentSeason = static_cast<Season>((currentSeason + 1) % SEASON_COUNT);

        // New season behavior mapping
        isRaining = (currentSeason == RAINY_SEASON);
        isSnowing = (currentSeason == WINTER);

        if (isRaining || isSnowing)
        {
            initWeatherEffects();
        }
        else
        {
            rainDrops.clear();
            snowFlakes.clear();
        }
    }

    // Update snow accumulation
    if (currentSeason == WINTER && isSnowing)
    {
        snowAccumulationLevel += dt * 0.008f;
        if (snowAccumulationLevel > 0.25f)
            snowAccumulationLevel = 0.25f;
    }
    else
    {
        snowAccumulationLevel -= dt * 0.015f;
        if (snowAccumulationLevel < 0.0f)
        {
            snowAccumulationLevel = 0.0f;
            groundSnow.clear();
        }
    }
}

void updateCamera()
{
    glLoadIdentity();

    switch (camera.mode)
    {
    case 1:
    { // Follow camera
        float camDist = 10.0f;
        float camHeight = 4.0f;
        float lookAhead = 5.0f;

        float rad = car.rotation * 3.14159f / 180.0f;

        // Camera position: behind the car
        float behindX = car.posX - camDist * sin(rad);
        float behindZ = car.posZ - camDist * cos(rad);

        // Look-at point: ahead of the car
        float lookX = car.posX + lookAhead * sin(rad);
        float lookZ = car.posZ + lookAhead * cos(rad);

        gluLookAt(behindX, camHeight, behindZ, // Camera position
                  lookX, 1.5f, lookZ,          // Look at point
                  0.0f, 1.0f, 0.0f);           // scale object
        break;
    }

    case 2: // Top-down camera
        gluLookAt(car.posX, 20.0f, car.posZ,
                  car.posX, 0.0f, car.posZ,
                  0.0f, 0.0f, -1.0f);
        break;

    case 3:
    { // First-person camera
        float eyeHeight = 1.5f;
        float eyeForward = 1.0f;
        float rad = car.rotation * 3.14159f / 180.0f;

        float eyeX = car.posX + eyeForward * sin(rad);
        float eyeZ = car.posZ + eyeForward * cos(rad);
        float lookX = car.posX + 25.0f * sin(rad);
        float lookZ = car.posZ + 25.0f * cos(rad);

        gluLookAt(eyeX, eyeHeight, eyeZ,
                  lookX, eyeHeight - 0.2f, lookZ,
                  0.0f, 1.0f, 0.0f);
        break;
    }

    case 4:
    { // Free camera: Camera follows car but can rotate freely
        float rad = car.rotation * 3.14159f / 180.0f;

        // Camera position follows car but with offset based on camera rotation
        float offsetX = -10.0f * sin(camera.angleY * 3.14159f / 180.0f);
        float offsetZ = -10.0f * cos(camera.angleY * 3.14159f / 180.0f);

        camera.x = car.posX + offsetX;
        camera.z = car.posZ + offsetZ;
        camera.y = car.posY + 5.0f; // Keep camera height relative to car

        // Look at point is in front of car based on camera direction
        float lookX = camera.x + sin(camera.angleY * 3.14159f / 180.0f) * cos(camera.angleX * 3.14159f / 180.0f);
        float lookY = camera.y + sin(camera.angleX * 3.14159f / 180.0f);
        float lookZ = camera.z + cos(camera.angleY * 3.14159f / 180.0f) * cos(camera.angleX * 3.14159f / 180.0f);

        gluLookAt(camera.x, camera.y, camera.z,
                  lookX, lookY, lookZ,
                  0.0f, 1.0f, 0.0f);
        break;
    }
    }
}

void update(int value)
{
    if (!game.isOver && !game.isWon)
    {
        float dt = 0.016f;
        game.time += dt;

        // Automatic day/night toggle timer
        dayNightTimer += dt;
        if (dayNightTimer >= DAY_NIGHT_DURATION)
        {
            dayNightTimer = 0.0f;
            nightMode = !nightMode;
        }

        updateSeason(dt);
        updateFallingLeaves(dt);
        updateWeatherEffects();

        // Car physics
        if (fabs(car.acceleration) > 0.001f)
        {
            car.speed += car.acceleration;
            car.speed = std::max(-MAX_SPEED * 0.6f, std::min(MAX_SPEED, car.speed));
        }

        car.speed *= FRICTION;
        if (fabs(car.speed) < 0.001f)
            car.speed = 0.0f;

        float rad = car.rotation * 3.14159f / 180.0f;
        car.posX += car.speed * sin(rad);
        car.posZ += car.speed * cos(rad);

        if (fabs(car.speed) > 0.01f)
        {
            float steeringEffect = car.steeringAngle * (0.3f + fabs(car.speed) * 1.2f);
            car.rotation += steeringEffect;
        }

        car.steeringAngle *= 0.75f;
        if (fabs(car.steeringAngle) < 0.05f)
            car.steeringAngle = 0.0f;

        car.wheelRotation -= car.speed * 90.0f;
        if (car.wheelRotation > 360.0f)
            car.wheelRotation -= 360.0f;
        if (car.wheelRotation < -360.0f)
            car.wheelRotation += 360.0f;

        checkCollisions();

        // Road boundaries
        float maxX = ROAD_WIDTH / 2.0f - 1.5f;
        if (car.posX < -maxX)
        {
            car.posX = -maxX;
            car.speed *= 0.4f;
        }
        if (car.posX > maxX)
        {
            car.posX = maxX;
            car.speed *= 0.4f;
        }

        // Road looping
        if (car.posZ < -ROAD_LENGTH / 2.0f)
            car.posZ = ROAD_LENGTH / 2.0f;
        if (car.posZ > ROAD_LENGTH / 2.0f)
            car.posZ = -ROAD_LENGTH / 2.0f;
    }

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Background color
    if (nightMode)
    {
        glClearColor(skyColorNight[0], skyColorNight[1], skyColorNight[2], 1.0f);
    }
    else
    {
        glClearColor(seasonColors[currentSeason].sky[0],
                     seasonColors[currentSeason].sky[1],
                     seasonColors[currentSeason].sky[2], 1.0f);
    }

    updateCamera();

    // Lighting setup
    if (lightingEnabled)
    {
        GLfloat lightIntensity = nightMode ? 0.35f : 1.0f;
        if (currentSeason == WINTER)
            lightIntensity *= 0.9f;

        // Rainy season darker ambient
        if (currentSeason == RAINY_SEASON && !nightMode)
            lightIntensity *= 0.7f;

        GLfloat lightColor[] = {lightIntensity, lightIntensity,
                                nightMode ? lightIntensity * 0.9f : lightIntensity, 1.0f};
        GLfloat lightPos[] = {30.0f, 35.0f, -80.0f, 1.0f};

        glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor);
        glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

        GLfloat ambientLight[] = {nightMode ? 0.2f : 0.4f,
                                  nightMode ? 0.2f : 0.4f,
                                  nightMode ? 0.25f : 0.4f, 1.0f};
        // darker ambient in rainy season
        if (currentSeason == RAINY_SEASON && !nightMode)
        {
            ambientLight[0] *= 0.7f;
            ambientLight[1] *= 0.7f;
            ambientLight[2] *= 0.8f;
        }
        glLightfv(GL_LIGHT0, GL_AMBIENT, ambientLight);
    }

    // Fog setup
    if (fogEnabled)
    {
        glEnable(GL_FOG);
        GLfloat fogColor[] = {0.6f, 0.6f, 0.65f, 1.0f};
        if (nightMode)
        {
            fogColor[0] = 0.1f;
            fogColor[1] = 0.1f;
            fogColor[2] = 0.2f;
        }
        // rainy season denser fog
        if (currentSeason == RAINY_SEASON)
            glFogf(GL_FOG_DENSITY, 0.03f);
        else
            glFogf(GL_FOG_DENSITY, 0.015f);
        glFogfv(GL_FOG_COLOR, fogColor);
        glFogi(GL_FOG_MODE, GL_EXP2);
    }
    else
    {
        glDisable(GL_FOG);
    }

    drawSkybox();
    drawSunMoon();
    drawRoad();
    drawEnvironment();
    drawWeatherEffects();

    glPushMatrix();
    glTranslatef(car.posX, 0.0f, car.posZ);
    // <<< FIXED ROTATION OFFSET: make car model face movement direction correctly >>>
    glRotatef(car.rotation - 90.0f, 0.0f, 1.0f, 0.0f);
    drawCar();
    glPopMatrix();

    drawHUD();

    glutSwapBuffers();
}

void resetGame()
{
    car.posX = car.posY = car.posZ = 0.0f;
    car.speed = car.rotation = car.wheelRotation = 0.0f;
    car.steeringAngle = car.acceleration = 0.0f;

    game.score = game.coinsCollected = 0;
    game.time = 0.0f;
    game.isOver = game.isWon = false;

    currentSeason = SPRING;
    seasonTimer = 0.0f;
    // assign weather based on new season mapping
    isRaining = (currentSeason == RAINY_SEASON);
    isSnowing = (currentSeason == WINTER);
    snowAccumulationLevel = 0.0f;

    camera.mode = 1;
    camera.x = 0.0f;
    camera.y = 5.0f;
    camera.z = 30.0f;
    camera.angleX = camera.angleY = 0.0f;

    fallingLeaves.clear();
    groundLeaves.clear();
    snowFlakes.clear();
    groundSnow.clear();

    dayNightTimer = 0.0f;
    nightMode = false;

    initializeEnvironment();
    initWeatherEffects();
}

void keyboard(unsigned char key, int x, int y)
{
    if ((game.isOver || game.isWon) && (key == 'r' || key == 'R'))
    {
        resetGame();
        return;
    }

    if (game.isOver || game.isWon)
        return;

    switch (key)
    {
    // Driving - ALWAYS works regardless of camera mode
    case 'w':
    case 'W':
        car.acceleration = ACCEL_FORWARD;
        break;
    case 's':
    case 'S':
        car.acceleration = -ACCEL_REVERSE;
        break;
    case 'a':
    case 'A':
        car.steeringAngle = 2.5f;
        break;
    case 'd':
    case 'D':
        car.steeringAngle = -2.5f;
        break;
    case ' ':
        car.speed *= BRAKE_POWER;
        car.acceleration = 0.0f;
        break;

    // Camera
    case 'c':
    case 'C':
        camera.mode = (camera.mode % 4) + 1;
        if (camera.mode == 4)
        {
            // Initialize free camera position behind car
            camera.x = car.posX;
            camera.y = car.posY + 5.0f;
            camera.z = car.posZ - 10.0f;
            camera.angleY = car.rotation;
            camera.angleX = -10.0f;
        }
        break;

    // Environment
    case 'l':
    case 'L':
        lightingEnabled = !lightingEnabled;
        break;
    case 'n':
    case 'N':
        nightMode = !nightMode;
        dayNightTimer = 0.0f; // reset auto timer when manually toggled
        break;
    case 'f':
    case 'F':
        fogEnabled = !fogEnabled;
        break;

    case 'r':
    case 'R':
        carBodyColor[0] = (rand() % 80 + 20) / 100.0f;
        carBodyColor[1] = (rand() % 80 + 20) / 100.0f;
        carBodyColor[2] = (rand() % 80 + 20) / 100.0f;
        break;

    case 't':
    case 'T':
        currentSeason = static_cast<Season>((currentSeason + 1) % SEASON_COUNT);
        seasonTimer = 0.0f;
        isRaining = (currentSeason == RAINY_SEASON);
        isSnowing = (currentSeason == WINTER);
        if (isRaining || isSnowing)
            initWeatherEffects();
        else
        {
            rainDrops.clear();
            snowFlakes.clear();
        }
        break;

    case 'i':
    case 'I':
        game.showInstructions = !game.showInstructions;
        break;

    case 27:
        exit(0);
        break;
    }
    glutPostRedisplay();
}

void keyboardUp(unsigned char key, int x, int y)
{
    if (game.isOver || game.isWon)
        return;

    switch (key)
    {
    case 'w':
    case 'W':
    case 's':
    case 'S':
        car.acceleration = 0.0f;
        break;
    case 'a':
    case 'A':
    case 'd':
    case 'D':
        car.steeringAngle = 0.0f;
        break;
    }
}

void specialKeys(int key, int x, int y)
{
    if (game.isOver || game.isWon)
        return;

    if (camera.mode == 4)
    {
        // Free camera rotation controls
        switch (key)
        {
        case GLUT_KEY_UP:
            camera.angleX -= 3.0f;
            if (camera.angleX < -89.0f)
                camera.angleX = -89.0f;
            break;
        case GLUT_KEY_DOWN:
            camera.angleX += 3.0f;
            if (camera.angleX > 89.0f)
                camera.angleX = 89.0f;
            break;
        case GLUT_KEY_LEFT:
            camera.angleY -= 3.0f;
            break;
        case GLUT_KEY_RIGHT:
            camera.angleY += 3.0f;
            break;
        case GLUT_KEY_PAGE_UP:
            camera.y += 0.5f;
            break;
        case GLUT_KEY_PAGE_DOWN:
            camera.y -= 0.5f;
            break;
        }
    }
    else
    {
        // Normal driving controls for other camera modes
        switch (key)
        {
        case GLUT_KEY_UP:
            car.acceleration = ACCEL_FORWARD;
            break;
        case GLUT_KEY_DOWN:
            car.acceleration = -ACCEL_REVERSE;
            break;
        case GLUT_KEY_LEFT:
            car.steeringAngle = 2.5f;
            break;
        case GLUT_KEY_RIGHT:
            car.steeringAngle = -2.5f;
            break;
        }
    }
    glutPostRedisplay();
}

void specialKeysUp(int key, int x, int y)
{
    if (game.isOver || game.isWon)
        return;

    if (camera.mode != 4)
    {
        switch (key)
        {
        case GLUT_KEY_UP:
        case GLUT_KEY_DOWN:
            car.acceleration = 0.0f;
            break;
        case GLUT_KEY_LEFT:
        case GLUT_KEY_RIGHT:
            car.steeringAngle = 0.0f;
            break;
        }
    }
}

void init()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_NORMALIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glShadeModel(GL_SMOOTH);

    GLfloat lightPos[] = {30.0f, 35.0f, -80.0f, 1.0f};
    GLfloat whiteLight[] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat ambient[] = {0.4f, 0.4f, 0.4f, 1.0f};

    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, whiteLight);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_SPECULAR, whiteLight);

    glMatrixMode(GL_PROJECTION);
    gluPerspective(50.0f, 1.5f, 0.5f, 600.0f);
    glMatrixMode(GL_MODELVIEW);

    initializeEnvironment();
    initWeatherEffects();
}

void reshape(int w, int h)
{
    if (h == 0)
        h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(50.0f, (float)w / h, 0.5f, 600.0f);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1200, 800);
    glutInitWindowPosition(100, 50);
    glutCreateWindow("Enhanced 3D Car Driving Simulator - Collect 50 Coins!");

    init();

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutSpecialFunc(specialKeys);
    glutSpecialUpFunc(specialKeysUp);
    glutTimerFunc(0, update, 0);

    printf("========================================\n");
    printf("Enhanced Car Driving Simulator Started!\n");
    printf("========================================\n");
    printf("Features:\n");
    printf("- 6 Seasons: Summer, Rainy, Autumn, Late Autumn, Winter, Spring\n");
    printf("- Automatic Day/Night toggle every %ds (press 'N' to toggle manually)\n", (int)DAY_NIGHT_DURATION);
    printf("- Weather effects (rain/snow)\n");
    printf("- 6 obstacle types\n");
    printf("- 4 camera modes\n");
    printf("- Season cycle duration: %ds\n", (int)SEASON_DURATION);
    printf("\nGoal: Collect all 50 coins!\n");
    printf("Press 'I' in-game for full controls\n");
    printf("========================================\n");

    glutMainLoop();
    return 0;
}
