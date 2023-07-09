#version 330 core

layout (location = 0) in vec3 Position;
layout (location = 1) in vec2 TexCoord;
layout (location = 2) in vec3 Normal;

struct WVP{
    mat4 world;
    mat4 view;
    mat4 projection;
};

uniform WVP wvp;

out vec2 TexCoord0;
out vec3 Normal0;
out vec3 LocalPos0;

void main(){
    gl_Position = wvp.projection * wvp.view * wvp.world * vec4(Position, 1.0);
    TexCoord0 = TexCoord;
    Normal0 = Normal;
    LocalPos0 = Position;
}