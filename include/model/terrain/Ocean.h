#ifndef TECTONIC_OCEAN_H
#define TECTONIC_OCEAN_H

#include "model/Model.h"

class Ocean : public Model {
    Ocean();
    ~Ocean();

    void init();

private:

    class SpectrumTexture{
    public:
        void init(int32_t winWidth, int32_t winHeight);
        void clean();
        void initTextures(int32_t winWidth, int32_t winHeight);
        void enableWriting() const;
        void disableWriting() const;

    private:
        int32_t m_width = 0, m_height = 0;

        GLuint m_spectrumTexture = -1;
    };

};

#endif //TECTONIC_OCEAN_H
