#ifndef RAINBOW_GEOMETRY_H
#define RAINBOW_GEOMETRY_H

#include <vector>
#include <glm/glm.hpp>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct RainbowVertex {
    glm::vec3 Position;
    glm::vec2 TexCoords;
};

class RainbowGeometry {
public:
    static void createArc(std::vector<RainbowVertex>& vertices, std::vector<unsigned int>& indices,
        float radius, float thickness, float height, int segments) {

        vertices.clear();
        indices.clear();

        // Создаем полноценную дугу с толщиной (не просто линию)
        float startAngle = 0.0f;           // 0 градусов (левая сторона)
        float endAngle = M_PI;             // 180 градусов (правая сторона)

        // Создаем два слоя: внутренний и внешний радиус
        float innerRadius = radius - thickness * 0.5f;
        float outerRadius = radius + thickness * 0.5f;

        for (int i = 0; i <= segments; i++) {
            float t = i / (float)segments;
            float angle = startAngle + (endAngle - startAngle) * t;

            float cosA = cos(angle);
            float sinA = sin(angle);

            // Внутренняя точка
            float x1 = cosA * innerRadius;
            float y1 = sinA * innerRadius + height;
            float z1 = 0.0f;

            // Внешняя точка
            float x2 = cosA * outerRadius;
            float y2 = sinA * outerRadius + height;
            float z2 = 0.0f;

            // Текстурные координаты
            float texU = t;

            vertices.push_back({ {x1, y1, z1}, {texU, 0.0f} });
            vertices.push_back({ {x2, y2, z2}, {texU, 1.0f} });

            if (i < segments) {
                int baseIdx = i * 2;
                int nextBaseIdx = (i + 1) * 2;

                // Два треугольника для каждого сегмента
                indices.push_back(baseIdx);
                indices.push_back(baseIdx + 1);
                indices.push_back(nextBaseIdx);

                indices.push_back(nextBaseIdx);
                indices.push_back(baseIdx + 1);
                indices.push_back(nextBaseIdx + 1);
            }
        }
    }

    // Метод для создания радуги с размерами под ландшафт
    static void createArcForTerrain(std::vector<RainbowVertex>& vertices, std::vector<unsigned int>& indices,
        float terrainWidth, float terrainHeight, int segments) {

        vertices.clear();
        indices.clear();

        // Радуга должна занимать всю ширину ландшафта
        float arcWidth = terrainWidth * 0.8f;  // 80% ширины ландшафта
        float arcHeight = terrainHeight * 0.3f; // 30% высоты ландшафта

        float radius = arcWidth / 2.0f;
        float thickness = 8.0f;
        float yOffset = terrainHeight * 0.15f;

        float startAngle = 0.0f;
        float endAngle = M_PI;

        float innerRadius = radius - thickness * 0.5f;
        float outerRadius = radius + thickness * 0.5f;

        for (int i = 0; i <= segments; i++) {
            float t = i / (float)segments;
            float angle = startAngle + (endAngle - startAngle) * t;

            float cosA = cos(angle);
            float sinA = sin(angle);

            // Внутренняя точка
            float x1 = cosA * innerRadius;
            float y1 = sinA * innerRadius + yOffset;
            float z1 = 0.0f;

            // Внешняя точка
            float x2 = cosA * outerRadius;
            float y2 = sinA * outerRadius + yOffset;
            float z2 = 0.0f;

            vertices.push_back({ {x1, y1, z1}, {t, 0.0f} });
            vertices.push_back({ {x2, y2, z2}, {t, 1.0f} });

            if (i < segments) {
                int baseIdx = i * 2;
                int nextBaseIdx = (i + 1) * 2;

                indices.push_back(baseIdx);
                indices.push_back(baseIdx + 1);
                indices.push_back(nextBaseIdx);

                indices.push_back(nextBaseIdx);
                indices.push_back(baseIdx + 1);
                indices.push_back(nextBaseIdx + 1);
            }
        }
    }
};

#endif // RAINBOW_GEOMETRY_H