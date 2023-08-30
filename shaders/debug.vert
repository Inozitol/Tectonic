#include shaders/buffersLayout.glsl

uniform mat4 u_WVP;
uniform mat4 u_world;
uniform mat4 u_bonesMatrices[MAX_BONES];

out vec3 Normal0;

void main() {
    vec4 localPos = vec4(0.0f);
    vec4 localNormal = vec4(0.0f);

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
        localPos = boneTransform * vec4(Position, 1.0);
        localNormal = boneTransform * vec4(Normal, 0.0);
    }else{
        localPos = vec4(Position, 1.0f);
        localNormal = vec4(Normal, 0.0f);
    }

    gl_Position = u_WVP * localPos;
    Normal0 = normalize((u_world * localNormal).xyz);
}