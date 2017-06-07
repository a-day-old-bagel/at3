uniform mat4 mvp;
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;
in vec3 tePosition[3];
in vec4 tePatchDistance[3];
in vec2 teTexCoord[3];
in float teDepth[3];
out vec3 gFacetNormal;
out vec4 gPatchDistance;
out vec3 gTriDistance;
out vec2 gTexCoord;
out float gDepth;

void main() {
    vec3 A = tePosition[2] - tePosition[0];
    vec3 B = tePosition[1] - tePosition[0];
    gFacetNormal = (mvp * vec4(normalize(cross(A, B)), 0.0)).xyz;

    gPatchDistance = tePatchDistance[0];
    gTriDistance = vec3(1.0, 0.0, 0.0);
    gTexCoord = teTexCoord[0];
    gDepth = teDepth[0];
    gl_Position = gl_in[0].gl_Position; EmitVertex();

    gPatchDistance = tePatchDistance[1];
    gTriDistance = vec3(0.0, 1.0, 0.0);
    gTexCoord = teTexCoord[1];
    gDepth = teDepth[1];
    gl_Position = gl_in[1].gl_Position; EmitVertex();

    gPatchDistance = tePatchDistance[2];
    gTriDistance = vec3(0.0, 0.0, 1.0);
    gTexCoord = teTexCoord[2];
    gDepth = teDepth[2];
    gl_Position = gl_in[2].gl_Position; EmitVertex();

    EndPrimitive();
}