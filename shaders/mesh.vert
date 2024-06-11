#version 460

#extension GL_GOOGLE_include_directive : require

#include "input_layouts.glsl"
#include "primitives.glsl"

layout (location = 0) out vec3 outNormal;
layout (location = 1) out vec3 outColor;
layout (location = 2) out vec2 outUV;
layout (location = 3) out vec3 outWPos;

void main(){
    Vertex v = PushConstants.vertexBuffer.vertices[gl_VertexIndex];
    outWPos = (PushConstants.modelMatrix * vec4(v.pos,1.0f)).xyz;
    gl_Position = sceneData.viewproj * vec4(outWPos,1.0f);

    outNormal = normalize(transpose(inverse(mat3(PushConstants.modelMatrix))) * v.normal);
    outUV.x = v.uvX;
    outUV.y = v.uvY;
    outColor = v.color.xyz * materialData.colorFactors.xyz;
}