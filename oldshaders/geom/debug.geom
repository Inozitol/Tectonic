layout (triangles) in;
layout (line_strip, max_vertices = 12) out;

in vec3 Normal0[];

void genNormalLine(int index){
    gl_Position = gl_in[index].gl_Position;
    EmitVertex();

    gl_Position = (gl_in[index].gl_Position + vec4(Normal0[index], 0.0f) * DEBUG_NORMAL_MAGNITUDE);
    gl_PrimitiveID = 0;
    EmitVertex();

    EndPrimitive();

    gl_Position = (gl_in[index].gl_Position + vec4(Normal0[index], 0.0f) * DEBUG_NORMAL_MAGNITUDE);
    EmitVertex();

    gl_Position = (gl_in[index].gl_Position
                   + vec4(Normal0[index], 0.0f) * DEBUG_NORMAL_MAGNITUDE
                   + vec4(Normal0[index], 0.0f) * (DEBUG_NORMAL_MAGNITUDE/5));
    gl_PrimitiveID = 1;

    EmitVertex();
    EndPrimitive();
}

void main() {
    genNormalLine(0);
    genNormalLine(1);
    genNormalLine(2);
}