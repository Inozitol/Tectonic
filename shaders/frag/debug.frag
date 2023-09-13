out vec4 FragColor;

void main() {
    if(gl_PrimitiveID == 0)
        FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
    else if(gl_PrimitiveID == 1){
        FragColor = vec4(1.0f, 0.0f, 0.0f, 1.0f);
    }
}