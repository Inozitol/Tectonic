#ifndef TECTONIC_MATERIAL_H
#define TECTONIC_MATERIAL_H

#include "Texture.h"

class PBRMaterial{
    float roughness = 0.0f;
    bool isMetal = false;
    glm::vec3 color = glm::vec3(0.0f, 0.0f, 0.0f);
};

/**
 * Class with information about material on a texture.
 * Contains textures for diffusion and specular light information.
 */
class Material {
public:
    glm::vec3 ambientColor = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 diffuseColor = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 specularColor = glm::vec3(0.0f, 0.0f, 0.0f);

    Texture* diffuse = nullptr;
    Texture* specularExp = nullptr;

    PBRMaterial PBPMat;
};

#endif //TECTONIC_MATERIAL_H
