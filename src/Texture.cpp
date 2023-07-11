
#include <iostream>
#include <utility>
#include <GL/gl.h>
#include "Texture.h"
#include "stb_image.h"

Texture::Texture(GLenum tex_target, std::string  file_name)
:_tex_target(tex_target), _file_name(std::move(file_name)){}

Texture::Texture(GLenum tex_target, u_char *data, uint32_t length, uint8_t channels) :_tex_target(tex_target) {
    int x=0,y=0,bpp=0;
    u_char* image_data = stbi_load_from_memory(data, length, &x, &y, &bpp, channels);
    if(!image_data){
        throw texture_exception("Unable to load texture from memory");
    }
    load_data(image_data, x, y, bpp);
    stbi_image_free(image_data);
}

void Texture::load() {
    //stbi_set_flip_vertically_on_load(1);
    int width = 0, height = 0, bpp = 0;
    u_char* image_data = stbi_load(_file_name.c_str(), &width, &height, &bpp, 0);
    if(!image_data){
        throw texture_exception("Unable to load texture [", _file_name, "]");
    }

    std::cout << "Loaded texture [" << _file_name << "]" << std::endl;

    load_data(image_data, width, height, bpp);

    stbi_image_free(image_data);
}

void Texture::load_data(u_char* image_data, int width, int height, int bpp) {
    glGenTextures(1, &_tex_object);
    glBindTexture(_tex_target, _tex_object);
    if(_tex_target == GL_TEXTURE_2D){
        switch(bpp){
            case 1:
                glTexImage2D(_tex_target, 0, GL_RED, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, image_data);
                break;
            case 3:
            case 4:
                glTexImage2D(_tex_target, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image_data);
                break;
            default:
                throw texture_exception("Texture type not implemented");
        }
    }else{
        throw texture_exception("Support for texture target is not implemented");
    }

    glTexParameterf(_tex_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(_tex_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameterf(_tex_target, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(_tex_target, GL_TEXTURE_WRAP_T, GL_CLAMP);

    glBindTexture(_tex_target, 0);
}

void Texture::bind(GLenum tex_unit) const {
    glActiveTexture(tex_unit);
    glBindTexture(_tex_target, _tex_object);
}

