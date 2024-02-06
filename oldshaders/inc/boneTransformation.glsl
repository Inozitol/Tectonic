uniform mat4 u_bonesMatrices[MAX_BONES];

mat4 boneTransform(){
    mat4 boneTransform = mat4(0.0f);
    for (int i = 0; i < MAX_BONES_INFLUENCE; i++){
        if (BoneID[i] == -1){
            continue;
        }
        if (BoneID[i] >= MAX_BONES){
            boneTransform = mat4(1.0f);
            break;
        }
        boneTransform += u_bonesMatrices[BoneID[i]] * Weight[i];
    }
    return boneTransform;
}