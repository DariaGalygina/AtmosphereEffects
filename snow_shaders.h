#ifndef SNOW_SHADERS_H
#define SNOW_SHADERS_H

#include <string>

namespace SnowShaders {

    const std::string vertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in float aSize;
layout (location = 2) in float aFallSpeed;
layout (location = 3) in float aOffset;
layout (location = 4) in float aRotation;
layout (location = 5) in float aRotationSpeed;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time;
uniform vec3 windDirection;
uniform float windStrength;
uniform float snowfallDensity;
uniform float maxTerrainHeight;

out float alpha;
out float size;
out float rotation;
out float sway;

void main()
{
    vec3 position = aPos;
    
    if (snowfallDensity < 1.0 && mod(aOffset, 1.0) > snowfallDensity) {
        gl_Position = vec4(0.0, 0.0, 0.0, 0.0);
        return;
    }
    
    position.y -= aFallSpeed * 0.02;
    
    position.x += windDirection.x * windStrength * 0.03;
    position.z += windDirection.z * windStrength * 0.03;

    sway = sin(time * 1.5 + aOffset) * cos(time * 0.8 + aOffset * 1.3) * 0.15;
    position.x += sway;
    position.z += sway * 0.7;
    position.y += sin(time * 2.0 + aOffset) * 0.02;
    
    rotation = aRotation + time * aRotationSpeed;
    if (rotation > 6.28318) rotation -= 6.28318;
    
    if (position.y < -20.0) {
        position.y = maxTerrainHeight + 100.0 + mod(aOffset, 50.0);
        position.x = mod(position.x + 500.0, 1000.0) - 250.0;
        position.z = mod(position.z + 500.0, 1000.0) - 250.0;
    }
    
    alpha = 0.9 * snowfallDensity;
    size = aSize;
    
    vec4 viewPos = view * model * vec4(position, 1.0);
    float distance = length(viewPos.xyz);
    float pointSize = aSize * 120.0 / (1.0 + distance * 0.05);
    
    gl_Position = projection * view * model * vec4(position, 1.0);
    gl_PointSize = pointSize;
}
)";

    const std::string fragmentShader = R"(
#version 330 core
in float alpha;
in float size;
in float rotation;
in float sway;

out vec4 FragColor;

float drawSnowflake(vec2 coord, float size) {
    float cosR = cos(rotation);
    float sinR = sin(rotation);
    vec2 rotated = vec2(
        coord.x * cosR - coord.y * sinR,
        coord.x * sinR + coord.y * cosR
    );
    
    float d = length(rotated);

    float snowflake = 1.0 - smoothstep(size * 0.7, size, d);

    for (int i = 0; i < 6; i++) {
        float angle = float(i) * 3.14159 / 3.0;
        vec2 rayDir = vec2(cos(angle), sin(angle));
        float rayDot = dot(normalize(rotated), rayDir);
        
        if (rayDot > 0.9) {
            float rayDist = d * (1.0 + sin(angle * 2.0 + rotation) * 0.08);
            float ray = 1.0 - smoothstep(size * 0.2, size * 0.9, rayDist);
            snowflake = max(snowflake, ray * 0.7);
        }
    }
    
    return snowflake;
}

void main()
{
    vec2 coord = gl_PointCoord * 2.0 - 1.0;
    
    float snowflake = drawSnowflake(coord, 0.7);
    
    if (snowflake < 0.1) discard;
    
    float distance = length(coord);
    float edge = 1.0 - smoothstep(0.6, 0.9, distance);
    
    vec3 snowColor = vec3(0.96, 0.97, 0.99);
    
    float colorVariation = sin(sway * 10.0) * 0.03 + 0.97;
    snowColor *= colorVariation;
    
    float brightness = 1.0 - distance * 0.2;
    brightness *= (0.9 + size * 0.1); 
    
    float finalAlpha = alpha * snowflake * edge * brightness;
    
    finalAlpha *= (1.0 - smoothstep(0.0, 0.4, distance));
    
    FragColor = vec4(snowColor, finalAlpha);
}
)";

}  

#endif 