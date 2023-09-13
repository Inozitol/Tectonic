
#include shaders/inc/buffersLayout.glsl
#include shaders/inc/boneTransformation.glsl

uniform mat4 u_WVP;
uniform mat4 u_LightWVP;
uniform mat4 u_world;

out vec2 TexCoord0;
out vec3 Normal0;
out vec3 WorldPos0;
out vec4 LightSpacePos;
flat out ivec4 BoneIDs0;
out vec4 Weights0;
out mat3 TBN;

void main(){
    vec4 localPos = vec4(Position, 1.0f);
    vec4 localNormal = vec4(Normal, 0.0f);

    localPos = #BONE_SWITCH[localPos | boneTransform()*localPos]
    localNormal = #BONE_SWITCH[localNormal | boneTransform()*localNormal]

    gl_Position = u_WVP * localPos;
    TexCoord0 = TexCoord;
    Normal0 = (u_world * localNormal).xyz;
    WorldPos0 = (u_world * localPos).xyz;
    LightSpacePos = u_LightWVP * localPos;

    #BONE_SWITCH[vec3 T = normalize(vec3(u_world * vec4(Tangent, 0.0f))) | vec3 T = normalize(vec3(u_world * boneTransform() * vec4(Tangent, 0.0f)))]
    #BONE_SWITCH[vec3 B = normalize(vec3(u_world * vec4(BiTangent, 0.0f))) | vec3 B = normalize(vec3(u_world * boneTransform() * vec4(BiTangent, 0.0f)))]
    #BONE_SWITCH[vec3 N = normalize(vec3(u_world * vec4(Normal, 0.0f))) | vec3 N = normalize(vec3(u_world * boneTransform() * vec4(Normal, 0.0f)))]

    //vec3 T = normalize(Tangent);
    //vec3 B = normalize(BiTangent);
    //vec3 N = normalize(Normal);

    TBN = mat3(T,B,N);
}