#include "shader/Shader.h"

#include "utils.h"

Shader::~Shader() {
    for(unsigned int & it : m_shaderObjList){
        glDeleteShader(it);
    }
    if(m_shaderProgram != 0){
        glDeleteProgram(m_shaderProgram);
        m_shaderProgram = -1;
    }
}

void Shader::init() {
    m_shaderProgram = glCreateProgram();

    if(m_shaderProgram == 0){
        throw shaderException("Unable to create shader program");
    }
}

void Shader::enable() const {
    glUseProgram(m_shaderProgram);
}

void Shader::addShader(GLenum type, const char *filename) {

    std::string shaderText;
    if(!Utils::readFile(filename, shaderText)){
        throw shaderException("Unable to open shader file: ", filename);
    }

    // We can inject defines used in C++ into GLSL by prefixing the source code with them
    std::string prefix;
    if(!Utils::readFile("include/defs/ShaderDefines.h", prefix)){
        throw shaderException("Unable to load ShaderDefines.h file");
    }

    replaceIncludes(shaderText);

    shaderText.insert(0, prefix);
    shaderText.insert(0, versionString);

    GLuint shader_obj = glCreateShader(type);
    if(!shader_obj){
        throw shaderException("Unable to create shader object for shader ",filename);
    }
    m_shaderObjList.push_back(shader_obj);

    const GLchar* p[1];
    p[0] = shaderText.c_str();
    GLint lengths[1] = {static_cast<GLint>(shaderText.size()) };

    glShaderSource(shader_obj, 1, p, lengths);
    glCompileShader(shader_obj);

    GLint success;
    glGetShaderiv(shader_obj, GL_COMPILE_STATUS, &success);
    if(!success){
        GLchar infolog[1024];
        glGetShaderInfoLog(shader_obj, 1024, nullptr, infolog);
        throw shaderException(infolog);
    }
    glAttachShader(m_shaderProgram, shader_obj);
}

void Shader::finalize() {
    GLint success = 0;
    GLchar err_log[1024] = {0};
    glLinkProgram(m_shaderProgram);
    glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &success);
    if(!success){
        glGetProgramInfoLog(m_shaderProgram, sizeof(err_log), nullptr, err_log);
        throw shaderException(err_log);
    }

    glValidateProgram(m_shaderProgram);
    glGetProgramiv(m_shaderProgram, GL_VALIDATE_STATUS, &success);
    if(!success){
        glGetProgramInfoLog(m_shaderProgram, sizeof(err_log), nullptr, err_log);
        throw shaderException(err_log);
    }

    for(const auto& shader: m_shaderObjList){
        glDeleteShader(shader);
    }

    m_shaderObjList.clear();
}

GLint Shader::uniformLocation(const char *uniformName) const {
    GLint location = glGetUniformLocation(m_shaderProgram, uniformName);

    if(location == INVALID_UNIFORM_LOC){
        fprintf(stderr, "Unable to get uniform location [%s]\n", uniformName);
        //throw shaderException("Unable to get uniform location\n", "Uniform: ", uniformName);
    }
    return location;
}

void Shader::replaceIncludes(std::string &codeString, std::string prefix) {
    std::smatch match;
    prefix.append(R"(\s+(.+)\n)");

    while(std::regex_search(codeString, match, std::regex(prefix))){
        const std::string path = match[1];
        std::string code;
        Utils::readFile(path.c_str(), code);
        codeString.replace(match.position(),match.length(),code);
    }
}
