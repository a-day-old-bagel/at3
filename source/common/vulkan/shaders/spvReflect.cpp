/*
 * spvReflect generates reflection files for SPIR-V. This is a standalone executable that gets called from CMake
 * during the at3 build process.
 *
 * This file, as well as some of the shader code found alongside it, was adapted from original code written by
 * Kyle Halladay, carrying the following license, which license can also be found in the file LICENSE.
 *
 * MIT License
 *
 * Copyright (c) 2018 Kyle Halladay
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <spirv_glsl.hpp>
#include <string>
#include <vector>
#include <cstdint>
#include <cstdio>
#include <codecvt>
#include <experimental/filesystem>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

namespace fs = std::experimental::filesystem;

struct BlockMember
{
  std::string name;
  uint32_t size;
  uint32_t offset;
};

enum class BlockType : uint8_t
{
    UNIFORM = 0,
    TEXTURE = 1,
    SEPARATETEXTURE = 2,
    SAMPLER = 3,
    TEXTUREARRAY = 4,
    SAMPLERARRAY = 5
};

std::string BlockTypeNames[] =
    {
        "UNIFORM",
        "TEXTURE",
        "SEPARATETEXTURE",
        "SAMPLER",
        "TEXTUREARRAY",
        "SAMPLERARRAY"
    };

struct InputBlock
{
  std::string name;
  uint32_t size;
  std::vector<BlockMember> members;
  uint32_t set;
  uint32_t binding;
  BlockType type;
  uint32_t arrayLen;
};

struct ShaderData
{
  InputBlock pushConstants;
  std::vector<InputBlock> descriptorSets;
};


void writeInputGroup(rapidjson::PrettyWriter<rapidjson::StringBuffer>& writer, std::vector<InputBlock>& descSet, std::string inputGroupName)
{
  writer.Key(inputGroupName.c_str());
  writer.StartArray();

  for (InputBlock& block : descSet)
  {
    writer.StartObject();
    writer.Key("set");
    writer.Int(block.set);
    writer.Key("binding");
    writer.Int(block.binding);
    writer.Key("name");
    writer.String(block.name.c_str());
    writer.Key("size");
    writer.Int(block.size);
    writer.Key("arrayLen");
    writer.Int(block.arrayLen);
    writer.Key("type");
    writer.String(BlockTypeNames[(uint8_t)block.type].c_str());
    writer.Key("members");
    writer.StartArray();
    for (uint32_t i = 0; i < block.members.size(); ++i)
    {
      writer.StartObject();
      writer.Key("name");
      writer.String(block.members[i].name.c_str());
      writer.Key("size");
      writer.Int(block.members[i].size);
      writer.Key("offset");
      writer.Int(block.members[i].offset);
      writer.EndObject();

    }
    writer.EndArray();
    writer.EndObject();
  }

  writer.EndArray();
}

std::string getReflectionString(ShaderData& data)
{

  using namespace rapidjson;
  StringBuffer s;
  PrettyWriter<StringBuffer> writer(s);

  writer.StartObject();

  if (data.pushConstants.size > 0)
  {
    writer.Key("push_constants");
    writer.StartObject();
    writer.Key("size");
    writer.Int(data.pushConstants.size);

    writer.Key("elements");
    writer.StartArray();

    for (uint32_t i = 0; i < data.pushConstants.members.size(); ++i)
    {
      writer.StartObject();
      writer.Key("name");
      writer.String(data.pushConstants.members[i].name.c_str());
      writer.Key("size");
      writer.Int(data.pushConstants.members[i].size);
      writer.Key("offset");
      writer.Int(data.pushConstants.members[i].offset);
      writer.EndObject();
    }
    writer.EndArray();
    writer.EndObject();
  }

  writeInputGroup(writer, data.descriptorSets, "descriptor_sets");


  writer.EndObject();
  return s.GetString();
}

std::string baseTypeToString(spirv_cross::SPIRType::BaseType type);

void createUniformBlockForResource(InputBlock* outBlock, spirv_cross::Resource res, spirv_cross::CompilerGLSL& compiler)
{
	uint32_t id = res.id;
	std::vector<spirv_cross::BufferRange> ranges = compiler.get_active_buffer_ranges(id);
	const spirv_cross::SPIRType& ub_type = compiler.get_type(res.base_type_id);

	outBlock->name = res.name;
	for (auto& range : ranges)
	{
		BlockMember mem;
		mem.name = compiler.get_member_name(res.base_type_id, range.index);
		mem.size = range.range;
		mem.offset = range.offset;

		auto type = compiler.get_type(res.type_id);

		auto baseType = type.basetype;
		std::string baseTypeString = baseTypeToString(baseType);
		std::string mName = compiler.get_member_name(res.base_type_id, range.index);

		outBlock->members.push_back(mem);
	}

	outBlock->size = compiler.get_declared_struct_size(ub_type);
	outBlock->binding = compiler.get_decoration(res.id, spv::DecorationBinding);
	outBlock->set = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
	outBlock->arrayLen = 1;
	outBlock->type = BlockType::UNIFORM;
}

void createSeparateSamplerBlockForResource(InputBlock* outBlock, spirv_cross::Resource res, spirv_cross::CompilerGLSL& compiler)
{
	uint32_t id = res.id;

	outBlock->name = res.name;
	outBlock->set = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
	outBlock->binding = compiler.get_decoration(res.id, spv::DecorationBinding);

	//.array is an array of uint32_t array sizes (for cases of more than one array)
	outBlock->arrayLen = compiler.get_type(res.type_id).array[0];

	outBlock->type = outBlock->arrayLen == 1 ? BlockType::SAMPLER : BlockType::SAMPLERARRAY;
}

void createSeparateTextureBlockForResource(InputBlock* outBlock, spirv_cross::Resource res, spirv_cross::CompilerGLSL& compiler)
{
	uint32_t id = res.id;

	outBlock->name = res.name;
	outBlock->set = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
	outBlock->binding = compiler.get_decoration(res.id, spv::DecorationBinding);

	outBlock->arrayLen = 1;

	if (compiler.get_type(res.type_id).array.size() > 0)
	{
		outBlock->arrayLen = compiler.get_type(res.type_id).array[0];
	}
	outBlock->type = outBlock->arrayLen == 1 ? BlockType::SEPARATETEXTURE : BlockType::TEXTUREARRAY;
}

void createTextureBlockForResource(InputBlock* outBlock, spirv_cross::Resource res, spirv_cross::CompilerGLSL& compiler)
{
	uint32_t id = res.id;

	outBlock->name = res.name;
	outBlock->set = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
	outBlock->binding = compiler.get_decoration(res.id, spv::DecorationBinding);
	outBlock->arrayLen = 1;
	outBlock->type = BlockType::TEXTURE;
}

int main(int argc, const char** argv) {

  if (argc != 3) {
    printf("ShaderPipeline: usage: ShaderPipeline <SPIR-V input dir> <reflection output dir> \n");
    return 1;
  }
  std::string shaderInPath = argv[1];
  std::string reflOutPath = argv[2];

  printf("Compiling reflections: \n\tSPV dir is %s, \n\tREFL dir is %s\n", shaderInPath.c_str(), reflOutPath.c_str());

  fs::create_directories(reflOutPath);
  std::vector<std::string> inputFiles;
  for (auto & p : fs::directory_iterator(shaderInPath)) {
    inputFiles.push_back(p.path().string());
  }

	// generate reflection files for all built shaders
	{
		for (uint32_t i = 0; i < inputFiles.size(); ++i)
		{
			// Dont read anything that's not SPIR-V
      if (inputFiles[i].find(".spv") == std::string::npos) continue;

			ShaderData data = {};

      std::string spvPath = fs::absolute(inputFiles[i]).string();
      std::string reflPath = spvPath;
			reflPath.replace(reflPath.find(".spv"),std::string(".spv").length(),".refl");

      printf("\tReflecting spv: %s\n", spvPath.c_str());
      fflush(stdout);
      FILE* shaderFile = fopen(spvPath.c_str(), "rb");
			assert(shaderFile);

			fseek(shaderFile, 0, SEEK_END);
			size_t filesize = ftell(shaderFile);
			size_t wordSize = sizeof(uint32_t);
			size_t wordCount = filesize / wordSize;
			rewind(shaderFile);

			uint32_t* ir = (uint32_t*)malloc(sizeof(uint32_t) * wordCount);

			fread(ir, filesize, 1, shaderFile);
			fclose(shaderFile);

			spirv_cross::CompilerGLSL glsl(ir, wordCount);

			spirv_cross::ShaderResources resources = glsl.get_shader_resources();

			for (spirv_cross::Resource res : resources.push_constant_buffers)
			{
				createUniformBlockForResource(&data.pushConstants, res, glsl);
			}

			data.descriptorSets.resize(resources.uniform_buffers.size() + resources.sampled_images.size() + resources.separate_images.size() + resources.separate_samplers.size());

			uint32_t idx = 0;
			for (spirv_cross::Resource res : resources.uniform_buffers)
			{
				createUniformBlockForResource(&data.descriptorSets[idx++], res, glsl);
			}

			for (spirv_cross::Resource res : resources.sampled_images)
			{
				createTextureBlockForResource(&data.descriptorSets[idx++], res, glsl);
			}

			for (spirv_cross::Resource res : resources.separate_images)
			{
				createSeparateTextureBlockForResource(&data.descriptorSets[idx++], res, glsl);
			}

			for (spirv_cross::Resource res : resources.separate_samplers)
			{
				createSeparateSamplerBlockForResource(&data.descriptorSets[idx++], res, glsl);
			}

			std::sort(data.descriptorSets.begin(), data.descriptorSets.end(), [](const InputBlock& lhs, const InputBlock& rhs)
			{
				if (lhs.set != rhs.set) return lhs.set < rhs.set;
				return lhs.binding < rhs.binding;
			});


			//write out material
			std::string shader = getReflectionString(data);
      FILE* file = fopen(reflPath.c_str(), "w");
			int results = fputs(shader.c_str(), file);
			assert(results != EOF);
			fclose(file);
		}
	}

	printf("Reflections generated successfully.\n");
	return 0;
}

std::string baseTypeToString(spirv_cross::SPIRType::BaseType type)
{
	std::string names[] = {
		"Unknown",
		"Void",
		"Boolean",
		"Char",
		"Int",
		"UInt",
		"Int64",
		"UInt64",
		"AtomicCounter",
		"Float",
		"Double",
		"Struct",
		"Image",
		"SampledImage",
		"Sampler"
	};

	return names[type];

}
