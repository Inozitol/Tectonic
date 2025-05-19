#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_buffer_reference : require

#include "input_layouts.glsl"

layout(set = 1, binding = 0) uniform PickingBufferInput{
    uint id;
} pickingDataInput;

layout(buffer_reference, std430, set = 1, binding = 1) writeonly buffer PickingBuffer {
    uint id[];
} pickingDataOutput;

layout (location = 0) out vec4 outFragColor;

void main() {
    pickingDataOutput.id[0] = pickingDataInput.id;
    outFragColor = vec4(pickingDataInput.id,pickingDataInput.id,pickingDataInput.id,1.0);
}