#ifndef TECTONIC_SHADER_H
#define TECTONIC_SHADER_H

#include <list>
#include <regex>

#include "extern/glad/glad.h"
#include "exceptions.h"

static const char* versionString = "#version 450 core\n";

/**
 * @brief Base class for managing shader code.
 */
class Shader {
public:
    Shader() = default;
    virtual ~Shader();

    /**
     * @brief Initializes the object by creating a shader program.
     */
    virtual void init();

    /**
     * @brief Uses the internal shader program.
     */
    void enable() const;

protected:

    /**
     * @brief Creates a new shader object with given type from a file.
     * @param type Type of shader being created.
     * @param filename Path to a file with the GLSL shader code.
     */
    void addShader(GLenum type, const char *filename);

    /**
     * Links and validates the shader program.
     * Should be called after calling all addShader methods.
     */
    void finalize();

    /**
     * Receives a location of a uniform. Should be called after finalize method.
     * @param uniformName Name of uniform to receive.
     * @return Location of the uniform.
     */
    GLint uniformLocation(const char* uniformName) const;

private:

    static void replaceIncludes(std::string& codeString, std::string prefix = "#include");

    std::list<GLuint> m_shaderObjList;
    GLuint m_shaderProgram = -1;
};

#endif //TECTONIC_SHADER_H
