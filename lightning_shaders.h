#ifndef LIGHTNING_SHADERS_H
#define LIGHTNING_SHADERS_H

#include <string>

namespace LightningShaders {
    const std::string vertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time;
uniform float lightningIntensity;
uniform float lightningSeed;
uniform float lightningScale;
uniform vec3 cameraPos;

out vec2 TexCoord;
out float alpha;
out float glowIntensity;

void main() {
    TexCoord = aTexCoord;

    float flicker = sin(time * 30.0 + lightningSeed) * 0.15 + 0.85;
    flicker *= (0.7 + sin(time * 45.0 + lightningSeed * 2.0) * 0.1);
    flicker = clamp(flicker, 0.5, 1.2);

    vec3 pos = aPos;
    float noise1 = sin(pos.y * 8.0 + time * 50.0) * 0.02;
    float noise2 = cos(pos.y * 12.0 - time * 40.0) * 0.015;
    float noise3 = sin((pos.y + lightningSeed) * 20.0 + time * 60.0) * 0.01;
    pos.x += noise1 + noise2 + noise3;

    glowIntensity = (0.8 + sin(pos.y * 15.0 - time * 80.0) * 0.2) * flicker;
    glowIntensity *= (0.7 + sin(time * 20.0) * 0.15);

    alpha = 0.9 * flicker;

    vec4 worldPos = model * vec4(pos, 1.0);
    gl_Position = projection * view * worldPos;
}
)";

    const std::string fragmentShader = R"(
#version 330 core
in vec2 TexCoord;
in float alpha;
in float glowIntensity;

uniform float time;
uniform float lightningIntensity;
uniform float rainIntensity;

out vec4 FragColor;

void main() {
    float center = abs(TexCoord.x - 0.5) * 2.0;
    float distanceFromCenter = 1.0 - center;

    float coreGlow = pow(distanceFromCenter, 1.5) * 1.2;

    float outerGlow = pow(distanceFromCenter, 0.8) * 0.6;

    float sparkle = sin(TexCoord.y * 80.0 + time * 200.0) * 0.15;
    sparkle += sin(TexCoord.y * 160.0 - time * 250.0) * 0.1;
    sparkle = max(0.0, sparkle);

    float intensity = coreGlow + outerGlow + sparkle;
    intensity *= glowIntensity * lightningIntensity;

    float colorShift = sin(TexCoord.y * 50.0 + time * 100.0) * 0.1;
    
    vec3 color1 = vec3(0.95, 0.92, 1.0);  
    vec3 color2 = vec3(0.7, 0.6, 1.0);   
    vec3 color3 = vec3(0.5, 0.7, 1.0);    
    
    vec3 finalColor = mix(color1, color2, colorShift + 0.3);
    finalColor = mix(finalColor, color3, sin(TexCoord.y * 30.0) * 0.3 + 0.3); 

    float rainBoost = 1.0 + rainIntensity * 0.5;
    finalColor *= (0.8 + intensity * 1.5) * rainBoost;
    
    float finalAlpha = alpha * intensity * (0.7 + sparkle * 0.5);
    finalAlpha = clamp(finalAlpha, 0.0, 0.95);
    
    FragColor = vec4(finalColor, finalAlpha);
}
)";
}

#endif