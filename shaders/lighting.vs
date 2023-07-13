layout (location = 0) in vec3 Position;
layout (location = 1) in vec2 TexCoord;
layout (location = 2) in vec3 Normal;

uniform mat4 u_WVP;
uniform mat4 u_LightWVP;

out vec2 TexCoord0;
out vec3 Normal0;
out vec3 LocalPos0;
out vec4 LightSpacePos;

void main(){
    gl_Position = u_WVP * vec4(Position, 1.0);
    TexCoord0 = TexCoord;
    Normal0 = Normal;
    LocalPos0 = Position;
    LightSpacePos = u_LightWVP * vec4(Position, 1.0);
}