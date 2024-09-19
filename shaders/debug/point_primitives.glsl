#extension GL_EXT_buffer_reference : require

struct PointVertex{
    vec3 pos;
    vec3 color;
};

layout(buffer_reference, std430) readonly buffer PointVertexBuffer{
    PointVertex vertices[];
};

layout(push_constant) uniform constants{
    mat4 modelMatrix;
    PointVertexBuffer vertexBuffer;
} PushConstants;
