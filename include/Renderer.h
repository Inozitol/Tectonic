#ifndef TECTONIC_RENDERER_H
#define TECTONIC_RENDERER_H

#include <queue>

#include "extern/glad/glad.h"
#include "model/Model.h"
#include "camera/GameCamera.h"
#include "shader/LightingShader.h"
#include "shader/shadow/ShadowMapShader.h"
#include "shader/shadow/ShadowCubeMapFBO.h"
#include "shader/shadow/ShadowMapFBO.h"
#include "shader/PickingShader.h"
#include "meta/Signal.h"
#include "meta/Slot.h"
#include "model/anim/Animation.h"
#include "PickingTexture.h"
#include "exceptions.h"
#include "Window.h"
#include "SceneTypes.h"
#include "shader/DebugShader.h"
#include "model/terrain/Terrain.h"
#include "shader/TerrainShader.h"
#include "Logger.h"

class Renderer {
public:
    Renderer(Renderer const&) = delete;
    void operator=(Renderer const&) = delete;

    static Renderer& getInstance(){
        static Renderer instance;
        return instance;
    }

    std::shared_ptr<Window> window;

    void queueModelRender(const ObjectData& object, Model* model);
    void queueSkinnedModelRender(const SkinnedObjectData& object, SkinnedModel* skinnedModel);
    void setTerrainModelRender(const std::shared_ptr<Terrain>& terrain);
    void renderQueues();
    void setWindowSize(int32_t width, int32_t height);
    void setGameCamera(const std::shared_ptr<GameCamera>& camera) { m_gameCamera = camera; };
    void setDirectionalLight(DirectionalLight* light) { m_dirLight = light; }
    void setSpotLight(std::array<SpotLight, MAX_SPOT_LIGHTS>* lights) { m_spotLights = lights;}
    void setSpotLightCount(decltype(MAX_SPOT_LIGHTS) count) { m_spotLightsCount = count; }
    void setPointLight(std::array<PointLight, MAX_POINT_LIGHTS>* lights) { m_pointLights = lights; }
    void setPointLightCount(decltype(MAX_POINT_LIGHTS) count) { m_pointLightsCount = count; }

    static void glfwErrorCallback(int, const char* msg);
    static void GLAPIENTRY
    openGLErrorCallback(GLenum source,
                        GLenum type,
                        GLuint id,
                        GLenum severity,
                        GLsizei length,
                        const GLchar* message,
                        const void* userParam);

    LightingShader      m_lightingShader;
    ShadowMapShader     m_shadowMapShader;
    ShadowMapFBO        m_shadowMapFBO;
    ShadowCubeMapFBO    m_shadowCubeMapFBO;
    PickingShader       m_pickingShader;
    PickingTexture      m_pickingTexture;
    DebugShader         m_debugShader;
    TerrainShader       m_terrainShader;

    Signal<objectIndex_t> sig_objectClicked;
    Signal<skinnedObjectIndex_t> sig_skinnedObjectClicked;

    /**
     * @brief Informs the renderer when the cursor is being pressed
     */
    Slot<bool> slt_cursorPressed{[this](bool isPressed){
        m_cursorPressed = isPressed;
    }};

    /**
     * @brief Informs the renderer about new window dimension
     */
    Slot<int32_t, int32_t> slt_windowDimensions{[this](int32_t width, int32_t height){
        setWindowSize(width, height);
    }};

    /**
     * @brief Informs the renderer about new cursor position
     */
    Slot<double, double> slt_updateCursorPos{[this](double x, double y){
        m_cursorPosX = static_cast<int32_t>(x);
        m_cursorPosY = static_cast<int32_t>(y);
    }};

    /**
     * @brief Informs th renderer with mouse pressed position
     */
    Slot<double, double> slt_updateCursorPressedPos{[this](double x, double y){
        m_cursorPosX = static_cast<int32_t>(x);
        m_cursorPosY = static_cast<int32_t>(y);
        m_cursorPressed = true;
    }};

    /**
     * @brief Toggles between debug rendering mode
     */
    Slot<> slt_toggleDebug{[this](){
        m_debugEnabled = !m_debugEnabled;
    }};

private:
    Renderer();
    ~Renderer();

    using meshQueue_t = std::vector<std::pair<const Material*, std::vector<Drawable>>>;
    using vaoQueue_t = std::unordered_map<GLuint, meshQueue_t>;

    using skinnedMeshQueue_t = std::vector<std::pair<std::pair<const Material*,boneTransfoms_t>, std::vector<SkinnedDrawable>>>;
    using skinnedVaoQueue_t = std::unordered_map<GLuint, skinnedMeshQueue_t>;

    vaoQueue_t m_drawQueue;
    skinnedVaoQueue_t m_skinnedDrawQueue;
    std::shared_ptr<Terrain> m_terrain;

    void initGLFW();
    static void initGL();
    void initShaders();

    void clearRender() const;

    void lightingPass(const meshQueue_t& queue);
    void shadowPass(const meshQueue_t& queue);
    void pickingPass(const meshQueue_t& queue);
    void debugPass(const meshQueue_t& queue);

    void lightingPass(const skinnedMeshQueue_t& queue);
    void shadowPass(const skinnedMeshQueue_t& queue);
    void pickingPass(const skinnedMeshQueue_t& queue);
    void debugPass(const skinnedMeshQueue_t& queue);

    void renderModelLight(const Drawable& drawable);
    void renderModelShadow(const Drawable& drawable);
    void renderModelPicking(const Drawable& drawable);
    void renderModelDebug(const Drawable& drawable);

    void renderModelLight(const SkinnedDrawable& drawable);
    void renderModelShadow(const SkinnedDrawable& drawable);
    void renderModelPicking(const SkinnedDrawable& drawable);
    void renderModelDebug(const SkinnedDrawable& drawable);

    void renderTerrain();

    static inline void renderMesh(const MeshInfo& mesh);

    int32_t m_windowWidth{};
    int32_t m_windowHeight{};

    std::shared_ptr<GameCamera> m_gameCamera = nullptr;
    DirectionalLight* m_dirLight = nullptr;
    std::array<SpotLight, MAX_SPOT_LIGHTS>* m_spotLights = nullptr;
    decltype(MAX_SPOT_LIGHTS) m_spotLightsCount = 0;
    std::array<PointLight, MAX_POINT_LIGHTS>* m_pointLights = nullptr;
    decltype(MAX_POINT_LIGHTS) m_pointLightsCount = 0;

    bool m_cursorPressed = false;
    int32_t m_cursorPosX = 0, m_cursorPosY = 0;

    bool m_debugEnabled = false;

    Logger m_logger = Logger("Renderer");
};

#endif //TECTONIC_RENDERER_H
