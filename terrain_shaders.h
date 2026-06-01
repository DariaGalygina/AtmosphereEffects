#ifndef TERRAIN_SHADERS_H
#define TERRAIN_SHADERS_H

#include <string> 

namespace TerrainShaders {

    const std::string vertexShader = R"(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform int snowEnabled;
uniform int rainEnabled;
uniform float time;
uniform float snowfallDensity;
uniform float rainfallDensity;
uniform float maxTerrainHeight;
uniform vec3 windDirection;
uniform float windStrength;
uniform float sunIntensity;

out float height;
out vec3 FragPos;
out vec3 Normal;
out vec3 WorldPos;
out float snowIntensity;
out vec2 TexCoords;

float noise(vec2 pos) {
    return sin(pos.x * 0.1) * cos(pos.y * 0.1) * 0.5 + 0.5;
}

float smoothNoise(vec2 pos) {
    vec2 i = floor(pos);
    vec2 f = fract(pos);
    
    float a = noise(i);
    float b = noise(i + vec2(1.0, 0.0));
    float c = noise(i + vec2(0.0, 1.0));
    float d = noise(i + vec2(1.0, 1.0));
    
    vec2 u = f * f * (3.0 - 2.0 * f);
    
    return mix(a, b, u.x) + (c - a) * u.y * (1.0 - u.x) + (d - b) * u.x * u.y;
}

void main()
{
    height = aPos.y;
    vec3 originalPos = aPos;

    if (height < 10.0) {
        originalPos.y = 10.0;

        if (rainEnabled == 1) {
            float microRipple = sin(originalPos.x * 1.5 + time * 5.0) * cos(originalPos.z * 1.5 + time * 4.5);
            originalPos.y += microRipple * 0.02 * rainfallDensity;
        }
    }
    
    FragPos = vec3(model * vec4(originalPos, 1.0));
    WorldPos = originalPos;
    Normal = mat3(transpose(inverse(model))) * aNormal;
    
    TexCoords = aPos.xz * 0.1;
    
    snowIntensity = 0.0;
    if (snowEnabled == 1) {
        float snowAmount = snowfallDensity;

        float snowBaseLevel;
        if (snowAmount <= 0.7) {
            snowBaseLevel = 80.0 - (snowAmount * 71.43);
        } else if (snowAmount <= 0.85) {
            float t = (snowAmount - 0.7) / 0.15;
            snowBaseLevel = 30.0 - t * 25.0;
        } else if (snowAmount <= 0.95) {
            float t = (snowAmount - 0.85) / 0.1;
            snowBaseLevel = 5.0 - t * 10.0;
        } else {
            float t = (snowAmount - 0.95) / 0.05;
            snowBaseLevel = -5.0 - t * 3.0;
        }
        snowBaseLevel = clamp(snowBaseLevel, -8.0, 80.0);

        float heightFactor;
        if (snowBaseLevel <= 0.0) {
            heightFactor = 1.0;
        } else {
            heightFactor = smoothstep(snowBaseLevel, maxTerrainHeight, height);
            if (snowAmount > 0.6) {
                float correction = pow((snowAmount - 0.6) / 0.4, 2.5) * 0.25;
                heightFactor = mix(heightFactor, 1.0, correction);
            }
        }

        float slope = 1.0 - max(dot(Normal, vec3(0.0, 1.0, 0.0)), 0.0);
        float slopeFactor = 1.0 - smoothstep(0.15, 0.7, slope);

        float noise1 = smoothNoise(WorldPos.xz * 0.015 + time * 0.0015) * 0.35;
        float noise2 = smoothNoise(WorldPos.xz * 0.04 - time * 0.001) * 0.25;
        float noise3 = smoothNoise(WorldPos.xz * 0.08) * 0.2;
        float noise4 = smoothNoise(WorldPos.xz * 0.15) * 0.12;
        float noise5 = smoothNoise(WorldPos.xz * 0.3) * 0.08;
        float noiseFactor = noise1 + noise2 + noise3 + noise4 + noise5;

        vec3 windDirNorm = normalize(windDirection);
        float windExposure = max(0.0, dot(Normal, windDirNorm));
        float windFactor = 1.0 - windExposure * (0.2 + snowAmount * 0.1);

        float randomVariation = smoothNoise(WorldPos.xz * 0.12 + time * 0.003);
        randomVariation = mix(0.65, 1.0, randomVariation);
        
        float rawSnow = heightFactor * slopeFactor * noiseFactor * windFactor * randomVariation;
        rawSnow = clamp(rawSnow, 0.0, 1.0);

        float intensityMultiplier;
        if (snowAmount <= 0.7) {
            intensityMultiplier = 0.2 + snowAmount * 0.78;
        } else if (snowAmount <= 0.8) {
            float t = (snowAmount - 0.7) / 0.1;
            intensityMultiplier = 0.746 + t * 0.044;
        } else if (snowAmount <= 0.85) {
            float t = (snowAmount - 0.8) / 0.05;
            intensityMultiplier = 0.790 + t * 0.015;
        } else if (snowAmount <= 0.9) {
            float t = (snowAmount - 0.85) / 0.05;
            intensityMultiplier = 0.805 + t * 0.010;
        } else if (snowAmount <= 0.95) {
            float t = (snowAmount - 0.9) / 0.05;
            intensityMultiplier = 0.815 + t * 0.005;
        } else {
            float t = (snowAmount - 0.95) / 0.05;
            intensityMultiplier = 0.820 + t * 0.003;
        }
        
        snowIntensity = rawSnow * intensityMultiplier;
        snowIntensity = clamp(snowIntensity, 0.0, 0.85);
        snowIntensity = smoothstep(0.0, 0.12, snowIntensity);
        snowIntensity *= (0.99 + sin(time * 0.005) * 0.01);
    }
    
    gl_Position = projection * view * model * vec4(originalPos, 1.0);
}
)";

    const std::string fragmentShader = R"(
#version 330 core
out vec4 FragColor;
in float height;
in vec3 FragPos;
in vec3 Normal;
in vec3 WorldPos;
in float snowIntensity;
in vec2 TexCoords;

uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 lightColor;
uniform int snowEnabled;
uniform int rainEnabled;
uniform float snowfallDensity;
uniform float rainfallDensity;
uniform float time;
uniform vec3 windDirection;
uniform float windStrength;
uniform float maxTerrainHeight;
uniform float sunIntensity;
uniform float lightningFlash;

uniform sampler2D grassTexture;
uniform sampler2D rockTexture;
uniform sampler2D sandTexture;
uniform sampler2D waterTexture;
uniform sampler2D snowTexture;
uniform float textureScale;

float calculateWaterRipple(vec3 worldPos, float timeVal, float intensity, float speed, bool isRaining, float rainDensity) {
    vec2 coord1 = worldPos.xz * 0.4;
    vec2 coord2 = worldPos.xz * 0.8;
    vec2 coord3 = worldPos.xz * 1.6;
    vec2 coord4 = worldPos.xz * 3.2;
    
    float ripple1 = sin(coord1.x + timeVal * speed) * cos(coord1.y + timeVal * speed * 0.9);
    float ripple2 = sin(coord2.x * 1.3 - timeVal * speed * 1.1) * 0.6;
    float ripple3 = cos(coord3.y * 1.2 + timeVal * speed * 1.3) * 0.5;
    float ripple4 = sin(coord4.x + coord4.y + timeVal * speed * 1.8) * 0.4;
    float ripple5 = sin((worldPos.x * 2.5 + worldPos.z * 1.5) + timeVal * speed * 2.2) * 0.3;
    
    float totalRipple = (ripple1 + ripple2 + ripple3 + ripple4 + ripple5) * 0.18;

    float rainRipple = 0.0;
    if (isRaining) {
        vec2 rainCoord = worldPos.xz * 2.0;
        rainRipple = sin(rainCoord.x * 2.0 + timeVal * 15.0) * cos(rainCoord.y * 2.0 + timeVal * 14.0);
        rainRipple += sin(rainCoord.x * 4.0 + timeVal * 30.0) * 0.4;
        rainRipple += cos(rainCoord.y * 5.0 + timeVal * 28.0) * 0.4;
        rainRipple += sin((rainCoord.x + rainCoord.y) * 3.0 + timeVal * 25.0) * 0.3;
        rainRipple = rainRipple * 0.15 * rainDensity;
    }
    
    float result = totalRipple + rainRipple;
    return clamp(result * intensity, -intensity * 0.8, intensity * 0.8);
}

float waterNoise(vec2 pos, float timeVal) {
    vec2 p = floor(pos);
    vec2 f = fract(pos);
    float n = sin(p.x * 127.1 + p.y * 311.7 + timeVal) * 43758.5453;
    return fract(n);
}

vec3 getBaseLandscapeColor(float height, vec2 texCoords, vec3 worldPos, float timeVal, bool isRaining, float rainDensity) {
    vec3 grassColor = texture(grassTexture, texCoords).rgb;
    vec3 rockColor = texture(rockTexture, texCoords).rgb;
    vec3 sandColor = texture(sandTexture, texCoords).rgb;
    vec3 waterColor = texture(waterTexture, texCoords).rgb;
    
    if (height < 10.0) {
        float deepWater = smoothstep(0.0, 10.0, height);
        vec3 deepWaterColor = waterColor * 0.25;
        vec3 shallowWaterColor = waterColor * 0.75;
        
        vec3 waterBase = mix(deepWaterColor, shallowWaterColor, deepWater);
        
        float rippleStrength = 0.18;
        float rippleSpeed = 2.0;
        float ripple = calculateWaterRipple(worldPos, timeVal, rippleStrength, rippleSpeed, isRaining, rainDensity);

        float noiseVal1 = waterNoise(worldPos.xz * 1.5, timeVal * 0.5);
        float noiseVal2 = waterNoise(worldPos.xz * 3.0, timeVal * 0.8);
        float noiseVal3 = waterNoise(worldPos.xz * 6.0, timeVal * 1.2);
        float waterTextureNoise = (noiseVal1 + noiseVal2 + noiseVal3) * 0.15 - 0.075;

        vec3 rippleColor = vec3(0.08, 0.14, 0.25) * (ripple + waterTextureNoise);
        waterBase += rippleColor;

        float brightnessVar = 0.92 + sin(worldPos.x * 1.8 + worldPos.z * 1.8 + timeVal * 1.5) * 0.08;
        brightnessVar += cos(worldPos.z * 2.2 - worldPos.x * 1.5 + timeVal * 1.2) * 0.06;
        waterBase *= brightnessVar;
        
        return waterBase;
    } 
    else if (height < 13.0) {
        float t = smoothstep(10.0, 13.0, height);
        vec3 waterShallow = waterColor * 0.8;
        return mix(waterShallow, sandColor, t);
    } 
    else if (height < 17.0) {
        float t = smoothstep(13.0, 17.0, height);
        vec3 darkSand = sandColor * 0.85;
        return mix(sandColor, darkSand, t);
    } 
    else if (height < 40.0) {
        float t = smoothstep(17.0, 40.0, height);
        vec3 brightGrass = grassColor * 1.2;
        return mix(grassColor, brightGrass, t);
    } 
    else if (height < 60.0) {
        float t = smoothstep(40.0, 60.0, height);
        return mix(grassColor, rockColor, t);
    } 
    else if (height < 80.0) {
        float t = smoothstep(60.0, 80.0, height);
        vec3 darkRock = rockColor * 0.7;
        return mix(rockColor, darkRock, t);
    } 
    else {
        float t = smoothstep(80.0, maxTerrainHeight, height);
        return mix(rockColor * 0.6, vec3(0.75, 0.78, 0.82), t);
    }
}

vec3 getRainyLandscapeColor(float height, vec2 texCoords, vec3 worldPos, float timeVal, float rainDensity) {
    vec3 grassColor = texture(grassTexture, texCoords).rgb;
    vec3 rockColor = texture(rockTexture, texCoords).rgb;
    vec3 sandColor = texture(sandTexture, texCoords).rgb;
    vec3 waterColor = texture(waterTexture, texCoords).rgb;
    
    if (height < 10.0) {
        vec3 rainyWater = waterColor * 0.45;
        vec3 deepWater = waterColor * 0.25;
        float t = smoothstep(0.0, 10.0, height);
        
        vec3 waterBase = mix(deepWater, rainyWater, t);

        float rippleStrength = 0.28 + rainDensity * 0.2;
        float rippleSpeed = 2.8;
        float ripple = calculateWaterRipple(worldPos, timeVal, rippleStrength, rippleSpeed, true, rainDensity);

        float noiseVal = waterNoise(worldPos.xz * 2.0, timeVal * 1.5);
        float rainTexture = (noiseVal - 0.5) * 0.15 * rainDensity;
        
        vec3 rippleColor = vec3(0.06, 0.12, 0.22) * (ripple + rainTexture);
        waterBase += rippleColor;

        float brightnessVar = 0.88 + sin(worldPos.x * 2.5 + worldPos.z * 2.0 + timeVal * 2.0) * 0.12;
        brightnessVar += cos(worldPos.z * 3.0 - worldPos.x * 2.0 + timeVal * 1.8) * 0.1;
        waterBase *= brightnessVar;
        
        return waterBase;
    } 
    else if (height < 13.0) {
        float t = smoothstep(10.0, 13.0, height);
        vec3 wetSand = sandColor * 0.7;
        return mix(waterColor * 0.6, wetSand, t);
    } 
    else if (height < 17.0) {
        float t = smoothstep(13.0, 17.0, height);
        return mix(sandColor * 0.7, grassColor * 0.5, t);
    } 
    else if (height < 40.0) {
        float t = smoothstep(17.0, 40.0, height);
        vec3 wetGrass = grassColor * 0.6;
        return mix(grassColor * 0.7, wetGrass, t);
    } 
    else if (height < 60.0) {
        float t = smoothstep(40.0, 60.0, height);
        vec3 wetRock = rockColor * 0.6;
        return mix(grassColor * 0.5, wetRock, t);
    } 
    else if (height < 80.0) {
        float t = smoothstep(60.0, 80.0, height);
        vec3 darkWetRock = rockColor * 0.5;
        return mix(rockColor * 0.6, darkWetRock, t);
    } 
    else {
        float t = smoothstep(80.0, maxTerrainHeight, height);
        return mix(rockColor * 0.5, vec3(0.6, 0.65, 0.7), t);
    }
}

vec3 getSnowColor(vec2 texCoords, float height, float snowAmount) {
    vec3 snowTexColor = texture(snowTexture, texCoords).rgb;
    vec3 baseSnow = vec3(0.95, 0.96, 0.98);
    
    float brightness = 0.92 + snowAmount * 0.04;
    baseSnow *= brightness;
    
    vec3 finalSnow = mix(baseSnow, snowTexColor, 0.1);
    
    if (height > 40.0) {
        finalSnow = mix(finalSnow, vec3(1.0, 1.0, 1.0), 0.25);
    }
    
    return finalSnow;
}

vec3 calculateLighting(vec3 normal, vec3 lightDir, vec3 viewDir, vec3 albedo, float roughness, float metallic, float sunIntensityValue, float lightningIntensity) {
    float sunFactor = max(0.2, sunIntensityValue);
    float flashFactor = 1.0 + lightningIntensity * 2.0;
    
    vec3 ambient = 0.3 * albedo * sunFactor * flashFactor;
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * albedo * sunFactor * flashFactor;
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0 / roughness);
    vec3 specular = spec * lightColor * (0.4 + metallic * 0.4) * sunFactor * flashFactor;
    
    return ambient + diffuse + specular;
}

void main() {
    vec3 baseColor;
    vec2 finalTexCoords = TexCoords * textureScale;
    
    bool isRaining = (rainEnabled == 1);
    float rainDensityVal = rainfallDensity;
    
    if (isRaining) {
        baseColor = getRainyLandscapeColor(height, finalTexCoords, WorldPos, time, rainDensityVal);
    } else {
        baseColor = getBaseLandscapeColor(height, finalTexCoords, WorldPos, time, isRaining, rainDensityVal);
    }
    
    vec3 finalColor = baseColor;
    
    if (snowEnabled == 1) {
        float snowAmount = snowIntensity;
        vec3 snowColor = getSnowColor(finalTexCoords, height, snowAmount);
        finalColor = mix(baseColor, snowColor, snowAmount);
        finalColor += vec3(0.02, 0.02, 0.03) * snowAmount;
        
        if (snowAmount > 0.15) {
            vec3 lightDir = normalize(lightPos - FragPos);
            float lightDot = max(dot(normalize(Normal), lightDir), 0.0);
            if (lightDot > 0.4) {
                float sparkle = sin(FragPos.x * 12.0 + time * 25.0) * 0.5 + 0.5;
                sparkle *= sin(FragPos.z * 10.0 + time * 20.0) * 0.5 + 0.5;
                sparkle *= lightDot * snowAmount * 0.3;
                finalColor += vec3(sparkle, sparkle * 0.95, sparkle);
            }
        }
        
        if (isRaining && snowAmount > 0.3) {
            finalColor = mix(finalColor, finalColor * 0.92, 0.2);
        }
    }
    
    vec3 lightDir = normalize(lightPos - FragPos);
    vec3 viewDir = normalize(viewPos - FragPos);
    
    float roughness = 0.7;
    float metallic = 0.1;
    
    if (height < 10.0) {
        roughness = 0.08;
        metallic = 0.35;
    }
    else if (height > 60.0) {
        roughness = 0.9;
        metallic = 0.05;
    }
    else if (height > 17.0 && height < 60.0) {
        roughness = 0.8;
        metallic = 0.02;
    }
    else {
        roughness = 0.6;
        metallic = 0.03;
    }
    
    if (snowEnabled == 1 && snowIntensity > 0.3) {
        roughness = mix(roughness, 0.85, snowIntensity * 0.5);
    }

    vec3 finalNormal = normalize(Normal);
    if (height < 10.0) {
        float rippleAmount = calculateWaterRipple(WorldPos, time, 0.45, 2.0, isRaining, rainDensityVal);
        float noiseOffset = waterNoise(WorldPos.xz * 2.5, time * 1.2) * 0.12;

        float rainRippleBoost = isRaining ? rainDensityVal * 0.3 : 0.0;
        
        finalNormal.x += (rippleAmount + noiseOffset) * (0.5 + rainRippleBoost);
        finalNormal.z += (rippleAmount + noiseOffset) * (0.5 + rainRippleBoost);
        finalNormal = normalize(finalNormal);
    }
    
    vec3 litColor = calculateLighting(finalNormal, lightDir, viewDir, finalColor, roughness, metallic, sunIntensity, lightningFlash);

    if (height < 10.0) {
        vec3 reflectDir = reflect(-viewDir, finalNormal);
        float sunGlint = pow(max(dot(reflectDir, lightDir), 0.0), 32.0);
        float rippleIntensity = abs(calculateWaterRipple(WorldPos, time, 0.6, 2.0, isRaining, rainDensityVal));
        float rainBoost = isRaining ? (1.0 + rainDensityVal) : 1.0;
        float glintIntensity = (0.15 + rippleIntensity * 0.7) * sunIntensity * rainBoost;
        
        vec3 glintColor = sunGlint * vec3(1.0, 0.92, 0.7) * 0.5 * glintIntensity;
        litColor += glintColor;

        float fresnel = pow(1.0 - abs(dot(finalNormal, viewDir)), 0.5);
        litColor = mix(litColor, vec3(0.5, 0.7, 0.9), fresnel * 0.15);
    }
    
    if (isRaining && height > 10.0 && height < 60.0) {
        vec3 lightDirRain = normalize(lightPos - FragPos);
        vec3 viewDirRain = normalize(viewPos - FragPos);
        vec3 halfwayDirRain = normalize(lightDirRain + viewDirRain);
        float wetSpecular = pow(max(dot(finalNormal, halfwayDirRain), 0.0), 64.0);
        litColor += wetSpecular * 0.25 * rainfallDensity;
    }
    
    float distance = length(FragPos - viewPos);
    float hazeFactor = smoothstep(200.0, 800.0, distance);
    vec3 hazeColor = vec3(0.6, 0.65, 0.7);
    litColor = mix(litColor, hazeColor, hazeFactor * 0.2);
    
    FragColor = vec4(litColor, 1.0);
}
)";

}

#endif