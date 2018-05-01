#version 450 core
#extension GL_ARB_separate_shader_objects : enable

layout (binding = 1) uniform sampler2D diffuseColorSampler;

layout(location=0) in vec2 fragUV;
layout(location=1) in vec3 fragNormal;
layout(location=0) out vec4 outColor;

const vec3 incident = normalize(vec3(1.0, 0.2, 1.0));
const vec4 sunColor = vec4(vec3(1.0, 0.95, 0.9) * 1.2, 1.0);
const vec4 skyColor = vec4(vec3(0.9, 0.95, 1.0) * 1.1, 1.0);

void main()
{
    vec4 diffuseColor = texture(diffuseColorSampler, fragUV, 0.0);
    vec3 norm = normalize(fragNormal);

    float sunAmount = max(0, dot(norm,  incident));
    float skyAmount = 0.33 * (1.0 - sunAmount);
    vec4 lighting = ((sunAmount * sunColor) + (skyAmount * skyColor));
    outColor = diffuseColor * lighting;

//    outColor = vec4(fragUV.x, fragUV.y, 1.0, 1.0);
}
