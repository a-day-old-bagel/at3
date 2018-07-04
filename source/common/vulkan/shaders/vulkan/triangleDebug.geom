#version 450 core

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(triangles) in;
layout(line_strip, max_vertices = 5) out;

layout(location = 0) in vec3 vsNorm[];
layout(location = 1) in mat4 vsVp[];
layout(location = 0) out vec3 outColor;

const vec3 edgeColor = vec3(1.0, 1.0, 0.0);
const vec3 normStartColor = vec3(1.0, 0.0, 1.0);
const vec3 normEndColor = vec3(0.0, 1.0, 1.0);
const float normLength = 0.25; // Triangle normals are displayed as lines with this length.

const bool enabled = false;

void main() {
    if (!enabled) { return; }
    vec3 normStart = vec3(0.0);
    vec3 normEnd = vec3(0.0);

    // draw the triangle edges while averaging their vertex positions to find the center of the triangle
    for(int i = 0; i < gl_in.length(); i++) {
        gl_Position = vsVp[0] * gl_in[i].gl_Position;
        outColor = edgeColor;
        EmitVertex();
        normStart += gl_in[i].gl_Position.xyz;
        normEnd += vsNorm[i];
    }
    EndPrimitive();

    // perform the division to complete the averages
    normStart /= 3.0;
    normEnd /= 3.0;

    // draw the triangle normal at the center of the triangle, applying the matrix transformation at the last moment.
    gl_Position = vsVp[0] * vec4(normStart, 1.0);
    outColor = normStartColor;
    EmitVertex();
    gl_Position = vsVp[0] * vec4(normStart + normEnd * normLength, 1.0);
    outColor = normEndColor;
    EmitVertex();
    EndPrimitive();
}
