#ifndef TECTONIC_SHADER_H
#define TECTONIC_SHADER_H

#include <list>
#include <regex>
#include <array>
#include <unordered_map>

#include "extern/glad/glad.h"
#include "exceptions.h"

static const char* versionString = "#version 450 core\n";

/**
 * @brief Base class for managing shader code.
 */
class Shader {
public:
    enum class ShaderType {
        UNKNOWN = 0,
        BASIC_SHADER = 1 << 0,
        BONE_SHADER = 1 << 1
    };

    explicit Shader(ShaderType types);
    virtual ~Shader();

    /**
     * @brief Initializes the object by creating a shader program.
     */
    virtual void init();

    /**
     * @brief Deallocates shader programs
     */
    void clean();

    /**
     * @brief Uses the internal shader program.
     */
    void enable(ShaderType type = ShaderType::BASIC_SHADER);

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

    uint32_t cacheUniform(const char* uniformName, ShaderType type = ShaderType::UNKNOWN);
    GLint getUniformLocation(uint32_t index) const;

    //void loadUniforms();

    ShaderType m_typeEnabled = ShaderType::BASIC_SHADER;

private:

    static void replaceIncludes(std::string& codeString, std::string prefix = "#include");
    static void splitBoneAnimation(std::string& codeString, std::string& boneCodeString, std::string lookup = "#BONE_SWITCH");

    std::list<GLuint> m_shaderObjList;
    std::list<GLuint> m_boneShaderObjList;
    GLuint m_shaderProgram = -1;
    GLuint m_boneShaderProgram = -1;

    ShaderType m_shaderTypes;

    std::unordered_map<ShaderType, std::unordered_map<uint32_t, GLint>> m_uniformCache;
    uint32_t m_uniformCount = 0;
};

Shader::ShaderType operator|(Shader::ShaderType ls, Shader::ShaderType rs);
bool operator&(Shader::ShaderType ls, Shader::ShaderType rs);

#endif //TECTONIC_SHADER_H
