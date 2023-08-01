layout (location = 0) in vec3 Position;

uniform mat4 u_WVP;

void main(){
    gl_Position = u_WVP * vec4(Position, 1.0f);
}