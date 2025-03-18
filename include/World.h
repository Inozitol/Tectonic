#pragma once

#include "engine/model/Model.h"
#include "engine/TecCore.h"
#include "engine/model/terrain/Terrain.h"
#include "engine/vulkan/Skybox.h"

struct World {
    World() = default;
    ~World() = default;

    void clear();

    std::vector<Model> models{};
    Terrain* terrain = nullptr;
    Skybox* skybox = nullptr;
    Transformation transformation{};

    VktTypes::GPU::SceneData sceneData;

    void updateScene();
    void gatherDrawContext(VktTypes::DrawContext &ctx) const;

    /*
    DirectionalLight                            m_dirLight;
    std::array<SpotLight, MAX_SPOT_LIGHTS>      m_spotLights;
    decltype(MAX_SPOT_LIGHTS)                   m_spotLightsCount;
    std::array<PointLight, MAX_POINT_LIGHTS>    m_pointLights;
    decltype(MAX_POINT_LIGHTS)                  m_pointLightsCount;
*/

};