#version 460

#extension GL_GOOGLE_include_directive : require

#include "input_layouts.glsl"
#include "primitives_skin.glsl"

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec3 outUV;
layout (location = 3) out vec3 outWPos;

void main(){
    Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
    mat4 skinMat =
        v.jointWeights.x * PushConstants.jointsBuffer.jointMatrices[int(v.jointIndices.x)] +
        v.jointWeights.y * PushConstants.jointsBuffer.jointMatrices[int(v.jointIndices.y)] +
        v.jointWeights.z * PushConstants.jointsBuffer.jointMatrices[int(v.jointIndices.z)] +
        v.jointWeights.w * PushConstants.jointsBuffer.jointMatrices[int(v.jointIndices.w)];

    outWPos = (PushConstants.modelMatrix * skinMat * vec4(v.pos,1.0f)).xyz;
    gl_Position = sceneData.viewproj * PushConstants.modelMatrix * vec4(outWPos,1.0f);

    outNormal = normalize(transpose(inverse(mat3(PushConstants.modelMatrix * skinMat))) * v.normal);
    outColor = v.color.xyz * materialData.colorFactors.xyz;
    outUV.x = v.uvX;
    outUV.y = v.uvY;
}