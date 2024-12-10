#ifndef TECTONIC_ENGINECORE_H
#define TECTONIC_ENGINECORE_H

#include "Animatrix.h"


#include <queue>

#include "Logger.h"
#include "Window.h"
#include "camera/GameCamera.h"
#include "connector/Slot.h"
#include "defs/ConfigDefs.h"
#include "exceptions.h"

#include "engine/model/gltf2tec.h"
#include "vulkan/VktCore.h"

class EngineCore {
public:
    EngineCore(EngineCore const &) = delete;
    void operator=(EngineCore const &) = delete;

    static EngineCore &getInstance() {
        static EngineCore instance;
        return instance;
    }

    void run();

    /**
     * @brief De-initializes engine.
     */
    void clean();

    void setWindowSize(int32_t width, int32_t height);

    static void glfwErrorCallback(int, const char *msg);

    /**
     * @brief Informs the engine when the cursor is being pressed
     */
    Slot<bool> slt_cursorPressed{[this](bool isPressed) {
        m_cursorPressed = isPressed;
    }};

    /**
     * @brief Informs the engine about new m_window dimension
     */
    Slot<int32_t, int32_t> slt_windowDimensions{[this](int32_t width, int32_t height) {
        setWindowSize(width, height);
    }};

    /**
     * @brief Informs the engine about new cursor position
     */
    Slot<double, double> slt_updateCursorPos{[this](double x, double y) {
        m_cursorPosX = static_cast<int32_t>(x);
        m_cursorPosY = static_cast<int32_t>(y);
    }};

    /**
     * @brief Informs th engine with mouse pressed position
     */
    Slot<double, double> slt_updateCursorPressedPos{[this](double x, double y) {
        m_cursorPosX = static_cast<int32_t>(x);
        m_cursorPosY = static_cast<int32_t>(y);
        m_cursorPressed = true;
    }};

private:
    EngineCore();
    ~EngineCore();

    VktCore &m_vktCore = VktCore::getInstance();
    std::unordered_map<VktCore::objectID_t, VktCore::EngineObject*> m_objects;

    static void initGLFW();
    void initKeyGroups();
    void initKeyboard();
    void initCursor();
    void initGameCamera();

    // TODO TMP
    std::unique_ptr<Animatrix> bobAnimatrix;

    bool m_isInitialized = false;

    std::unique_ptr<Window> m_window;
    Keyboard m_keyboard;
    Cursor m_cursor;

    int32_t m_windowWidth{};
    int32_t m_windowHeight{};

    std::shared_ptr<GameCamera> m_gameCamera = nullptr;

    bool m_cursorPressed = false;
    int32_t m_cursorPosX = 0, m_cursorPosY = 0;

    bool m_debugEnabled = false;

    Logger m_logger = Logger("EngineCore");
};

#endif//TECTONIC_ENGINECORE_H
