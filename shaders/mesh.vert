#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

#include "inputLayouts.glsl"

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec3 outUV;

struct Vertex{
    vec3 pos;
    float uvX;
    vec3 normal;
    float uvY;
    vec4 color;
    vec4 jointIndices;
    vec4 jointWeights;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer{
    Vertex vertices[];
};

layout(buffer_reference, std430) readonly buffer JointsBuffer{
    mat4 jointMatrices[];
};

layout(push_constant) uniform constants{
    mat4 renderMatrix;
    VertexBuffer vertexBuffer;
    JointsBuffer jointsBuffer;
} PushConstants;


void main(){
    Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
    mat4 skinMat =
        v.jointWeights.x * PushConstants.jointsBuffer.jointMatrices[int(v.jointIndices.x)] +
        v.jointWeights.y * PushConstants.jointsBuffer.jointMatrices[int(v.jointIndices.y)] +
        v.jointWeights.z * PushConstants.jointsBuffer.jointMatrices[int(v.jointIndices.z)] +
        v.jointWeights.w * PushConstants.jointsBuffer.jointMatrices[int(v.jointIndices.w)];

    vec4 pos = vec4(v.pos, 1.0f);
    gl_Position = sceneData.viewproj * PushConstants.renderMatrix * skinMat * pos;

    outNormal = normalize(transpose(inverse((PushConstants.renderMatrix * skinMat))) * vec4(v.normal, 0.0f)).xyz;
    outColor = v.color.xyz * materialData.colorFactors.xyz;
    outUV.x = v.uvX;
    outUV.y = v.uvY;
}