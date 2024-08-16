#version 460

#extension GL_GOOGLE_include_directive : require

#include "../input_layouts.glsl"
#include "../primitives_skin.glsl"

layout (location = 0) out vec3 outNormal;

void main(){
    Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

    mat4 skinMat =
        v.jointWeights.x * PushConstants.jointsBuffer.jointMatrices[int(v.jointIndices.x)] +
        v.jointWeights.y * PushConstants.jointsBuffer.jointMatrices[int(v.jointIndices.y)] +
        v.jointWeights.z * PushConstants.jointsBuffer.jointMatrices[int(v.jointIndices.z)] +
        v.jointWeights.w * PushConstants.jointsBuffer.jointMatrices[int(v.jointIndices.w)];

    mat4 mvMatrix = sceneData.view * PushConstants.modelMatrix;

    outNormal = normalize(transpose(inverse((mvMatrix * skinMat))) * vec4(v.normal, 0.0f)).xyz;
    outNormal = normalize(skinMat * vec4(v.normal, 0.0f)).xyz;
    gl_Position = skinMat * vec4(v.pos,1.0f);

}