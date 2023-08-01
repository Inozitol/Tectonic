layout (location = 0) in vec3 Position;
layout (location = 1) in vec2 TexCoord;
layout (location = 2) in vec3 Normal;

uniform mat4 u_WVP;
uniform mat4 u_LightWVP;
uniform mat4 u_world;

out vec2 TexCoord0;
out vec3 Normal0;
out vec3 LocalPos0;
out vec3 WorldPos0;
out vec4 LightSpacePos;

void main(){
    vec4 pos4 = vec4(Position, 1.0);
    gl_Position = u_WVP * pos4;
    TexCoord0 = TexCoord;
    Normal0 = Normal;
    LocalPos0 = Position;
    WorldPos0 = (u_world * pos4).xyz;
    LightSpacePos = u_LightWVP * pos4;
}