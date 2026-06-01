#ifndef LIGHTNING_GEOMETRY_H
#define LIGHTNING_GEOMETRY_H

#include <vector>
#include <glm/glm.hpp>
#include <random>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

struct LightningVertex {
    glm::vec3 Position;
    glm::vec2 TexCoords;

    LightningVertex(glm::vec3 pos, glm::vec2 uv) : Position(pos), TexCoords(uv) {}
};

class LightningGeometry {
public:

    static void createRealisticLightning(std::vector<LightningVertex>& vertices,
        std::vector<unsigned int>& indices,
        float width, float height,
        int segments = 12, int branches = 3) {
        vertices.clear();
        indices.clear();

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> distortDist(-0.3f, 0.3f);
        std::uniform_real_distribution<float> branchPosDist(0.2f, 0.8f);
        std::uniform_real_distribution<float> branchAngleDist(-0.5f, 0.5f);

        std::vector<glm::vec2> mainPoints;
        for (int i = 0; i <= segments; i++) {
            float t = (float)i / segments;
            float y = t * height - height / 2;

            float xOffset = 0.0f;
            float distortion = 0.0f;

            if (i > 0 && i < segments) {
                distortion = sin(t * M_PI * 3.0f) * 0.15f;
                distortion += sin(t * M_PI * 7.0f) * 0.08f;
                distortion += distortDist(gen) * 0.2f;
                xOffset = distortion * width;
            }

            mainPoints.push_back(glm::vec2(xOffset, y));
        }

        float currentWidth = width * 0.15f; 

        for (size_t i = 0; i < mainPoints.size(); i++) {
            glm::vec2 p = mainPoints[i];

            float t = (float)i / (mainPoints.size() - 1);
            float w = currentWidth * (1.0f - std::abs(t - 0.5f) * 0.8f);

            glm::vec2 dir(0, 1);
            if (i < mainPoints.size() - 1) {
                dir = glm::normalize(mainPoints[i + 1] - p);
            }
            else if (i > 0) {
                dir = glm::normalize(p - mainPoints[i - 1]);
            }

            glm::vec2 perp(-dir.y, dir.x);

            glm::vec3 p1(p.x - perp.x * w, p.y - perp.y * w, 0);
            glm::vec3 p2(p.x + perp.x * w, p.y + perp.y * w, 0);

            vertices.push_back(LightningVertex(p1, glm::vec2(0, t)));
            vertices.push_back(LightningVertex(p2, glm::vec2(1, t)));

            if (i > 0) {
                int baseIdx = (i - 1) * 2;
                int currIdx = i * 2;
                indices.push_back(baseIdx);
                indices.push_back(baseIdx + 1);
                indices.push_back(currIdx);
                indices.push_back(currIdx);
                indices.push_back(baseIdx + 1);
                indices.push_back(currIdx + 1);
            }
        }

        std::uniform_real_distribution<float> branchLength(0.3f, 0.6f);
        std::uniform_real_distribution<float> branchThick(0.03f, 0.08f);

        for (int b = 0; b < branches; b++) {
            float branchT = branchPosDist(gen);
            int branchIdx = (int)(branchT * (mainPoints.size() - 1));
            glm::vec2 startPos = mainPoints[branchIdx];

            float branchLen = branchLength(gen) * height;
            float branchAngle = branchAngleDist(gen) * 1.5f;
            float branchWidth = branchThick(gen) * width;

            glm::vec2 direction(cos(branchAngle), sin(branchAngle));
            if (startPos.x > 0) direction.x = -std::abs(direction.x);
            else direction.x = std::abs(direction.x);

            int branchSegments = 6;
            std::vector<glm::vec2> branchPoints;
            for (int s = 0; s <= branchSegments; s++) {
                float t = (float)s / branchSegments;
                glm::vec2 offset = direction * (t * branchLen);
                float curve = sin(t * M_PI) * 0.15f;
                offset.x += curve * branchWidth * 2;
                branchPoints.push_back(startPos + offset);
            }

            int startVertexIdx = vertices.size();
            for (size_t s = 0; s < branchPoints.size(); s++) {
                glm::vec2 p = branchPoints[s];
                float t = (float)s / (branchPoints.size() - 1);
                float w = branchWidth * (1.0f - t * 0.5f);

                glm::vec2 dir(0, 1);
                if (s < branchPoints.size() - 1) {
                    dir = glm::normalize(branchPoints[s + 1] - p);
                }
                else if (s > 0) {
                    dir = glm::normalize(p - branchPoints[s - 1]);
                }

                glm::vec2 perp(-dir.y, dir.x);

                glm::vec3 p1(p.x - perp.x * w, p.y - perp.y * w, 0);
                glm::vec3 p2(p.x + perp.x * w, p.y + perp.y * w, 0);

                vertices.push_back(LightningVertex(p1, glm::vec2(0, t)));
                vertices.push_back(LightningVertex(p2, glm::vec2(1, t)));

                if (s > 0) {
                    int baseIdx = startVertexIdx + (s - 1) * 2;
                    int currIdx = startVertexIdx + s * 2;
                    indices.push_back(baseIdx);
                    indices.push_back(baseIdx + 1);
                    indices.push_back(currIdx);
                    indices.push_back(currIdx);
                    indices.push_back(baseIdx + 1);
                    indices.push_back(currIdx + 1);
                }
            }
        }

        addSparkBranches(vertices, indices, mainPoints, width, height);
    }

    static void createZigzagLightning(std::vector<LightningVertex>& vertices,
        std::vector<unsigned int>& indices,
        float width, float height) {
        vertices.clear();
        indices.clear();

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> offsetDist(-0.4f, 0.4f);
        std::uniform_real_distribution<float> segmentDist(0.05f, 0.15f);

        std::vector<glm::vec2> points;
        points.push_back(glm::vec2(0, -height / 2));

        float currentY = -height / 2;
        float currentX = 0;

        while (currentY < height / 2) {
            currentY += segmentDist(gen) * height;
            currentX += offsetDist(gen) * width;
            points.push_back(glm::vec2(currentX, currentY));
        }
        points.push_back(glm::vec2(currentX, height / 2));

        float lineWidth = width * 0.1f;
        for (size_t i = 0; i < points.size(); i++) {
            glm::vec2 p = points[i];
            float t = (float)i / (points.size() - 1);
            float w = lineWidth * (1.0f - std::abs(t - 0.5f) * 0.6f);

            glm::vec2 dir(0, 1);
            if (i < points.size() - 1) {
                dir = glm::normalize(points[i + 1] - p);
            }
            else if (i > 0) {
                dir = glm::normalize(p - points[i - 1]);
            }

            glm::vec2 perp(-dir.y, dir.x);

            vertices.push_back(LightningVertex(glm::vec3(p.x - perp.x * w, p.y - perp.y * w, 0), glm::vec2(0, t)));
            vertices.push_back(LightningVertex(glm::vec3(p.x + perp.x * w, p.y + perp.y * w, 0), glm::vec2(1, t)));

            if (i > 0) {
                int baseIdx = (i - 1) * 2;
                int currIdx = i * 2;
                indices.push_back(baseIdx);
                indices.push_back(baseIdx + 1);
                indices.push_back(currIdx);
                indices.push_back(currIdx);
                indices.push_back(baseIdx + 1);
                indices.push_back(currIdx + 1);
            }
        }
    }

    static void createPlane(std::vector<LightningVertex>& vertices,
        std::vector<unsigned int>& indices,
        float width, float height) {
        vertices.clear();
        indices.clear();

        float halfW = width / 2.0f;
        float halfH = height / 2.0f;

        vertices.push_back(LightningVertex(glm::vec3(-halfW, -halfH, 0.0f), glm::vec2(0.0f, 0.0f)));
        vertices.push_back(LightningVertex(glm::vec3(halfW, -halfH, 0.0f), glm::vec2(1.0f, 0.0f)));
        vertices.push_back(LightningVertex(glm::vec3(halfW, halfH, 0.0f), glm::vec2(1.0f, 1.0f)));
        vertices.push_back(LightningVertex(glm::vec3(-halfW, halfH, 0.0f), glm::vec2(0.0f, 1.0f)));

        indices.push_back(0);
        indices.push_back(1);
        indices.push_back(2);
        indices.push_back(2);
        indices.push_back(3);
        indices.push_back(0);
    }

private:
    static void addSparkBranches(std::vector<LightningVertex>& vertices,
        std::vector<unsigned int>& indices,
        const std::vector<glm::vec2>& mainPoints,
        float width, float height) {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_real_distribution<float> sparkProb(0.0f, 1.0f);
        std::uniform_real_distribution<float> sparkLength(0.05f, 0.15f);

        for (size_t i = 1; i < mainPoints.size() - 1; i++) {
            if (sparkProb(gen) > 0.3f) continue;

            glm::vec2 p = mainPoints[i];
            float dir = (p.x > 0) ? -1.0f : 1.0f;
            float len = sparkLength(gen) * height;

            glm::vec2 endPos = p + glm::vec2(dir * len, (sparkProb(gen) - 0.5f) * len);

            int startIdx = vertices.size();
            float w = width * 0.03f;

            vertices.push_back(LightningVertex(glm::vec3(p.x - w, p.y, 0), glm::vec2(0, 0)));
            vertices.push_back(LightningVertex(glm::vec3(p.x + w, p.y, 0), glm::vec2(1, 0)));
            vertices.push_back(LightningVertex(glm::vec3(endPos.x - w, endPos.y, 0), glm::vec2(0, 1)));
            vertices.push_back(LightningVertex(glm::vec3(endPos.x + w, endPos.y, 0), glm::vec2(1, 1)));

            indices.push_back(startIdx);
            indices.push_back(startIdx + 1);
            indices.push_back(startIdx + 2);
            indices.push_back(startIdx + 2);
            indices.push_back(startIdx + 1);
            indices.push_back(startIdx + 3);
        }
    }
}; 

#endif