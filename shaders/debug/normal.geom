#version 460

#extension GL_GOOGLE_include_directive : require

#include "../input_layouts.glsl"
#include "../primitives.glsl"

layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

layout (location = 0) in vec3 inNormal[];

layout (location = 0) out vec3 outColor;

void main(void)
{
    float normalLength = 0.02;
    for(int i=0; i<gl_in.length(); i++)
    {
        vec3 pos = gl_in[i].gl_Position.xyz;
        vec3 normal = inNormal[i].xyz;

        gl_Position = sceneData.viewproj * (PushConstants.modelMatrix * vec4(pos, 1.0));
        outColor = vec3(1.0, 0.0, 0.0);
        EmitVertex();

        gl_Position = sceneData.viewproj * (PushConstants.modelMatrix * vec4(pos + normal * normalLength, 1.0));
        outColor = vec3(0.0, 0.0, 1.0);
        EmitVertex();

        EndPrimitive();
    }
}