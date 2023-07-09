#ifndef TECTONIC_MATERIAL_H
#define TECTONIC_MATERIAL_H

#include "Texture.h"

class PBRMaterial{
    float roughness = 0.0f;
    bool is_metal = false;
    glm::vec3 color = glm::vec3(0.0f, 0.0f, 0.0f);
};

class Material{
public:
    glm::vec3 ambient_color = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 diffuse_color = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 specular_color = glm::vec3(0.0f, 0.0f, 0.0f);

    Texture* diffuse = nullptr;
    Texture* specular_exponent = nullptr;

    PBRMaterial pbr_mat;
};

#endif //TECTONIC_MATERIAL_H
