
uniform uint u_objectIndex;
uniform uint u_objectFlags;

out uvec2 FragColor;

void main() {
    FragColor = uvec2(u_objectIndex, u_objectFlags);
}