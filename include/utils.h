#ifndef TECTONIC_UTILS_H
#define TECTONIC_UTILS_H

#include <fstream>
#include <iostream>
#include <cstring>

#include "extern/glad/glad.h"

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
#define INVALID_UNIFORM_LOC 0xFFFFFFFF

namespace Utils{
    /**
     * @brief Class for binding a VAO by creating an object on stack.
     */
    class EphemeralVAOBind{
    public:
        explicit EphemeralVAOBind(GLuint vao){ glBindVertexArray(vao); }
        ~EphemeralVAOBind(){ glBindVertexArray(0); }
    };

    bool readFile(const char* filename, std::string& content);
}

#endif //TECTONIC_UTILS_H
