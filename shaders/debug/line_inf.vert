#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

#include "../input_layouts.glsl"
#include "./point_primitives.glsl"

layout (location = 0) out vec3 outColor;

void main(){
    PointVertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
    int lineID = gl_VertexIndex % 2;

    outColor = v.color;
    if(lineID == 0) {
        gl_Position = sceneData.viewproj * vec4(v.pos, 1.0f);
    }else{
        gl_Position = sceneData.viewproj * vec4(v.pos.x, v.pos.y*(1 << 20), v.pos.z, 1.0f);
    }
}