#extension GL_EXT_buffer_reference : require

struct Vertex{
    vec3 pos;
    float uvX;
    vec3 normal;
    float uvY;
    vec3 color;
};

layout(buffer_reference, std430) readonly buffer VertexBuffer{
    Vertex vertices[];
};

layout(push_constant) uniform constants{
    mat4 modelMatrix;
    VertexBuffer vertexBuffer;
} PushConstants;
