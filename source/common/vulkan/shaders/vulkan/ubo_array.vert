#version 450 core

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

struct tdata {
	mat4 mvp;
	mat4 it_mv;
};

layout(set = 0, binding = 0) uniform TRANSFORM_DATA {
	tdata d[512];
} transform;

//layout(push_constant) uniform transformData {
//	uint tform;
//} idx;

layout(push_constant) uniform InstanceIndices {
	uint raw;
} indices;

layout(location=0) in vec3 vertex;
layout(location=1) in vec2 uv;
layout(location=2) in vec3 normal;

layout(location=0) out vec2 fragUV;
layout(location=1) out vec3 fragNorm;

void main()
{
    uint slotIndex = indices.raw & 0x1FFu;

//	gl_Position = transform.d[idx.tform].mvp * vec4(vertex, 1.0);
//	fragNorm =  (transform.d[idx.tform].it_mv * vec4(normal, 0.0)).xyz;

    gl_Position = transform.d[slotIndex].mvp * vec4(vertex, 1.0);
	fragNorm =  (transform.d[slotIndex].it_mv * vec4(normal, 0.0)).xyz;

	fragUV = uv;
}
