#version 460

#extension GL_GOOGLE_include_directive : require

#include "input_layouts.glsl"
#include "primitives.glsl"

layout (location = 0) out vec3 outUVW;

void main(){
    Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
    outUVW = v.pos;
    //outUVW.xy *= -1.0f;
    mat4 view = mat4(mat3(sceneData.view));
    gl_Position = sceneData.proj * view * vec4(v.pos, 1.0f);
}