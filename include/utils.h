#ifndef TECTONIC_UTILS_H
#define TECTONIC_UTILS_H

#define ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))
#define INVALID_UNIFORM_LOCATION 0xFFFFFFFF

class EmpheralBindVAO{
public:
    EmpheralBindVAO(GLuint vao){glBindVertexArray(vao);}
    ~EmpheralBindVAO(){glBindVertexArray(0);}
};

#endif //TECTONIC_UTILS_H
