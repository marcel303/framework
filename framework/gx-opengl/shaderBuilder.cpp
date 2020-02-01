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

#if ENABLE_OPENGL

#include "internal.h"
#include "shaderBuilder.h"
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

bool buildOpenglText(const char * text, const char shaderType, const char * outputs, std::string & result)
{
	Assert(shaderType == 'v' || shaderType == 'p');

	std::vector<std::string> lines;
	TextIO::LineEndings lineEndings;

	if (!TextIO::loadText(text, lines, lineEndings))
	{
		return false;
	}
	else
	{
		StringBuilder sb;
		StringBuilder text;

	#if USE_LEGACY_OPENGL
		const char * header =
R"HEADER(
#define shader_attrib attribute
#define shader_in varying
#define shader_out varying

#define texture texture2D

#define tex2D texture
)HEADER";
		sb.Append(header);
		sb.Append('\n');
	#else
		const char * header =
R"HEADER(
#define shader_attrib in
#define shader_in in
#define shader_out out

#define tex2D texture
)HEADER";
		sb.Append(header);
		sb.Append('\n');
	#endif
	
	#if !USE_LEGACY_OPENGL
		if (shaderType == 'p')
		{
			bool * usedOutputs = (bool*)alloca(g_shaderOutputs.size() * sizeof(bool));
			memset(usedOutputs, 0, g_shaderOutputs.size() * sizeof(bool));
			
			for (int i = 0; outputs[i] != 0; ++i)
			{
				const ShaderOutput * output = findShaderOutput(outputs[i]);
				
				if (output == nullptr)
				{
					logError("unknown shader output: %c", outputs[i]);
					return false;
				}
				
				// todo : detect if a pass is added more than once
				
				sb.AppendFormat("layout(location = %d) out %s %s;\n",
					i,
					output->outputType.c_str(),
					output->outputName.c_str(),
					output->outputType.c_str());
				
				const int passIndex = output - g_shaderOutputs.data();
				usedOutputs[passIndex] = true;
			}
			
			// use regular variables for unused outputs
			
			for (size_t i = 0; i < g_shaderOutputs.size(); ++i)
			{
				if (usedOutputs[i])
					continue;
				
				auto & output = g_shaderOutputs[i];
				
				sb.AppendFormat("%s %s;\n", output.outputType.c_str(), output.outputName.c_str());
			}
		}
	#endif
	
		struct Uniform
		{
			std::string type;
			std::string name;
			std::string buffer_name;
			
			bool operator<(const Uniform & other) const
			{
				return buffer_name < other.buffer_name;
			}
		};
		
		std::vector<Uniform> uniforms;
	
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
				
				sb.AppendFormat("shader_attrib %s %s;\n", type, name);
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
				
				uniforms.push_back(u);
			}
			else
			{
				text.AppendFormat("%s\n", line.c_str());
			}
		}
		
		// sort uniforms by buffer name
		
		std::sort(uniforms.begin(), uniforms.end());
		
		// write uniforms
		
		auto beginUniformBuffer = [&](const char * name)
		{
			sb.AppendFormat("layout (std140) uniform %s\n", name);
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
			if (u.buffer_name.empty())
			{
				sb.AppendFormat("uniform %s %s;\n", u.type.c_str(), u.name.c_str());
			}
			else
			{
				if (strcmp(currentUniformBufferName, u.buffer_name.c_str()) != 0)
				{
					if (currentUniformBufferName[0] != 0)
						endUniformBuffer();
					
					currentUniformBufferName = u.buffer_name.c_str();
					beginUniformBuffer(currentUniformBufferName);
					
					nextBufferIndex++;
				}
				
				sb.AppendFormat("\t%s %s;\n", u.type.c_str(), u.name.c_str());
			}
		}
		
		if (currentUniformBufferName[0] != 0)
			endUniformBuffer();
		
		sb.Append("#define main() shaderMain()\n");
		sb.Append('\n');
		{
			sb.Append(text.ToString());
		}
		sb.Append('\n');
		sb.Append("#undef main\n");
		sb.Append("\n");
		
		sb.Append("void main()\n");
		sb.Append("{\n");
		{
		#if !USE_LEGACY_OPENGL
			if (shaderType == 'p')
			{
				for (int i = 0; outputs[i] != 0; ++i)
				{
					const ShaderOutput * output = findShaderOutput(outputs[i]);
					
					if (output == nullptr)
					{
						logError("unknown shader output: %c", outputs[i]);
						return false;
					}
					
					sb.AppendFormat("\t%s = %s(0.0);\n",
						output->outputName.c_str(),
						output->outputType.c_str());
				}
				
				sb.Append("\n");
			}
		#endif
		
			sb.Append("\tshaderMain();\n");
		}
		sb.Append("}\n");
		
		result = sb.ToString();
		
		return true;
	}
}

#endif
