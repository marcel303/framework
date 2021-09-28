/*
	Copyright (C) 2020 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#include "framework.h"

#if ENABLE_METAL

#include "shaderBuilder.h"

#include "internal.h"
#include "StringBuilder.h"
#include "StringEx.h"
#include "TextIO.h"
#include <algorithm>

static bool is_whitespace(const char c)
{
	return isspace(c);
}

// eats an arbitrary word and stores the result in 'word'. has built-in support for quoted strings
static bool eat_word_v2(char *& line, const char *& word)
{
	while (*line != 0 && is_whitespace(*line) == true)
		line++;
	
	if (*line == 0)
		return false;
	
	const bool isQuoted = *line == '"';

	if (isQuoted)
	{
		line++;

		word = line;
		
		while (*line != 0 && *line != '"')
			line++;
	}
	else
	{
		word = line;
		
		while (*line != 0 && *line != ';' && is_whitespace(*line) == false)
			line++;
	}
	
	if (line > word)
	{
		if (*line != 0)
		{
			*line = 0;
			line++;
		}
		
		return true;
	}
	else
	{
		return false;
	}
}

// checks if 'line' begin with 'word' and eats 'word' from 'line' when it does
static bool eat_word(char * & line, const char * word)
{
	while (is_whitespace(*line))
		line++;
	
	int index = 0;
	
	while (word[index] != 0 && word[index] != ';')
	{
		if (line[index] != word[index])
			return false;
		
		index++;
	}

	if (is_whitespace(line[index]) == false && line[index] != 0)
		return false;

	while (is_whitespace(line[index]))
			index++;

	line += index;
	return true;
}

static void report_error(const char * line, const char * format, ...)
{
	char text[1024];
	va_list ap;
	va_start(ap, format);
	vsprintf_s(text, sizeof(text), format, ap);
	va_end(ap);
	
	printf("error: %s\n", text);
}

bool buildMetalText(const char * text, const char shaderType, const char * outputs, std::string & result)
{
	std::vector<std::string> lines;
	TextIO::LineEndings lineEndings;

	if (!TextIO::loadText(text, lines, lineEndings))
	{
		return false;
	}
	else
	{
		struct Attribute
		{
			std::string type;
			std::string name;
			std::string index;
		};
		
		struct Uniform
		{
			std::string type;
			std::string name;
			std::string buffer_name;
			int index = 0;
			
			bool operator<(const Uniform & other) const
			{
				return buffer_name < other.buffer_name;
			}
		};
		
		struct InputOutput
		{
			std::string type;
			std::string name;
		};
		
		std::vector<Attribute> attributes;
		std::vector<Uniform> uniforms;
		std::vector<InputOutput> inputOutputs;
		
		std::vector<std::string> contents;
		
		int texture_index = 0;
		
		for (auto & line : lines)
		{
			char * linePtr = (char*)line.c_str();
			
			if (eat_word(linePtr, "shader_attrib"))
			{
				const char * type;
				const char * name;
				const char * index;
				
				if (!eat_word_v2(linePtr, type))
				{
					report_error(line.c_str(), "attribute is missing type");
					return false;
				}
				
				if (!eat_word_v2(linePtr, name))
				{
					report_error(line.c_str(), "attribute is missing name");
					return false;
				}
				
				if (!eat_word_v2(linePtr, index))
				{
					report_error(line.c_str(), "attribute is missing index");
					return false;
				}
				
				Attribute a;
				a.type = type;
				a.name = name;
				a.index = index;
				
				attributes.push_back(a);
			}
			else if (eat_word(linePtr, "shader_out") || eat_word(linePtr, "shader_in"))
			{
				const char * type;
				const char * name;
				
				if (!eat_word_v2(linePtr, type))
				{
					report_error(line.c_str(), "shader output is missing type");
					return false;
				}
				
				if (!eat_word_v2(linePtr, name))
				{
					report_error(line.c_str(), "shader output is missing name");
					return false;
				}
				
				InputOutput io;
				io.type = type;
				io.name = name;
				
				inputOutputs.push_back(io);
			}
			else if (eat_word(linePtr, "uniform"))
			{
				const char * type;
				const char * name;
				
				if (!eat_word_v2(linePtr, type))
				{
					report_error(line.c_str(), "uniform is missing type");
					return false;
				}
				
				if (!eat_word_v2(linePtr, name))
				{
					report_error(line.c_str(), "uniform is missing name");
					return false;
				}
				
				const char * buffer_name = nullptr;
				
				for (;;)
				{
					const char * option;
					
					if (!eat_word_v2(linePtr, option))
						break;
					
					if (strcmp(option, "buffer") == 0)
					{
						if (!eat_word_v2(linePtr, buffer_name))
						{
							report_error(line.c_str(), "expected shader attribute buffer name");
							return false;
						}
					}
					else if (strcmp(option, "//") == 0)
					{
						// end looking for options when we encounter a comment
						break;
					}
					else
					{
						report_error(line.c_str(), "unknown shader attribute option: %s", option);
						return false;
					}
				}
				
				Uniform u;
				u.type = type;
				u.name = name;
				
				if (buffer_name != nullptr)
					u.buffer_name = buffer_name;
				else
					u.buffer_name = std::string("ShaderUniforms");
				
				if (strcmp(type, "sampler2D") == 0)
					u.index = texture_index++;
				if (strcmp(type, "sampler3D") == 0)
					u.index = texture_index++;
				
				uniforms.push_back(u);
			}
			else
			{
				contents.push_back(line);
			}
		}
		
		// sort uniforms by buffer name
		
		std::sort(uniforms.begin(), uniforms.end());
		
		// generate text
		
		StringBuilder sb;
		
static const char * header =
R"SHADER(
#include <metal_stdlib>

#define _SHADER_ 1

#define _SHADER_DEBUGGING_ 0
#define _SHADER_TARGET_LOWPOWER_ 0

using namespace metal;

// standard types
typedef float2 vec2;
typedef float3 vec3;
typedef float4 vec4;
typedef int2 ivec2;
typedef int3 ivec3;
typedef int4 ivec4;
typedef bool2 bvec2;
typedef bool3 bvec3;
typedef bool4 bvec4;
typedef float2x2 mat2;
typedef float2x2 mat2x2;
typedef float3x3 mat3;
typedef float3x3 mat3x3;
typedef float4x4 mat4;
typedef float4x4 mat4x4;

struct sampler2D
{
	texture2d<float> t;
	sampler s;
};

struct sampler3D
{
	texture3d<float> t;
	sampler s;
};

// texture sampling
#define texture(sampler, coord) sampler.t.sample(sampler.s, coord)
#define textureOffset(sampler, coord, offset) sampler.t.sample(sampler.s, coord, offset)
vec2 textureSize(sampler2D sampler, uint level)
{
	return vec2(
		sampler.t.get_width(level),
		sampler.t.get_height(level));
}
vec3 textureSize(sampler3D sampler, uint level)
{
	return vec3(
		sampler.t.get_width(level),
		sampler.t.get_height(level),
		sampler.t.get_depth(level));
}
#define texelFetch(sampler, coord, level) sampler.t.read(gx::to_uint(coord), level)
#define texelFetchOffset(sampler, coord, level, offset) sampler.t.read(gx::to_uint(coord + offset), level)

// standard library
#define dFdx dfdx
#define dFdy dfdy
#define discard discard_fragment()
#define inversesqrt rsqrt
float mod(float x, float y) { return fmod(x, y); }
vec2 mod(vec2 x, vec2 y) { return fmod(x, y); }
vec3 mod(vec3 x, vec3 y) { return fmod(x, y); }
vec4 mod(vec4 x, vec4 y) { return fmod(x, y); }
float atan(float x, float y) { return atan2(x, y); }

// helper functions
namespace gx
{
	uint2 to_uint(vec2 v) { return uint2(v); }
	uint2 to_uint(ivec2 v) { return uint2(v); }
	uint3 to_uint(vec3 v) { return uint3(v); }
	uint3 to_uint(ivec3 v) { return uint3(v); }
}
)SHADER";

		sb.Append(header);
		sb.Append('\n');
	
		if (shaderType == 'v')
		{
			// --- vertex shader generation --
			
			sb.Append("class ShaderMain\n");
			sb.Append("{\n");
			sb.Append("public:\n");
			{
				sb.Append("\t// attributes\n");
				for (auto & a : attributes)
					sb.AppendFormat("\t%s %s;\n", a.type.c_str(), a.name.c_str());
				sb.Append("\t\n");
				
				sb.Append("\t// uniforms\n");
				for (auto & u : uniforms)
				{
					const char * array_start = strchr(u.name.c_str(), '[');
					if (array_start != nullptr)
					{
						auto name = u.name.substr(0, array_start - u.name.c_str());
						sb.AppendFormat("\tconstant %s * %s;\n", u.type.c_str(), name.c_str());
					}
					else
					{
						sb.AppendFormat("\t%s %s;\n", u.type.c_str(), u.name.c_str());
					}
				}
				sb.Append("\t\n");
				
				sb.Append("\t// outputs\n");
				sb.Append("\tfloat4 gl_Position;\n");
				sb.Append("\tuint gl_VertexID;\n");
				sb.Append("\tuint gl_InstanceID;\n");
				for (auto & io : inputOutputs)
					sb.AppendFormat("\t%s %s;\n", io.type.c_str(), io.name.c_str());
				sb.Append("\t\n");
				
				sb.Append("\t// code\n");
				for (auto & line : contents)
					sb.AppendFormat("\t%s\n", line.c_str());
			}
			sb.Append("};\n");
			sb.Append("\n");
			
			sb.Append("#undef texture\n");
			sb.Append("\n");
			
			sb.Append("struct ShaderInputs\n");
			sb.Append("{\n");
			{
				for (auto & a : attributes)
					sb.AppendFormat("\t%s %s [[attribute(%s)]];\n", a.type.c_str(), a.name.c_str(), a.index.c_str());
			}
			sb.Append("};\n");
			sb.Append("\n");
			
			sb.Append("struct ShaderVaryings\n");
			sb.Append("{\n");
			{
				sb.Append("\tfloat4 builtin_position [[position]];\n");
				for (auto & io : inputOutputs)
					sb.AppendFormat("\t%s %s;\n", io.type.c_str(), io.name.c_str());
			}
			sb.Append("};\n");
			sb.Append("\n");

			//
			
			auto beginUniformBuffer = [&](const char * name)
			{
				sb.AppendFormat("struct %s\n", name);
				sb.Append("{\n");
			};
			
			auto endUniformBuffer = [&]()
			{
				sb.Append("};\n");
				sb.Append("\n");
			};
			
			const char * currentUniformBufferName = "";
			int nextBufferIndex = 8;
			std::string bufferArguments;
			
			for (auto & u : uniforms)
			{
				if (u.type == "sampler2D" || u.type == "sampler3D")
					continue;
				
				if (strcmp(currentUniformBufferName, u.buffer_name.c_str()) != 0)
				{
					if (currentUniformBufferName[0] != 0)
						endUniformBuffer();
					
					currentUniformBufferName = u.buffer_name.c_str();
					beginUniformBuffer(currentUniformBufferName);
					
					bufferArguments += String::FormatC("\tconstant %s & uniforms_%s [[buffer(%d)]],\n",
						currentUniformBufferName,
						currentUniformBufferName,
						nextBufferIndex);
					
					nextBufferIndex++;
				}
				
				sb.AppendFormat("\t%s %s;\n", u.type.c_str(), u.name.c_str());
			}
			
			if (currentUniformBufferName[0] != 0)
				endUniformBuffer();
			
			//
			
			sb.Append("struct ShaderTextures\n");
			sb.Append("{\n");
			{
				for (auto & u : uniforms)
				{
					if (u.type == "sampler2D")
					{
						sb.AppendFormat("\ttexture2d<float> %s [[texture(%d)]];\n", u.name.c_str(), u.index);
						sb.AppendFormat("\tsampler %s_sampler [[sampler(%d)]];\n", u.name.c_str(), u.index);
					}
					else if (u.type == "sampler3D")
					{
						sb.AppendFormat("\ttexture3d<float> %s [[texture(%d)]];\n", u.name.c_str(), u.index);
						sb.AppendFormat("\tsampler %s_sampler [[sampler(%d)]];\n", u.name.c_str(), u.index);
					}
				}
			}
			sb.Append("};\n");
			sb.Append("\n");
			
			//
		
			sb.Append("vertex ShaderVaryings shader_main(\n");
			sb.Append("\tShaderInputs inputs [[stage_in]],\n");
			sb.Append(bufferArguments.c_str());
			sb.Append("\tShaderTextures textures,\n");
			sb.Append("\tuint vertexId [[vertex_id]],\n");
			sb.Append("\tuint instanceId [[instance_id]])\n");
			sb.Append("{\n");
			sb.Append("\tShaderMain m;\n");
			{
				sb.Append("\tm.gl_VertexID = vertexId;\n");
				sb.Append("\tm.gl_InstanceID = instanceId;\n");
				for (auto & a : attributes)
					sb.AppendFormat("\tm.%s = inputs.%s;\n", a.name.c_str(), a.name.c_str());
				for (auto & u : uniforms)
				{
					if (u.type == "sampler2D" || u.type == "sampler3D")
						continue;
					
					const char * array_start = strchr(u.name.c_str(), '[');
					if (array_start != nullptr)
					{
						auto name = u.name.substr(0, array_start - u.name.c_str());
						sb.AppendFormat("\tm.%s = uniforms_%s.%s;\n", name.c_str(), u.buffer_name.c_str(), name.c_str());
					}
					else
					{
						sb.AppendFormat("\tm.%s = uniforms_%s.%s;\n", u.name.c_str(), u.buffer_name.c_str(), u.name.c_str());
					}
				}
				for (auto & u : uniforms)
				{
					if (u.type == "sampler2D")
					{
						sb.AppendFormat("\tm.%s.t = textures.%s;\n", u.name.c_str(), u.name.c_str());
						sb.AppendFormat("\tm.%s.s = textures.%s_sampler;\n", u.name.c_str(), u.name.c_str());
					}
					else if (u.type == "sampler3D")
					{
						sb.AppendFormat("\tm.%s.t = textures.%s;\n", u.name.c_str(), u.name.c_str());
						sb.AppendFormat("\tm.%s.s = textures.%s_sampler;\n", u.name.c_str(), u.name.c_str());
					}
				}
			}
			sb.Append("\t\n");
			sb.Append("\tm.main();\n");
			sb.Append("\t\n");
			sb.Append("\tShaderVaryings outputs;\n");
			sb.Append("\toutputs.builtin_position = m.gl_Position;\n");
			for (auto & io : inputOutputs)
				sb.AppendFormat("\toutputs.%s = m.%s;\n", io.name.c_str(), io.name.c_str());
			sb.Append("\treturn outputs;\n");
			sb.Append("}\n");
		}
		else if (shaderType == 'p')
		{
			// --- fragment shader generation ---
			
			sb.Append("class ShaderMain\n");
			sb.Append("{\n");
			sb.Append("public:\n");
			{
				sb.Append("\t// uniforms\n");
				for (auto & u : uniforms)
				{
					const char * array_start = strchr(u.name.c_str(), '[');
					if (array_start != nullptr)
					{
						auto name = u.name.substr(0, array_start - u.name.c_str());
						sb.AppendFormat("\tconstant %s * %s;\n", u.type.c_str(), name.c_str());
					}
					else
					{
						sb.AppendFormat("\t%s %s;\n", u.type.c_str(), u.name.c_str());
					}
				}
				sb.Append("\t\n");
				
				sb.Append("\t// varyings\n");
				for (auto & io : inputOutputs)
					sb.AppendFormat("\t%s %s;\n", io.type.c_str(), io.name.c_str());
				sb.Append("\t\n");
				
				sb.Append("\t// inputs\n");
				sb.Append("\tfloat4 gl_FragCoord;\n");
				sb.Append("\t\n");
				
				sb.Append("\t// outputs\n");
				for (auto & output : g_shaderOutputs)
					sb.AppendFormat("\t%s %s;\n",
						output.outputType.c_str(),
						output.outputName.c_str());
				sb.Append("\t\n");
				
				sb.Append("\t// code\n");
				for (auto & line : contents)
					sb.AppendFormat("\t%s\n", line.c_str());
			}
			sb.Append("};\n");
			sb.Append("\n");
			
			sb.Append("#undef texture\n");
			sb.Append("\n");
			
			sb.Append("struct ShaderVaryings\n");
			sb.Append("{\n");
			{
				sb.Append("\tfloat4 builtin_position [[position]];\n");
				for (auto & io : inputOutputs)
					sb.AppendFormat("\t%s %s;\n", io.type.c_str(), io.name.c_str());
			}
			sb.Append("};\n");
			sb.Append("\n");

			//
			
			auto beginUniformBuffer = [&](const char * name)
			{
				sb.AppendFormat("struct %s\n", name);
				sb.Append("{\n");
			};
			
			auto endUniformBuffer = [&]()
			{
				sb.Append("};\n");
				sb.Append("\n");
			};
			
			const char * currentUniformBufferName = "";
			int nextBufferIndex = 1;
			std::string bufferArguments;

			for (auto & u : uniforms)
			{
				if (u.type == "sampler2D" || u.type == "sampler3D")
					continue;
				
				if (strcmp(currentUniformBufferName, u.buffer_name.c_str()) != 0)
				{
					if (currentUniformBufferName[0] != 0)
						endUniformBuffer();
					
					currentUniformBufferName = u.buffer_name.c_str();
					beginUniformBuffer(currentUniformBufferName);
					
					bufferArguments += String::FormatC("\tconstant %s & uniforms_%s [[buffer(%d)]],\n",
						currentUniformBufferName,
						currentUniformBufferName,
						nextBufferIndex);
					
					nextBufferIndex++;
				}

				sb.AppendFormat("\t%s %s;\n", u.type.c_str(), u.name.c_str());
			}
			
			if (currentUniformBufferName[0] != 0)
				endUniformBuffer();
			
			//
			
			sb.Append("struct ShaderTextures\n");
			sb.Append("{\n");
			{
				for (auto & u : uniforms)
				{
					if (u.type == "sampler2D")
					{
						sb.AppendFormat("\ttexture2d<float> %s [[texture(%d)]];\n", u.name.c_str(), u.index);
						sb.AppendFormat("\tsampler %s_sampler [[sampler(%d)]];\n", u.name.c_str(), u.index);
					}
					else if (u.type == "sampler3D")
					{
						sb.AppendFormat("\ttexture3d<float> %s [[texture(%d)]];\n", u.name.c_str(), u.index);
						sb.AppendFormat("\tsampler %s_sampler [[sampler(%d)]];\n", u.name.c_str(), u.index);
					}
				}
			}
			sb.Append("};\n");
			sb.Append("\n");
			
			sb.Append("struct ShaderOutputs\n");
			sb.Append("{\n");
			{
				for (int i = 0; outputs[i] != 0; ++i)
				{
					const ShaderOutput * output = findShaderOutput(outputs[i]);
					
					if (output == nullptr)
					{
						logError("unknown shader output: %c", outputs[i]);
						return false;
					}
					
					sb.AppendFormat("\t%s shaderOutput_%d [[color(%d)]];\n",
						output->outputType.c_str(),
						i,
						i);
				}
			}
			sb.Append("};\n");
			sb.Append("\n");
			
			sb.Append("fragment ShaderOutputs shader_main(\n");
			sb.Append("\tShaderVaryings varyings [[stage_in]],\n");
			sb.Append(bufferArguments.c_str());
			sb.Append("\tShaderTextures textures)\n");
			sb.Append("{\n");
			sb.Append("\tShaderMain m;\n");
			{
				for (auto & u : uniforms)
				{
					if (u.type == "sampler2D" || u.type == "sampler3D")
						continue;
					
					const char * array_start = strchr(u.name.c_str(), '[');
					if (array_start != nullptr)
					{
						auto name = u.name.substr(0, array_start - u.name.c_str());
						sb.AppendFormat("\tm.%s = uniforms_%s.%s;\n", name.c_str(), u.buffer_name.c_str(), name.c_str());
					}
					else
					{
						sb.AppendFormat("\tm.%s = uniforms_%s.%s;\n", u.name.c_str(), u.buffer_name.c_str(), u.name.c_str());
					}
				}
				
				for (auto & u : uniforms)
				{
					if (u.type == "sampler2D")
					{
						sb.AppendFormat("\tm.%s.t = textures.%s;\n", u.name.c_str(), u.name.c_str());
						sb.AppendFormat("\tm.%s.s = textures.%s_sampler;\n", u.name.c_str(), u.name.c_str());
					}
					else if (u.type == "sampler3D")
					{
						sb.AppendFormat("\tm.%s.t = textures.%s;\n", u.name.c_str(), u.name.c_str());
						sb.AppendFormat("\tm.%s.s = textures.%s_sampler;\n", u.name.c_str(), u.name.c_str());
					}
				}
				
				for (auto & io : inputOutputs)
					sb.AppendFormat("\tm.%s = varyings.%s;\n", io.name.c_str(), io.name.c_str());
			}
			sb.Append("\tm.gl_FragCoord = varyings.builtin_position;\n");
			sb.Append("\t\n");
			sb.Append("\tm.main();\n");
			sb.Append("\t\n");
			
			sb.Append("\tShaderOutputs outputs;\n");
			for (int i = 0; outputs[i] != 0; ++i)
			{
				const ShaderOutput * output = findShaderOutput(outputs[i]);
			
				if (output == nullptr)
				{
					logError("unknown shader output: %c", outputs[i]);
					return false;
				}
				
				sb.AppendFormat("\toutputs.shaderOutput_%d = m.%s;\n",
					i,
					output->outputName.c_str());
			}
			sb.Append("\treturn outputs;\n");
			sb.Append("}\n");
		}
		
		result = sb.ToString();
		
		return true;
	}
}

#endif
