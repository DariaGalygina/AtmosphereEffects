#ifndef CLOUD_SHADERS_H
#define CLOUD_SHADERS_H

#include <string>

namespace CloudShaders {
    const std::string vertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time;
uniform float speed;
uniform float heightScale;

out vec2 TexCoord;

void main()
{
    // Анимация текстурных координат
    vec2 uv = vec2(aTexCoord.x + time * speed, aTexCoord.y * heightScale);
    TexCoord = uv;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

    const std::string fragmentShader = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D photoTex;      // Фотография облаков (канал R)
uniform sampler2D noiseTex;      // Шумовая текстура (канал G)
uniform sampler2D detailTex;     // Детализирующая текстура (канал B)
uniform sampler2D gradientTex;   // Градиент заката (канал A)

uniform float cloudCover;        // Покрытие облаками (0-1)
uniform float cloudDensity;      // Плотность облаков (0-1)
uniform float sunsetFactor;      // Интенсивность заката (0-1)
uniform float time;

void main()
{
    // Загрузка текстур
    float photo = texture(photoTex, TexCoord).r;
    float noise = texture(noiseTex, TexCoord * 0.8).r;
    float detail = texture(detailTex, TexCoord * 2.0).r;
    float sunsetGrad = texture(gradientTex, vec2(sunsetFactor, 0.5)).a;
    
    // Основная форма облаков (по статье)
    float baseCloud = max(photo, noise);
    float clouds = baseCloud * 2.0 - 1.0;
    clouds += (1.0 - cloudCover) * 0.5;
    clouds = clamp(clouds, 0.0, 1.0);
    clouds = pow(clouds, 1.5);
    clouds = clouds * (0.7 + detail * 0.3);
    
    // Влияние плотности
    clouds = clouds * cloudDensity;
    
    // Цвет с учётом заката
    vec3 cloudColor = vec3(1.0, 1.0, 1.0) * (1.0 - sunsetGrad) + vec3(0.8, 0.4, 0.2) * sunsetGrad;
    
    // Итоговый цвет с альфа-каналом для смешивания
    float alpha = clamp(clouds * 1.2, 0.0, 0.95);
    FragColor = vec4(cloudColor, alpha);
}
)";
}

#endif