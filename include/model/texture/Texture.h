#ifndef TECTONIC_TEXTURE_H
#define TECTONIC_TEXTURE_H

#include "exceptions.h"
#include "Logger.h"
#include <string>
#include <unordered_map>
#include <memory>

/**
 * Class representing a texture. Stores texture data inside OpenGL buffers.
 */
class Texture {
public:
    Texture(GLenum tex_target, u_char *data, int32_t length, uint8_t channels, const std::string& fileName = "");
    Texture(GLenum tex_target, const std::string& file_name);

    static std::shared_ptr<Texture> createTexture(GLenum tex_target, u_char *data, int32_t length, uint8_t channels, const std::string& fileName = "");
    static std::shared_ptr<Texture> createTexture(GLenum tex_target, const std::string& file_name);

    void bind(GLenum tex_unit) const;
    void unbind(GLenum tex_unit) const;
    [[nodiscard]] GLuint64 getHandle() const;

    const std::string& name();

private:
    void loadData(u_char* data, int32_t width, int32_t height, uint8_t bpp);

    static std::unordered_map<std::string, std::shared_ptr<Texture>> m_loadedTextures;

    std::string m_fileName;
    GLenum m_texTarget = -1;
    GLuint m_texObject = -1;

    GLuint64 m_bindlessHandle = -1;

    static Logger m_logger;
};

#endif //TECTONIC_TEXTURE_H
