
#include shaders/inc/buffersLayout.glsl
#include shaders/inc/boneTransformation.glsl

uniform mat4 u_WVP;
uniform mat4 u_world;

out vec3 WorldPos0;

void main(){
    vec4 localPos = vec4(Position, 1.0f);

    localPos = #BONE_SWITCH[localPos | boneTransform()*localPos]

    gl_Position = u_WVP * localPos;
    WorldPos0 = (u_world * localPos).xyz;
}