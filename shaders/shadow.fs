in vec3 WorldPos;
out float LightToPixelDist;

uniform vec3 u_lightWorldPos;

void main(){
    vec3 LightToVertex = WorldPos - u_lightWorldPos;
    LightToPixelDist = length(LightToVertex);
}