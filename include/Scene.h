#ifndef TECTONIC_SCENE_H
#define TECTONIC_SCENE_H

#include <unordered_map>
#include <memory>
#include <set>
#include <limits>
#include <utility>

#include "model/Model.h"
#include "Transformation.h"
#include "shader/LightingShader.h"
#include "shader/shadow/ShadowMapFBO.h"
#include "shader/shadow/ShadowCubeMapFBO.h"
#include "shader/shadow/ShadowMapShader.h"
#include "shader/PickingShader.h"
#include "camera/GameCamera.h"
#include "exceptions.h"
#include "defs/ShaderDefines.h"
#include "model/anim/Animation.h"
#include "Keyboard.h"
#include "Cursor.h"
#include "PickingTexture.h"
#include "Renderer.h"
#include "SceneTypes.h"
#include "model/terrain/Terrain.h"

#include "meta/meta.h"
#include "model/terrain/Skybox.h"

class Scene {
public:
    Scene();
    ~Scene() = default;

    void setGameCamera(const std::shared_ptr<GameCamera>& gameCamera);

    std::shared_ptr<GameCamera> getGameCamera();

    void setWindowDimension(std::pair<int32_t, int32_t> dimensions);

    modelIndex_t insertModel(const std::shared_ptr<Model>& model);
    std::shared_ptr<Model> getModel(modelIndex_t modelIndex);

    skinnedModelIndex_t insertSkinnedModel(const std::shared_ptr<SkinnedModel>& model);
    std::shared_ptr<SkinnedModel> getSkinnedModel(skinnedModelIndex_t skinnedModelIndex);

    void insertTerrain(const std::shared_ptr<Terrain>& terrain);
    std::shared_ptr<Terrain> getTerrain();

    void insertSkybox(const std::shared_ptr<Skybox>& skybox);
    std::shared_ptr<Skybox> getSkybox();

    objectIndex_t createObject(modelIndex_t modelIndex);
    ObjectData& getObject(objectIndex_t objectIndex);

    skinnedObjectIndex_t createSkinnedObject(skinnedModelIndex_t skinnedModelIndex);
    SkinnedObjectData& getSkinnedObject(skinnedObjectIndex_t skinnedObjectIndex);

    DirectionalLight& getDirectionalLight();

    spotLightIndex_t createSpotLight();
    SpotLight& getSpotLight(spotLightIndex_t spotLightIndex);

    pointLightIndex_t createPointLight();
    PointLight& getPointLight(pointLightIndex_t pointLightIndex);

    void handleMouseEvent(double x, double y);
    void handleKeyEvent(int32_t key);

    void renderScene();

    /**
     * @brief Updates the game camera with new mouse positions.
     */
    Slot<double, double> slt_updateMousePosition{[this](double x, double y){
        handleMouseEvent(x, y);
    }};

    /**
     * @brief Updates the game camera with specific keyboard button press.
     */
    Slot<int32_t> slt_receiveKeyboardButton{[this](int32_t key){
        handleKeyEvent(key);
    }};

    /**
     * @brief Toggles between perspective and orthographic projection.
     */
    Slot<> slt_togglePerspective{[this](){
        m_gameCamera->toggleProjection();
        m_gameCamera->createProjectionMatrix();
    }};

    /**
     * @brief Informs the scene when cursor is being pressed
     */
    Slot<bool> slt_cursorPressed{[this](bool isPressed){
        m_cursorIsPressed = isPressed;
    }};

    /**
     * @brief Informs the scene about new cursor position
     */
    Slot<double, double> slt_updateCursorPos{[this](double x, double y){
        m_cursorPosX = static_cast<int32_t>(x);
        m_cursorPosY = static_cast<int32_t>(y);
    }};

    Slot<objectIndex_t> slt_objectClicked{[this](objectIndex_t objectIndex){
        if(m_objectMap.contains(objectIndex))
            m_objectMap.at(objectIndex).first.clicked();
    }};

    Slot<skinnedObjectIndex_t> slt_skinnedObjectClicked{[this](skinnedObjectIndex_t objectIndex){
        if(m_skinnedObjectMap.contains(objectIndex))
            m_skinnedObjectMap.at(objectIndex).first.clicked();
    }};

private:
    std::unordered_map<modelIndex_t, std::shared_ptr<Model>> m_modelMap;
    std::unordered_map<objectIndex_t, std::pair<ObjectData, modelIndex_t>> m_objectMap;
    std::unordered_map<skinnedModelIndex_t, std::shared_ptr<SkinnedModel>> m_skinnedModelMap;
    std::unordered_map<skinnedObjectIndex_t, std::pair<SkinnedObjectData, skinnedModelIndex_t>> m_skinnedObjectMap;
    std::shared_ptr<Terrain> m_terrain;

    std::shared_ptr<GameCamera>         m_gameCamera;
    Transformation                      m_worldTransform;

    std::set<modelIndex_t>              m_usedModelIndexes;
    std::set<objectIndex_t>             m_usedObjectIndexes;
    std::set<skinnedModelIndex_t>       m_usedSkinnedModelIndexes;
    std::set<skinnedObjectIndex_t>      m_usedSkinnedObjectIndexes;

    DirectionalLight                            m_dirLight;
    std::array<SpotLight, MAX_SPOT_LIGHTS>      m_spotLights;
    decltype(MAX_SPOT_LIGHTS)                   m_spotLightsCount;
    std::array<PointLight, MAX_POINT_LIGHTS>    m_pointLights;
    decltype(MAX_POINT_LIGHTS)                  m_pointLightsCount;

    std::shared_ptr<Skybox> m_skybox = nullptr;

    Renderer& m_renderer = Renderer::getInstance();

    std::pair<int32_t, int32_t> m_winDimensions;

    PickingTexture m_pickingTexture;
    bool m_cursorIsPressed = false;
    int32_t m_cursorPosX = 0, m_cursorPosY = 0;

    static Logger m_logger;
};

#endif //TECTONIC_SCENE_H
