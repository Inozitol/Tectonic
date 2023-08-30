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
#include "model/Animation.h"
#include "Keyboard.h"
#include "Cursor.h"
#include "PickingTexture.h"
#include "Renderer.h"
#include "SceneTypes.h"

#include "meta/meta.h"

class Scene {
public:
    Scene();
    ~Scene() = default;

    void setGameCamera(const std::shared_ptr<GameCamera>& gameCamera);

    std::shared_ptr<GameCamera> getGameCamera();

    void setWindowDimension(std::pair<int32_t, int32_t> dimensions);

    [[nodiscard]] meshIndex_t insertMesh(const std::shared_ptr<Model>& mesh);
    std::shared_ptr<Model> getMesh(meshIndex_t meshIndex);

    [[nodiscard]] objectIndex_t createObject(meshIndex_t modelIndex);
    ObjectData& getObject(objectIndex_t objectIndex);

    DirectionalLight& getDirectionalLight();

    spotLightIndex_t createSpotLight();
    SpotLight& getSpotLight(spotLightIndex_t spotLightIndex);
    void eraseSpotLight(spotLightIndex_t spotLightIndex);

    pointLightIndex_t createPointLight();
    PointLight& getPointLight(pointLightIndex_t pointLightIndex);
    void erasePointLight(pointLightIndex_t pointLightIndex);

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

private:
    std::unordered_map<meshIndex_t, std::shared_ptr<Model>> m_modelMap;
    std::unordered_map<objectIndex_t, std::pair<ObjectData, meshIndex_t>> m_objectMap;

    std::shared_ptr<GameCamera>         m_gameCamera;
    Transformation                      m_worldTransform;

    std::set<meshIndex_t>               m_usedMeshIndexes;
    std::set<objectIndex_t>             m_usedObjectIndexes;

    DirectionalLight                            m_dirLight;
    std::array<SpotLight, MAX_SPOT_LIGHTS>      m_spotLights;
    decltype(MAX_SPOT_LIGHTS)                   m_spotLightsCount;
    std::array<PointLight, MAX_POINT_LIGHTS>    m_pointLights;
    decltype(MAX_POINT_LIGHTS)                  m_pointLightsCount;

    Renderer& m_renderer = Renderer::getInstance();

    std::pair<int32_t, int32_t> m_winDimensions;

    PickingTexture m_pickingTexture;
    bool m_cursorIsPressed = false;
    int32_t m_cursorPosX = 0, m_cursorPosY = 0;
};

#endif //TECTONIC_SCENE_H
