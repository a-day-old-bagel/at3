const bool DrawLines = false;
const vec4 InnerLineColor = vec4(1.0, 1.0, 1.0, 1.0);

in vec3 gFacetNormal;
in vec3 gTriDistance;
in vec4 gPatchDistance;
in vec2 gTexCoord;
in float gDepth;
out vec4 fragColor;

uniform sampler2D texture0;
uniform sampler2D terrain;

const vec3 incident = normalize(vec3(1.0, 0.2, 0.5));
const vec4 light = vec4(1.0, 0.95, 0.9, 1.0) * 1.1;

float amplify(float d, float scale, float offset) {
    d = scale * d + offset;
    d = clamp(d, 0, 1);
    d = 1 - exp2(-2*d*d);
    return d;
}

void main(){
    vec3 normal = normalize(texture(terrain, gTexCoord).xyz);
    vec4 color = vec4(texture(texture0, gTexCoord).rgb, 1.0);
    float noise_factor = texture(texture0, gTexCoord*32).a+0.1;

    float dot_surface_incident = max(0, dot(normal, incident));

    color = color * light * noise_factor * (max(0.1, dot_surface_incident)+0.05)*1.5;

    if (DrawLines) {
        float d1 = min(min(gTriDistance.x, gTriDistance.y), gTriDistance.z);
        float d2 = min(min(min(gPatchDistance.x, gPatchDistance.y), gPatchDistance.z), gPatchDistance.w);
        d1 = 1 - amplify(d1, 50, -0.5);
        d2 = amplify(d2, 100, -0.1);
        color = d2 * color + d1 * d2 * InnerLineColor;
    }

//    color = mix(color, color * 0.5 + vec4(0.5, 0.5, 0.5, 1.0), gDepth * 0.005);

    fragColor = color;
}