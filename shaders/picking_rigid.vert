#version 460

#extension GL_GOOGLE_include_directive : require

#include "input_layouts.glsl"
#include "primitives.glsl"

void main(){
    Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
    vec3 WPos = (PushConstants.modelMatrix * vec4(v.pos,1.0f)).xyz;
    gl_Position = sceneData.viewproj * vec4(WPos,1.0f);
}