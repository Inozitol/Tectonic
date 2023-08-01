layout (location = 0) in vec3 Position;
layout (location = 1) in vec2 TexCoord;
layout (location = 2) in vec3 Normal;

uniform mat4 u_WVP;
uniform mat4 u_world;

out vec3 WorldPos;

void main(){
    vec4 pos4 = vec4(Position, 1.0);
    gl_Position = u_WVP * pos4;
    WorldPos = (u_world * pos4).xyz;
}