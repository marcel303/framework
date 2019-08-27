/*
	Copyright (C) 2017 Marcel Smit
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

#import "framework.h"

#if ENABLE_METAL

#import "shaderBuilder.h"

// todo : should be .cpp file

#include "StringBuilder.h"
#include "StringEx.h"
#include "TextIO.h"

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
				
				if (strcmp(type, "sampler2D") == 0)
					u.index = texture_index++;
				
				uniforms.push_back(u);
			}
			else
			{
				contents.push_back(line);
			}
		}
		
		StringBuilder<32 * 1024> sb; // todo : replace with a more efficient and growing string builder
		
		sb.Append("#include <metal_stdlib>\n");
		sb.Append("\n");
		sb.Append("using namespace metal;\n");
		sb.Append("\n");
		sb.Append("typedef float2 vec2;\n");
		sb.Append("typedef float3 vec3;\n");
		sb.Append("typedef float4 vec4;\n");
		sb.Append("typedef int2 ivec2;\n");
		sb.Append("typedef int3 ivec3;\n");
		sb.Append("typedef int4 ivec4;\n");
		sb.Append("typedef float2x2 mat2;\n");
		sb.Append("typedef float2x2 mat2x2;\n");
		sb.Append("typedef float3x3 mat3;\n");
		sb.Append("typedef float3x3 mat3x3;\n");
		sb.Append("typedef float4x4 mat4;\n");
		sb.Append("typedef float4x4 mat4x4;\n");
		sb.Append("\n");
	// todo : use per-texture samplers
		//sb.Append("#define texture(s, c) s.sample(s ## _sampler, c)\n");
		sb.Append("constexpr sampler sampler_linear_wrap(mag_filter::linear, min_filter::linear, address::clamp_to_edge);\n");
		sb.Append("#define texture(s, c) s.sample(sampler_linear_wrap, c)\n");
		sb.Append("#define textureOffset(s, c, o) s.sample(sampler_linear_wrap, c, o)\n");
		sb.Append("#define sampler2D texture2d<float>\n");
		sb.Append("#define textureSize(t, level) float2(t.get_width(level), t.get_height(level))\n");
		sb.Append("#define dFdx dfdx\n");
		sb.Append("#define dFdy dfdy\n");
		sb.Append("#define discard discard_fragment()\n");
		sb.Append("#define inversesqrt rsqrt\n");
		sb.Append("float mod(float x, float y) { return fmod(x, y); }\n");
		sb.Append("vec2 mod(vec2 x, vec2 y) { return fmod(x, y); }\n");
		sb.Append("vec3 mod(vec3 x, vec3 y) { return fmod(x, y); }\n");
		sb.Append("vec4 mod(vec4 x, vec4 y) { return fmod(x, y); }\n");
		sb.Append("float atan(float x, float y) { return atan2(x, y); }\n");
		sb.Append("\n");
	
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
				for (auto & io : inputOutputs)
					sb.AppendFormat("\t%s %s;\n", io.type.c_str(), io.name.c_str());
				sb.Append("\t\n");
				
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

			sb.Append("struct ShaderUniforms\n");
			sb.Append("{\n");
			{
				for (auto & u : uniforms)
					sb.AppendFormat("\t%s %s;\n", u.type.c_str(), u.name.c_str());
			}
			sb.Append("};\n");
			sb.Append("\n");
			
			sb.Append("vertex ShaderVaryings shader_main(\n");
			sb.Append("\tShaderInputs inputs [[stage_in]],\n");
			sb.Append("\tconstant ShaderUniforms & uniforms [[buffer(1)]],\n");
			sb.Append("\tuint vertexId [[vertex_id]])\n");
			sb.Append("{\n");
			sb.Append("\tShaderMain m;\n");
			{
				sb.Append("\tm.gl_VertexID = vertexId;\n");
				for (auto & a : attributes)
					sb.AppendFormat("\tm.%s = inputs.%s;\n", a.name.c_str(), a.name.c_str());
				for (auto & u : uniforms)
				{
					const char * array_start = strchr(u.name.c_str(), '[');
					if (array_start != nullptr)
					{
						auto name = u.name.substr(0, array_start - u.name.c_str());
						sb.AppendFormat("\tm.%s = uniforms.%s;\n", name.c_str(), name.c_str());
					}
					else
					{
						sb.AppendFormat("\tm.%s = uniforms.%s;\n", u.name.c_str(), u.name.c_str());
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
					if (u.type != "sampler2D")
					{
						sb.AppendFormat("\t%s %s;\n", u.type.c_str(), u.name.c_str());
					}
				}
				sb.Append("\t\n");
				
				sb.Append("\t// textures\n");
				for (auto & u : uniforms)
				{
					if (u.type == "sampler2D")
					{
						sb.AppendFormat("\ttexture2d<float> %s;\n", u.name.c_str());
						sb.AppendFormat("\tsampler %s_sampler;\n", u.name.c_str());
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
				sb.Append("\tfloat4 shader_fragColor;\n");
				sb.Append("\tfloat4 shader_fragNormal;\n");
				sb.Append("\t\n");
				
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

			sb.Append("struct ShaderUniforms\n");
			sb.Append("{\n");
			{
				for (auto & u : uniforms)
				{
					if (u.type != "sampler2D")
					{
						sb.AppendFormat("\t%s %s;\n", u.type.c_str(), u.name.c_str());
					}
				}
			}
			sb.Append("};\n");
			sb.Append("\n");
			
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
				}
			}
			sb.Append("};\n");
			sb.Append("\n");
			
			sb.Append("struct ShaderOutputs\n");
			sb.Append("{\n");
			{
				for (int i = 0; outputs[i] != 0; ++i)
				{
					if (outputs[i] == 'c')
						sb.AppendFormat("\tfloat4 fragColor [[color(%d)]];\n", i);
					if (outputs[i] == 'n')
						sb.AppendFormat("\tfloat4 fragNormal [[color(%d)]];\n", i);
				}
			}
			sb.Append("};\n");
			sb.Append("\n");
			
			sb.Append("fragment ShaderOutputs shader_main(\n");
			sb.Append("\tShaderVaryings varyings [[stage_in]],\n");
			sb.Append("\tconstant ShaderUniforms & uniforms [[buffer(1)]],\n");
			sb.Append("\tShaderTextures textures)\n");
			sb.Append("{\n");
			sb.Append("\tShaderMain m;\n");
			{
				for (auto & u : uniforms)
					if (u.type != "sampler2D")
						sb.AppendFormat("\tm.%s = uniforms.%s;\n", u.name.c_str(), u.name.c_str());
				for (auto & u : uniforms)
					if (u.type == "sampler2D")
						sb.AppendFormat("\tm.%s = textures.%s;\n", u.name.c_str(), u.name.c_str());
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
			// todo : support MRT
				if (outputs[i] == 'c')
					sb.Append("\toutputs.fragColor = m.shader_fragColor;\n");
				if (outputs[i] == 'n')
					sb.Append("\toutputs.fragNormal = m.shader_fragNormal;\n");
			}
			sb.Append("\treturn outputs;\n");
			sb.Append("}\n");
		}
		
		result = sb.ToString();
		
		return true;
	}
}

#endif
