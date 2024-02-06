
#include shaders/inc/buffersLayout.glsl

uniform mat4 u_WVP;
uniform float u_minHeight;
uniform float u_maxHeight;

out vec4 Color0;
out vec2 TexCoord0;
out vec3 Pos0;
out vec3 Normal0;

void main(){
    vec4 localPos = vec4(Position, 1.0f);
    vec4 localNormal = vec4(Normal, 0.0f);

    gl_Position = u_WVP * localPos;
    TexCoord0 = TexCoord;
    Normal0 = localNormal.xyz;
    Pos0 = localPos.xyz;

    float deltaHeight = u_maxHeight - u_minHeight;
    float heightRation = (Position.y - u_minHeight) / deltaHeight;
    float c = heightRation * 0.8 + 0.2;
    Color0 = vec4(c,c,c,1.0);
}