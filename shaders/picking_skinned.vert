#version 460

#extension GL_GOOGLE_include_directive : require

#include "input_layouts.glsl"
#include "primitives_skin.glsl"

void main(){
    Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
    mat4 skinMat =
    v.jointWeights.x * PushConstants.jointsBuffer.jointMatrices[int(v.jointIndices.x)] +
    v.jointWeights.y * PushConstants.jointsBuffer.jointMatrices[int(v.jointIndices.y)] +
    v.jointWeights.z * PushConstants.jointsBuffer.jointMatrices[int(v.jointIndices.z)] +
    v.jointWeights.w * PushConstants.jointsBuffer.jointMatrices[int(v.jointIndices.w)];

    vec3 WPos = (PushConstants.modelMatrix * skinMat * vec4(v.pos,1.0f)).xyz;
    gl_Position = sceneData.viewproj * vec4(WPos,1.0f);
}