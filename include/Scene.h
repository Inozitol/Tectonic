#ifndef TECTONIC_SCENE_H
#define TECTONIC_SCENE_H

#include <unordered_map>
#include <memory>
#include <set>
#include <limits>
#include <utility>

#include "Model.h"
#include "Transformation.h"
#include "shader/LightingShader.h"
#include "shader/shadow/ShadowMapFBO.h"
#include "shader/shadow/ShadowCubeMapFBO.h"
#include "shader/shadow/ShadowMapShader.h"
#include "camera/GameCamera.h"
#include "exceptions.h"
#include "defs/ShaderDefines.h"
#include "Animation.h"

struct ObjectData{
    Transformation transformation;
    Animation animation;
};

class Scene {
public:
    Scene() = default;
    ~Scene() = default;

    void setLightingShader(const std::shared_ptr<LightingShader>& lightingShader);
    void setShadowShader(const std::shared_ptr<ShadowMapShader>& shadowShader);
    void setShadowMapFBO(const std::shared_ptr<ShadowMapFBO>& shadowMapFBO);
    void setShadowCubeMapFBO(const std::shared_ptr<ShadowCubeMapFBO>& shadowCubeMapFBO);
    void setGameCamera(const std::shared_ptr<GameCamera>& gameCamera);

    std::shared_ptr<GameCamera> getGameCamera();

    void setWindowDimension(std::pair<int32_t, int32_t> dimensions);

    using meshIndex_t = uint32_t;
    using objectIndex_t = uint32_t;

    [[nodiscard]] meshIndex_t insertMesh(const std::shared_ptr<Model>& mesh);
    std::shared_ptr<Model> getMesh(meshIndex_t meshIndex);
    void eraseMesh(meshIndex_t meshIndex);

    [[nodiscard]] objectIndex_t createObject(meshIndex_t meshIndex);
    ObjectData & getObject(objectIndex_t objectIndex);
    void eraseObject(objectIndex_t objectIndex);

    using spotLightIndex_t = uint32_t;
    using pointLightIndex_t = uint32_t;

    void insertDirectionalLight(std::shared_ptr<DirectionalLight> dirLight);
    std::shared_ptr<DirectionalLight> getDirectionalLight();
    void eraseDirectionalLight();

    spotLightIndex_t insertSpotLight(const std::shared_ptr<SpotLight>& spotLight);
    std::shared_ptr<SpotLight> getSpotLight(spotLightIndex_t spotLightIndex);
    void eraseSpotLight(spotLightIndex_t spotLightIndex);

    pointLightIndex_t insertPointLight(const std::shared_ptr<PointLight>& pointLight);
    std::shared_ptr<PointLight> getPointLight(pointLightIndex_t pointLightIndex);
    void erasePointLight(pointLightIndex_t pointLightIndex);

    void handleMouseEvent(double x, double y);
    void handleKeyEvent(int32_t key);

    void renderScene();

private:
    void lightingPass();
    void shadowPass();

    void renderMeshLight(const std::shared_ptr<Model>& mesh, Transformation& transformation);
    void renderMeshShadow(const std::shared_ptr<Model>& mesh, Transformation& transformation);

    std::unordered_map<meshIndex_t, std::shared_ptr<Model>> m_meshMap;
    std::unordered_map<objectIndex_t, std::pair<ObjectData, meshIndex_t>> m_objectMap;

    std::shared_ptr<LightingShader>     m_lightingShader;
    std::shared_ptr<ShadowMapShader>    m_shadowShader;
    std::shared_ptr<ShadowMapFBO>       m_shadowMapFBO;
    std::shared_ptr<ShadowCubeMapFBO>   m_shadowCubeMapFBO;

    std::shared_ptr<GameCamera>         m_gameCamera;
    Transformation                      m_worldTransform;

    std::set<meshIndex_t>               m_usedMeshIndexes;
    std::set<objectIndex_t>             m_usedObjectIndexes;

    std::shared_ptr<DirectionalLight>   m_dirLight;
    std::shared_ptr<SpotLight>          m_spotLights[MAX_SPOT_LIGHTS];
    std::shared_ptr<PointLight>         m_pointLights[MAX_POINT_LIGHTS];

    std::pair<int32_t, int32_t> m_winDimensions;
};

#endif //TECTONIC_SCENE_H
