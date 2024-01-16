#ifndef TECTONIC_SKYBOX_H
#define TECTONIC_SKYBOX_H

#include "model/texture/CubemapTexture.h"
#include "model/Model.h"

class Skybox : public Model {
    friend class EngineCore;
public:
    Skybox();
    void init(const std::array<std::string, CUBEMAP_SIDE_COUNT>& filenames);
private:
    std::unique_ptr<CubemapTexture> m_cubemapTex = nullptr;
};


#endif //TECTONIC_SKYBOX_H
