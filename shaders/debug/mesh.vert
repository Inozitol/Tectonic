#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

#include "../input_layouts.glsl"
#include "../primitives.glsl"

layout (location = 0) out vec3 outNormal;

void main(){
    Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];

    mat4 mvMatrix = sceneData.view * PushConstants.modelMatrix;
    mat3 normMatrix = transpose(inverse(mat3(mvMatrix)));

    //outNormal = normalize(normMatrix * v.normal);
    outNormal = v.normal;
    gl_Position = vec4(v.pos, 1.0f);
}