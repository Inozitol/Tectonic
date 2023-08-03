layout (location = POSITION_LOCATION) in vec3 Position;
layout (location = TEX_COORD_LOCATION) in vec2 TexCoord;
layout (location = NORMAL_LOCATION) in vec3 Normal;
layout (location = BONE_ID_LOCATION) in ivec4 BoneID;
layout (location = BONE_WEIGHT_LOCATION) in vec4 Weight;

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

void main(){
    vec4 totalPos = vec4(0.0f);
    for(int i = 0; i < MAX_BONES_INFLUENCE; i++){
        if(BoneID[i] == -1){
            continue;
        }
        if(BoneID[i] >= MAX_BONES){
            totalPos = vec4(Position, 1.0f);
            break;
        }
        vec4 localPos = u_bonesMatrices[BoneID[i]] * vec4(Position, 1.0f);
        totalPos += localPos * Weight[i];
        vec3 localNormal = mat3(u_bonesMatrices[BoneID[i]]) * Normal;
    }

    gl_Position = u_WVP * totalPos;
    TexCoord0 = TexCoord;
    Normal0 = Normal;
    LocalPos0 = Position;
    WorldPos0 = (u_world * totalPos).xyz;
    LightSpacePos = u_LightWVP * totalPos;
}