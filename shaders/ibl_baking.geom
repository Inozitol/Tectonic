#version 460

layout(set = 2, binding = 0) uniform GeometryIBLData{
    int data;
} geometryIBLData;

layout (location = 0) in vec3 inUVW[];

layout (location = 0) out vec3 outUVW;

layout (triangles) in;
layout (triangle_strip, max_vertices = 18) out;

mat4 perspectiveMat4(const float fov, const float aspect, const float zNear, const float zFar){
    float tanHalfFov = tan(fov / 2.0);
    mat4 result = mat4(0.0);
    result[0][0] = 1.0 / (aspect * tanHalfFov);
    result[1][1] = 1.0 / tanHalfFov;
    result[2][2] = -(zFar + zNear) / (zFar - zNear);
    result[2][3] = -1.0;
    result[3][2] = -(2.0 * zFar * zNear) / (zFar - zNear);
    return result;
}

mat4 lookAt(const vec3 eye, const vec3 center, const vec3 up){
    vec3 f = normalize(center - eye);
    vec3 s = normalize(cross(f, up));
    vec3 u = cross(s,f);

    mat4 result = mat4(1.0);
    result[0][0] = s.x;
    result[1][0] = s.y;
    result[2][0] = s.z;
    result[0][1] = u.x;
    result[1][1] = u.y;
    result[2][1] = u.z;
    result[0][2] =-f.x;
    result[1][2] =-f.y;
    result[2][2] =-f.z;
    result[3][0] =-dot(s, eye);
    result[3][1] =-dot(u, eye);
    result[3][2] = dot(f, eye);
    return result;
}

mat4 captureProjection = perspectiveMat4(radians(90.0), 1.0, 0.1, 10.0);
mat4 captureViews[6] = mat4[6](
            lookAt(vec3(0.0,0.0,0.0), vec3(1.0,0.0,0.0),   vec3(0.0,-1.0,0.0)),
            lookAt(vec3(0.0,0.0,0.0), vec3(-1.0,0.0,0.0),  vec3(0.0,-1.0,0.0)),
            lookAt(vec3(0.0,0.0,0.0), vec3(0.0,1.0,0.0),   vec3(0.0,0.0,1.0)),
            lookAt(vec3(0.0,0.0,0.0), vec3(0.0,-1.0,0.0),  vec3(0.0,0.0,-1.0)),
            lookAt(vec3(0.0,0.0,0.0), vec3(0.0,0.0,1.0),   vec3(0.0,-1.0,0.0)),
            lookAt(vec3(0.0,0.0,0.0), vec3(0.0,0.0,-1.0),  vec3(0.0,-1.0,0.0))
);

void main() {

    for(int i = 0; i < 6; i++) {
        for (int j = 0; j < gl_in.length(); j++) {
            gl_Layer = i;
            gl_Position = captureProjection * captureViews[i] * gl_in[j].gl_Position;
            outUVW = inUVW[j];
            EmitVertex();
        }
        EndPrimitive();
    }
}