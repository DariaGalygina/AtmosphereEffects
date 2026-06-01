#ifndef RAIN_SHADERS_H
#define RAIN_SHADERS_H

#include <string>

namespace RainShaders {

    const std::string vertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in float aSize;
layout (location = 2) in float aFallSpeed;
layout (location = 3) in float aTrailLength;
layout (location = 4) in vec3 aVelocity;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time;
uniform vec3 windDirection;
uniform float windStrength;
uniform float rainfallDensity;
uniform float rainIntensity;
uniform float maxTerrainHeight;

out float alpha;
out float size;
out float trailLength;
out float velocityFactor; 
out float brightness;
out float rainDensity; 

void main()
{
    vec3 position = aPos;
    
    if (rainfallDensity < 1.0 && mod(position.x + position.z, 10.0) > rainfallDensity * 10.0) {
        gl_Position = vec4(0.0, 0.0, 0.0, 0.0);
        return;
    }
    
    position.y -= aFallSpeed * 0.03;

    position.x += (windDirection.x * windStrength + aVelocity.x) * 0.08;
    position.z += (windDirection.z * windStrength + aVelocity.z) * 0.08;

    float jitterX = sin(time * 25.0 + position.x * 15.0) * 0.01;
    float jitterZ = cos(time * 20.0 + position.z * 12.0) * 0.01;
    position.x += jitterX;
    position.z += jitterZ;
    
    if (position.y < -20.0) {
        position.y = maxTerrainHeight + 300.0 + mod(position.x + position.z, 200.0);
        position.x = mod(position.x + 800.0, 1600.0) - 400.0;
        position.z = mod(position.z + 800.0, 1600.0) - 400.0;
    }
    
    alpha = 0.85 * rainfallDensity * rainIntensity;
    size = aSize * (2.2 + rainfallDensity * 0.3);
    trailLength = aTrailLength * (1.5 + rainfallDensity * 0.5);
    velocityFactor = length(aVelocity);
    brightness = 1.2 + rainfallDensity * 0.3;
    rainDensity = rainfallDensity; 
    
    vec4 viewPos = view * model * vec4(position, 1.0);
    float distance = length(viewPos.xyz);
    float pointSize = size * 18.0 / (1.0 + distance * 0.02);
    
    pointSize = max(pointSize, 3.0);
    
    gl_Position = projection * view * model * vec4(position, 1.0);
    gl_PointSize = pointSize;
}
)";

    const std::string fragmentShader = R"(
#version 330 core
in float alpha;
in float size;
in float trailLength;
in float velocityFactor;
in float brightness;
in float rainDensity; 

out vec4 FragColor;

float drawRaindrop(vec2 coord, float speedFactor) {
    vec2 stretchedCoord = vec2(coord.x * 0.5, coord.y);
    
    float dropBody = 1.0 - smoothstep(0.4, 0.9, length(stretchedCoord));
    
    float tailLen = trailLength * (1.0 + speedFactor * 0.5);
    float tail = 0.0;
    
    if (coord.y > 0.0) {
        float mainTail = 1.0 - smoothstep(0.05, tailLen, coord.y);
        float tailWidth = 1.0 - smoothstep(0.0, 0.25, abs(coord.x));
        tail = mainTail * tailWidth * 0.8;
        
        float secondaryTail = 1.0 - smoothstep(0.1, tailLen * 1.5, coord.y);
        float secondaryWidth = 1.0 - smoothstep(0.0, 0.4, abs(coord.x));
        tail = max(tail, secondaryTail * secondaryWidth * 0.4);
    }
    
    return max(dropBody, tail);
}

float waterRipple(vec2 coord, float rainIntensityVal, float timeVal) {
    // Базовая рябь от капли
    float distToCenter = length(coord - 0.5);

    float mainRipple = 0.0;
    float rippleSpeed = timeVal * 8.0;
    float rippleFreq = (distToCenter * 15.0 - rippleSpeed);
    mainRipple = sin(rippleFreq) * 0.6;
    mainRipple += sin(rippleFreq * 2.3) * 0.3;
    mainRipple += sin(rippleFreq * 4.7) * 0.15;

    float falloff = 1.0 - smoothstep(0.1, 0.7, distToCenter);
    mainRipple *= falloff;

    float secondaryRipple = 0.0;
    for(int i = 1; i <= 3; i++) {
        float ringDist = abs(distToCenter - (0.25 + float(i) * 0.12));
        float ringWidth = 0.08;
        float ringIntensity = 0.3 / float(i);
        float ringPhase = timeVal * 12.0 - ringDist * 25.0;
        secondaryRipple += sin(ringPhase) * exp(-ringDist * ringDist / (ringWidth * ringWidth)) * ringIntensity;
    }

    float intensityMultiplier = 0.3 + rainIntensityVal * 1.2;
    intensityMultiplier = clamp(intensityMultiplier, 0.3, 1.5);

    float rippleIntensity = (mainRipple + secondaryRipple) * intensityMultiplier;

    vec2 randomCoord = coord * 10.0;
    float randomNoise = fract(sin(randomCoord.x * 12.9898 + randomCoord.y * 78.233) * 43758.5453);
    float dropSparkle = 0.0;
    if (randomNoise > 0.97 && rainIntensityVal > 0.3) {
        dropSparkle = 0.5 * (1.0 - distToCenter * 1.5) * rainIntensityVal;
    }
    
    rippleIntensity += dropSparkle;
    
    return clamp(rippleIntensity, 0.0, 1.2);
}

void main()
{
    vec2 coord = gl_PointCoord * 2.0 - 1.0;
    
    float raindrop = drawRaindrop(coord, velocityFactor);
    
    if (raindrop < 0.2) discard;
    
    float distance = length(coord);
    float verticalPos = (coord.y + 1.0) * 0.5;
    
    float verticalAlpha = 1.0 - verticalPos * 0.5;
    
    vec3 rainColor = vec3(0.75, 0.82, 0.95);
    
    float colorVariation = sin(coord.x * 80.0 + coord.y * 60.0) * 0.15 + 0.85;
    rainColor *= colorVariation;

    float dropBrightness = 1.0 - distance * 0.3;
    dropBrightness *= (1.0 + size * 0.3);
    
    dropBrightness *= (1.0 + velocityFactor * 0.2);
    
    dropBrightness *= brightness;
    
    float finalAlpha = alpha * raindrop * verticalAlpha * dropBrightness * 1.2;
    
    finalAlpha *= (1.0 - smoothstep(0.0, 0.7, distance));
    finalAlpha = pow(finalAlpha, 0.8);

    float innerGlow = 1.0 - smoothstep(0.0, 0.4, distance);
    rainColor += innerGlow * 0.15 * vec3(0.9, 0.92, 1.0);
    
    finalAlpha = min(finalAlpha, 0.95);
    
    if (rainDensity > 0.7) {
        rainColor *= 1.1;
        finalAlpha *= 1.1;
    }

    float rippleEffect = waterRipple(gl_PointCoord, rainDensity, 0.0);

    float rippleGlow = rippleEffect * 0.3 * rainDensity;
    rainColor += vec3(0.6, 0.7, 0.9) * rippleGlow;
    finalAlpha += rippleGlow * 0.2;
    
    FragColor = vec4(rainColor, finalAlpha);
}
)";

}

#endif