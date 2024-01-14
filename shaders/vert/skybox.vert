#include shaders/inc/buffersLayout.glsl

uniform mat4 u_VP;

out vec3 TexCoord0;

void main() {
    vec4 localPos = vec4(Position, 1.0f);
    gl_Position = (u_VP * localPos).xyww;
    TexCoord0 = Position;
}