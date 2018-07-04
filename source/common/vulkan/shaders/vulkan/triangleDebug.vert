#version 450 core

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout(location = 0) in vec3 modelVertex;
layout(location = 1) in vec2 unused;
layout(location = 2) in vec3 modelNormal;

layout(location = 0) out vec3 vsNorm;
layout(location = 1) out mat4 vsVp;

struct transform {
	mat4 vp;
	mat4 m;
};
layout(set = 0, binding = 0) uniform UboPage {
	transform slot[512];
} uboPage;
layout(push_constant) uniform InstanceIndices {
	uint raw;
} indices;

void main() {
    uint index = indices.raw & 0x1FFu;


//    gl_Position = uboPage.slot[index].mvp * vec4(modelVertex, 1.0);
//	  vsNorm =  (uboPage.slot[index].mvp * vec4(modelVertex + modelNormal * 0.1, 1.0)).xyz;

//    gl_Position = uboPage.slot[index].mvp * vec4(modelVertex, 1.0);
//	vsNorm = modelNormal;
//	mvp = uboPage.slot[index].mvp;


//    gl_Position = uboPage.slot[index].vp * uboPage.slot[index].m * vec4(modelVertex, 1.0);
    gl_Position = uboPage.slot[index].m * vec4(modelVertex, 1.0);
    vsVp = uboPage.slot[index].vp;
	vsNorm = normalize((uboPage.slot[index].m * vec4(modelNormal, 0.0)).xyz);





//	vsNorm =  (uboPage.slot[index].custom * vec4(modelVertex + modelNormal * 0.1, 1.0)).xyz;
//	vsNorm =  vec4(modelVertex + modelNormal * 0.1, 1.0).xyz;
//	vsNorm =  (uboPage.slot[index].mvp * vec4(modelVertex + vec3(0.0, 0.0, 1.0), 1.0)).xyz;
}
