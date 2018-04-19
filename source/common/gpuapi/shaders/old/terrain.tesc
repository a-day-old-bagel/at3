
layout(vertices = 4) out;

out vec3 tcPosition[];

uniform vec2 screen_size;
uniform mat4 mvp;
uniform mat4 modelView;
uniform mat4 projection;
uniform float lod_fidelity;
uniform float maxPatchSize;
uniform float maxFieldViewDot;

#define ID gl_InvocationID

bool offscreen_scr(vec4 screenVert) {
    if(screenVert.z < -0.5){
        return true;
    }
    return any(
        bvec4(lessThan(screenVert.xy, vec2(-8.0)), // -1.7
        greaterThan(screenVert.xy, vec2(8.0)))     //  1.7
    );
}

bool offscreen_view(vec4 viewVert) {
    return (dot(normalize(viewVert.xyz), vec3(0.0, 0.0, -1.0)) < maxFieldViewDot) && (length(viewVert) > maxPatchSize);
}

void spherify(inout vec4 v0, inout vec4 v1) {
    vec4 center = (v0 + v1) * 0.5;
    float len = distance(v0, v1);
    v0 = center;
    v1 = center;
    // FIXME: is length of horiz line double what it should be?
    v0.x -= len;
    v1.x += len;
}

vec4 multView(vec4 vertex){
    return modelView * vertex;
}

vec4 multProj(vec4 vertex) {
    vec4 result = projection * vertex;
    result /= result.w;
    return result;
}

vec2 screenSpace(vec4 vertex){
    return (clamp(vertex.xy, -1.3, 1.3) + 1) * (screen_size * 0.5);
}

float screenLevel(vec2 v0, vec2 v1){
    return clamp(distance(v0, v1) * lod_fidelity, 1, 64);
}

float level(vec4 v0, vec4 v1){
    vec4 v0Sph = v0, v1Sph = v1;
    spherify(v0Sph, v1Sph);
    v0Sph = multProj(v0Sph);
    v1Sph = multProj(v1Sph);
    vec2 ss0 = screenSpace(v0Sph);
    vec2 ss1 = screenSpace(v1Sph);
    return screenLevel(ss0, ss1);
}

void main(){
//    tcPosition[ID] = vPosition[ID];
    tcPosition[ID] = gl_in[ID].gl_Position.xyz;
    if(ID == 0){
        vec4 v0 = multView(gl_in[0].gl_Position);
        vec4 v1 = multView(gl_in[1].gl_Position);
        vec4 v2 = multView(gl_in[2].gl_Position);
        vec4 v3 = multView(gl_in[3].gl_Position);

//        if(all(bvec4(offscreen_scr(multProj(v0)), offscreen_scr(multProj(v1)),
//                     offscreen_scr(multProj(v2)), offscreen_scr(multProj(v3)))))
        if(all(bvec4(offscreen_view(v0), offscreen_view(v1),
                     offscreen_view(v2), offscreen_view(v3))))
        {
            gl_TessLevelInner[0] = 0;
            gl_TessLevelInner[1] = 0;
            gl_TessLevelOuter[0] = 0;
            gl_TessLevelOuter[1] = 0;
            gl_TessLevelOuter[2] = 0;
            gl_TessLevelOuter[3] = 0;
        }
        else{
            float e0 = level(v1, v2);
            float e1 = level(v0, v1);
            float e2 = level(v3, v0);
            float e3 = level(v2, v3);

            float maxLevel = max(max(e0, e1), max(e2, e3));

            gl_TessLevelInner[0] = maxLevel; // mix(e1, e2, 0.5);
            gl_TessLevelInner[1] = maxLevel; // mix(e0, e3, 0.5);
            gl_TessLevelOuter[0] = e0;
            gl_TessLevelOuter[1] = e1;
            gl_TessLevelOuter[2] = e2;
            gl_TessLevelOuter[3] = e3;
        }
    }
    gl_out[ID].gl_Position = gl_in[ID].gl_Position;
}
