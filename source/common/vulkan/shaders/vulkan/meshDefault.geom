#version 450 core

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (triangles) in;
//layout (line_strip, max_vertices = 6) out;
layout (triangle_strip, max_vertices = 3) out;

layout(location = 0) in vec3 inNormal[];
layout(location = 1) in vec2 inUV[];
layout(location = 0) out vec3 outNormal;
layout(location = 1) out vec2 outUV;

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

void main()
{


//    uint index = indices.raw & 0x1FFu;
//    mat4 mvp = uboPage.slot[index].mvp;
//	float normalLength = 0.02;
//	for(int i=0; i<gl_in.length(); i++)
//	{
//		vec3 pos = gl_in[i].gl_Position.xyz;
//		vec3 normal = inNormal[i].xyz;
//
////		gl_Position = ubo.projection * (ubo.model * vec4(pos, 1.0));
//		gl_Position = mvp * vec4(pos, 1.0);
//		outColor = vec3(1.0, 0.0, 0.0);
//		outUV = inUV[i];
//		EmitVertex();
//
////		gl_Position = ubo.projection * (ubo.model * vec4(pos + normal * normalLength, 1.0));
//		gl_Position = mvp * vec4(pos + normal * normalLength, 1.0);
//		outColor = vec3(0.0, 0.0, 1.0);
//		outUV = inUV[i] + vec2(0.5);
//		EmitVertex();
//
//		EndPrimitive();
//	}



    for(int i=0; i<gl_in.length(); i++) {
        gl_Position = gl_in[i].gl_Position;
        outNormal = inNormal[i];
        outUV = inUV[i];
        EmitVertex();

//        gl_Position = gl_in[i].gl_Position;
//        outNormal = inNormal[i];
//        outUV = inUV[i];
//        EmitVertex();
//
//        gl_Position = gl_in[i].gl_Position + vec4(inNormal[i], 1.0);
//        outNormal = inNormal[i];
//        outUV = inUV[i];
//        EmitVertex();
    }
}
