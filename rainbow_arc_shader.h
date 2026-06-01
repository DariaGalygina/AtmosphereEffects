#ifndef RAINBOW_ARC_SHADER_H
#define RAINBOW_ARC_SHADER_H

#include <string>

namespace RainbowArcShader {

    const std::string vertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

out vec2 TexCoord;
out vec3 WorldPos;
out float EdgeFade;

void main()
{
    WorldPos = vec3(model * vec4(aPos, 1.0));
    TexCoord = aTexCoord;
    EdgeFade = aTexCoord.y;  // Плавное затухание по краям
    
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

    const std::string fragmentShader = R"(
#version 330 core
in vec2 TexCoord;
in vec3 WorldPos;
in float EdgeFade;

out vec4 FragColor;

uniform float time;
uniform float rainbowIntensity;
uniform float rainbowVisibility;
uniform float rainIntensity;
uniform vec3 cameraPos;
uniform float lightningFlash;

void main()
{
    // Радуга только во время дождя
    if (rainIntensity < 0.2f) {
        discard;
    }
    
    // ПОПЕРЕЧНЫЙ ГРАДИЕНТ: цвет зависит от V (вертикальной координаты, 0-1)
    float v = TexCoord.y;
    
    // Плавное затухание по краям (EdgeFade от 0 на краях до 1 в центре)
    float edgeFadeValue = EdgeFade;
    edgeFadeValue = sin(edgeFadeValue * 3.14159);  // Плавный синусоидальный спад
    
    // Цвета радуги от нижнего края к верхнему:
    vec3 colors[7];
    colors[0] = vec3(1.0, 0.2, 0.2);     // Красный (низ)
    colors[1] = vec3(1.0, 0.5, 0.1);     // Оранжевый
    colors[2] = vec3(1.0, 0.9, 0.1);     // Жёлтый
    colors[3] = vec3(0.2, 0.9, 0.2);     // Зелёный
    colors[4] = vec3(0.1, 0.6, 1.0);     // Голубой
    colors[5] = vec3(0.2, 0.2, 0.9);     // Синий
    colors[6] = vec3(0.7, 0.2, 0.7);     // Фиолетовый (верх)
    
    int idx = int(v * 6.0);
    float frac = v * 6.0 - float(idx);
    
    vec3 color;
    if (idx >= 6) {
        color = colors[6];
    } else {
        color = mix(colors[idx], colors[idx + 1], frac);
    }
    
    // Добавляем лёгкое свечение от молнии
    float flashEffect = 1.0 + lightningFlash * 0.2;
    
    // Итоговая яркость и прозрачность с плавным затуханием по краям
    float alpha = 0.7f * rainbowIntensity * rainIntensity * edgeFadeValue * flashEffect;
    
    // Добавляем небольшое свечение
    color += vec3(0.2, 0.1, 0.3) * alpha * 0.3;
    
    FragColor = vec4(color, alpha);
}
)";

} // namespace RainbowArcShader

#endif // RAINBOW_ARC_SHADER_H