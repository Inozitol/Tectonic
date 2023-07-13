layout (location = 0) in vec3 Position;

uniform mat4 wvp;

void main(){
    gl_Position = wvp * vec4(Position, 1.0);
}