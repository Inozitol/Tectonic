
#include shaders/buffersLayout.glsl

uniform mat4 u_WVP;
uniform mat4 u_LightWVP;
uniform mat4 u_world;
uniform mat4 u_bonesMatrices[MAX_BONES];

out vec2 TexCoord0;
out vec3 Normal0;
out vec3 LocalPos0;
out vec3 WorldPos0;
out vec4 LightSpacePos;
flat out ivec4 BoneIDs0;
out vec4 Weights0;
out mat3 TBN;

void main(){
    vec4 localPos = vec4(Position, 1.0f);
    vec4 localNormal = vec4(Normal, 0.0f);

    if (BoneID[0] != -1){
        mat4 boneTransform = mat4(0.0f);
        for (int i = 0; i < MAX_BONES_INFLUENCE; i++){
            if (BoneID[i] == -1){
                continue;
            }
            if (BoneID[i] >= MAX_BONES){
                localPos = vec4(Position, 1.0f);
                break;
            }
            boneTransform += u_bonesMatrices[BoneID[i]] * Weight[i];
        }
        localPos = boneTransform * localPos;
        localNormal = boneTransform * localNormal;
    }

    gl_Position = u_WVP * localPos;
    TexCoord0 = TexCoord;
    Normal0 = (u_world * localNormal).xyz;
    LocalPos0 = localPos.xyz;
    WorldPos0 = (u_world * localPos).xyz;
    LightSpacePos = u_LightWVP * localPos;

    vec3 T = normalize(vec3(u_world * vec4(Tangent, 0.0f)));
    vec3 B = normalize(vec3(u_world * vec4(BiTangent, 0.0f)));
    vec3 N = normalize(Normal0);

    //vec3 T = normalize(Tangent);
    //vec3 B = normalize(BiTangent);
    //vec3 N = normalize(Normal);

    TBN = mat3(T,B,N);
}