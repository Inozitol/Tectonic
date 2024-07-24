
#include <GLFW/glfw3.h>
#include <cstdio>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include "engine/EngineCore.h"
#include "engine/vulkan/VktCore.h"

int main(){
    //m_window = g_renderer.m_window;

    /*
        m_window->connectKeyboard(g_keyboard);
        m_window->connectCursor(g_cursor);

        initKeyGroups();
        initScenes();

        renderLoop();
    } catch(tectonicException& te){
        fprintf(stderr, "%s", te.what());
    }*/

    try {
        EngineCore& core = EngineCore::getInstance();
        core.run();
        core.clean();
    } catch(tectonicException& te){
        fprintf(stderr, "%s", te.what());
    }
    return 0;
}