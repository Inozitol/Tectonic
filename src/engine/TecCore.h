#pragma once

#include <queue>

#include "scene/model/Animatrix.h"

#include "utils/Logger.h"
#include "engine/io/Window.h"
#include "camera/GameCamera.h"
#include "connector/Slot.h"
#include "defs/ConfigDefs.h"
#include "utils/exceptions.h"

#include "../../tools/gltf2tec/gltf2tec.h"
#include "vulkan/VktCore.h"
#include "Player.h"

struct World;

struct TecCore
{
    TecCore();
    ~TecCore();

    /** Starts the main game loop */
    void run();

    /** Destroys all resources of the engine */
    void clean();

    static void glfwErrorCallback(int, const char* msg);

    void initKeyGroups();
    void initKeyboard();
    void initCursor();
    void initWorld();

    bool isInitialized = false;

    Keyboard keyboard;
    Cursor cursor;

    World* world = nullptr;

    float deltaTime = 0.0;
    float currTime = 0.0;
    float prevTime = 0.0;
};
