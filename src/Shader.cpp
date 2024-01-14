#include "shader/Shader.h"

#include "utils.h"

Shader::Shader(Shader::ShaderType types) :m_shaderTypes(types) {}

Shader::~Shader() {
    clean();
}

void Shader::init() {
    if(m_shaderTypes & ShaderType::BASIC_SHADER)
        m_shaderProgram = glCreateProgram();

    if(m_shaderTypes & ShaderType::BONE_SHADER)
        m_boneShaderProgram = glCreateProgram();

    if(m_shaderProgram == 0 || m_boneShaderProgram == 0){
        throw shaderException("Unable to create shader program");
    }
}

void Shader::clean() {
    for(unsigned int & it : m_shaderObjList){
        glDeleteShader(it);
    }
    if(m_shaderProgram != -1){
       glDeleteProgram(m_shaderProgram);
       m_shaderProgram = -1;
    }

    if(m_boneShaderProgram != -1){
        glDeleteProgram(m_boneShaderProgram);
        m_boneShaderProgram = -1;
    }
}

void Shader::enable(ShaderType type) {
    if(!(m_shaderTypes & type))
        throw shaderException("Trying to enable invalid shader type");

    switch(type){
        case ShaderType::BASIC_SHADER:
            glUseProgram(m_shaderProgram);
            break;
        case ShaderType::BONE_SHADER:
            glUseProgram(m_boneShaderProgram);
            break;
    }
    m_typeEnabled = type;
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
    shaderText.insert(0, prefixString);

    // Vertex shaders in bone supported shaders need to be split into BASIC and BONE shaders
    if(type == GL_VERTEX_SHADER && (m_shaderTypes & ShaderType::BONE_SHADER)){
        std::string boneCodeShader;
        splitBoneAnimation(shaderText, boneCodeShader);

        GLuint shaderObj = glCreateShader(type);
        GLuint boneShaderObj = glCreateShader(type);
        if(!shaderObj || !boneShaderObj){
            throw shaderException("Unable to create shader object for shader ", filename);
        }
        m_shaderObjList.push_back(shaderObj);
        m_boneShaderObjList.push_back(boneShaderObj);

        const GLchar* sCode[1];
        sCode[0] = shaderText.c_str();
        GLint sLengths[1] = {static_cast<GLint>(shaderText.size()) };

        glShaderSource(shaderObj, 1, sCode, sLengths);
        glCompileShader(shaderObj);

        const GLchar* bCode[1];
        bCode[0] = boneCodeShader.c_str();
        GLint bLengths[1] = {static_cast<GLint>(boneCodeShader.size()) };

        glShaderSource(boneShaderObj, 1, bCode, bLengths);
        glCompileShader(boneShaderObj);

        GLint success;
        glGetShaderiv(shaderObj, GL_COMPILE_STATUS, &success);
        if(!success){
            GLchar infolog[1024];
            glGetShaderInfoLog(shaderObj, 1024, nullptr, infolog);
            throw shaderException(infolog);
        }

        glGetShaderiv(boneShaderObj, GL_COMPILE_STATUS, &success);
        if(!success){
            GLchar infolog[1024];
            glGetShaderInfoLog(boneShaderObj, 1024, nullptr, infolog);
            throw shaderException(infolog);
        }

        glAttachShader(m_shaderProgram, shaderObj);
        glAttachShader(m_boneShaderProgram, boneShaderObj);
    }else{
        GLuint shaderObj = glCreateShader(type);
        if (!shaderObj) {
            throw shaderException("Unable to create shader object for shader ", filename);
        }
        m_shaderObjList.push_back(shaderObj);
        m_boneShaderObjList.push_back(shaderObj);

        const GLchar* p[1];
        p[0] = shaderText.c_str();
        GLint lengths[1] = {static_cast<GLint>(shaderText.size()) };

        glShaderSource(shaderObj, 1, p, lengths);
        glCompileShader(shaderObj);

        GLint success;
        glGetShaderiv(shaderObj, GL_COMPILE_STATUS, &success);
        if(!success){
            GLchar infolog[1024];
            glGetShaderInfoLog(shaderObj, 1024, nullptr, infolog);
            throw shaderException(infolog);
        }
        if(m_shaderTypes & ShaderType::BASIC_SHADER)
            glAttachShader(m_shaderProgram, shaderObj);
        if(m_shaderTypes & ShaderType::BONE_SHADER)
            glAttachShader(m_boneShaderProgram, shaderObj);
    }
}

void Shader::finalize() {
    GLint success = 0;
    GLchar err_log[1024] = {0};

    if(m_shaderTypes & ShaderType::BASIC_SHADER) {
        glLinkProgram(m_shaderProgram);
        glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(m_shaderProgram, sizeof(err_log), nullptr, err_log);
            throw shaderException(err_log);
        }

        glValidateProgram(m_shaderProgram);
        glGetProgramiv(m_shaderProgram, GL_VALIDATE_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(m_shaderProgram, sizeof(err_log), nullptr, err_log);
            throw shaderException(err_log);
        }

        for (const auto &shader: m_shaderObjList) {
            glDeleteShader(shader);
        }
        m_shaderObjList.clear();
    }

    if(m_shaderTypes & ShaderType::BONE_SHADER) {
        glLinkProgram(m_boneShaderProgram);
        glGetProgramiv(m_boneShaderProgram, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(m_boneShaderProgram, sizeof(err_log), nullptr, err_log);
            throw shaderException(err_log);
        }

        glValidateProgram(m_boneShaderProgram);
        glGetProgramiv(m_boneShaderProgram, GL_VALIDATE_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(m_boneShaderProgram, sizeof(err_log), nullptr, err_log);
            throw shaderException(err_log);
        }

        for (const auto &shader: m_boneShaderObjList) {
            glDeleteShader(shader);
        }

        m_boneShaderObjList.clear();
    }
}

uint32_t Shader::cacheUniform(const char* uniformName, ShaderType sourceShaders){
    if(sourceShaders == ShaderType::UNKNOWN){
        sourceShaders = m_shaderTypes;
    }

    uint32_t index = m_uniformCount;
    m_uniformCount++;

    GLint location = INVALID_UNIFORM_LOC;

    if(m_shaderTypes & ShaderType::BASIC_SHADER && sourceShaders & ShaderType::BASIC_SHADER){
        location = glGetUniformLocation(m_shaderProgram, uniformName);
        if(location == INVALID_UNIFORM_LOC){
            fprintf(stderr, "Invalid BASIC_SHADER uniform of name [%s]\n", uniformName);
        }
        m_uniformCache[ShaderType::BASIC_SHADER][index] = location;
    }

    if(m_shaderTypes & ShaderType::BONE_SHADER && sourceShaders & ShaderType::BONE_SHADER){
        location = glGetUniformLocation(m_boneShaderProgram, uniformName);
        if(location == INVALID_UNIFORM_LOC){
            fprintf(stderr, "Invalid BONE_SHADER uniform of name [%s]\n", uniformName);
        }
        m_uniformCache[ShaderType::BONE_SHADER][index] = location;
    }

    return index;
}

GLint Shader::getUniformLocation(uint32_t index) const {
    return m_uniformCache.at(m_typeEnabled).at(index);
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

void Shader::splitBoneAnimation(std::string& codeString, std::string& boneCodeString, std::string lookup){
    std::smatch match;
    lookup.append(R"(\s*?\[\s*?(.*?)\s*?\|\s*?(.*?)\s*?\]\n)");

    boneCodeString = codeString;

    while(std::regex_search(codeString, match, std::regex(lookup))){
        std::string code = match[1];
        code.append(";");
        codeString.replace(match.position(),match.length(),code);
    }

    while(std::regex_search(boneCodeString, match, std::regex(lookup))){
        std::string code = match[2];
        code.append(";");
        boneCodeString.replace(match.position(),match.length(),code);
    }
}

Shader::ShaderType operator|(Shader::ShaderType ls, Shader::ShaderType rs){
    return static_cast<Shader::ShaderType>(
            static_cast<std::underlying_type<Shader::ShaderType>::type>(ls) |
            static_cast<std::underlying_type<Shader::ShaderType>::type>(rs));
}

bool operator&(Shader::ShaderType ls, Shader::ShaderType rs){
    return static_cast<Shader::ShaderType>(
                   static_cast<std::underlying_type<Shader::ShaderType>::type>(ls) &
                   static_cast<std::underlying_type<Shader::ShaderType>::type>(rs)) == rs;
}

/*
void Shader::loadUniforms() {
    GLint maxUniformNameLen, uniformCount;
    glGetProgramiv(m_shaderProgram, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxUniformNameLen);
    glGetProgramiv(m_shaderProgram, GL_ACTIVE_UNIFORMS, &uniformCount);

    GLint read, size;
    GLenum type;

    std::vector<GLchar> uniformName(maxUniformNameLen,0);

    for(GLint i = 0; i < uniformCount; i++){
        glGetActiveUniform(m_shaderProgram, i, maxUniformNameLen, &read, &size, &type, uniformName.data());
        m_locals[ShaderType::BASIC_SHADER][uniformName.data()] = glGetAttribLocation(m_shaderProgram, uniformName.data());
    }

    glGetProgramiv(m_boneShaderProgram, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxUniformNameLen);
    glGetProgramiv(m_boneShaderProgram, GL_ACTIVE_UNIFORMS, &uniformCount);

    for(GLint i = 0; i < uniformCount; i++){
        glGetActiveUniform(m_boneShaderProgram, i, maxUniformNameLen, &read, &size, &type, uniformName.data());
        m_locals[ShaderType::BONE_SHADER][uniformName.data()] = glGetAttribLocation(m_boneShaderProgram, uniformName.data());
    }
}
*/