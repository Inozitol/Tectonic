
#include <GLFW/glfw3.h>
#include <cstdio>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>
#include "engine/EngineCore.h"
#include "engine/vulkan/VktCore.h"

int main(){
    try {
        EngineCore& core = EngineCore::getInstance();
        core.run();
        core.clean();
    } catch(tectonicException& te){
        fprintf(stderr, "%s", te.what());
    }
    return 0;
}