in vec3 WorldPos0;
out float LightToPixelDist;

uniform vec3 u_lightWorldPos;

void main(){
    vec3 LightToVertex = WorldPos0 - u_lightWorldPos;
    LightToPixelDist = length(LightToVertex);
}