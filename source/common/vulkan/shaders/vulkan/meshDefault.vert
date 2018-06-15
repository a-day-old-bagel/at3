#version 450 core

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location=0) in vec3 vertex;
layout(location=1) in vec2 uv;
layout(location=2) in vec3 normal;

layout(location=0) out vec3 fragNorm;
layout(location=1) out vec2 fragUV;

struct transform {
	mat4 mvp;
	mat4 it_mv;
};
layout(set = 0, binding = 0) uniform UboPage {
	transform slot[512];
} uboPage;
layout(push_constant) uniform InstanceIndices {
	uint raw;
} indices;

void main() {
    uint index = indices.raw & 0x1FFu;
    gl_Position = uboPage.slot[index].mvp * vec4(vertex, 1.0);
	fragNorm =  (uboPage.slot[index].it_mv * vec4(normal, 0.0)).xyz;
	fragUV = uv;
}
