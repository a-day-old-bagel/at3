
in vec2 texCoord;
in float depth;
out vec4 fragment;

uniform sampler2D texture0;
uniform sampler2D terrain;
uniform sampler2D noise_tile;

vec3 incident = normalize(vec3(1.0, 0.2, 0.5));
vec4 light = vec4(1.0, 0.95, 0.9, 1.0) * 1.1;

void main(){
    vec3 normal = normalize(texture(terrain, texcoord).xyz);
    vec4 color = texture0(diffuse, texcoord);
    float noise_factor = texture0(noise_tile, texcoord*32).r+0.1;

    float dot_surface_incident = max(0, dot(normal, incident));

    color = color * light * noise_factor * (max(0.1, dot_surface_incident)+0.05)*1.5;
    fragment = mix(color, color*0.5+vec4(0.5, 0.5, 0.5, 1.0), depth*2.0);
}