#define _CRT_SECURE_NO_WARNINGS
#define _CRT_NONSTDC_NO_DEPRECATE
#include <cstddef>
#include <cstdio>
#include <cstdarg> 

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "terrain_shaders.h"
#include "snow_shaders.h"
#include "rain_shaders.h"
#include "fog_shaders.h"
#include "lightning_shaders.h"
#include "LightningGeometry.h"
#include <iostream>
#include <glad/glad.h>
#include <glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <random>
#include <fstream>
#include <sstream>
#include <chrono>
#include <cmath>
#include <map>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

struct Snowflake {
    glm::vec3 position;
    float size;
    float fallSpeed;
    float swayOffset;
    float rotation;
    float rotationSpeed;
};

struct Raindrop {
    glm::vec3 position;
    float size;
    float fallSpeed;
    float trailLength;
    glm::vec3 velocity;
};

struct Sun {
    glm::vec3 position;
    float intensity;
    float angle;
    float speed;
    bool enabled;
    float radius;
    float visualRadius;
};

Sun sun;
float sunCycleTime = 0.0f;
float sunCycleDuration = 160.0f;

float lastLightningTime = 0.0f;
float nextLightningDelay = 0.0f;
bool lightningFlash = false;
float flashIntensity = 0.0f;
float boltDuration = 0.0f;
float currentLightningPosX = 0.0f;
float currentLightningPosZ = 0.0f;

bool preFlashActive = false;
float preFlashIntensity = 0.0f;
float preFlashDuration = 0.0f;
std::vector<glm::vec3> flashRegions;

glm::vec3 cameraPos = glm::vec3(0.0f, 100.0f, 200.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
float cameraSpeed = 2.0f;
float yaw = -90.0f, pitch = 0.0f;
float lastX = 400, lastY = 300;
bool firstMouse = true;

bool snowEnabled = false;
bool rainEnabled = false;
bool starsOnlyMode = false;
bool ctrlPressed = false;
bool toggleSnowRequest = false;
bool toggleRainRequest = false;
bool toggleFogRequest = false;
bool toggleLightningRequest = false;
bool toggleSunRequest = false;
bool toggleStarsOnlyRequest = false;

float snowfallDensity = 0.5f;
float snowWindStrength = 0.3f;
glm::vec3 snowWindDirection = glm::vec3(0.3f, 0.0f, 0.7f);

float rainfallDensity = 0.5f;
float rainWindStrength = 0.5f;
glm::vec3 rainWindDirection = glm::vec3(0.5f, 0.0f, 0.5f);
float rainIntensity = 0.5f;

bool fogEnabled = false;
float fogDensity = 0.008f;
float fogGradient = 1.2f;
glm::vec3 fogColor = glm::vec3(0.5f, 0.6f, 0.7f);

bool lightningEnabled = false;
float lightningIntensity = 0.9f;
float lightningFrequency = 5.0f;

bool sunEnabled = false;
float sunSpeedMultiplier = 1.0f;

float textureScale = 0.1f;
unsigned int grassTexture = 0;
unsigned int rockTexture = 0;
unsigned int sandTexture = 0;
unsigned int waterTexture = 0;
unsigned int snowTexture = 0;
unsigned int moonTexture = 0;

float maxTerrainHeight = 0.0f;

unsigned int lightningShader = 0;
unsigned int lightningVAO = 0, lightningVBO = 0, lightningEBO = 0;
int lightningIndexCount = 0;

unsigned int sunVAO = 0, sunVBO = 0, sunEBO = 0;
int sunIndexCount = 0;
unsigned int sunShader = 0;

unsigned int moonVAO = 0, moonVBO = 0, moonEBO = 0;
int moonIndexCount = 0;
unsigned int moonShader = 0;

unsigned int skyboxVAO = 0, skyboxVBO = 0;
unsigned int skyboxShader = 0;

int mapWidth = 0, mapHeight = 0;
int windowWidth = 0, windowHeight = 0;

glm::vec3 moonPosition = glm::vec3(0.0f, 180.0f, 0.0f);

namespace SkyboxShaders {
    const std::string vertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
uniform mat4 projection;
uniform mat4 view;
out vec3 TexCoords;
void main()
{
    TexCoords = aPos;
    vec4 pos = projection * view * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}
)";

    const std::string fragmentShader = R"(
#version 330 core
in vec3 TexCoords;
out vec4 FragColor;
uniform float time;
uniform float sunIntensity;
uniform float sunAngle;
uniform bool isRaining;
uniform bool isSnowing;
uniform bool starsOnlyMode;
uniform float lightningFlashIntensity;
uniform float preFlashIntensity;
uniform int preFlashCount;
uniform vec3 preFlashPositions[8];

float starPattern(vec2 uv, float timeVal, float brightness, float sizeOffset) {
    float stars = 0.0;
    vec2 grid = floor(uv * 120.0);
    vec2 fracGrid = fract(uv * 120.0);
    
    float seed = dot(grid, vec2(12.9898, 78.233));
    float r = fract(sin(seed) * 43758.5453);
    float twinkle = 0.6 + sin(timeVal * 1.2 + seed) * 0.4;
    
    if (r > 0.992) {
        float starSize = min(0.12, 0.03 + r * 0.09 + sizeOffset * 0.05);
        vec2 starUv = fracGrid - 0.5;
        float distToCenter = length(starUv);
        
        if (distToCenter < starSize) {
            float intensity = 1.0 - smoothstep(0.0, starSize, distToCenter);
            intensity *= (0.7 + r * 0.3);
            stars = intensity * twinkle * brightness;
        }
    }
    return stars;
}

void main()
{
    vec3 dir = normalize(TexCoords);
    float y = clamp(TexCoords.y, 0.0, 1.0);
    float angle = sunAngle;
    
    vec2 sphericalUv = vec2(
        atan(dir.z, dir.x) / (2.0 * 3.14159),
        acos(dir.y) / 3.14159
    );
    
    vec3 nightColor = vec3(0.02, 0.02, 0.06);
    vec3 dawnColor = vec3(0.35, 0.22, 0.18);
    vec3 dayColorTop = vec3(0.25, 0.55, 0.85);
    vec3 dayColorBottom = vec3(0.55, 0.78, 0.92);
    vec3 duskColor = vec3(0.38, 0.24, 0.2);
    
    vec3 snowNightColor = vec3(0.01, 0.01, 0.04);
    vec3 snowDawnColor = vec3(0.15, 0.12, 0.18);
    vec3 snowDayColor = vec3(0.22, 0.28, 0.35);
    vec3 snowDuskColor = vec3(0.18, 0.14, 0.20);
    
    float totalFlashIntensity = lightningFlashIntensity + preFlashIntensity;
    vec3 flashColor = vec3(0.45, 0.4, 0.35) * totalFlashIntensity;
    
    if (starsOnlyMode) {
        vec3 skyGradient = mix(vec3(0.05, 0.06, 0.12), vec3(0.10, 0.12, 0.20), y);
        vec3 finalColor = skyGradient;
        
        float stars = 0.0;
        for (int i = 0; i < 4; i++) {
            float offset = float(i) * 0.25;
            stars += starPattern(sphericalUv + offset, time, 1.0, float(i) * 0.3);
            stars += starPattern(sphericalUv * 1.3 + offset * 0.5, time * 0.8, 0.7, float(i) * 0.2);
            stars += starPattern(sphericalUv * 1.8 + offset * 0.8, time * 1.2, 0.5, float(i) * 0.1);
        }
        
        vec3 starColor = vec3(1.0, 0.95, 0.85);
        finalColor += starColor * stars * 0.8;
        
        FragColor = vec4(finalColor, 1.0);
        return;
    }
    
    if (isRaining) {
        vec3 rainColor = vec3(0.10, 0.13, 0.20);
        
        float regionFlash = 0.0;
        for (int i = 0; i < preFlashCount; i++) {
            vec3 region = preFlashPositions[i];
            float distToRegion = distance(sphericalUv, region.xy);
            if (distToRegion < region.z) {
                regionFlash += (1.0 - distToRegion / region.z) * preFlashIntensity;
            }
        }
        regionFlash = clamp(regionFlash, 0.0, 1.0);
        
        rainColor += vec3(0.35, 0.28, 0.22) * (lightningFlashIntensity + regionFlash * 0.8);
        FragColor = vec4(rainColor, 1.0);
        return;
    }
    
    if (isSnowing) {
        vec3 skyColor;
        if (angle < 0.5f) {
            float t = angle / 0.5f;
            t = t * t;
            skyColor = mix(snowNightColor, snowDawnColor, t);
        }
        else if (angle < 1.0f) {
            float t = (angle - 0.5f) / 0.5f;
            t = t * t;
            skyColor = mix(snowDawnColor, snowDayColor, t);
        }
        else if (angle < 2.2f) {
            skyColor = mix(snowDayColor, snowDayColor * 0.9, y);
        }
        else if (angle < 2.6f) {
            float t = (angle - 2.2f) / 0.4f;
            t = t * t;
            skyColor = mix(snowDayColor, snowDuskColor, t);
        }
        else {
            float t = (angle - 2.6f) / 0.54f;
            t = min(t, 1.0f);
            t = t * t;
            skyColor = mix(snowDuskColor, snowNightColor, t);
        }
        
        if (angle < 0.4f || angle > 2.9f) {
            float stars = 0.0;
            for (int i = 0; i < 3; i++) {
                stars += starPattern(sphericalUv, time, 0.5, float(i) * 0.2);
                stars += starPattern(sphericalUv * 1.5, time * 0.8, 0.3, float(i) * 0.15);
            }
            skyColor += vec3(0.8, 0.75, 0.65) * stars * 0.3;
        }
        
        float regionFlash = 0.0;
        for (int i = 0; i < preFlashCount; i++) {
            vec3 region = preFlashPositions[i];
            float distToRegion = distance(sphericalUv, region.xy);
            if (distToRegion < region.z) {
                regionFlash += (1.0 - distToRegion / region.z) * preFlashIntensity;
            }
        }
        regionFlash = clamp(regionFlash, 0.0, 1.0);
        
        skyColor += vec3(0.4, 0.35, 0.3) * (lightningFlashIntensity + regionFlash * 0.8);
        FragColor = vec4(skyColor, 1.0);
        return;
    }
    
    vec3 skyColor;
    
    if (angle < 0.5f) {
        float t = angle / 0.5f;
        t = t * t;
        skyColor = mix(nightColor, dawnColor, t);
    }
    else if (angle < 1.0f) {
        float t = (angle - 0.5f) / 0.5f;
        t = t * t;
        skyColor = mix(dawnColor, dayColorBottom, t);
    }
    else if (angle < 2.2f) {
        skyColor = mix(dayColorBottom, dayColorTop, y);
    }
    else if (angle < 2.6f) {
        float t = (angle - 2.2f) / 0.4f;
        t = t * t;
        skyColor = mix(dayColorBottom, duskColor, t);
    }
    else {
        float t = (angle - 2.6f) / 0.54f;
        t = min(t, 1.0f);
        t = t * t;
        skyColor = mix(duskColor, nightColor, t);
    }
    
    if (angle > 1.0f && angle < 2.2f) {
        float gradient = y * 0.5f;
        skyColor = mix(skyColor, dayColorTop, gradient);
    }
    
    float horizonGlow = 0.0;
    if (angle < 0.6f) {
        horizonGlow = (0.6f - angle) / 0.6f * 0.05f;
    }
    if (angle > 2.5f) {
        horizonGlow = (angle - 2.5f) / 0.64f * 0.04f;
    }
    
    float glowFactor = max(0.0, 1.0f - y * 2.0f);
    skyColor += vec3(0.45, 0.25, 0.12) * horizonGlow * glowFactor;
    
    if (angle < 0.4f || angle > 2.9f) {
        float stars = 0.0;
        for (int i = 0; i < 4; i++) {
            stars += starPattern(sphericalUv, time, 1.0, float(i) * 0.2);
            stars += starPattern(sphericalUv * 1.5, time * 0.8, 0.6, float(i) * 0.15);
            stars += starPattern(sphericalUv * 2.2, time * 1.2, 0.4, float(i) * 0.1);
        }
        stars = stars * (1.0 - smoothstep(0.3, 0.5, angle));
        skyColor += vec3(0.85, 0.8, 0.7) * stars * 0.4;
    }
    
    float regionFlash = 0.0;
    for (int i = 0; i < preFlashCount; i++) {
        vec3 region = preFlashPositions[i];
        float distToRegion = distance(sphericalUv, region.xy);
        if (distToRegion < region.z) {
            regionFlash += (1.0 - distToRegion / region.z) * preFlashIntensity;
        }
    }
    regionFlash = clamp(regionFlash, 0.0, 1.0);
    
    skyColor += vec3(0.45, 0.4, 0.35) * (lightningFlashIntensity + regionFlash * 0.8);
    
    FragColor = vec4(skyColor, 1.0);
}
)";
}

namespace MoonShaders {
    const std::string vertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec2 TexCoord;
out vec3 WorldPos;
void main()
{
    TexCoord = aTexCoord;
    WorldPos = vec3(model * vec4(aPos, 1.0));
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

    const std::string fragmentShader = R"(
#version 330 core
in vec2 TexCoord;
in vec3 WorldPos;
out vec4 FragColor;
uniform sampler2D moonTexture;
uniform float moonIntensity;
uniform float time;
uniform vec3 cameraPos;
void main()
{
    vec4 texColor = texture(moonTexture, TexCoord);
    
    float glow = 0.0;
    float distToCenter = length(TexCoord - 0.5);
    if (distToCenter < 0.5) {
        glow = (1.0 - distToCenter * 1.5) * 0.3;
    }
    
    float brightness = moonIntensity * (0.9 + sin(time * 0.3) * 0.1);
    vec3 finalColor = texColor.rgb * brightness;
    finalColor += vec3(0.6, 0.55, 0.45) * glow * brightness;
    
    FragColor = vec4(finalColor, 1.0);
}
)";
}

glm::vec3 getSunColor(float sunAngle) {
    float t = sunAngle / M_PI;

    float phase;
    if (t < 0.5f) {
        phase = t * 2.0f; 
    }
    else {
        phase = (1.0f - t) * 2.0f;  
    }

    glm::vec3 dawnColor = glm::vec3(1.0f, 0.6f, 0.35f);
    glm::vec3 dayColor = glm::vec3(1.0f, 0.98f, 0.92f);
    glm::vec3 color = dawnColor * (1.0f - phase) + dayColor * phase;

    return color;
}

float getSunIntensity(float sunAngle) {
    return 1.0f;
}

void updateSun(float deltaTime) {
    if (!sunEnabled) return;

    sunCycleTime += deltaTime * sun.speed * sunSpeedMultiplier;
    if (sunCycleTime > M_PI) sunCycleTime -= M_PI;
    if (sunCycleTime < 0.0f) sunCycleTime += M_PI;

    sun.angle = sunCycleTime;

    float orbitRadius = std::max(mapWidth, mapHeight) * 0.7f;

    float x = -cos(sun.angle) * orbitRadius;
    float z = 0.0f;

    float minHeight = maxTerrainHeight - 120.0f;
    float maxHeight = maxTerrainHeight + 150.0f;
    float t = pow(sin(sun.angle), 1.3f);
    float y = minHeight + t * (maxHeight - minHeight);

    sun.position = glm::vec3(x + mapWidth / 2.0f, y, mapHeight / 2.0f);
    sun.intensity = 1.0f; 
}

void createSphereGeometry(unsigned int& VAO, unsigned int& VBO, unsigned int& EBO, int& indexCount, float radius) {
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords;
    std::vector<unsigned int> indices;

    int sectors = 64;
    int stacks = 64;

    for (int i = 0; i <= stacks; ++i) {
        float V = (float)i / (float)stacks;
        float phi = V * M_PI;

        for (int j = 0; j <= sectors; ++j) {
            float U = (float)j / (float)sectors;
            float theta = U * 2.0f * M_PI;

            float x = radius * sin(phi) * cos(theta);
            float y = radius * cos(phi);
            float z = radius * sin(phi) * sin(theta);

            vertices.push_back(glm::vec3(x, y, z));
            texCoords.push_back(glm::vec2(U, V));
        }
    }

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < sectors; ++j) {
            int first = i * (sectors + 1) + j;
            int second = first + sectors + 1;

            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);

            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }

    indexCount = indices.size();

    std::vector<float> interleavedData;
    for (size_t i = 0; i < vertices.size(); i++) {
        interleavedData.push_back(vertices[i].x);
        interleavedData.push_back(vertices[i].y);
        interleavedData.push_back(vertices[i].z);
        interleavedData.push_back(texCoords[i].x);
        interleavedData.push_back(texCoords[i].y);
    }

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, interleavedData.size() * sizeof(float), interleavedData.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void createSunGeometry() {
    createSphereGeometry(sunVAO, sunVBO, sunEBO, sunIndexCount, 1.0f);
    std::cout << "Sun sphere geometry created with " << sunIndexCount << " indices" << std::endl;
}

void createMoonGeometry() {
    createSphereGeometry(moonVAO, moonVBO, moonEBO, moonIndexCount, 1.0f);
    std::cout << "Moon sphere geometry created with " << moonIndexCount << " indices" << std::endl;
}

void createSkybox() {
    float skyboxVertices[] = {
        -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f, 1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
        1.0f, -1.0f, -1.0f, 1.0f, -1.0f,  1.0f, 1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f, 1.0f,  1.0f, -1.0f, 1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, 1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f, 1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
        -1.0f,  1.0f, -1.0f, 1.0f,  1.0f, -1.0f, 1.0f,  1.0f,  1.0f,
        1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, 1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, 1.0f, -1.0f,  1.0f
    };
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

namespace SunShaders {
    const std::string vertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
out vec2 TexCoord;
out vec3 WorldPos;
void main()
{
    TexCoord = aTexCoord;
    WorldPos = vec3(model * vec4(aPos, 1.0));
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

    const std::string fragmentShader = R"(
#version 330 core
in vec2 TexCoord;
in vec3 WorldPos;
out vec4 FragColor;
uniform float sunIntensity;
uniform vec3 sunColor;
uniform float time;
uniform vec3 cameraPos;

void main()
{
    vec2 centerVec = TexCoord - 0.5;
    float distToCenter = length(centerVec);
    
    float core = 1.0 - smoothstep(0.0, 0.6, distToCenter);
    core = pow(core, 0.5);

    float glow = 1.0 - smoothstep(0.0, 0.95, distToCenter);
    glow = pow(glow, 1.2);

    float halo = exp(-distToCenter * distToCenter * 4.0);
    halo = clamp(halo, 0.0, 1.0);

    vec3 finalColor = sunColor;

    float brightness = core * 0.9 + glow * 0.4 + halo * 0.3;
    brightness = clamp(brightness, 0.6, 1.2);
    
    finalColor = finalColor * brightness;

    finalColor = finalColor * 1.4;

    float pulse = 0.98 + sin(time * 2.5) * 0.02;
    finalColor *= pulse;

    finalColor *= sunIntensity;

    finalColor = max(finalColor, vec3(0.95, 0.8, 0.6));
    
    FragColor = vec4(finalColor, 1.0);
}
)";
}

void generateLightningPoints(std::vector<glm::vec3>& points, const glm::vec3& start, const glm::vec3& end, float displacement, float minDisplacement, std::mt19937& gen) {
    float length = glm::distance(start, end);
    if (length < 0.2f || displacement < minDisplacement) {
        points.push_back(end);
        return;
    }
    glm::vec3 mid = (start + end) * 0.5f;
    glm::vec3 dir = glm::normalize(end - start);
    glm::vec3 perp = glm::normalize(glm::cross(dir, glm::vec3(0, 1, 0)));
    if (std::abs(perp.y) > 0.9f) perp = glm::normalize(glm::cross(dir, glm::vec3(1, 0, 0)));
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    float offsetX = displacement * (dist(gen) * 2.0f - 1.0f);
    float offsetY = displacement * (dist(gen) * 2.0f - 1.0f) * 0.5f;
    mid += perp * offsetX;
    mid += glm::vec3(0.0f, offsetY, 0.0f);
    float newDisplacement = displacement * 0.55f;
    generateLightningPoints(points, start, mid, newDisplacement, minDisplacement, gen);
    generateLightningPoints(points, mid, end, newDisplacement, minDisplacement, gen);
}

void createSingleComplexLightning(float posX, float posZ) {
    std::vector<LightningVertex> vertices;
    std::vector<unsigned int> indices;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    std::uniform_real_distribution<float> branchChance(0.0f, 1.0f);
    float xOffset = dist(gen) * 2.0f;
    float zOffset = dist(gen) * 1.5f;
    glm::vec3 start(xOffset, 1.0f, zOffset);
    glm::vec3 end(xOffset * 0.4f, -1.0f, zOffset * 0.3f);
    std::vector<glm::vec3> mainPoints;
    mainPoints.push_back(start);
    float displacement = glm::distance(start, end) * 0.22f;
    generateLightningPoints(mainPoints, start, end, displacement, 0.15f, gen);
    struct BranchData { std::vector<glm::vec3> points; float width; float brightness; };
    std::vector<BranchData> allBranches;
    BranchData mainBranch;
    mainBranch.points = mainPoints;
    mainBranch.width = 0.11f;
    mainBranch.brightness = 1.0f;
    allBranches.push_back(mainBranch);
    for (size_t i = 2; i < mainPoints.size() - 2; i += 2) {
        if (branchChance(gen) > 0.55f) {
            glm::vec3 branchStart = mainPoints[i];
            glm::vec3 prev = mainPoints[i - 1];
            glm::vec3 next = mainPoints[i + 1];
            glm::vec3 mainDir = glm::normalize(next - prev);
            glm::vec3 perp = glm::normalize(glm::cross(mainDir, glm::vec3(0, 1, 0)));
            if (std::abs(perp.y) > 0.9f) perp = glm::normalize(glm::cross(mainDir, glm::vec3(1, 0, 0)));
            float angle = (dist(gen) * 1.0f + 0.5f);
            glm::vec3 branchDir = glm::normalize(mainDir * 0.5f + perp * angle);
            float branchLen = 0.5f + dist(gen) * 0.6f;
            glm::vec3 branchEnd = branchStart + branchDir * branchLen;
            std::vector<glm::vec3> branchPoints;
            branchPoints.push_back(branchStart);
            float branchDisp = branchLen * 0.18f;
            generateLightningPoints(branchPoints, branchStart, branchEnd, branchDisp, 0.1f, gen);
            if (branchPoints.size() >= 2) {
                BranchData branch;
                branch.points = branchPoints;
                branch.width = 0.06f;
                branch.brightness = 0.75f;
                allBranches.push_back(branch);
                if (branchChance(gen) > 0.6f && branchPoints.size() > 3) {
                    int subIdx = branchPoints.size() / 2;
                    glm::vec3 subStart = branchPoints[subIdx];
                    glm::vec3 subPrev = branchPoints[subIdx - 1];
                    glm::vec3 subNext = branchPoints[subIdx + 1];
                    glm::vec3 subDir = glm::normalize(subNext - subPrev);
                    glm::vec3 subPerp = glm::normalize(glm::cross(subDir, glm::vec3(0, 1, 0)));
                    float subAngle = dist(gen) * 1.2f;
                    glm::vec3 subBranchDir = glm::normalize(subDir * 0.6f + subPerp * subAngle);
                    float subLen = branchLen * (0.3f + dist(gen) * 0.3f);
                    glm::vec3 subEnd = subStart + subBranchDir * subLen;
                    std::vector<glm::vec3> subPoints;
                    subPoints.push_back(subStart);
                    generateLightningPoints(subPoints, subStart, subEnd, subLen * 0.15f, 0.08f, gen);
                    if (subPoints.size() >= 2) {
                        BranchData subBranch;
                        subBranch.points = subPoints;
                        subBranch.width = 0.035f;
                        subBranch.brightness = 0.55f;
                        allBranches.push_back(subBranch);
                    }
                }
            }
        }
    }
    for (const auto& branch : allBranches) {
        if (branch.points.size() < 2) continue;
        int startIdx = vertices.size();
        for (size_t i = 0; i < branch.points.size(); i++) {
            const glm::vec3& p = branch.points[i];
            float t = (float)i / (branch.points.size() - 1);
            float localWidth = branch.width * (0.5f + std::sin(t * M_PI) * 0.8f);
            if (i > 0 && i < branch.points.size() - 1) {
                glm::vec3 prev = branch.points[i - 1];
                glm::vec3 next = branch.points[i + 1];
                glm::vec3 dir1 = glm::normalize(p - prev);
                glm::vec3 dir2 = glm::normalize(next - p);
                float dot = glm::clamp(glm::dot(dir1, dir2), -1.0f, 1.0f);
                float angle = acos(dot);
                localWidth *= (1.0f + angle * 0.7f);
            }
            glm::vec3 dir(0, 0, 1);
            if (i < branch.points.size() - 1) dir = glm::normalize(branch.points[i + 1] - p);
            else if (i > 0) dir = glm::normalize(p - branch.points[i - 1]);
            glm::vec3 right = glm::normalize(glm::cross(dir, glm::vec3(0, 1, 0)));
            if (std::abs(right.x) < 0.1f && std::abs(right.z) < 0.1f) right = glm::vec3(1, 0, 0);
            float brightness = branch.brightness * (0.5f + std::sin(t * M_PI) * 0.6f);
            vertices.push_back(LightningVertex(p - right * localWidth, glm::vec2(0.0f, brightness)));
            vertices.push_back(LightningVertex(p + right * localWidth, glm::vec2(1.0f, brightness)));
            if (i > 0) {
                int baseIdx = startIdx + (i - 1) * 2;
                int currIdx = startIdx + i * 2;
                indices.push_back(baseIdx);
                indices.push_back(baseIdx + 1);
                indices.push_back(currIdx);
                indices.push_back(currIdx);
                indices.push_back(baseIdx + 1);
                indices.push_back(currIdx + 1);
            }
        }
    }
    lightningIndexCount = indices.size();
    if (lightningIndexCount > 0 && !vertices.empty()) {
        glBindBuffer(GL_ARRAY_BUFFER, lightningVBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(LightningVertex), vertices.data(), GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lightningEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
        std::cout << "Lightning created at (" << posX << ", " << posZ << ")" << std::endl;
    }
}

void updateLightning(float currentTime, int mapWidth, int mapHeight) {
    if (!lightningEnabled || !rainEnabled || rainfallDensity < 0.3f) {
        lightningFlash = false;
        flashIntensity = 0.0f;
        preFlashActive = false;
        preFlashIntensity = 0.0f;
        return;
    }

    if (!preFlashActive && !lightningFlash && currentTime - lastLightningTime > nextLightningDelay - 0.4f && nextLightningDelay > 0.4f) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> centerXDist(-0.7f, 0.7f);
        std::uniform_real_distribution<float> centerYDist(0.1f, 0.8f);
        std::uniform_real_distribution<float> radiusDist(0.15f, 0.35f);
        std::uniform_int_distribution<int> flashCountDist(2, 5);

        preFlashActive = true;
        preFlashDuration = 0.3f;
        preFlashIntensity = lightningIntensity * 0.5f;

        flashRegions.clear();
        int flashCount = flashCountDist(gen);
        for (int i = 0; i < flashCount; i++) {
            glm::vec3 region;
            region.x = centerXDist(gen);
            region.y = centerYDist(gen);
            region.z = radiusDist(gen);
            flashRegions.push_back(region);
        }

        std::cout << "Pre-flash lightning with " << flashCount << " regions in the sky!" << std::endl;
    }

    if (preFlashActive) {
        preFlashDuration -= 0.016f;
        if (preFlashDuration <= 0.0f) {
            preFlashActive = false;
            preFlashIntensity = 0.0f;
            flashRegions.clear();
        }
        else {
            preFlashIntensity = lightningIntensity * 0.5f * (0.5f + sin(currentTime * 25.0f) * 0.5f);
        }
    }

    if (!lightningFlash && !preFlashActive && currentTime - lastLightningTime > nextLightningDelay) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> posXDist(50.0f, mapWidth - 50.0f);
        std::uniform_real_distribution<float> posZDist(50.0f, mapHeight - 50.0f);
        std::uniform_real_distribution<float> delayDist(lightningFrequency * 0.7f, lightningFrequency * 1.5f);
        currentLightningPosX = posXDist(gen);
        currentLightningPosZ = posZDist(gen);
        createSingleComplexLightning(currentLightningPosX, currentLightningPosZ);
        lightningFlash = true;
        flashIntensity = lightningIntensity;
        lastLightningTime = currentTime;
        nextLightningDelay = delayDist(gen);
        boltDuration = 0.12f;
        std::cout << "Lightning strike at X=" << currentLightningPosX << ", Z=" << currentLightningPosZ << std::endl;
    }
    if (lightningFlash) {
        boltDuration -= 0.016f;
        if (boltDuration <= 0) {
            lightningFlash = false;
            flashIntensity = 0.0f;
            lightningIndexCount = 0;
        }
        else {
            flashIntensity = lightningIntensity * (0.5f + sin(currentTime * 200.0f) * 0.5f);
        }
    }
}

unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        std::cout << "Loaded texture: " << path << std::endl;
    }
    else {
        std::cerr << "Failed to load texture: " << path << std::endl;
        textureID = 0;
    }
    stbi_image_free(data);
    return textureID;
}

std::vector<glm::vec3> calculateNormals(const std::vector<glm::vec3>& vertices, const std::vector<unsigned int>& indices) {
    std::vector<glm::vec3> normals(vertices.size(), glm::vec3(0.0f));
    for (size_t i = 0; i < indices.size(); i += 3) {
        unsigned int i0 = indices[i];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];
        glm::vec3 v0 = vertices[i0];
        glm::vec3 v1 = vertices[i1];
        glm::vec3 v2 = vertices[i2];
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 triNormal = glm::normalize(glm::cross(edge1, edge2));
        normals[i0] += triNormal;
        normals[i1] += triNormal;
        normals[i2] += triNormal;
    }
    for (auto& n : normals) n = glm::normalize(n);
    return normals;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    windowWidth = width;
    windowHeight = height;
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;
    float sensitivity = 0.1f;
    xoffset *= sensitivity;
    yoffset *= sensitivity;
    yaw += xoffset;
    pitch += yoffset;
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_LEFT_CONTROL || key == GLFW_KEY_RIGHT_CONTROL) {
        if (action == GLFW_PRESS) ctrlPressed = true;
        else if (action == GLFW_RELEASE) ctrlPressed = false;
    }
    if (key == GLFW_KEY_1 && action == GLFW_PRESS && ctrlPressed) toggleSnowRequest = true;
    if (key == GLFW_KEY_2 && action == GLFW_PRESS && ctrlPressed) toggleRainRequest = true;
    if (key == GLFW_KEY_3 && action == GLFW_PRESS && ctrlPressed) toggleFogRequest = true;
    if (key == GLFW_KEY_5 && action == GLFW_PRESS && ctrlPressed) toggleLightningRequest = true;
    if (key == GLFW_KEY_6 && action == GLFW_PRESS && ctrlPressed) toggleSunRequest = true;
    if (key == GLFW_KEY_7 && action == GLFW_PRESS && ctrlPressed) toggleStarsOnlyRequest = true;
    if (sunEnabled) {
        if (key == GLFW_KEY_KP_8 && action == GLFW_PRESS) { sunSpeedMultiplier *= 1.5f; if (sunSpeedMultiplier > 10.0f) sunSpeedMultiplier = 10.0f; std::cout << "Sun speed: " << sunSpeedMultiplier << "x" << std::endl; }
        if (key == GLFW_KEY_KP_2 && action == GLFW_PRESS) { sunSpeedMultiplier /= 1.5f; if (sunSpeedMultiplier < 0.1f) sunSpeedMultiplier = 0.1f; std::cout << "Sun speed: " << sunSpeedMultiplier << "x" << std::endl; }
        if (key == GLFW_KEY_KP_4 && action == GLFW_PRESS) { sunCycleTime -= 0.05f; if (sunCycleTime < 0.0f) sunCycleTime += M_PI; std::cout << "Sun angle: " << (sunCycleTime / M_PI * 100.0f) << "%" << std::endl; }
        if (key == GLFW_KEY_KP_6 && action == GLFW_PRESS) { sunCycleTime += 0.05f; if (sunCycleTime > M_PI) sunCycleTime -= M_PI; std::cout << "Sun angle: " << (sunCycleTime / M_PI * 100.0f) << "%" << std::endl; }
        if (key == GLFW_KEY_KP_5 && action == GLFW_PRESS) { sunSpeedMultiplier = 1.0f; std::cout << "Sun speed reset" << std::endl; }
    }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS) { textureScale = glm::max(textureScale - 0.01f, 0.02f); std::cout << "Texture scale: " << textureScale << std::endl; }
    if (key == GLFW_KEY_X && action == GLFW_PRESS) { textureScale = glm::min(textureScale + 0.01f, 0.5f); std::cout << "Texture scale: " << textureScale << std::endl; }
    if (key == GLFW_KEY_KP_9 && action == GLFW_PRESS) { lightningIntensity = glm::min(lightningIntensity + 0.1f, 1.0f); std::cout << "Lightning intensity: " << lightningIntensity << std::endl; }
    if (key == GLFW_KEY_KP_7 && action == GLFW_PRESS) { lightningIntensity = glm::max(lightningIntensity - 0.1f, 0.1f); std::cout << "Lightning intensity: " << lightningIntensity << std::endl; }
    if (key == GLFW_KEY_KP_MULTIPLY && action == GLFW_PRESS) { lightningFrequency = glm::max(lightningFrequency - 0.5f, 1.5f); std::cout << "Lightning frequency: every " << lightningFrequency << " sec" << std::endl; }
    if (key == GLFW_KEY_KP_DIVIDE && action == GLFW_PRESS) { lightningFrequency = glm::min(lightningFrequency + 0.5f, 10.0f); std::cout << "Lightning frequency: every " << lightningFrequency << " sec" << std::endl; }
    if (snowEnabled) {
        if (key == GLFW_KEY_UP && action == GLFW_PRESS) { snowfallDensity = glm::min(snowfallDensity + 0.1f, 1.0f); std::cout << "Snow density: " << snowfallDensity << std::endl; }
        if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) { snowfallDensity = glm::max(snowfallDensity - 0.1f, 0.1f); std::cout << "Snow density: " << snowfallDensity << std::endl; }
    }
    if (rainEnabled) {
        if (key == GLFW_KEY_UP && action == GLFW_PRESS) { rainfallDensity = glm::min(rainfallDensity + 0.1f, 1.0f); std::cout << "Rain density: " << rainfallDensity << std::endl; }
        if (key == GLFW_KEY_DOWN && action == GLFW_PRESS) { rainfallDensity = glm::max(rainfallDensity - 0.1f, 0.1f); std::cout << "Rain density: " << rainfallDensity << std::endl; }
    }
    if (key == GLFW_KEY_LEFT_BRACKET && action == GLFW_PRESS) { fogDensity = glm::max(fogDensity - 0.001f, 0.001f); std::cout << "Fog density: " << fogDensity << std::endl; }
    if (key == GLFW_KEY_RIGHT_BRACKET && action == GLFW_PRESS) { fogDensity = glm::min(fogDensity + 0.001f, 0.05f); std::cout << "Fog density: " << fogDensity << std::endl; }

    if (key == GLFW_KEY_F12 && action == GLFW_PRESS) {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        if (glfwGetWindowMonitor(window) == nullptr) {
            glfwSetWindowMonitor(window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
        }
        else {
            glfwSetWindowMonitor(window, nullptr, 100, 100, 1280, 720, mode->refreshRate);
        }
    }
}

void processInput(GLFWwindow* window, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) glfwSetWindowShouldClose(window, true);
    float cameraSpeedCurrent = cameraSpeed * deltaTime * 50.0f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) cameraPos += cameraSpeedCurrent * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) cameraPos -= cameraSpeedCurrent * cameraFront;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeedCurrent;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeedCurrent;
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) cameraPos += cameraSpeedCurrent * cameraUp;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) cameraPos -= cameraSpeedCurrent * cameraUp;
}

unsigned int createShaderProgram(const std::string& vertexShader, const std::string& fragmentShader) {
    unsigned int vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    const char* vertexSource = vertexShader.c_str();
    glShaderSource(vertexShaderID, 1, &vertexSource, NULL);
    glCompileShader(vertexShaderID);
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &success);
    if (!success) { glGetShaderInfoLog(vertexShaderID, 512, NULL, infoLog); std::cerr << "ERROR::VERTEX_SHADER\n" << infoLog << std::endl; return 0; }
    unsigned int fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fragmentSource = fragmentShader.c_str();
    glShaderSource(fragmentShaderID, 1, &fragmentSource, NULL);
    glCompileShader(fragmentShaderID);
    glGetShaderiv(fragmentShaderID, GL_COMPILE_STATUS, &success);
    if (!success) { glGetShaderInfoLog(fragmentShaderID, 512, NULL, infoLog); std::cerr << "ERROR::FRAGMENT_SHADER\n" << infoLog << std::endl; return 0; }
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShaderID);
    glAttachShader(shaderProgram, fragmentShaderID);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) { glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog); std::cerr << "ERROR::PROGRAM_LINKING\n" << infoLog << std::endl; return 0; }
    glDeleteShader(vertexShaderID);
    glDeleteShader(fragmentShaderID);
    return shaderProgram;
}

std::vector<Snowflake> initializeSnowflakes(int count, int mapWidth, int mapHeight) {
    std::vector<Snowflake> snowflakes(count);
    std::random_device rd;
    std::mt19937 gen(rd());
    float snowStartHeight = maxTerrainHeight + 50.0f;
    float snowEndHeight = maxTerrainHeight + 200.0f;
    std::uniform_real_distribution<> posX(-mapWidth * 0.5f, mapWidth * 1.5f);
    std::uniform_real_distribution<> posZ(-mapHeight * 0.5f, mapHeight * 1.5f);
    std::uniform_real_distribution<> posY(snowStartHeight, snowEndHeight);
    std::uniform_real_distribution<> sizeDist(1.2f, 2.0f);
    std::uniform_real_distribution<> fallSpeedDist(20.0f, 40.0f);
    std::uniform_real_distribution<> offsetDist(0, 1000);
    std::uniform_real_distribution<> rotationDist(0, 6.28318f);
    std::uniform_real_distribution<> rotationSpeedDist(1.0f, 3.0f);
    for (int i = 0; i < count; ++i) {
        snowflakes[i] = { glm::vec3((float)posX(gen), (float)posY(gen), (float)posZ(gen)), (float)sizeDist(gen), (float)fallSpeedDist(gen), (float)offsetDist(gen), (float)rotationDist(gen), (float)rotationSpeedDist(gen) };
    }
    return snowflakes;
}

std::vector<Raindrop> initializeRaindrops(int count, int mapWidth, int mapHeight) {
    std::vector<Raindrop> raindrops(count);
    std::random_device rd;
    std::mt19937 gen(rd());
    float rainStartHeight = maxTerrainHeight + 100.0f;
    float rainEndHeight = maxTerrainHeight + 300.0f;
    std::uniform_real_distribution<> posX(-mapWidth * 0.5f, mapWidth * 1.5f);
    std::uniform_real_distribution<> posZ(-mapHeight * 0.5f, mapHeight * 1.5f);
    std::uniform_real_distribution<> posY(rainStartHeight, rainEndHeight);
    std::uniform_real_distribution<> sizeDist(0.3f, 0.7f);
    std::uniform_real_distribution<> fallSpeedDist(80.0f, 150.0f);
    std::uniform_real_distribution<> trailDist(1.5f, 3.0f);
    std::uniform_real_distribution<> velXDist(-0.5f, 0.5f);
    std::uniform_real_distribution<> velZDist(-0.5f, 0.5f);
    for (int i = 0; i < count; ++i) {
        raindrops[i] = { glm::vec3((float)posX(gen), (float)posY(gen), (float)posZ(gen)), (float)sizeDist(gen), (float)fallSpeedDist(gen), (float)trailDist(gen), glm::vec3((float)velXDist(gen), -1.0f, (float)velZDist(gen)) };
    }
    return raindrops;
}

void updateSnowflakes(std::vector<Snowflake>& snowflakes, float deltaTime, const glm::vec3& windDir, float windStr, int mapWidth, int mapHeight, float density) {
    for (auto& snowflake : snowflakes) {
        if (density < 1.0f && fmod(snowflake.swayOffset, 1.0f) > density) continue;
        snowflake.position.y -= snowflake.fallSpeed * deltaTime;
        snowflake.position.x += windDir.x * windStr * deltaTime * 2.0f;
        snowflake.position.z += windDir.z * windStr * deltaTime * 2.0f;
        float swayX = sin(glfwGetTime() * 1.5f + snowflake.swayOffset) * 0.1f;
        float swayZ = cos(glfwGetTime() * 1.2f + snowflake.swayOffset * 1.3f) * 0.1f;
        snowflake.position.x += swayX;
        snowflake.position.z += swayZ;
        snowflake.rotation += snowflake.rotationSpeed * deltaTime;
        if (snowflake.rotation > 6.28318f) snowflake.rotation -= 6.28318f;
        if (snowflake.position.y < -20.0f) {
            snowflake.position.y = maxTerrainHeight + 150.0f + fmod(snowflake.swayOffset, 100.0f);
            snowflake.position.x = fmod(snowflake.position.x + mapWidth * 2.0f, mapWidth * 2.0f) - mapWidth * 0.5f;
            snowflake.position.z = fmod(snowflake.position.z + mapHeight * 2.0f, mapHeight * 2.0f) - mapHeight * 0.5f;
        }
    }
}

void updateRaindrops(std::vector<Raindrop>& raindrops, float deltaTime, const glm::vec3& windDir, float windStr, int mapWidth, int mapHeight, float density) {
    for (auto& raindrop : raindrops) {
        if (density < 1.0f && fmod(raindrop.position.x + raindrop.position.z, 10.0f) > density * 10.0f) continue;
        raindrop.position.y -= raindrop.fallSpeed * deltaTime;
        raindrop.position.x += (windDir.x * windStr + raindrop.velocity.x) * deltaTime * 5.0f;
        raindrop.position.z += (windDir.z * windStr + raindrop.velocity.z) * deltaTime * 5.0f;
        float jitterX = sin(glfwGetTime() * 10.0f + raindrop.position.x) * 0.02f;
        float jitterZ = cos(glfwGetTime() * 8.0f + raindrop.position.z) * 0.02f;
        raindrop.position.x += jitterX;
        raindrop.position.z += jitterZ;
        if (raindrop.position.y < -20.0f) {
            raindrop.position.y = maxTerrainHeight + 200.0f + fmod(raindrop.position.x + raindrop.position.z, 100.0f);
            raindrop.position.x = fmod(raindrop.position.x + mapWidth * 2.0f, mapWidth * 2.0f) - mapWidth * 0.5f;
            raindrop.position.z = fmod(raindrop.position.z + mapHeight * 2.0f, mapHeight * 2.0f) - mapHeight * 0.5f;
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_real_distribution<> velXDist(-0.5f, 0.5f);
            std::uniform_real_distribution<> velZDist(-0.5f, 0.5f);
            raindrop.velocity.x = (float)velXDist(gen);
            raindrop.velocity.z = (float)velZDist(gen);
        }
    }
}

int main() {
    if (!glfwInit()) { std::cerr << "Failed to initialize GLFW" << std::endl; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

    GLFWwindow* window = glfwCreateWindow(mode->width, mode->height, "Mountain Landscape - Ctrl+1: Snow | Ctrl+2: Rain | Ctrl+3: Fog | Ctrl+5: Lightning | Ctrl+6: Sun | Ctrl+7: Stars Only | F12: Windowed", primaryMonitor, nullptr);
    if (!window) {
        window = glfwCreateWindow(1280, 720, "Mountain Landscape - Ctrl+1: Snow | Ctrl+2: Rain | Ctrl+3: Fog | Ctrl+5: Lightning | Ctrl+6: Sun | Ctrl+7: Stars Only | F12: Windowed", nullptr, nullptr);
        if (!window) { std::cerr << "Failed to create GLFW window" << std::endl; glfwTerminate(); return -1; }
    }

    windowWidth = mode->width;
    windowHeight = mode->height;

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { std::cerr << "Failed to initialize GLAD" << std::endl; return -1; }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_PROGRAM_POINT_SIZE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    unsigned int terrainShader = createShaderProgram(TerrainShaders::vertexShader, TerrainShaders::fragmentShader);
    if (terrainShader == 0) return -1;
    unsigned int snowShader = createShaderProgram(SnowShaders::vertexShader, SnowShaders::fragmentShader);
    if (snowShader == 0) return -1;
    unsigned int rainShader = createShaderProgram(RainShaders::vertexShader, RainShaders::fragmentShader);
    if (rainShader == 0) return -1;
    unsigned int fogShader = createShaderProgram(FogShaders::vertexShader, FogShaders::fragmentShader);
    if (fogShader == 0) return -1;
    lightningShader = createShaderProgram(LightningShaders::vertexShader, LightningShaders::fragmentShader);
    if (lightningShader == 0) return -1;
    sunShader = createShaderProgram(SunShaders::vertexShader, SunShaders::fragmentShader);
    if (sunShader == 0) return -1;
    skyboxShader = createShaderProgram(SkyboxShaders::vertexShader, SkyboxShaders::fragmentShader);
    if (skyboxShader == 0) return -1;

    unsigned int moonShader = createShaderProgram(MoonShaders::vertexShader, MoonShaders::fragmentShader);
    if (moonShader == 0) return -1;
    createMoonGeometry();

    std::cout << "\n=== Loading Textures ===" << std::endl;
    grassTexture = loadTexture("grass.png");
    rockTexture = loadTexture("rock.png");
    sandTexture = loadTexture("sand.png");
    waterTexture = loadTexture("water.png");
    snowTexture = loadTexture("snow.png");
    moonTexture = loadTexture("moon.jpg");

    if (moonTexture == 0) {
        std::cerr << "Warning: moon.jpg not found, moon will not be visible" << std::endl;
    }

    if (grassTexture == 0) {
        unsigned char dummyData[4] = { 100, 150, 100, 255 };
        glGenTextures(1, &grassTexture);
        glBindTexture(GL_TEXTURE_2D, grassTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, dummyData);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
    if (rockTexture == 0) rockTexture = grassTexture;
    if (sandTexture == 0) sandTexture = grassTexture;
    if (waterTexture == 0) waterTexture = grassTexture;
    if (snowTexture == 0) snowTexture = grassTexture;
    std::cout << "========================\n" << std::endl;
    int mapChannels;
    unsigned char* image = stbi_load("map.png", &mapWidth, &mapHeight, &mapChannels, 1);
    if (!image) { std::cerr << "Failed to load heightmap" << std::endl; return -1; }
    std::vector<float> heights(mapWidth * mapHeight);
    float heightScale = 100.0f;
    maxTerrainHeight = 0.0f;
    for (int y = 0; y < mapHeight; ++y) {
        for (int x = 0; x < mapWidth; ++x) {
            heights[y * mapWidth + x] = image[y * mapWidth + x] / 255.0f;
            float h = heights[y * mapWidth + x] * heightScale;
            if (h > maxTerrainHeight) maxTerrainHeight = h;
        }
    }
    stbi_image_free(image);
    std::cout << "\n=== Terrain Bounds ===" << std::endl;
    std::cout << "Map width: " << mapWidth << ", Map height: " << mapHeight << std::endl;
    std::cout << "Max terrain height: " << maxTerrainHeight << std::endl;
    std::cout << "=====================\n" << std::endl;
    sun.speed = M_PI / sunCycleDuration;
    sun.angle = M_PI / 4.0f;
    sunCycleTime = M_PI / 4.0f;
    sun.enabled = false;
    sun.intensity = 1.0f;
    createSunGeometry();
    createSkybox();
    glUseProgram(terrainShader);
    glUniform1f(glGetUniformLocation(terrainShader, "maxTerrainHeight"), maxTerrainHeight);
    glUniform1i(glGetUniformLocation(terrainShader, "rainEnabled"), 0);
    glUniform1i(glGetUniformLocation(terrainShader, "grassTexture"), 0);
    glUniform1i(glGetUniformLocation(terrainShader, "rockTexture"), 1);
    glUniform1i(glGetUniformLocation(terrainShader, "sandTexture"), 2);
    glUniform1i(glGetUniformLocation(terrainShader, "waterTexture"), 3);
    glUniform1i(glGetUniformLocation(terrainShader, "snowTexture"), 4);
    glUniform1f(glGetUniformLocation(terrainShader, "textureScale"), textureScale);
    glUseProgram(snowShader);
    glUniform1f(glGetUniformLocation(snowShader, "maxTerrainHeight"), maxTerrainHeight);
    glUseProgram(rainShader);
    glUniform1f(glGetUniformLocation(rainShader, "maxTerrainHeight"), maxTerrainHeight);
    glUseProgram(0);
    std::vector<glm::vec3> vertices;
    for (int y = 0; y < mapHeight; ++y) {
        for (int x = 0; x < mapWidth; ++x) {
            float heightValue = heights[y * mapWidth + x];
            vertices.push_back(glm::vec3(x, heightValue * heightScale, y));
        }
    }
    std::vector<unsigned int> indices;
    for (int y = 0; y < mapHeight - 1; ++y) {
        for (int x = 0; x < mapWidth - 1; ++x) {
            int topLeft = y * mapWidth + x;
            int topRight = topLeft + 1;
            int bottomLeft = (y + 1) * mapWidth + x;
            int bottomRight = bottomLeft + 1;
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
    std::vector<glm::vec3> normals = calculateNormals(vertices, indices);
    unsigned int terrainVAO, terrainVBO, terrainEBO, terrainNBO;
    glGenVertexArrays(1, &terrainVAO);
    glGenBuffers(1, &terrainVBO);
    glGenBuffers(1, &terrainEBO);
    glGenBuffers(1, &terrainNBO);
    glBindVertexArray(terrainVAO);
    glBindBuffer(GL_ARRAY_BUFFER, terrainVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), vertices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, terrainNBO);
    glBufferData(GL_ARRAY_BUFFER, normals.size() * sizeof(glm::vec3), normals.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrainEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glBindVertexArray(0);
    const int SNOWFLAKE_COUNT = 8000;
    auto snowflakes = initializeSnowflakes(SNOWFLAKE_COUNT, mapWidth, mapHeight);
    const int RAINDROP_COUNT = 15000;
    auto raindrops = initializeRaindrops(RAINDROP_COUNT, mapWidth, mapHeight);
    unsigned int snowVAO, snowVBO;
    glGenVertexArrays(1, &snowVAO);
    glGenBuffers(1, &snowVBO);
    glBindVertexArray(snowVAO);
    std::vector<float> snowData;
    for (const auto& flake : snowflakes) {
        snowData.push_back(flake.position.x);
        snowData.push_back(flake.position.y);
        snowData.push_back(flake.position.z);
        snowData.push_back(flake.size);
        snowData.push_back(flake.fallSpeed);
        snowData.push_back(flake.swayOffset);
        snowData.push_back(flake.rotation);
        snowData.push_back(flake.rotationSpeed);
    }
    glBindBuffer(GL_ARRAY_BUFFER, snowVBO);
    glBufferData(GL_ARRAY_BUFFER, snowData.size() * sizeof(float), snowData.data(), GL_STREAM_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(4 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(7 * sizeof(float)));
    glEnableVertexAttribArray(5);
    glBindVertexArray(0);
    unsigned int rainVAO, rainVBO;
    glGenVertexArrays(1, &rainVAO);
    glGenBuffers(1, &rainVBO);
    glBindVertexArray(rainVAO);
    std::vector<float> rainData;
    for (const auto& drop : raindrops) {
        rainData.push_back(drop.position.x);
        rainData.push_back(drop.position.y);
        rainData.push_back(drop.position.z);
        rainData.push_back(drop.size);
        rainData.push_back(drop.fallSpeed);
        rainData.push_back(drop.trailLength);
        rainData.push_back(drop.velocity.x);
        rainData.push_back(drop.velocity.y);
        rainData.push_back(drop.velocity.z);
    }
    glBindBuffer(GL_ARRAY_BUFFER, rainVBO);
    glBufferData(GL_ARRAY_BUFFER, rainData.size() * sizeof(float), rainData.data(), GL_STREAM_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(4 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(5 * sizeof(float)));
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(4);
    glBindVertexArray(0);
    glGenVertexArrays(1, &lightningVAO);
    glGenBuffers(1, &lightningVBO);
    glGenBuffers(1, &lightningEBO);
    glBindVertexArray(lightningVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lightningVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(LightningVertex) * 10000, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, lightningEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(unsigned int) * 20000, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(LightningVertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(LightningVertex), (void*)sizeof(glm::vec3));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    lightningIndexCount = 0;
    std::cout << "Lightning system initialized with region-based pre-flash effects!\n" << std::endl;
    unsigned int framebuffer;
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    unsigned int screenTexture;
    glGenTextures(1, &screenTexture);
    glBindTexture(GL_TEXTURE_2D, screenTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, windowWidth, windowHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, screenTexture, 0);
    unsigned int rbo;
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, windowWidth, windowHeight);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) std::cout << "ERROR::FRAMEBUFFER not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    float quadVertices[] = { -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, -1.0f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f };
    unsigned int quadVAO, quadVBO;
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    glm::vec3 lightPos(mapWidth / 2.0f, maxTerrainHeight + 150.0f, mapHeight / 2.0f);
    glm::vec3 lightColor(1.0f, 0.95f, 0.85f);
    float lastTime = glfwGetTime();
    float deltaTime = 0.0f;
    lastLightningTime = glfwGetTime();
    nextLightningDelay = lightningFrequency;
    std::cout << "\n=== Mountain Landscape Started ===\n";
    std::cout << "Controls: WASD+Space/Shift | Mouse | Ctrl+1-7 | KP8/2/4/6/5/7/9/*//+/- | Z/X | [ ] | F12: Toggle Fullscreen\n";
    std::cout << "  Ctrl+1 - Snow | Ctrl+2 - Rain | Ctrl+3 - Fog | Ctrl+5 - Lightning | Ctrl+6 - Sun | Ctrl+7 - Stars Only Mode\n";
    std::cout << "  Moon appears only in Stars Only Mode (Ctrl+7) as a 3D sphere in the sky!\n";

    moonPosition = glm::vec3(mapWidth / 2.0f, maxTerrainHeight + 150.0f, mapHeight / 2.0f);

    moonPosition.x += 80.0f;
    moonPosition.y += 80.0f;
    moonPosition.z -= 50.0f;

    while (!glfwWindowShouldClose(window)) {
        float currentTime = glfwGetTime();
        deltaTime = currentTime - lastTime;
        if (deltaTime > 0.1f) deltaTime = 0.1f;
        lastTime = currentTime;
        processInput(window, deltaTime);

        if (toggleSnowRequest) {
            if (rainEnabled) std::cout << "Cannot enable snow while rain is active" << std::endl;
            else {
                snowEnabled = !snowEnabled;
                if (snowEnabled) starsOnlyMode = false;
                std::cout << "Snow " << (snowEnabled ? "ENABLED" : "DISABLED") << std::endl;
                if (snowEnabled) {
                    std::cout << "  Snowflakes are now larger and more visible against darker sky!" << std::endl;
                }
            }
            toggleSnowRequest = false;
        }
        if (toggleRainRequest) {
            if (snowEnabled) std::cout << "Cannot enable rain while snow is active" << std::endl;
            else {
                rainEnabled = !rainEnabled;
                if (rainEnabled) starsOnlyMode = false;
                std::cout << "Rain " << (rainEnabled ? "ENABLED" : "DISABLED") << std::endl;
                if (!rainEnabled) { lightningFlash = false; flashIntensity = 0.0f; preFlashActive = false; preFlashIntensity = 0.0f; }
            }
            toggleRainRequest = false;
        }
        if (toggleFogRequest) { fogEnabled = !fogEnabled; std::cout << "Fog " << (fogEnabled ? "ENABLED" : "DISABLED") << std::endl; toggleFogRequest = false; }
        if (toggleLightningRequest) {
            if (rainEnabled) { lightningEnabled = !lightningEnabled; std::cout << "Lightning " << (lightningEnabled ? "ENABLED" : "DISABLED") << std::endl; if (!lightningEnabled) { lightningFlash = false; flashIntensity = 0.0f; preFlashActive = false; preFlashIntensity = 0.0f; } }
            else { std::cout << "Lightning requires rain first (Ctrl+2)" << std::endl; }
            toggleLightningRequest = false;
        }
        if (toggleSunRequest) {
            sunEnabled = !sunEnabled;
            if (sunEnabled) starsOnlyMode = false;
            std::cout << "Sun " << (sunEnabled ? "ENABLED" : "DISABLED") << std::endl;
            if (sunEnabled) {
                updateSun(0.0f);
            }
            toggleSunRequest = false;
        }
        if (toggleStarsOnlyRequest) {
            starsOnlyMode = !starsOnlyMode;
            if (starsOnlyMode) {
                snowEnabled = false;
                rainEnabled = false;
                sunEnabled = false;
                lightningEnabled = false;
                fogEnabled = false;
                std::cout << "Stars Only Mode " << (starsOnlyMode ? "ENABLED" : "DISABLED") << std::endl;
                std::cout << "  Weather effects disabled. Enjoy the starry sky with the 3D moon!" << std::endl;
            }
            else {
                std::cout << "Stars Only Mode DISABLED" << std::endl;
            }
            toggleStarsOnlyRequest = false;
        }

        if (snowEnabled && !starsOnlyMode) updateSnowflakes(snowflakes, deltaTime, snowWindDirection, snowWindStrength, mapWidth, mapHeight, snowfallDensity);
        if (rainEnabled && !starsOnlyMode) updateRaindrops(raindrops, deltaTime, rainWindDirection, rainWindStrength, mapWidth, mapHeight, rainfallDensity);
        updateLightning(currentTime, mapWidth, mapHeight);
        updateSun(deltaTime);

        float currentSunIntensity = 1.0f;

        if (starsOnlyMode) {
            lightPos = glm::vec3(mapWidth / 2.0f, maxTerrainHeight + 150.0f, mapHeight / 2.0f);
            lightColor = glm::vec3(0.25f, 0.28f, 0.35f);
            currentSunIntensity = 0.35f;
        }
        else if (sunEnabled) {
            lightPos = sun.position;
            lightColor = getSunColor(sun.angle);
            currentSunIntensity = 1.0f;
        }
        else {
            lightPos = glm::vec3(mapWidth / 2.0f, maxTerrainHeight + 150.0f, mapHeight / 2.0f);
            if (snowEnabled) {
                lightColor = glm::vec3(0.55f, 0.58f, 0.65f);
                currentSunIntensity = 0.6f;
            }
            else if (rainEnabled) {
                lightColor = glm::vec3(0.70f, 0.68f, 0.65f);
                currentSunIntensity = 0.75f;
            }
            else {
                lightColor = glm::vec3(1.0f, 0.95f, 0.85f);
                currentSunIntensity = 1.0f;
            }
        }

        if (snowEnabled && !starsOnlyMode) {
            std::vector<float> updatedSnowData;
            for (const auto& flake : snowflakes) {
                updatedSnowData.push_back(flake.position.x); updatedSnowData.push_back(flake.position.y); updatedSnowData.push_back(flake.position.z);
                updatedSnowData.push_back(flake.size); updatedSnowData.push_back(flake.fallSpeed); updatedSnowData.push_back(flake.swayOffset);
                updatedSnowData.push_back(flake.rotation); updatedSnowData.push_back(flake.rotationSpeed);
            }
            glBindBuffer(GL_ARRAY_BUFFER, snowVBO); glBufferSubData(GL_ARRAY_BUFFER, 0, updatedSnowData.size() * sizeof(float), updatedSnowData.data());
        }
        if (rainEnabled && !starsOnlyMode) {
            std::vector<float> updatedRainData;
            for (const auto& drop : raindrops) {
                updatedRainData.push_back(drop.position.x); updatedRainData.push_back(drop.position.y); updatedRainData.push_back(drop.position.z);
                updatedRainData.push_back(drop.size); updatedRainData.push_back(drop.fallSpeed); updatedRainData.push_back(drop.trailLength);
                updatedRainData.push_back(drop.velocity.x); updatedRainData.push_back(drop.velocity.y); updatedRainData.push_back(drop.velocity.z);
            }
            glBindBuffer(GL_ARRAY_BUFFER, rainVBO); glBufferSubData(GL_ARRAY_BUFFER, 0, updatedRainData.size() * sizeof(float), updatedRainData.data());
        }
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glm::mat4 model = glm::mat4(1.0f);
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)windowWidth / (float)windowHeight, 0.1f, 2000.0f);
        glDepthFunc(GL_LEQUAL);
        glUseProgram(skyboxShader);
        glm::mat4 skyboxView = glm::mat4(glm::mat3(view));
        glUniformMatrix4fv(glGetUniformLocation(skyboxShader, "view"), 1, GL_FALSE, glm::value_ptr(skyboxView));
        glUniformMatrix4fv(glGetUniformLocation(skyboxShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform1f(glGetUniformLocation(skyboxShader, "time"), currentTime);
        glUniform1f(glGetUniformLocation(skyboxShader, "sunIntensity"), currentSunIntensity);
        glUniform1f(glGetUniformLocation(skyboxShader, "sunAngle"), sun.angle);
        glUniform1i(glGetUniformLocation(skyboxShader, "isRaining"), rainEnabled ? 1 : 0);
        glUniform1i(glGetUniformLocation(skyboxShader, "isSnowing"), snowEnabled ? 1 : 0);
        glUniform1i(glGetUniformLocation(skyboxShader, "starsOnlyMode"), starsOnlyMode ? 1 : 0);
        glUniform1f(glGetUniformLocation(skyboxShader, "lightningFlashIntensity"), flashIntensity);
        glUniform1f(glGetUniformLocation(skyboxShader, "preFlashIntensity"), preFlashIntensity);

        glUniform1i(glGetUniformLocation(skyboxShader, "preFlashCount"), (int)flashRegions.size());
        for (size_t i = 0; i < flashRegions.size() && i < 8; i++) {
            std::string uniformName = "preFlashPositions[" + std::to_string(i) + "]";
            glUniform3fv(glGetUniformLocation(skyboxShader, uniformName.c_str()), 1, glm::value_ptr(flashRegions[i]));
        }

        glBindVertexArray(skyboxVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glDepthFunc(GL_LESS);

        if (moonTexture != 0 && starsOnlyMode) {
            glUseProgram(moonShader);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_DEPTH_TEST);

            glm::mat4 moonModel = glm::mat4(1.0f);
            moonModel = glm::translate(moonModel, moonPosition);
            float moonScale = 20.0f;
            moonModel = glm::scale(moonModel, glm::vec3(moonScale));

            glUniformMatrix4fv(glGetUniformLocation(moonShader, "model"), 1, GL_FALSE, glm::value_ptr(moonModel));
            glUniformMatrix4fv(glGetUniformLocation(moonShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(moonShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

            float moonIntensity = 1.0f;
            glUniform1f(glGetUniformLocation(moonShader, "moonIntensity"), moonIntensity);
            glUniform1f(glGetUniformLocation(moonShader, "time"), currentTime);
            glUniform3fv(glGetUniformLocation(moonShader, "cameraPos"), 1, glm::value_ptr(cameraPos));

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, moonTexture);
            glUniform1i(glGetUniformLocation(moonShader, "moonTexture"), 0);

            glBindVertexArray(moonVAO);
            glDrawElements(GL_TRIANGLES, moonIndexCount, GL_UNSIGNED_INT, 0);

            glUseProgram(0);
        }

        glUseProgram(terrainShader);
        glUniform1f(glGetUniformLocation(terrainShader, "maxTerrainHeight"), maxTerrainHeight);
        glUniform3fv(glGetUniformLocation(terrainShader, "lightPos"), 1, glm::value_ptr(lightPos));
        glUniform3fv(glGetUniformLocation(terrainShader, "lightColor"), 1, glm::value_ptr(lightColor));
        glUniform3fv(glGetUniformLocation(terrainShader, "viewPos"), 1, glm::value_ptr(cameraPos));
        glUniform1i(glGetUniformLocation(terrainShader, "snowEnabled"), snowEnabled ? 1 : 0);
        glUniform1i(glGetUniformLocation(terrainShader, "rainEnabled"), rainEnabled ? 1 : 0);
        glUniform1f(glGetUniformLocation(terrainShader, "snowfallDensity"), snowfallDensity);
        glUniform1f(glGetUniformLocation(terrainShader, "rainfallDensity"), rainfallDensity);
        glUniform1f(glGetUniformLocation(terrainShader, "time"), currentTime);
        glUniform1f(glGetUniformLocation(terrainShader, "textureScale"), textureScale);
        glUniform1f(glGetUniformLocation(terrainShader, "lightningFlash"), flashIntensity);
        glUniform1f(glGetUniformLocation(terrainShader, "sunIntensity"), currentSunIntensity);
        if (snowEnabled) { glUniform3fv(glGetUniformLocation(terrainShader, "windDirection"), 1, glm::value_ptr(snowWindDirection)); glUniform1f(glGetUniformLocation(terrainShader, "windStrength"), snowWindStrength); }
        else if (rainEnabled) { glUniform3fv(glGetUniformLocation(terrainShader, "windDirection"), 1, glm::value_ptr(rainWindDirection)); glUniform1f(glGetUniformLocation(terrainShader, "windStrength"), rainWindStrength); }
        glUniformMatrix4fv(glGetUniformLocation(terrainShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(terrainShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(terrainShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, grassTexture);
        glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, rockTexture);
        glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, sandTexture);
        glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, waterTexture);
        glActiveTexture(GL_TEXTURE4); glBindTexture(GL_TEXTURE_2D, snowTexture);
        glBindVertexArray(terrainVAO);
        glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);

        if (snowEnabled && !rainEnabled && !starsOnlyMode) {
            glUseProgram(snowShader);
            glUniform1f(glGetUniformLocation(snowShader, "maxTerrainHeight"), maxTerrainHeight);
            glUniformMatrix4fv(glGetUniformLocation(snowShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix4fv(glGetUniformLocation(snowShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(snowShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
            glUniform1f(glGetUniformLocation(snowShader, "time"), currentTime);
            glUniform3fv(glGetUniformLocation(snowShader, "windDirection"), 1, glm::value_ptr(snowWindDirection));
            glUniform1f(glGetUniformLocation(snowShader, "windStrength"), snowWindStrength);
            glUniform1f(glGetUniformLocation(snowShader, "snowfallDensity"), snowfallDensity);
            glUniform1f(glGetUniformLocation(snowShader, "lightningFlash"), flashIntensity);
            glUniform1f(glGetUniformLocation(snowShader, "sunIntensity"), currentSunIntensity);
            glBindVertexArray(snowVAO);
            glDrawArrays(GL_POINTS, 0, SNOWFLAKE_COUNT);
        }
        if (rainEnabled && !snowEnabled && !starsOnlyMode) {
            glUseProgram(rainShader);
            glUniform1f(glGetUniformLocation(rainShader, "maxTerrainHeight"), maxTerrainHeight);
            glUniformMatrix4fv(glGetUniformLocation(rainShader, "model"), 1, GL_FALSE, glm::value_ptr(model));
            glUniformMatrix4fv(glGetUniformLocation(rainShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(rainShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
            glUniform1f(glGetUniformLocation(rainShader, "time"), currentTime);
            glUniform3fv(glGetUniformLocation(rainShader, "windDirection"), 1, glm::value_ptr(rainWindDirection));
            glUniform1f(glGetUniformLocation(rainShader, "windStrength"), rainWindStrength);
            glUniform1f(glGetUniformLocation(rainShader, "rainfallDensity"), rainfallDensity);
            glUniform1f(glGetUniformLocation(rainShader, "rainIntensity"), rainIntensity);
            glUniform1f(glGetUniformLocation(rainShader, "lightningFlash"), flashIntensity);
            glUniform1f(glGetUniformLocation(rainShader, "sunIntensity"), currentSunIntensity);
            glBindVertexArray(rainVAO);
            glDrawArrays(GL_POINTS, 0, RAINDROP_COUNT);
        }
        if (lightningFlash && lightningIndexCount > 0 && lightningEnabled && rainEnabled && !starsOnlyMode) {
            glUseProgram(lightningShader);
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE);
            glm::mat4 lightningModel = glm::mat4(1.0f);
            float centerY = maxTerrainHeight + 50.0f;
            lightningModel = glm::translate(lightningModel, glm::vec3(currentLightningPosX, centerY, currentLightningPosZ));
            float distanceToCamera = glm::distance(cameraPos, glm::vec3(currentLightningPosX, centerY, currentLightningPosZ));
            float scaleX = 40.0f + distanceToCamera * 0.05f;
            float scaleY = 60.0f + distanceToCamera * 0.07f;
            scaleX = glm::clamp(scaleX, 35.0f, 70.0f);
            scaleY = glm::clamp(scaleY, 50.0f, 90.0f);
            lightningModel = glm::scale(lightningModel, glm::vec3(scaleX, scaleY, 1.0f));
            glm::vec3 toCamera = glm::normalize(cameraPos - glm::vec3(currentLightningPosX, centerY, currentLightningPosZ));
            float angle = atan2(toCamera.x, toCamera.z);
            lightningModel = glm::rotate(lightningModel, angle, glm::vec3(0.0f, 1.0f, 0.0f));
            glUniformMatrix4fv(glGetUniformLocation(lightningShader, "model"), 1, GL_FALSE, glm::value_ptr(lightningModel));
            glUniformMatrix4fv(glGetUniformLocation(lightningShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(lightningShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
            glUniform1f(glGetUniformLocation(lightningShader, "time"), currentTime);
            glUniform1f(glGetUniformLocation(lightningShader, "lightningIntensity"), flashIntensity);
            glUniform3fv(glGetUniformLocation(lightningShader, "cameraPos"), 1, glm::value_ptr(cameraPos));
            glBindVertexArray(lightningVAO);
            glDrawElements(GL_TRIANGLES, lightningIndexCount, GL_UNSIGNED_INT, 0);
            glEnable(GL_DEPTH_TEST);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glUseProgram(0);
        }

        if (sunEnabled && !starsOnlyMode) {
            glUseProgram(sunShader);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_DEPTH_TEST);

            glDepthMask(GL_FALSE);

            glm::mat4 sunModel = glm::mat4(1.0f);
            sunModel = glm::translate(sunModel, sun.position);
            float sunScale = 34.0f;
            sunModel = glm::scale(sunModel, glm::vec3(sunScale));

            glUniformMatrix4fv(glGetUniformLocation(sunShader, "model"), 1, GL_FALSE, glm::value_ptr(sunModel));
            glUniformMatrix4fv(glGetUniformLocation(sunShader, "view"), 1, GL_FALSE, glm::value_ptr(view));
            glUniformMatrix4fv(glGetUniformLocation(sunShader, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
            glUniform1f(glGetUniformLocation(sunShader, "sunIntensity"), 1.0f);

            glm::vec3 sunColorValue = getSunColor(sun.angle);
            glUniform3fv(glGetUniformLocation(sunShader, "sunColor"), 1, glm::value_ptr(sunColorValue));

            glUniform1f(glGetUniformLocation(sunShader, "time"), currentTime);
            glUniform3fv(glGetUniformLocation(sunShader, "cameraPos"), 1, glm::value_ptr(cameraPos));

            glBindVertexArray(sunVAO);
            glDrawElements(GL_TRIANGLES, sunIndexCount, GL_UNSIGNED_INT, 0);

            glDepthMask(GL_TRUE);

            glUseProgram(0);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        if (fogEnabled && !starsOnlyMode) {
            glUseProgram(fogShader);
            glUniform1i(glGetUniformLocation(fogShader, "screenTexture"), 0);
            glUniform1f(glGetUniformLocation(fogShader, "fogDensity"), fogDensity);
            glUniform1f(glGetUniformLocation(fogShader, "fogGradient"), fogGradient);
            glUniform3fv(glGetUniformLocation(fogShader, "fogColor"), 1, glm::value_ptr(fogColor));
            glUniform3fv(glGetUniformLocation(fogShader, "cameraPos"), 1, glm::value_ptr(cameraPos));
            glUniform1f(glGetUniformLocation(fogShader, "time"), currentTime);
            glUniform1i(glGetUniformLocation(fogShader, "fogEnabled"), 1);
            glUniform1f(glGetUniformLocation(fogShader, "lightningFlash"), flashIntensity);
        }
        else {
            glUseProgram(fogShader);
            glUniform1i(glGetUniformLocation(fogShader, "screenTexture"), 0);
            glUniform1i(glGetUniformLocation(fogShader, "fogEnabled"), 0);
            glUniform1f(glGetUniformLocation(fogShader, "lightningFlash"), flashIntensity);
        }
        glm::mat4 identityMat = glm::mat4(1.0f);
        glUniformMatrix4fv(glGetUniformLocation(fogShader, "model"), 1, GL_FALSE, glm::value_ptr(identityMat));
        glUniformMatrix4fv(glGetUniformLocation(fogShader, "view"), 1, GL_FALSE, glm::value_ptr(identityMat));
        glUniformMatrix4fv(glGetUniformLocation(fogShader, "projection"), 1, GL_FALSE, glm::value_ptr(identityMat));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, screenTexture);
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glDeleteVertexArrays(1, &terrainVAO);
    glDeleteBuffers(1, &terrainVBO);
    glDeleteBuffers(1, &terrainEBO);
    glDeleteBuffers(1, &terrainNBO);
    glDeleteVertexArrays(1, &snowVAO);
    glDeleteBuffers(1, &snowVBO);
    glDeleteVertexArrays(1, &rainVAO);
    glDeleteBuffers(1, &rainVBO);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);
    glDeleteVertexArrays(1, &lightningVAO);
    glDeleteBuffers(1, &lightningVBO);
    glDeleteBuffers(1, &lightningEBO);
    glDeleteVertexArrays(1, &sunVAO);
    glDeleteBuffers(1, &sunVBO);
    glDeleteBuffers(1, &sunEBO);
    glDeleteVertexArrays(1, &moonVAO);
    glDeleteBuffers(1, &moonVBO);
    glDeleteBuffers(1, &moonEBO);
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVBO);
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteTextures(1, &screenTexture);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteProgram(terrainShader);
    glDeleteProgram(snowShader);
    glDeleteProgram(rainShader);
    glDeleteProgram(fogShader);
    glDeleteProgram(lightningShader);
    glDeleteProgram(sunShader);
    glDeleteProgram(moonShader);
    glDeleteProgram(skyboxShader);
    glfwTerminate();
    return 0;
}