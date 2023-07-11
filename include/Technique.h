#ifndef TECTONIC_TECHNIQUE_H
#define TECTONIC_TECHNIQUE_H

#include <list>
#include <fstream>
#include "extern/glad/glad.h"
#include "exceptions.h"
#include "utils.h"

class Technique {
public:
    Technique();
    virtual ~Technique();
    virtual void init();
    void enable() const;
protected:
    void add_shader(GLenum type, const char* filename);
    void finalize();
    GLint uniform_location(const char* uniform_name) const;
    GLuint _shader_program;
private:
    typedef std::list<GLuint> shader_obj_list_t;
    shader_obj_list_t _shader_obj_list;

    static bool read_file(const char* filename, std::string& content);
};

#endif //TECTONIC_TECHNIQUE_H
