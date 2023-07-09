#include "Technique.h"

Technique::Technique() {
    _shader_program = 0;
}

Technique::~Technique() {
    for(unsigned int & it : _shader_obj_list){
        glDeleteShader(it);
    }
    if(_shader_program != 0){
        glDeleteProgram(_shader_program);
        _shader_program = 0;
    }
}

void Technique::init() {
    _shader_program = glCreateProgram();

    if(_shader_program == 0){
        throw technique_exception("Unable to create shader program");
    }
}

void Technique::add_shader(GLenum type, const char *filename) {
    std::string s;
    if(!read_file(filename, s)){
        throw technique_exception("Unable to open shader file");
    }
    GLuint shader_obj = glCreateShader(type);
    if(!shader_obj){
        throw technique_exception("Unable to create shader object");
    }
    _shader_obj_list.push_back(shader_obj);

    const GLchar* p[1];
    p[0] = s.c_str();
    GLint lengths[1] = {static_cast<GLint>(s.size()) };

    glShaderSource(shader_obj, 1, p, lengths);
    glCompileShader(shader_obj);

    GLint success;
    glGetShaderiv(shader_obj, GL_COMPILE_STATUS, &success);
    if(!success){
        GLchar infolog[1024];
        glGetShaderInfoLog(shader_obj, 1024, nullptr, infolog);
        throw technique_exception(infolog);
    }
    glAttachShader(_shader_program, shader_obj);
}

bool Technique::read_file(const char *filename, std::string &content) {
    std::ifstream f(filename);
    bool ret = false;

    if(f.is_open()){
        std::string line;
        while(std::getline(f, line)){
            content.append(line);
            content.append("\n");
        }
        f.close();
        ret = true;
    }
    return ret;
}

void Technique::finalize() {
    GLint success = 0;
    GLchar err_log[1024] = {0};
    glLinkProgram(_shader_program);
    glGetProgramiv(_shader_program, GL_LINK_STATUS, &success);
    if(!success){
        glGetProgramInfoLog(_shader_program, sizeof(err_log), nullptr, err_log);
        throw technique_exception(err_log);
    }

    glValidateProgram(_shader_program);
    glGetProgramiv(_shader_program, GL_VALIDATE_STATUS, &success);
    if(!success){
        glGetProgramInfoLog(_shader_program, sizeof(err_log), nullptr, err_log);
        throw technique_exception(err_log);
    }

    for(const auto& shader: _shader_obj_list){
        glDeleteShader(shader);
    }

    _shader_obj_list.clear();
}

GLint Technique::uniform_location(const char *uniform_name) const {
    GLint location = glGetUniformLocation(_shader_program, uniform_name);

    if(location == INVALID_UNIFORM_LOCATION){
        throw technique_exception("Unable to get uniform location");
    }
    return location;
}

void Technique::enable() const {
    glUseProgram(_shader_program);

}


