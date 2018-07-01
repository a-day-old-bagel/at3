#version 450 core

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(triangles) in;
layout(line_strip, max_vertices = 6) out;

layout(location = 0) in vec3 vsNorm[];
layout(location = 1) in mat4 vsVp[];
layout(location = 0) out vec3 outColor;

const float lineLength = 0.25;

void main() {
    for(int i = 0; i < gl_in.length(); i++) {
        gl_Position = vsVp[i] * gl_in[i].gl_Position;
        outColor = vec3(1.0, 0.0, 0.0);
        EmitVertex();

        gl_Position = vsVp[i] * vec4(gl_in[i].gl_Position.xyz + vsNorm[i] * lineLength, 1.0);
        outColor = vec3(0.0, 0.0, 1.0);
        EmitVertex();

        EndPrimitive();
    }
}
