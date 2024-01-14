
in vec3 TexCoord0;

out vec4 FragColor;

uniform samplerCube u_skybox;

void main() {
    FragColor = texture(u_skybox, TexCoord0);
    //FragColor = vec4(1.0,1.0,1.0,1.0);
}