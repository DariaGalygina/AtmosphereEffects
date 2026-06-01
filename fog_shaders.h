#ifndef FOG_SHADERS_H
#define FOG_SHADERS_H

#include <string>

namespace FogShaders {

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
    WorldPos = vec3(model * vec4(aPos, 1.0));
    TexCoord = aTexCoord;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

    const std::string fragmentShader = R"(
#version 330 core
in vec2 TexCoord;
in vec3 WorldPos;

out vec4 FragColor;

uniform sampler2D screenTexture;
uniform float fogDensity;
uniform float fogGradient;
uniform vec3 fogColor;
uniform vec3 cameraPos;
uniform float time;
uniform int fogEnabled;
uniform float lightningFlash;

float random(vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);
}

float smoothNoise(vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);
    
    float a = random(i);
    float b = random(i + vec2(1.0, 0.0));
    float c = random(i + vec2(0.0, 1.0));
    float d = random(i + vec2(1.0, 1.0));
    
    vec2 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);
    
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fractalNoise(vec2 st, int octaves, float persistence, float lacunarity, float speed) {
    float value = 0.0;
    float amplitude = 0.5;
    float frequency = 0.3;
    float maxValue = 0.0;
    
    for(int i = 0; i < octaves; i++) {
        vec2 animatedSt = st;
        animatedSt.x += time * speed * (float(i) * 0.4 + 0.6);
        animatedSt.y += time * speed * (float(i) * 0.3 + 0.7);
        
        value += amplitude * smoothNoise(animatedSt * frequency);
        maxValue += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }
    
    return value / maxValue;
}

float smoothWavePattern(vec2 pos, float timeVal) {
    float pattern = 0.0;
    
    pattern += sin(pos.x * 0.08 + timeVal * 0.4) * 0.3;
    pattern += cos(pos.y * 0.07 - timeVal * 0.35) * 0.3;
    pattern += sin((pos.x * 0.15 + pos.y * 0.1) * 0.8 + timeVal * 0.5) * 0.2;
    pattern += cos((pos.x * 0.12 - pos.y * 0.18) * 0.9 - timeVal * 0.45) * 0.2;
    pattern += sin(pos.x * 0.25 + timeVal * 0.8) * 0.1;
    pattern += cos(pos.y * 0.22 + timeVal * 0.75) * 0.1;
    
    return 0.5 + pattern * 0.2;
}

float movingFogWaves(vec3 worldPos, float timeVal) {
    vec2 pos = worldPos.xz;
    
    float baseWaves = 0.0;
    baseWaves += sin(pos.x * 0.025 + timeVal * 0.1) * 0.2;
    baseWaves += cos(pos.y * 0.02 - timeVal * 0.08) * 0.2;
    baseWaves += sin((pos.x * 0.04 + pos.y * 0.03) * 0.6 + timeVal * 0.12) * 0.15;
    baseWaves = 0.5 + baseWaves * 0.3;
    
    vec2 mediumPos = pos * 0.08;
    float mediumWaves = smoothWavePattern(mediumPos, timeVal * 0.5);
    
    vec2 fastPos = pos * 0.18;
    float fastWaves = 0.0;
    fastWaves += sin(fastPos.x * 0.8 + timeVal * 1.2) * 0.15;
    fastWaves += cos(fastPos.y * 0.7 - timeVal * 1.1) * 0.15;
    fastWaves += sin((fastPos.x * 1.2 + fastPos.y * 0.9) * 0.7 + timeVal * 1.4) * 0.1;
    fastWaves = 0.5 + fastWaves * 0.2;
    
    float fogWave = baseWaves * 0.4;
    fogWave += mediumWaves * 0.35;
    fogWave += fastWaves * 0.25;
    
    fogWave = smoothstep(0.3, 0.7, fogWave);
    
    return clamp(fogWave, 0.4, 0.7);
}

float getFogDensity(vec3 worldPos, float timeVal, float baseDensity) {
    float movingDensity = movingFogWaves(worldPos, timeVal);
    
    float heightFactor = exp(-worldPos.y * 0.02);
    heightFactor = smoothstep(0.25, 0.9, heightFactor);
    
    vec2 turbPos = worldPos.xz * 0.06;
    float turbulence = sin(turbPos.x + timeVal * 0.9) * cos(turbPos.y - timeVal * 0.8);
    turbulence = (turbulence + 1.0) * 0.15; 
    
    float noiseVal = fractalNoise(worldPos.xz * 0.04, 3, 0.5, 2.0, 0.3);
    noiseVal = smoothstep(0.3, 0.7, noiseVal) * 0.15;
    
    float density = movingDensity * 0.6;
    density += turbulence * 0.2;
    density += noiseVal * 0.2;
    
    density *= heightFactor;
    
    density = smoothstep(0.25, 0.65, density);
    
    float finalDensity = density * baseDensity * 1.5;
    
    return clamp(finalDensity, baseDensity * 0.3, baseDensity * 1.2);
}

vec3 getFogColorDynamic(vec3 worldPos, float timeVal, float fogAmount) {
    vec3 baseColor = fogColor;
    
    float warmZones = sin(worldPos.x * 0.06 + timeVal * 0.3) * cos(worldPos.z * 0.05 - timeVal * 0.25);
    warmZones = (warmZones + 1.0) * 0.5;
    warmZones = smoothstep(0.3, 0.7, warmZones);
    
    baseColor.r += warmZones * 0.04;
    baseColor.g += warmZones * 0.02;
    
    float heightNorm = clamp(worldPos.y / 150.0, 0.0, 1.0);
    heightNorm = smoothstep(0.0, 0.8, heightNorm);
    baseColor = mix(baseColor, vec3(0.62, 0.67, 0.72), heightNorm * 0.3);
    
    if(lightningFlash > 0.3) {
        float flashNorm = smoothstep(0.3, 0.8, lightningFlash);
        baseColor = mix(baseColor, vec3(0.85, 0.82, 0.75), flashNorm * 0.25);
    }
    
    return clamp(baseColor, 0.0, 1.0);
}

float getFogFactor(float distance, float density, float gradient) {
    float expFactor = exp(-distance * density * gradient);
    float fogFactor = 1.0 - expFactor;
    
    fogFactor = pow(fogFactor, 1.1);
    
    fogFactor = smoothstep(0.0, 0.9, fogFactor);
    
    return clamp(fogFactor, 0.0, 0.75);
}

void main()
{
    if (fogEnabled == 0) {
        FragColor = texture(screenTexture, TexCoord);
        return;
    }
    
    vec4 originalColor = texture(screenTexture, TexCoord);
    
    float distance = length(WorldPos - cameraPos);
    
    float density = getFogDensity(WorldPos, time, fogDensity);
    
    float fogFactor = getFogFactor(distance, density, fogGradient);
    
    float shimmer = sin(WorldPos.x * 0.08 + time * 1.5) * cos(WorldPos.z * 0.06 - time * 1.3);
    shimmer = (shimmer + 1.0) * 0.015;
    shimmer *= smoothstep(0.3, 0.8, fogFactor);
    fogFactor += shimmer;
    
    fogFactor = clamp(fogFactor, 0.0, 0.75);
    
    float distanceFactor = smoothstep(50.0, 400.0, distance);
    fogFactor *= (0.7 + distanceFactor * 0.3);
    
    vec3 fogColorResult = getFogColorDynamic(WorldPos, time, fogFactor);
    
    vec3 finalColor = mix(originalColor.rgb, fogColorResult, fogFactor);
    
    float glow = density * fogFactor * 0.05;
    finalColor += vec3(0.1, 0.08, 0.06) * glow;
    
    FragColor = vec4(finalColor, originalColor.a);
}
)"; 

} // namespace FogShaders

#endif // FOG_SHADERS_H