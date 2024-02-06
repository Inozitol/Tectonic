#include shaders/inc/buffersLayout.glsl
#include shaders/inc/boneTransformation.glsl

uniform mat4 u_WVP;

void main() {
    vec4 localPos = vec4(Position, 1.0f);

    localPos = #BONE_SWITCH[localPos | boneTransform()*localPos]

    gl_Position = u_WVP * localPos;
}