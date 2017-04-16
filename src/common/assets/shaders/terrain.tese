
layout(quads, fractional_odd_spacing, ccw) in;
in vec3 tcPosition[];
out vec3 tePosition;
out vec4 tePatchDistance;
out vec2 teTexCoord;
out float teDepth;

uniform sampler2D terrain;
uniform mat4 mvp;

void main(){
    float u = gl_TessCoord.x;
    float v = gl_TessCoord.y;

    vec4 a = mix(gl_in[1].gl_Position, gl_in[0].gl_Position, u);
    vec4 b = mix(gl_in[2].gl_Position, gl_in[3].gl_Position, u);
    vec3 tePosition = mix(a, b, v).xyz;
    tePatchDistance = vec4(u, v, 1-u, 1-v);
    teTexCoord = tePosition.xy;
    float height = texture(terrain, teTexCoord).a;
    gl_Position = mvp * vec4(teTexCoord, height, 1.0);
    teDepth = length(gl_Position);
}