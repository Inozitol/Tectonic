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

struct World;

struct TecCore
{
    TecCore();
    ~TecCore();

    void run();

    /**
     * @brief De-initializes engine.
     */
    void clean();

    void setWindowSize(int32_t width, int32_t height);

    static void glfwErrorCallback(int, const char* msg);

    std::unordered_map<VktCore::objectID_t, VktCore::EngineObject*> m_objects;

    void initKeyGroups();
    void initKeyboard();
    void initCursor();
    void initGameCamera();
    void initWorld();

    // TODO TMP
    std::unique_ptr<Animatrix> bobAnimatrix;

    bool m_isInitialized = false;

    Keyboard m_keyboard;
    Cursor m_cursor;

    int32_t m_windowWidth{};
    int32_t m_windowHeight{};

    GameCamera* gameCamera = nullptr;
    World* world = nullptr;

    bool m_cursorPressed = false;
    int32_t m_cursorPosX = 0, m_cursorPosY = 0;

    /**
   * @brief Informs the engine when the cursor is being pressed
   */
    Slot<bool> slt_cursorPressed{
        [this](bool isPressed)
        {
            m_cursorPressed = isPressed;
        }
    };

    /**
   * @brief Informs the engine about new m_window dimension
   */
    Slot<int32_t, int32_t> slt_windowDimensions{
        [this](int32_t width, int32_t height)
        {
            setWindowSize(width, height);
        }
    };

    /**
   * @brief Informs the engine about new cursor position
   */
    Slot<double, double> slt_updateCursorPos{
        [this](double x, double y)
        {
            m_cursorPosX = static_cast<int32_t>(x);
            m_cursorPosY = static_cast<int32_t>(y);
        }
    };

    /**
   * @brief Informs th engine with mouse pressed position
   */
    Slot<double, double> slt_updateCursorPressedPos{
        [this](double x, double y)
        {
            m_cursorPosX = static_cast<int32_t>(x);
            m_cursorPosY = static_cast<int32_t>(y);
            m_cursorPressed = true;
        }
    };

    bool m_debugEnabled = false;

    float deltaTime = 0.0;
    float currTime = 0.0;
    float prevTime = 0.0;
};

#endif//TECTONIC_ENGINECORE_H
