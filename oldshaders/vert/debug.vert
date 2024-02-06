#include shaders/inc/buffersLayout.glsl
#include shaders/inc/boneTransformation.glsl

uniform mat4 u_WVP;
uniform mat4 u_world;

out vec3 Normal0;

void main() {
    vec4 localPos = vec4(Position, 1.0f);
    vec4 localNormal = vec4(Normal, 0.0f);

    localPos = #BONE_SWITCH[localPos | boneTransform()*localPos]
    localNormal = #BONE_SWITCH[localNormal | boneTransform()*localNormal]

    gl_Position = u_WVP * localPos;
    Normal0 = normalize((u_world * localNormal).xyz);
}