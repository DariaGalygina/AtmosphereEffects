#ifndef RAINBOW_SHADERS_H
#define RAINBOW_SHADERS_H

#include <string>

namespace RainbowShaders {

    const std::string vertexShader = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float time;

out vec3 WorldPos;
out vec3 Normal;

void main()
{
    WorldPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

    const std::string fragmentShader = R"(
#version 330 core
in vec3 WorldPos;
in vec3 Normal;

out vec4 FragColor;

uniform float time;
uniform float rainbowIntensity;
uniform float rainIntensity;
uniform vec3 cameraPos;
uniform vec3 lightPos;

// Функция для получения радужного цвета на основе угла
vec3 getRainbowColor(float angle) {
    // 7 цветов радуги
    vec3 colors[7];
    colors[0] = vec3(1.0, 0.4, 0.4); // Красный
    colors[1] = vec3(1.0, 0.6, 0.2); // Оранжевый
    colors[2] = vec3(1.0, 0.8, 0.2); // Желтый
    colors[3] = vec3(0.3, 0.9, 0.3); // Зеленый
    colors[4] = vec3(0.2, 0.6, 1.0); // Голубой
    colors[5] = vec3(0.3, 0.3, 0.8); // Синий
    colors[6] = vec3(0.7, 0.3, 0.7); // Фиолетовый
    
    // Интерполяция между цветами
    float t = fract(angle * 1.5);
    int idx = int(angle * 6.0);
    
    if (idx >= 6) return colors[6];
    return mix(colors[idx], colors[idx + 1], t);
}

void main() {
    // Радуга появляется только во время дождя
    if (rainIntensity < 0.2) {
        FragColor = vec4(0.0);
        return;
    }
    
    vec3 normal = normalize(Normal);
    vec3 viewDir = normalize(cameraPos - WorldPos);
    vec3 lightDir = normalize(lightPos - WorldPos);
    vec3 reflectDir = reflect(-lightDir, normal);
    
    // Угол между направлением взгляда и отраженным светом
    float specAngle = max(dot(reflectDir, viewDir), 0.0);
    
    // Радужные блики появляются при определенных углах (эффект дисперсии)
    float rainbowSpecular = pow(specAngle, 4.0) * 0.5;
    
    // Создаем цветовой сдвиг для радужного эффекта
    vec3 rainbowColor = vec3(0.0);
    
    if (rainbowSpecular > 0.05) {
        // Разные углы для разных цветов (дисперсия)
        float redAngle = specAngle * 1.2;
        float greenAngle = specAngle * 1.1;
        float blueAngle = specAngle * 1.0;
        
        // Создаем радужный блик
        rainbowColor.r = pow(max(0.0, dot(reflectDir, viewDir) - 0.3), 2.0) * 1.5;
        rainbowColor.g = pow(max(0.0, dot(reflectDir, viewDir) - 0.2), 2.0) * 1.5;
        rainbowColor.b = pow(max(0.0, dot(reflectDir, viewDir) - 0.1), 2.0) * 1.5;
        
        // Радужный оттенок зависит от нормали и времени
        float hue = (normal.x * 0.5 + 0.5) * (normal.z * 0.5 + 0.5);
        hue = fract(hue + time * 0.1);
        
        vec3 spectralColor = getRainbowColor(hue);
        rainbowColor *= spectralColor * rainbowIntensity;
    }
    
    // Добавляем легкое радужное свечение на мокрых поверхностях
    float wetGlow = pow(1.0 - abs(normal.y), 2.0) * 0.3 * rainIntensity;
    vec3 wetRainbow = getRainbowColor(fract(WorldPos.x * 0.05 + time * 0.05));
    wetRainbow *= wetGlow * rainbowIntensity * 0.5;
    
    // Итоговый цвет радуги
    vec3 finalRainbow = rainbowColor + wetRainbow;
    float alpha = max(max(finalRainbow.r, finalRainbow.g), finalRainbow.b);
    alpha = clamp(alpha * 0.8, 0.0, 0.9);
    
    FragColor = vec4(finalRainbow, alpha);
}
)";

} // namespace RainbowShaders

#endif // RAINBOW_SHADERS_H