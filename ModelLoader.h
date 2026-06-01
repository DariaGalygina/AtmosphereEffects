#ifndef MODEL_LOADER_H
#define MODEL_LOADER_H

#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/glm.hpp>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
};

class ModelLoader {
public:
    static bool loadOBJ(const std::string& path,
        std::vector<Vertex>& outVertices,
        std::vector<unsigned int>& outIndices) {

        std::vector<glm::vec3> positions;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> texCoords;

        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "Failed to open OBJ file: " << path << std::endl;
            return false;
        }

        std::string line;
        while (std::getline(file, line)) {
            std::istringstream iss(line);
            std::string prefix;
            iss >> prefix;

            if (prefix == "v") {
                glm::vec3 pos;
                iss >> pos.x >> pos.y >> pos.z;
                positions.push_back(pos);
            }
            else if (prefix == "vn") {
                glm::vec3 norm;
                iss >> norm.x >> norm.y >> norm.z;
                normals.push_back(norm);
            }
            else if (prefix == "vt") {
                glm::vec2 tex;
                iss >> tex.x >> tex.y;
                texCoords.push_back(tex);
            }
            else if (prefix == "f") {
                std::string v1, v2, v3;
                iss >> v1 >> v2 >> v3;

                unsigned int posIndex[3], texIndex[3], normIndex[3];

                // Инициализация
                for (int i = 0; i < 3; i++) {
                    texIndex[i] = 0;
                    normIndex[i] = 0;
                }

                // Поддержка разных форматов
                if (v1.find("//") != std::string::npos) {
                    // Формат v//vn
                    sscanf_s(v1.c_str(), "%d//%d", &posIndex[0], &normIndex[0]);
                    sscanf_s(v2.c_str(), "%d//%d", &posIndex[1], &normIndex[1]);
                    sscanf_s(v3.c_str(), "%d//%d", &posIndex[2], &normIndex[2]);
                }
                else if (std::count(v1.begin(), v1.end(), '/') == 1) {
                    // Формат v/vt
                    sscanf_s(v1.c_str(), "%d/%d", &posIndex[0], &texIndex[0]);
                    sscanf_s(v2.c_str(), "%d/%d", &posIndex[1], &texIndex[1]);
                    sscanf_s(v3.c_str(), "%d/%d", &posIndex[2], &texIndex[2]);
                }
                else {
                    // Формат v/vt/vn
                    sscanf_s(v1.c_str(), "%d/%d/%d", &posIndex[0], &texIndex[0], &normIndex[0]);
                    sscanf_s(v2.c_str(), "%d/%d/%d", &posIndex[1], &texIndex[1], &normIndex[1]);
                    sscanf_s(v3.c_str(), "%d/%d/%d", &posIndex[2], &texIndex[2], &normIndex[2]);
                }

                for (int i = 0; i < 3; i++) {
                    Vertex vertex;
                    vertex.Position = positions[posIndex[i] - 1];
                    if (texIndex[i] > 0 && texIndex[i] <= texCoords.size())
                        vertex.TexCoords = texCoords[texIndex[i] - 1];
                    else
                        vertex.TexCoords = glm::vec2(0.0f);
                    if (normIndex[i] > 0 && normIndex[i] <= normals.size())
                        vertex.Normal = normals[normIndex[i] - 1];
                    else
                        vertex.Normal = glm::vec3(0.0f, 1.0f, 0.0f);
                    outVertices.push_back(vertex);
                    outIndices.push_back(outIndices.size());
                }
            }
        }

        file.close();
        std::cout << "Loaded " << outVertices.size() << " vertices from " << path << std::endl;
        return outVertices.size() > 0;
    }
};

#endif // MODEL_LOADER_H