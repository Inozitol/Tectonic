#version 460

#extension GL_GOOGLE_include_directive : require

#include "../primitives.glsl"

layout (location = 0) out vec3 outUVW;

void main(){
    Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
    outUVW = v.pos;
    gl_Position = vec4(v.pos, 1.0);
}