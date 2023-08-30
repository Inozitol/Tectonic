
uniform uint u_objectIndex;
uniform uint u_drawIndex;

out uvec3 FragColor;

void main() {
    FragColor = uvec3(u_objectIndex, u_drawIndex, gl_PrimitiveID);
}