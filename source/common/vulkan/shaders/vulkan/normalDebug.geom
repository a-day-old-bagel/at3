#version 450 core

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

layout (triangles) in;
layout (line_strip, max_vertices = 6) out;

layout(location = 0) in vec3 vsNorm[];
layout (location = 0) out vec3 outColor;

//struct transform {
//	mat4 mvp;
//	mat4 it_mv;
//};
//layout(set = 0, binding = 0) uniform UboPage {
//	transform slot[512];
//} uboPage;
//layout(push_constant) uniform InstanceIndices {
//	uint raw;
//} indices;

void main()
{


//    uint index = indices.raw & 0x1FFu;
//    mat4 mvp = uboPage.slot[index].mvp;
//	float normalLength = 0.02;
//	for(int i=0; i<gl_in.length(); i++)
//	{
//		vec3 pos = gl_in[i].gl_Position.xyz;
//		vec3 normal = vsNorm[i].xyz;
//
////		gl_Position = ubo.projection * (ubo.model * vec4(pos, 1.0));
//		gl_Position = mvp * vec4(pos, 1.0);
//		outColor = vec3(1.0, 0.0, 0.0);
//		EmitVertex();
//
////		gl_Position = ubo.projection * (ubo.model * vec4(pos + normal * normalLength, 1.0));
//		gl_Position = mvp * vec4(pos + normal * normalLength, 1.0);
//		outColor = vec3(0.0, 0.0, 1.0);
//		EmitVertex();
//
//		EndPrimitive();
//	}






    float normalLength = 0.02;
    for(int i = 0; i < gl_in.length(); i++) {
        gl_Position = gl_in[i].gl_Position;
        outColor = vec3(1.0, 0.0, 0.0);
        EmitVertex();

//        gl_Position = vec4(gl_in[i].gl_Position.xyz + vsNorm[i], 1.0);
        gl_Position = vec4(vsNorm[i], 1.0);
        outColor = vec3(0.0, 0.0, 1.0);
        EmitVertex();

        EndPrimitive();
    }






//    for(int i=0; i<gl_in.length(); i++) {
//        gl_Position = gl_in[i].gl_Position;
//        outNormal = vsNorm[i];
//        outUV = inUV[i];
//        EmitVertex();



//        gl_Position = gl_in[i].gl_Position;
//        outNormal = vsNorm[i];
//        outUV = inUV[i];
//        EmitVertex();
//
//        gl_Position = gl_in[i].gl_Position + vec4(vsNorm[i], 1.0);
//        outNormal = vsNorm[i];
//        outUV = inUV[i];
//        EmitVertex();
//    }

}
