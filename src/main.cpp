
#include "engine/TecCore.h"
#include "engine/GlobalMemory.h"
#include "engine/vulkan/VktCore.h"
#include <GLFW/glfw3.h>
#include <cstdio>
#include <glm/glm.hpp>
#include <glm/gtx/string_cast.hpp>

#include <execinfo.h>
#include <csignal>

void handler(int sig) {
    void *array[10];
    size_t size;

    // get void*'s for all entries on the stack
    size = backtrace(array, 10);

    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);
    exit(1);
}

int main(){
    signal(SIGFPE, handler);
    try {
        initMemory();
        TecCorePtr->run();
        TecCorePtr->clean();
    } catch(tectonicException& te){
        fprintf(stderr, "%s", te.what());
    }
    return 0;
}