#import "shaderBuilder.h"
#import <Metal/Metal.h>

id <MTLDevice> metal_get_device();

static const char * s_testShaderPs = R"SHADER(

	#include <metal_stdlib>

	using namespace metal;

	struct ShaderInputs
	{
		float4 position [[attribute(0)]];
		float4 color [[attribute(1)]];
		float2 texcoord [[attribute(2)]];
	};

	struct ShaderUniforms
	{
		float4 params;
	};

	struct ShaderTextures
	{
		texture2d<float> texture [[texture(0)]];
	};

	class ShaderMain
	{
	public:
		ShaderInputs inputs;
		ShaderUniforms uniforms;

		texture2d<float> textureResource;
		
		ShaderTextures textures;
		
		float4 member;
		
		float4 main()
		{
			float4 color = inputs.color;
			
			if (uniforms.params.x != 0.0)
			{
				constexpr sampler textureSampler(mag_filter::linear, min_filter::linear);
				
				color *= textures.texture.sample(textureSampler, inputs.texcoord);
			}
			
			return color;
		}
	};

	fragment float4 shader_main(
		ShaderInputs inputs [[stage_in]],
		constant ShaderUniforms & uniforms [[buffer(0)]],
		ShaderTextures textures)
	{
		ShaderMain m;
		
		m.inputs = inputs;
		m.uniforms = uniforms;
		m.textures = textures;
		
		return m.main();
	}
	
)SHADER";

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

bool buildMetalText(const char * text, const char shaderType, std::string & result)
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
				
				Uniform u;
				u.type = type;
				u.name = name;
				
				uniforms.push_back(u);
			}
			else
			{
				contents.push_back(line);
			}
		}
		
		StringBuilder<16000> sb;
		
		sb.Append("#include <metal_stdlib>\n");
		sb.Append("\n");
		sb.Append("using namespace metal;\n");
		sb.Append("\n");
		sb.Append("#define vec2 float2\n");
		sb.Append("#define vec3 float3\n");
		sb.Append("#define vec4 float4\n");
		sb.Append("#define ivec2 int2\n");
		sb.Append("#define ivec3 int3\n");
		sb.Append("#define ivec4 int4\n");
		sb.Append("#define mat4 float4x4\n");
		sb.Append("\n");
	
		if (shaderType == 'v')
		{
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
					sb.AppendFormat("\t%s %s;\n", u.type.c_str(), u.name.c_str());
				sb.Append("\t\n");
				
				sb.Append("\t// outputs\n");
				for (auto & io : inputOutputs)
					sb.AppendFormat("\t%s %s;\n", io.type.c_str(), io.name.c_str());
				sb.Append("\t\n");
				
				for (auto & line : contents)
					sb.AppendFormat("\t%s\n", line.c_str());
			}
			sb.Append("};\n");
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
				sb.Append("\tfloat4 position [[position]];\n");
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
			sb.Append("\tconstant ShaderUniforms & uniforms [[buffer(0)]])\n");
			sb.Append("{\n");
			sb.Append("\tShaderMain m;\n");
			{
				for (auto & a : attributes)
					sb.AppendFormat("\tm.%s = inputs.%s;\n", a.name.c_str(), a.name.c_str());
				for (auto & u : uniforms)
					sb.AppendFormat("\tm.%s = uniforms.%s;\n", u.name.c_str(), u.name.c_str());
			}
			sb.Append("\t\n");
			sb.Append("\tm.main();\n");
			sb.Append("\t\n");
			sb.Append("\tShaderVaryings outputs;\n");
			for (auto & io : inputOutputs)
				sb.AppendFormat("\toutputs.%s = m.%s;\n", io.name.c_str(), io.name.c_str());
			sb.Append("\treturn outputs;\n");
			sb.Append("}\n");
		}
		else
		{
			sb.Append("class ShaderMain\n");
			sb.Append("{\n");
			sb.Append("public:\n");
			{
				sb.Append("\t// uniforms\n");
				for (auto & u : uniforms)
					sb.AppendFormat("\t%s %s;\n", u.type.c_str(), u.name.c_str());
				sb.Append("\t\n");
				
				sb.Append("\t// varyings\n");
				for (auto & io : inputOutputs)
					sb.AppendFormat("\t%s %s;\n", io.type.c_str(), io.name.c_str());
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
			
			sb.Append("struct ShaderVaryings\n");
			sb.Append("{\n");
			{
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
			
			sb.Append("struct ShaderTextures\n");
			sb.Append("{\n");
			sb.Append("};\n");
			sb.Append("\n");
			
			sb.Append("fragment float4 shader_main(\n");
			sb.Append("\tShaderVaryings varyings [[stage_in]],\n");
			sb.Append("\tconstant ShaderUniforms & uniforms [[buffer(0)]],\n");
			sb.Append("\tShaderTextures textures)\n");
			sb.Append("{\n");
			sb.Append("\tShaderMain m;\n");
			{
				for (auto & u : uniforms)
					sb.AppendFormat("\tm.%s = uniforms.%s;\n", u.name.c_str(), u.name.c_str());
				for (auto & io : inputOutputs)
					sb.AppendFormat("\tm.%s = varyings.%s;\n", io.name.c_str(), io.name.c_str());
			}
			sb.Append("\t\n");
			sb.Append("\tm.main();\n");
			sb.Append("\t\n");
			sb.Append("\treturn m.shader_fragColor;\n");
			sb.Append("}\n");
		}
		
		result = sb.ToString();
		
		return true;
	}
}

static const char * s_testShader2Vs = R"SHADER(

include ShaderVS.txt

//

shader_out vec4 v_color;
shader_out vec3 v_normal;
shader_out vec2 v_texcoord0;

void main()
{
	vec4 position = unpackPosition();
	
	position = objectToProjection(position);
	
	vec4 color = unpackColor();
	
	vec3 normal = unpackNormal().xyz;

	normal = objectToView3(normal);
	normal = normalize(normal);
	
	vec2 texcoord = unpackTexcoord(0);
}

)SHADER";

static const char * s_testShader2Ps = R"SHADER(

include ShaderPS.txt

uniform vec4 params;
//uniform sampler2D source;

shader_in vec4 v_color;
shader_in vec3 v_normal;
shader_in vec2 v_texcoord0;

void main()
{
	vec4 result = v_color;

	// color clamp

	if (params.w != 0.0)
	{
		result = clamp(result, vec4(0.0), vec4(1.0));
	}

#if 0
	// texture
	
	if (params.x != 0.0)
	{
		vec4 texColor = texture(source, v_texcoord0);
		
		/*
		COLOR_MUL,
		COLOR_ADD,
		COLOR_SUB,
		COLOR_IGNORE
		*/
		
		if (params.y == 0.0)
		{
			result.rgb = result.rgb * texColor.rgb;
			result.a   = result.a   * texColor.a;
		}
		else if (params.y == 1.0)
		{
			result.rgb = result.rgb + texColor.rgb;
			result.a   = result.a   * texColor.a;
		}
		else if (params.y == 2.0)
		{
			result.rgb = result.rgb - texColor.rgb;
			result.a   = result.a   * texColor.a;
		}
		else if (params.y == 3.0)
		{
			result.rgb = texColor.rgb;
			result.a   = texColor.a;
		}
	}

	// color post

	result = applyColorPost(result, params.z);
#endif

	shader_fragColor = result;
	shader_fragNormal = vec4(v_normal, 0.0);
}

)SHADER";

//

#include "shaderPreprocess.h"

void metal_shadertest()
{
	@autoreleasepool
	{
		{
			id <MTLDevice> device = metal_get_device();
			
			NSError * error = nullptr;
			
			id <MTLLibrary> library_ps = [device newLibraryWithSource:[NSString stringWithCString:s_testShaderPs encoding:NSASCIIStringEncoding] options:nullptr error:&error];
			if (library_ps == nullptr && error != nullptr)
				NSLog(@"%@", error);

			id <MTLFunction> ps = [library_ps newFunctionWithName:@"shader_main"];
			
			[ps release];
			
			[library_ps release];
		}
		
		{
			std::vector<std::string> errorMessages;
			int fileId = 0;
			
			// process vs
			
			std::string preprocessedVs;
			preprocessShader(s_testShader2Vs, preprocessedVs, 0, errorMessages, fileId);
			printf("preprocessedVs:\n%s", preprocessedVs.c_str());
			
			std::string metalTextVs;
			buildMetalText(preprocessedVs.c_str(), 'v', metalTextVs);
			printf("metalTextVs:\n%s", metalTextVs.c_str());
			
			// process ps
			
			std::string preprocessedPs;
			preprocessShader(s_testShader2Ps, preprocessedPs, 0, errorMessages, fileId);
			printf("preprocessedPs:\n%s", preprocessedPs.c_str());
			
			std::string metalTextPs;
			buildMetalText(preprocessedPs.c_str(), 'p', metalTextPs);
			printf("metalTextPs:\n%s", metalTextPs.c_str());
			
			//
			
			id <MTLDevice> device = metal_get_device();
			
			NSError * error = nullptr;
			
			id <MTLLibrary> library_vs = [device newLibraryWithSource:[NSString stringWithCString:metalTextVs.c_str() encoding:NSASCIIStringEncoding] options:nullptr error:&error];
			if (library_vs == nullptr && error != nullptr)
				NSLog(@"%@", error);
			
			id <MTLLibrary> library_ps = [device newLibraryWithSource:[NSString stringWithCString:metalTextPs.c_str() encoding:NSASCIIStringEncoding] options:nullptr error:&error];
			if (library_ps == nullptr && error != nullptr)
				NSLog(@"%@", error);

			id <MTLFunction> vs = [library_vs newFunctionWithName:@"shader_main"];
			id <MTLFunction> ps = [library_ps newFunctionWithName:@"shader_main"];
			
			[vs release];
			[ps release];
			
			[library_vs release];
			[library_ps release];
		}
	}
}
