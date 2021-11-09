#include "framework.h"
#include "graph.h"
#include "graphEdit.h"
#include "internal.h"
#include "Parse.h" // Parse::Bool
#include "Path.h"
#include "ui.h"
#include "StringBuilder.h"
#include "StringEx.h"
#include "TextIO.h"
#include <map>
#include <string>
#include <string.h> // strcmp

#include "data/engine/ShaderCommon.txt" // VS_ constants

// todo : add min/max range for shader node inputs
// todo : add support for socket min/max to graph edit

#if !ENABLE_OPENGL
	#define ENABLE_UNUSED_VARIABLE_VOID 1
#else
	#define ENABLE_UNUSED_VARIABLE_VOID 0 // do not alter : (void)var is illegal in OpenGL
#endif

/*

VS nodes
	vs.input -- from buffer
	vs.output -- shader_out
		if referenced by ps and missing, default code is generated for position, normal, color, texcoord

PS nodes
	ps.input -- shader_in

VS/PS nodes
	immediate.float -- uniform
	
*/

struct ShaderText
{
	enum Stage
	{
		kStage_VS,
		kStage_PS,
		kStage_GlobalVS,
		kStage_GlobalPS,
		kStage_COUNT
	};
	
	std::vector<std::string> lines[kStage_COUNT];
};

struct ShaderTextLibrary
{
	std::map<std::string, ShaderText> shaderNodeToShaderText;

	std::map<std::string, std::string> fileToShaderNode;
};

/**
 * Scan shader nodes using a given path.
 * All files with the extension "txt" underneath the given path (including sub-directories) will be
 * will be scanned and a 'node type definition' created for them.
 *
 * Shader node file syntax:
 * in <type-name> <socket-name> [<default-value>]
 * out <type-name> <socket-name>
 * -- [<option>..]
 *
 * in: Specifies an input socket of the given type and name. An optional default value may be specified (defaults to 0).
 * out: Specifies an output socket of the given type and name.
 * --: Clears the current options and sets zero or more new options for the lines that come after it. If empty, all options are cleared.
 *
 * All other lines not starting with "in", "out" or "--" are considered "shader text" lines. These lines are the body of text
 * generated for the node. Local variables are generated for all input and output sockets.
 *
 * Socket names:
 * Socket names may begin with an underscore "_" to make them unique from shader language keyboards, such as "clamp"
 * or "sin" or "cos". For instance, "_clamp" will appear as "clamp" in the graph editor. However: the saved graph will still
 * refer to them by their original names. So please consider this before renaming variables, as it will invalidate older graphs.
 *
 * Types:
 * Acceptable types are,
 * float: A floating point number.
 * vec4: A four-component floating point vector.
 * bool: A boolean.
 * string: A string constant. NOTE: Strings will not create local variables, and are used (for now) by some built-in nodes recognized by the shader generator only.
 *
 * Options:
 * For now there are no options, yet. Ideas for options: Begin a code section to be included once, at the global scope (such as a shader include), limit the scope of the following lines to a specific shading language.
 *
 */
static void scanShaderNodes(const char * path, Graph_TypeDefinitionLibrary & types, ShaderTextLibrary & shaderTextLibrary)
{
	// list files, recursively
	
	auto files = listFiles(path, true);
	
	for (auto & file : files)
	{
		// skip this file if it isn't a text file
		
		if (Path::GetExtension(file, true) != "txt")
			continue;
			
		// attempt to load the file contents as text
		
		std::vector<std::string> lines;
		TextIO::LineEndings lineEndings;
		if (TextIO::load(file.c_str(), lines, lineEndings) == false)
		{
			logWarning("failed to load lines for file: %s", file.c_str());
			continue;
		}
		
		// fill in the node type definition and extract the shader text
		
		bool failed = false;
		
		Graph_TypeDefinition typeDefinition;
		typeDefinition.typeName = String::Replace(Path::GetBaseName(file), '-', '.');
		
		ShaderText shaderText;
		
		struct Options
		{
			bool emitStage[ShaderText::kStage_COUNT] = { };
			
			Options()
			{
			}
		};
		
		Options options;
		options.emitStage[ShaderText::kStage_VS] = true;
		options.emitStage[ShaderText::kStage_PS] = true;
		
		for (auto & line : lines)
		{
			if (String::StartsWithC(line.c_str(), "in "))
			{
				// line specifies an input socket
				// in <type-name> <name> [<default-value>] [additional-options..]
				
				std::vector<std::string> parts;
				splitString(line, parts);
				
				if (parts.size() < 3)
				{
					logError("missing input socket type and/or name");
					failed = true;
				}
				else
				{
					const auto & typeName = parts[1];
					const auto & name = parts[2];
					
					Graph_TypeDefinition::InputSocket inputSocket;
					inputSocket.typeName = typeName;
					inputSocket.name = name;
					if (name[0] == '_')
						inputSocket.displayName = name.c_str() + 1;
						
					if (typeName == "enum")
					{
						inputSocket.typeName = "int";
						inputSocket.enumName = String::FormatC("%s:%s", typeDefinition.typeName.c_str(), name.c_str());
					}
					
					if (parts.size() >= 4)
					{
						const auto & defaultValue = parts[3];
						
						inputSocket.defaultValue = defaultValue;
					}
							
					for (size_t i = 4; i < parts.size(); ++i)
					{
						if (false)
						{
							// handle additional options here
						}
						else
						{
							logError("unknown input socket option: %s", parts[i].c_str());
							failed = true;
						}
					}
					
					typeDefinition.inputSockets.push_back(inputSocket);
				}
			}
			else if (String::StartsWithC(line.c_str(), "out "))
			{
				// line specifies an output socket
				// out <type-name> <name>
				
				std::vector<std::string> parts;
				splitString(line, parts);
				
				if (parts.size() < 3)
				{
					logError("missing input socket type and/or name");
					failed = true;
				}
				else
				{
					const auto & typeName = parts[1];
					const auto & name = parts[2];
					
					Graph_TypeDefinition::OutputSocket outputSocket;
					outputSocket.typeName = typeName;
					outputSocket.name = name;
					if (name[0] == '_')
						outputSocket.displayName = name.c_str() + 1;
					
					for (size_t i = 3; i < parts.size(); ++i)
					{
						if (false)
						{
							// handle additional options here
						}
						else
						{
							logError("unknown output socket option: %s", parts[i].c_str());
							failed = true;
						}
					}
					
					typeDefinition.outputSockets.push_back(outputSocket);
				}
			}
			else if (String::StartsWithC(line.c_str(), "enum "))
			{
				const char * begin = line.c_str() + strlen("enum ");
				
				while (begin[0] != 0 && isspace(begin[0]))
					begin++;
				
				const char * end = begin;
				
				while (end[0] != 0 && !isspace(end[0]))
					end++;
				
				const std::string name(begin, end);
				
				if (name.empty())
				{
					logError("missing enum name");
					failed = true;
				}
				else
				{
				#if 1
					Graph_EnumDefinition enumDefinition;
					enumDefinition.enumName = String::FormatC("%s:%s", typeDefinition.typeName.c_str(), name.c_str());
				
					for (;;)
					{
						const char * value_begin = end;
						
						while (value_begin[0] != 0 && isspace(value_begin[0]))
							value_begin++;
						
						if (value_begin[0] == 0)
							break;
							
						const char * value_end = value_begin;
						
						while (value_end[0] != 0 && value_end[0] != ':')
							value_end++;
						
						if (value_end[0] != ':')
						{
							logError("missing ':' after enum value");
							failed = true;
							break;
						}
						
						const std::string value(value_begin, value_end);
						
						if (value.empty())
						{
							logError("missing enum value");
							failed = true;
							break;
						}
						
						const char * key_begin = value_end + 1;
						
						std::string key;
						
						if (key_begin[0] == '"')
						{
							key_begin++;
							
							const char * key_end = key_begin;
							
							while (key_end[0] != 0 && key_end[0] != '"')
								key_end++;
							
							if (key_end[0] != '"')
							{
								logError("missing trailing '\"' to terminate enum key");
								failed = true;
								break;
							}
							
							key = std::string(key_begin, key_end);
							
							key_end++;
							
							end = key_end;
						}
						else
						{
							const char * key_end = key_begin;
							
							while (key_end[0] != 0 && !isspace(key_end[0]))
								key_end++;
							
							key = std::string(key_begin, key_end);
							
							end = key_end;
						}
						
						if (key.empty())
						{
							logError("missing enum key");
							failed = true;
							break;
						}
						
						Graph_EnumDefinition::Elem elem;
						elem.name = key;
						elem.valueText = value;
						enumDefinition.enumElems.push_back(elem);
					}
				#endif
				
					types.enumDefinitions[enumDefinition.enumName] = enumDefinition;
				}
			}
			else if (String::StartsWithC(line.c_str(), "--"))
			{
				// -- [options..]
				
				std::vector<std::string> parts;
				splitString(line, parts);
				
				// reset options
				
				options = Options();
				
				// handle options
				
				bool emitVs = false;
				bool emitPs = false;
				bool emitGlobal = false;
				
			// todo : add deduplicate option for global sections : allows nodes to write textures, samplers and uniforms themselves
			
				for (size_t i = 1; i < parts.size(); ++i)
				{
					if (parts[i] == "vs")
						emitVs = true;
					else if (parts[i] == "ps")
						emitPs = true;
					else if (parts[i] == "once")
						emitGlobal = true;
					else
					{
						logError("unknown option(s): %s", parts[i].c_str());
						failed = true;
					}
				}
				
				if (emitVs == false && emitPs == false)
				{
					emitVs = true;
					emitPs = true;
				}
				
				if (emitGlobal)
				{
					options.emitStage[ShaderText::kStage_GlobalVS] = emitVs;
					options.emitStage[ShaderText::kStage_GlobalPS] = emitPs;
				}
				else
				{
					options.emitStage[ShaderText::kStage_VS] = emitVs;
					options.emitStage[ShaderText::kStage_PS] = emitPs;
				}
			}
			else
			{
				for (int i = 0; i < ShaderText::kStage_COUNT; ++i)
				{
					if (options.emitStage[i])
					{
						if (shaderText.lines[i].empty() && line.empty())
						{
							// ignore the first empty line(s) to make the generated text more pretty
						}
						else
						{
							shaderText.lines[i].push_back(line);
							shaderText.lines[i].back().push_back('\n');
						}
					}
				}
			}
		}
		
		// remove trailing empty line(s) to make the generated text more pretty
		
		for (int i = 0; i < ShaderText::kStage_COUNT; ++i)
		{
			auto & lines = shaderText.lines[i];
			
			while (lines.empty() == false && lines.back().size() == 1 && lines.back()[0] == '\n')
			{
				lines.pop_back();
			}
		
			//logDebug("shaderText[%d]: %s", shaderText.lines[i]c_str());
		}
		
		// set description for specific nodes
		
		if (typeDefinition.typeName == "ps.input")
		{
			typeDefinition.description =
				"position - object-space position\n"
				"normal - object-space normal\n"
				"position_view - view-space position\n"
				"normal_view - view-space normal\n"
				"color - color (rgba)\n"
				"texcoord1 - texture coordinate (uv)\n"
				"texcoord2 - texture coordinate (uv)";
		}
		
		// assign socket indices
		
		for (int i = 0; i < (int)typeDefinition.inputSockets.size(); ++i)
			typeDefinition.inputSockets[i].index = i;
		for (int i = 0; i < (int)typeDefinition.outputSockets.size(); ++i)
			typeDefinition.outputSockets[i].index = i;
		
		// check type definition is not a duplicate
		
		if (types.typeDefinitions.count(typeDefinition.typeName) != 0)
		{
			logError("duplicate shader node type: %s", typeDefinition.typeName.c_str());
			failed = true;
		}
		
		// register node type and shader text, if all went well
		
		if (failed == false)
		{
			shaderTextLibrary.shaderNodeToShaderText[typeDefinition.typeName] = shaderText;
			
			types.typeDefinitions[typeDefinition.typeName] = typeDefinition;
			
			shaderTextLibrary.fileToShaderNode[file] = typeDefinition.typeName;
		}
	}
}

class ShaderTextWriter
{
	static const int kStringBufferSize = 1 << 12;
	
	std::string m_text;
	
	int m_indent = 0;
	
public:
	ShaderTextWriter()
	{
		m_text.reserve(1 << 16);
	}
	
	void Indent()
	{
		m_indent++;
	}
	
	void Unindent()
	{
		Assert(m_indent > 0);
		
		m_indent--;
	}
	
	void Append(const char * text)
	{
		for (int i = 0; i < m_indent; ++i)
			m_text.push_back('\t');
			
		m_text.append(text);
	}
	
	void AppendFormat(const char * format, ...)
	{
		va_list va;
		va_start(va, format);
		char text[kStringBufferSize];
		vsprintf_s(text, sizeof(text), format, va);
		va_end(va);
		
		Append(text);
	}
	
	void AppendLine()
	{
		for (int i = 0; i < m_indent; ++i)
			m_text.push_back('\t');
			
		m_text.push_back('\n');
	}
	
	const char * ToString() const
	{
		return m_text.c_str();
	}
};

static std::string generateScopeName(const GraphNodeId nodeId)
{
	char name[32];
	sprintf_s(name, sizeof(name), "node_%d_", nodeId);
	
	return name;
}

static size_t calculateTypeNameLength(const std::string & typeName)
{
	// type names may be specialized to for instance include a semantic
	// interpretation of the type. an example of a specialized type name
	// is "vec4.color". for shader compilation, however, we need the
	// non-specialized name to generate valid code. so in the case of
	// "vec4.color" we want to generate "vec4" as a type. in order to
	// do so, we write the type name using "%.*s" as a formatting
	// string. with the length of the type name to write calculated by
	// this function
	
	const char * end = strchr(typeName.c_str(), '.'); // look for where the specialization part begins by looking for the first '.' character
	
	return
		end == nullptr
		? typeName.size()         // no specialization
		: end - typeName.c_str(); // return length up to the '.' character
}

static std::string makeColorString(const char * hex)
{
	const Color color = Color::fromHex(hex);
	
	char text[256];
	sprintf_s(text, sizeof(text), "%f, %f, %f, %f",
		color.r,
		color.g,
		color.b,
		color.a);
		
	return text;
}

static std::string applyTextSubstitutions(const char * text, const GraphNode & node)
{
	std::string result;
	
	while (text[0] != 0)
	{
		const char c = text[0];
		
		if (c == '#')
		{
			result.append(generateScopeName(node.id));
			
			text++;
		}
	#if 0
		else if (c == '$')
		{
			const char * name_begin = text + 1;
			
			const char * name_end = name_begin;
			
			while (name_end[0] != 0 && isalnum(name_end[0]))
				name_end++;
			
			const std::string name(name_begin, name_end);
			
			auto value_itr = node.inputValues.find(name);
			
			if (value_itr != node.inputValues.end())
			{
				auto & value = value_itr->second;
				
				result.append(value);
			}
			
			text = name_end;
		}
	#endif
		else
		{
			result.push_back(c);
			
			text++;
		}
	}
	
	return result;
}

/**
 * Check if the graph is valid and whether it references any unknown node types. When this check passes, it's safe to assume node and node type lookups will always succeed,
 * and links are pointing to valid nodes and sockets.
 * Performing graph verification prior to translating the graph into shader text means we don't need any safety checks performed on the results of various tryGet*** methods on
 * the graph and the type definition library.
 */
static bool verifyShaderGraph(const Graph & graph, const Graph_TypeDefinitionLibrary & typeDefinitionLibrary)
{
	bool result = true;
	
	for (auto & node_itr : graph.nodes)
	{
		auto & node = node_itr.second;
		
		auto * nodeType = typeDefinitionLibrary.tryGetTypeDefinition(node.typeName);
		
		if (nodeType == nullptr)
		{
			logError("failed to find node type: %s", node.typeName.c_str());
			result = false;
		}
	}
	
	for (auto & link_itr : graph.links)
	{
		auto & link = link_itr.second;
		
		auto * srcNode = graph.tryGetNode(link.srcNodeId);
		auto * dstNode = graph.tryGetNode(link.dstNodeId);
		
		if (srcNode == nullptr)
		{
			logError("failed to find link src node. nodeId=%d", link.srcNodeId);
			result = false;
		}
		
		if (dstNode == nullptr)
		{
			logError("failed to find link dst node. nodeId=%d", link.dstNodeId);
			result = false;
		}
		
		if (srcNode != nullptr)
		{
			auto * srcType = typeDefinitionLibrary.tryGetTypeDefinition(srcNode->typeName);
			
			if (srcType != nullptr)
			{
				if (link.srcNodeSocketIndex < 0 || link.srcNodeSocketIndex >= srcType->inputSockets.size())
				{
					logError("link src socket index is out of range. linkId=%d", link.id);
					result = false;
				}
			}
		}
		
		if (dstNode != nullptr)
		{
			auto * dstType = typeDefinitionLibrary.tryGetTypeDefinition(dstNode->typeName);
			
			if (dstType != nullptr)
			{
				if (link.dstNodeSocketIndex < 0 || link.dstNodeSocketIndex >= dstType->outputSockets.size())
				{
					logError("link dst socket index is out of range. linkId=%d", link.id);
					result = false;
				}
			}
		}
	}
	
	return result;
}

static bool generateShaderText_traverse(
	const Graph & graph,
	const GraphNode & node,
	const Graph_TypeDefinitionLibrary & typeDefinitionLibrary,
	const ShaderTextLibrary & shaderTextLibrary,
	const char shaderStage, // 'v' or 'p' for vs or ps
	ShaderTextWriter & shaderText,
	std::set<GraphNodeId> & traversedNodes,
	std::set<std::string> & nodeDependencies,
	std::map<int, int> * usedVsInputsByNode)
{
	bool result = true;
	
	if (shaderStage != 'v' &&
		shaderStage != 'p')
	{
		logError("invalid shader stage: %c", shaderStage);
		return false;
	}
	
	auto * srcNode = &node;
	
	// register node traversal
	
	Assert(traversedNodes.count(srcNode->id) == 0);
	
	traversedNodes.insert(srcNode->id);
	
	// register node dependency
	
	nodeDependencies.insert(srcNode->typeName);
	
	// generate shader text for dependencies
	
	for (auto & link_itr : graph.links)
	{
		auto & link = link_itr.second;
		
		if (link.srcNodeId == srcNode->id)
		{
			if (traversedNodes.count(link.dstNodeId) == 0)
			{
				auto * dstNode = graph.tryGetNode(link.dstNodeId);
				
				generateShaderText_traverse(
					graph,
					*dstNode,
					typeDefinitionLibrary,
					shaderTextLibrary,
					shaderStage,
					shaderText,
					traversedNodes,
					nodeDependencies,
					usedVsInputsByNode);
				
				if (usedVsInputsByNode != nullptr)
				{
					(*usedVsInputsByNode)[srcNode->id] |= (*usedVsInputsByNode)[dstNode->id];
				}
			}
		}
	}
	
	// generate shader text for node
	
	auto * srcType = typeDefinitionLibrary.tryGetTypeDefinition(srcNode->typeName);
	
	// generate outputs (global scope)
	
	auto srcScopeName = generateScopeName(srcNode->id);
	
	for (auto & output : srcType->outputSockets)
	{
		shaderText.AppendFormat("%.*s %s%s;\n",
			calculateTypeNameLength(output.typeName),
			output.typeName.c_str(),
			srcScopeName.c_str(),
			output.name.c_str());
	}
	
	shaderText.Append("{\n"); // local scope : begin
	shaderText.Indent();
	{
		// generate inputs
		
		bool generatedInputs = false;
		
		for (auto & input : srcType->inputSockets)
		{
			if (input.typeName == "string")
				continue;
				
			generatedInputs = true;
			
			const std::string * inputValue = &input.defaultValue;
			
			auto inputValue_itr = srcNode->inputValues.find(input.name);
			if (inputValue_itr != srcNode->inputValues.end())
				inputValue = &inputValue_itr->second;
				
			// <type> <name> = <default-value>
			
			shaderText.AppendFormat("%.*s %s = %.*s(%s);\n",
				calculateTypeNameLength(input.typeName),
				input.typeName.c_str(),
				input.name.c_str(),
				calculateTypeNameLength(input.typeName),
				input.typeName.c_str(),
				inputValue->empty()
				? "0"
				: input.typeName == "vec4.color"
				? makeColorString(inputValue->c_str()).c_str()
				: inputValue->c_str());
		}
		
		if (generatedInputs)
		{
			shaderText.AppendLine();
		}
		
		// assign inputs
		
		std::set<std::string> assignedInputs;
		
		for (auto & link_itr : graph.links)
		{
			auto & link = link_itr.second;
			
			if (link.srcNodeId == srcNode->id)
			{
				auto * dstNode = graph.tryGetNode(link.dstNodeId);
				auto * dstType = typeDefinitionLibrary.tryGetTypeDefinition(dstNode->typeName);
				
				auto & dstSocket = dstType->outputSockets[link.dstNodeSocketIndex];
				auto & srcSocket = srcType->inputSockets[link.srcNodeSocketIndex];
				
				auto dstScopeName = generateScopeName(dstNode->id);
				
				shaderText.AppendFormat("%s = %s%s;\n",
					srcSocket.name.c_str(),
					dstScopeName.c_str(),
					dstSocket.name.c_str());
					
				assignedInputs.insert(srcSocket.name);
			}
		}
		
		if (!assignedInputs.empty())
		{
			shaderText.AppendLine();
		}
		
		for (auto & input : srcType->inputSockets)
		{
			if (input.typeName == "string")
				continue;
			
			shaderText.AppendFormat("bool %s_isReferenced = %s;\n",
				input.name.c_str(),
				assignedInputs.count(input.name) == 0 ? "false" : "true");
		}
		
	#if ENABLE_UNUSED_VARIABLE_VOID
		if (generatedInputs)
		{
			shaderText.AppendLine();
			shaderText.Append("// suppress unused variable warning(s)\n");
		}
		
		for (auto & input : srcType->inputSockets)
		{
			if (input.typeName == "string")
				continue;
			
			shaderText.AppendFormat("(void)%s_isReferenced;\n",
				input.name.c_str());
		}
	#endif
		
		if (generatedInputs)
		{
			shaderText.AppendLine();
		}
		
		// generate flags
		
		shaderText.AppendFormat("bool isPassthrough = %s;\n",
			node.isPassthrough ? "true" : "false");
	#if ENABLE_UNUSED_VARIABLE_VOID
		shaderText.Append("(void)isPassthrough; // suppress unused variable warning\n");
	#endif
		
		shaderText.AppendLine();
		
		// generate outputs (local scope)
		
		auto srcScopeName = generateScopeName(srcNode->id);
		
		for (auto & output : srcType->outputSockets)
		{
			shaderText.AppendFormat("%.*s %s;\n",
				calculateTypeNameLength(output.typeName),
				output.typeName.c_str(),
				output.name.c_str());
		}
		
		if (!srcType->outputSockets.empty())
		{
			shaderText.AppendLine();
		}
		
		// emit node shader text
		
		auto text_itr = shaderTextLibrary.shaderNodeToShaderText.find(srcNode->typeName);
	
		Assert(text_itr != shaderTextLibrary.shaderNodeToShaderText.end());
	
		const auto & text = text_itr->second;
		
		const int stage =
			shaderStage == 'v' ? ShaderText::kStage_VS :
			shaderStage == 'p' ? ShaderText::kStage_PS :
			-1;
		
		if (text.lines[stage].empty())
		{
			logError("missing shader node text for node %s, stage %cs", node.typeName.c_str(), shaderStage);
			result = false;
		}
		else
		{
			for (auto & line : text.lines[stage])
			{
				shaderText.Append(applyTextSubstitutions(line.c_str(), node).c_str());
			}
			
			shaderText.AppendLine();
		}
		
		// assign outputs
		
		if (srcType->typeName == "immediate.float")
		{
			// uniform -> global scope output
			
			bool assignedResult = false;
			
			auto name_itr = node.inputValues.find("name");
			
			if (name_itr != node.inputValues.end())
			{
				auto & name = name_itr->second;
				
				if (srcNode->isPassthrough == false)
				{
					shaderText.AppendFormat("result = %s;\n",
						name.c_str());
						
					assignedResult = true;
				}
			}
			
			if (assignedResult == false)
			{
				shaderText.Append("result = 0.0;\n");
			}
			
			shaderText.AppendFormat("%s%s = result;\n",
				srcScopeName.c_str(),
				"result");
		}
		else if (srcType->typeName == "vs.input")
		{
			// buffer -> global scope output
			
			bool assignedResult = false;
			
			auto name_itr = node.inputValues.find("name");
			
			if (name_itr != node.inputValues.end())
			{
				auto & name = name_itr->second;
				
				if (srcNode->isPassthrough == false)
				{
					std::string loadCode;
					int vsInput = 0;
					
					if (name == "position")
					{
						loadCode = "unpackPosition()";
						vsInput = VS_POSITION;
					}
					else if (name == "normal")
					{
						loadCode = "unpackNormal()";
						vsInput = VS_NORMAL;
					}
					else if (name == "color")
					{
						loadCode = "unpackColor()";
						vsInput = VS_COLOR;
					}
					else if (name == "texcoord1")
					{
						loadCode = "vec4(unpackTexcoord(0), 0.0, 0.0)";
						vsInput = VS_TEXCOORD0;
					}
					else if (name == "texcoord2")
					{
						loadCode = "vec4(unpackTexcoord(1), 0.0, 0.0)";
						vsInput = VS_TEXCOORD1;
					}
					
					if (!loadCode.empty())
					{
						shaderText.AppendFormat("result = %s;\n",
							loadCode.c_str());
						
						assignedResult = true;
						
						const auto viewSpace_itr = node.inputValues.find("viewSpace");
				
						const bool viewSpace =
							viewSpace_itr == node.inputValues.end() ? false :
							Parse::Bool(viewSpace_itr->second);
							
						if (viewSpace)
						{
							shaderText.Append("result = objectToView(result);\n");
						}
						
						if (usedVsInputsByNode != nullptr)
						{
							(*usedVsInputsByNode)[srcNode->id] |= 1 << vsInput;
						}
					}
				}
			}
			
			if (assignedResult == false)
			{
				shaderText.Append("result = vec4(0.0);\n");
			}

			shaderText.AppendLine();
			
		#if ENABLE_UNUSED_VARIABLE_VOID
			shaderText.Append("(void)viewSpace; // suppress unused variable warning\n\n");
			shaderText.AppendLine();
		#endif
			
			shaderText.AppendFormat("%s%s = result;\n",
				srcScopeName.c_str(),
				"result");
		}
		else if (srcType->typeName == "vs.output")
		{
			// local variable -> varying
			
			auto name_itr = node.inputValues.find("name");
			
			if (name_itr != node.inputValues.end())
			{
				auto & name = name_itr->second;
				
				if (srcNode->isPassthrough)
				{
					shaderText.AppendFormat("v_%s = vec4(0.0);\n",
						name.c_str());
				}
				else
				{
					shaderText.AppendFormat("v_%s = %s;\n",
						name.c_str(),
						"xyzw");
				}
			}
		}
		else if (srcType->typeName == "ps.input")
		{
			// varying -> global scope output
			
			bool assignedResult = false;
			
			auto name_itr = node.inputValues.find("name");
			
			if (name_itr != node.inputValues.end())
			{
				auto & name = name_itr->second;
				
				if (srcNode->isPassthrough == false)
				{
					shaderText.AppendFormat("result = v_%s;\n",
						name.c_str());
					
					assignedResult = true;
					
					const auto normalize_itr = node.inputValues.find("_normalize");
			
					const bool normalize =
						normalize_itr == node.inputValues.end() ? false :
						Parse::Bool(normalize_itr->second);
						
					if (normalize)
					{
						shaderText.Append("result = normalize(result);\n");
					}
				}
			}
			
			if (assignedResult == false)
			{
				shaderText.Append("result = vec4(0.0);\n");
			}
			
			shaderText.AppendLine();
			
		#if ENABLE_UNUSED_VARIABLE_VOID
			shaderText.Append("(void)_normalize; // suppress unused variable warning\n");
			shaderText.AppendLine();
		#endif
				
			shaderText.AppendFormat("%s%s = result;\n",
				srcScopeName.c_str(),
				"result");
		}
		else
		{
			// local variable -> global scope output
			
			for (auto & output : srcType->outputSockets)
			{
				shaderText.AppendFormat("%s%s = %s;\n",
					srcScopeName.c_str(),
					output.name.c_str(),
					output.name.c_str());
			}
		}
	}
	shaderText.Unindent();
	shaderText.Append("}\n\n"); // local scope : end
	
	return result;
}

static bool generateVsShaderText(
	const Graph & graph,
	const Graph_TypeDefinitionLibrary & typeDefinitionLibrary,
	const ShaderTextLibrary & shaderTextLibrary,
	const std::set<std::string> & usedVsOutputs,
	const bool generateNodePreviewMode,
	ShaderTextWriter & shaderText,
	std::set<std::string> & nodeDepenencies,
	std::map<int, int> * usedVsInputsByNode)
{
	bool result = true;
	
	if (verifyShaderGraph(graph, typeDefinitionLibrary) == false)
	{
		return false;
	}
	
	// generate header
	
	shaderText.Append("include engine/ShaderVS.txt\n");
	shaderText.AppendLine();
	
	// generate uniforms
	
	std::set<std::string> generatedUniforms;
	
	for (auto & node_itr : graph.nodes)
	{
		auto & node = node_itr.second;
		
		if (node.isPassthrough)
			continue;
				
		if (node.typeName == "immediate.float")
		{
			auto name_itr = node.inputValues.find("name");
			
			if (name_itr != node.inputValues.end())
			{
				auto & name = name_itr->second;
			
				if (generatedUniforms.count(name) == 0)
				{
					generatedUniforms.insert(name);
					
					shaderText.AppendFormat("uniform float %s;\n",
						name.c_str());
				}
			}
		}
	}
	
	if (generateNodePreviewMode)
	{
		shaderText.Append("uniform float u_nodePreview_nodeId;\n");
		shaderText.Append("uniform float u_nodePreview_socketIndex;\n");
		
		generatedUniforms.insert("u_nodePreview_nodeId");
		generatedUniforms.insert("u_nodePreview_socketIndex");
	}
	
	if (!generatedUniforms.empty())
	{
		shaderText.AppendLine();
	}
	
	// generate vertex outputs (unused -> local variable)
	
	std::set<std::string> generatedVsOutputs;
					
	for (auto & node_itr : graph.nodes)
	{
		auto & node = node_itr.second;
		
		if (node.isPassthrough)
			continue;
				
		if (node.typeName != "vs.output")
			continue;

		auto name_itr = node.inputValues.find("name");
		
		if (name_itr == node.inputValues.end())
			continue;

		auto & name = name_itr->second;
		
		if (usedVsOutputs.count(name) != 0)
			continue;
		
		if (generatedVsOutputs.count(name) != 0)
			continue;
		
		generatedVsOutputs.insert(name);
		
		shaderText.AppendFormat("vec4 v_%s;\n",
			name.c_str());
	}
	
	// generate vertex outputs (used -> shader_out)
	
	for (auto & varying : usedVsOutputs)
	{
		if (generatedVsOutputs.count(varying) != 0)
			continue;
		
		generatedVsOutputs.insert(varying);
		
		shaderText.AppendFormat("shader_out vec4 v_%s;\n",
			varying.c_str());
	}
	
	if (generateNodePreviewMode)
	{
		shaderText.Append("shader_out vec4 v_nodePreview;\n");
		
		generatedVsOutputs.insert("v_nodePreview");
	}
	
	if (!generatedVsOutputs.empty())
	{
		shaderText.AppendLine();
	}
	
	// generate once text
	
	std::set<std::string> generatedOnceNodes;
	
	for (auto & node_itr : graph.nodes)
	{
		auto & node = node_itr.second;
		
		if (generatedOnceNodes.count(node.typeName) != 0)
			continue;
		
		generatedOnceNodes.insert(node.typeName);
		
		auto text_itr = shaderTextLibrary.shaderNodeToShaderText.find(node.typeName);
		
		if (text_itr != shaderTextLibrary.shaderNodeToShaderText.end())
		{
			auto & text = text_itr->second;
			
			auto & lines = text.lines[ShaderText::kStage_GlobalVS];
			
			if (!lines.empty())
			{
				for (auto & line : lines)
				{
					shaderText.Append(applyTextSubstitutions(line.c_str(), node).c_str());
				}
				
				shaderText.AppendLine();
			}
		}
	}
	
	// generate main function
	
	shaderText.Append("void main()\n"); // main : begin
	shaderText.Append("{\n"); // main : begin
	shaderText.Indent();
	{
		// generate code for vertex outputs
		
		std::set<GraphNodeId> traversedNodes;
		
		std::set<std::string> generatedVaryings;
		
		bool hasPositionOutput = false;
		
		for (auto & node_itr : graph.nodes)
		{
			auto & node = node_itr.second;
			
			if (node.isPassthrough)
				continue;
				
			if (node.typeName == "vs.output")
			{
				auto name_itr = node.inputValues.find("name");
				
				if (name_itr == node.inputValues.end())
					continue;
				
				auto & name = name_itr->second;
				
				generatedVaryings.insert(name);
			
				result &= generateShaderText_traverse(
					graph,
					node,
					typeDefinitionLibrary,
					shaderTextLibrary,
					'v',
					shaderText,
					traversedNodes,
					nodeDepenencies,
					usedVsInputsByNode);
			}
			
			if (node.typeName == "vs.output.position")
			{
				if (hasPositionOutput)
				{
					logError("found more than one node of type: vs.output.position");
					result = false;
				}
				
				hasPositionOutput = true;
			
				result &= generateShaderText_traverse(
					graph,
					node,
					typeDefinitionLibrary,
					shaderTextLibrary,
					'v',
					shaderText,
					traversedNodes,
					nodeDepenencies,
					usedVsInputsByNode);
			}
		}
		
		// generated code for vertex outputs (referenced but not generated by graph)
		
		for (auto & varying : usedVsOutputs)
		{
			if (generatedVaryings.count(varying) != 0)
				continue;
			
			if (varying == "position")
			{
				shaderText.Append("v_position = unpackPosition();\n");
			}
			else if (varying == "normal")
			{
				shaderText.Append("v_normal = unpackNormal();\n");
			}
			else if (varying == "position_view")
			{
				shaderText.Append("v_position_view = objectToView(unpackPosition());\n");
			}
			else if (varying == "normal_view")
			{
				shaderText.Append("v_normal_view = objectToView(unpackNormal());\n");
			}
			else if (varying == "color")
			{
				shaderText.Append("v_color = unpackColor();\n");
			}
			else if (varying == "texcoord1")
			{
				shaderText.Append("v_texcoord0 = vec4(unpackTexcoord(0), 0.0, 0.0);\n");
			}
			else if (varying == "texcoord2")
			{
				shaderText.Append("v_texcoord1 = vec4(unpackTexcoord(1), 0.0, 0.0);\n");
			}
			else
			{
			#if 0
				shaderText.AppendFormat("v_%s = vec4(0.0);\n",
					varying.c_str());
			#else
				logError("vs output '%s' is not generated by shader graph and isn't supported through built-in synthesis", varying.c_str());
				result = false;
			#endif
			}
		}

		// emit position output, if not output explicitly by the shader
		
		if (hasPositionOutput == false)
		{
			shaderText.Append("gl_Position = objectToProjection(unpackPosition());\n");
		}
		
		// emit node preview value
		
		if (generateNodePreviewMode)
		{
			shaderText.AppendLine();
			shaderText.Append("v_nodePreview = vec4(0.0);\n");
				
			// generate node/socket switch
			
			shaderText.AppendLine();
			shaderText.Append("int nodePreview_nodeId = int(u_nodePreview_nodeId);\n");
			shaderText.Append("int nodePreview_socketIndex = int(u_nodePreview_socketIndex);\n");
			
			shaderText.Append("if (nodePreview_nodeId != 0)\n");
			shaderText.Append("{\n");
			shaderText.Indent();
			{
				shaderText.Append("switch (nodePreview_nodeId)\n");
				shaderText.Append("{\n");
				shaderText.Indent();
				{
					for (auto nodeId : traversedNodes)
					{
						auto * node = graph.tryGetNode(nodeId);
						auto * nodeType = typeDefinitionLibrary.tryGetTypeDefinition(node->typeName);
						
						shaderText.AppendFormat("case %d:\n", nodeId);
						shaderText.Indent();
						{
							for (int i = 0; i < (int)nodeType->outputSockets.size(); ++i)
							{
								if (nodeType->outputSockets[i].typeName == "float")
								{
									shaderText.AppendFormat("if (nodePreview_socketIndex == %d) v_nodePreview = vec4(vec3(%s%s), 1.0);\n",
										i,
										generateScopeName(node->id).c_str(),
										nodeType->outputSockets[i].name.c_str());
								}
								else
								{
									shaderText.AppendFormat("if (nodePreview_socketIndex == %d) v_nodePreview = vec4(%s%s);\n",
										i,
										generateScopeName(node->id).c_str(),
										nodeType->outputSockets[i].name.c_str());
								}
							}
							shaderText.Append("break;\n");
						}
						shaderText.Unindent();
					}
				}
				shaderText.Unindent();
				shaderText.Append("}\n");
			}
			shaderText.Unindent();
			shaderText.Append("}\n");
		}
	}
	shaderText.Unindent();
	shaderText.Append("}\n"); // main : end

	return result;
}

static const GraphNode * findPsOutputNodeByName(const Graph & graph, const char * name)
{
	const GraphNode * result = nullptr;
	
	for (auto & node_itr : graph.nodes)
	{
		auto & node = node_itr.second;
		
		if (node.isPassthrough)
			continue;
				
		if (node.typeName == "ps.output")
		{
			auto name_itr = node.inputValues.find("name");
			
			if (name_itr != node.inputValues.end() && name_itr->second == name)
				result = &node;
		}
	}
	
	return result;
}

static bool generatePsShaderText(
	const Graph & graph,
	const Graph_TypeDefinitionLibrary & typeDefinitionLibrary,
	const ShaderTextLibrary & shaderTextLibrary,
	const char * forOutput,
	const char * toInput,
	const ShaderOutput ** shaderOutputs,
	const int numShaderOutputs,
	const bool generateNodePreviewMode,
	ShaderTextWriter & shaderText,
	std::set<std::string> & nodeDependencies)
{
	bool result = true;
	
	if (verifyShaderGraph(graph, typeDefinitionLibrary) == false)
	{
		return false;
	}
	
	// generate header

	shaderText.Append("include engine/ShaderPS.txt\n");
	shaderText.AppendLine();

	// generate uniforms
	
	std::set<std::string> generatedUniforms;
	
	for (auto & node_itr : graph.nodes)
	{
		auto & node = node_itr.second;
		
		if (node.isPassthrough)
			continue;
		
		if (node.typeName == "immediate.float")
		{
			auto name_itr = node.inputValues.find("name");
			
			if (name_itr != node.inputValues.end())
			{
				auto & name = name_itr->second;
			
				if (generatedUniforms.count(name) == 0)
				{
					generatedUniforms.insert(name);
					
					shaderText.AppendFormat("uniform float %s;\n",
						name.c_str());
				}
			}
		}
	}
	
	if (generateNodePreviewMode)
	{
		shaderText.Append("uniform float u_nodePreview_nodeId;\n");
		shaderText.Append("uniform float u_nodePreview_socketIndex;\n");
		
		generatedUniforms.insert("u_nodePreview_nodeId");
		generatedUniforms.insert("u_nodePreview_socketIndex");
	}
	
	if (!generatedUniforms.empty())
	{
		shaderText.AppendLine();
	}
	
	// generate varyings
	
	std::set<std::string> generatedPsInputs;
	
	for (auto & node_itr : graph.nodes)
	{
		auto & node = node_itr.second;
		
		if (node.isPassthrough)
			continue;
			
		if (node.typeName == "ps.input")
		{
			auto name_itr = node.inputValues.find("name");
			
			if (name_itr != node.inputValues.end())
			{
				auto & name = name_itr->second;
			
				if (generatedPsInputs.count(name) == 0)
				{
					generatedPsInputs.insert(name);
					
					shaderText.AppendFormat("shader_in vec4 v_%s;\n",
						name.c_str());
				}
			}
		}
	}
	
	if (generateNodePreviewMode)
	{
		shaderText.Append("shader_in vec4 v_nodePreview;\n");
		
		generatedPsInputs.insert("v_nodePreview");
	}
	
	if (!generatedPsInputs.empty())
	{
		shaderText.AppendLine();
	}
	
	// generate once text
	
	std::set<std::string> generatedOnceNodes;
	
	for (auto & node_itr : graph.nodes)
	{
		auto & node = node_itr.second;
		
		if (generatedOnceNodes.count(node.typeName) != 0)
			continue;
		
		generatedOnceNodes.insert(node.typeName);
		
		auto text_itr = shaderTextLibrary.shaderNodeToShaderText.find(node.typeName);
		
		if (text_itr != shaderTextLibrary.shaderNodeToShaderText.end())
		{
			auto & text = text_itr->second;
			
			auto & lines = text.lines[ShaderText::kStage_GlobalPS];
			
			if (!lines.empty())
			{
				for (auto & line : lines)
				{
					shaderText.Append(applyTextSubstitutions(line.c_str(), node).c_str());
				}
				
				shaderText.AppendLine();
			}
		}
	}
	
	// generate main function

	shaderText.Append("void main()\n"); // main : begin
	shaderText.Append("{\n"); // main : begin
	shaderText.Indent();
	{
		std::set<GraphNodeId> traversedNodes;
		
		for (int i = 0; i < numShaderOutputs; ++i)
		{
			if (i != 0)
			{
				shaderText.AppendLine();
			}
			
			auto * shaderOutput = shaderOutputs[i];
			
			auto * outputNode = findPsOutputNodeByName(graph, shaderOutput->longName.c_str());
			
			if (outputNode == nullptr)
			{
				logError("failed to find ps.output node for shader output: %s", shaderOutput->longName.c_str());
				result &= false;
				continue;
			}
			
			result &= generateShaderText_traverse(
				graph,
				*outputNode,
				typeDefinitionLibrary,
				shaderTextLibrary,
				'p',
				shaderText,
				traversedNodes,
				nodeDependencies,
				nullptr);

			shaderText.AppendFormat("%s = %s%s;\n",
				shaderOutput->outputName.c_str(),
				generateScopeName(outputNode->id).c_str(),
				"result");
		}
		
		if (generateNodePreviewMode && numShaderOutputs > 0)
		{
			// generate node/socket switch
			
			shaderText.AppendLine();
			shaderText.Append("int nodePreview_nodeId = int(u_nodePreview_nodeId);\n");
			shaderText.Append("int nodePreview_socketIndex = int(u_nodePreview_socketIndex);\n");
			
			shaderText.Append("if (nodePreview_nodeId != 0)\n");
			shaderText.Append("{\n");
			shaderText.Indent();
			{
				shaderText.Append("vec4 nodePreview_result = v_nodePreview;\n");
				shaderText.AppendLine();
				
				shaderText.Append("switch (nodePreview_nodeId)\n");
				shaderText.Append("{\n");
				shaderText.Indent();
				{
					for (auto nodeId : traversedNodes)
					{
						auto * node = graph.tryGetNode(nodeId);
						auto * nodeType = typeDefinitionLibrary.tryGetTypeDefinition(node->typeName);
						
						shaderText.AppendFormat("case %d:\n", nodeId);
						shaderText.Indent();
						{
							for (int i = 0; i < (int)nodeType->outputSockets.size(); ++i)
							{
								if (nodeType->outputSockets[i].typeName == "float")
								{
									shaderText.AppendFormat("if (nodePreview_socketIndex == %d) nodePreview_result = vec4(vec3(%s%s), 1.0);\n",
										i,
										generateScopeName(node->id).c_str(),
										nodeType->outputSockets[i].name.c_str());
								}
								else
								{
									shaderText.AppendFormat("if (nodePreview_socketIndex == %d) nodePreview_result = vec4(%s%s);\n",
										i,
										generateScopeName(node->id).c_str(),
										nodeType->outputSockets[i].name.c_str());
								}
							}
							shaderText.Append("break;\n");
						}
						shaderText.Unindent();
					}
				}
				shaderText.Unindent();
				shaderText.Append("}\n");
				
				shaderText.AppendLine();
				
				shaderText.AppendFormat("%s = nodePreview_result;\n",
					shaderOutputs[0]->outputName.c_str());
			}
			shaderText.Unindent();
			shaderText.Append("}\n");
		}
	}
	shaderText.Unindent();
	shaderText.Append("}\n"); // main : end
	
	return result;
}

static void scanUsedPsVaryings(const Graph & graph, std::set<std::string> & usedVaryings)
{
	for (auto & node_itr : graph.nodes)
	{
		auto & node = node_itr.second;
		
		if (node.isPassthrough)
			continue;
			
		if (node.typeName == "ps.input")
		{
			auto name_itr = node.inputValues.find("name");
		
			if (name_itr != node.inputValues.end())
			{
				auto & name = name_itr->second;
				
				usedVaryings.insert(name);
			}
		}
	}
}

static void drawDstSocketPreview(const GraphNodeId nodeId, const int dstSocketIndex, const int referencedVsInputs)
{
#if false // todo : determine referenced vs inputs at call site
	const int referencedVsInputs =
		usedVsInputsByNode.count(previewNodeId) != 0
		? usedVsInputsByNode[previewNodeId]
		: allUsedVsInputs; // todo : make this more accurate, by including vs input flags indirectly referenced by ps.input nodes and its deps
#endif

	int viewSx;
	int viewSy;
	framework.getCurrentViewportSize(viewSx, viewSy);
	
	// draw background
	
	setColor(colorWhite);
	drawUiRectCheckered(0, 0, viewSx, viewSy, 16.f);
	
	// draw preview shape
	
	Shader shader("shader");
	
	setShader(shader);
	{
		shader.setImmediate("time", framework.time);
		shader.setImmediate("u_nodePreview_nodeId", nodeId);
		shader.setImmediate("u_nodePreview_socketIndex", dstSocketIndex);
		
		setColor(colorWhite);
		
		const float kDistance = 4.f;
		
		if ((referencedVsInputs & (1 << VS_NORMAL)) != 0)
		{
			// using normals. draw as a 3d surface
			
			projectPerspective3d(60.f, .01f, 100.f);
			pushDepthTest(true, DEPTH_LESS);
			gxPushMatrix();
			{
				gxTranslatef(0, 0, kDistance);
				
			// todo : we need to provide texcoords also. draw a UV sphere ?
				fillCylinder(Vec3(), 1.f, 1.f, 100);
			}
			gxPopMatrix();
			popDepthTest();
			projectScreen2d();
		}
		else if ((referencedVsInputs & (1 << VS_POSITION)) != 0)
		{
			// using position. draw as a 3d surface
			
			projectPerspective3d(60.f, .01f, 100.f);
			pushDepthTest(true, DEPTH_LESS);
			gxPushMatrix();
			{
				gxTranslatef(0, 0, kDistance);
				
			// todo : we need to provide texcoords also. draw a UV sphere ?
				fillCylinder(Vec3(), 1.f, 1.f, 100);
			}
			gxPopMatrix();
			popDepthTest();
			projectScreen2d();
		}
		else if ((referencedVsInputs & (1 << VS_TEXCOORD0)) != 0)
		{
			// using texcoord. draw as 2d plane
			
			projectScreen2d();
			drawRect(0, 0, viewSx, viewSy);
		}
		else
		{
			// not using any vertex data or vertex data we don't currently provide. draw as a single color quad
			
			projectScreen2d();
			drawRect(0, 0, viewSx, viewSy);
		}
		
		shader.setImmediate("u_nodePreview_nodeId", kGraphNodeIdInvalid);
		shader.setImmediate("u_nodePreview_socketIndex", -1);
	}
	clearShader();
}

class ShaderGraphElem
{
public:
	std::string name;
	std::string outputs;
	
	std::set<std::string> nodeDepenencies;
	
	Shader shader;
	
	ShaderGraphElem(const char * in_name, const char * in_outputs)
		: name(in_name)
		, outputs(in_outputs)
	{
	}
	
	void load(const Graph_TypeDefinitionLibrary & typeDefinitionLibrary, const ShaderTextLibrary & shaderTextLibrary)
	{
		// resolve filename
		
		const char * resolved_filename = framework.resolveResourcePath(name.c_str());
		
		// collect shader outputs
		
		const char * shaderOutputNames = globals.shaderOutputs;
		
		std::vector<const ShaderOutput*> shaderOutputs;
		
		for (int i = 0; shaderOutputNames[i] != 0; ++i)
		{
			auto name = shaderOutputNames[i];
			auto * shaderOutput = findShaderOutput(name);
			shaderOutputs.push_back(shaderOutput);
		}

		// load graph
		
		Graph graph;
		graph.load(resolved_filename, &typeDefinitionLibrary);
		
		// generate vs and ps shader text
		
		std::set<std::string> usedVaryings;
		scanUsedPsVaryings(graph, usedVaryings);
		
		ShaderTextWriter vsWriter;
		ShaderTextWriter psWriter;
		
		generateVsShaderText(graph, typeDefinitionLibrary, shaderTextLibrary, usedVaryings, false, vsWriter, nodeDepenencies, nullptr);
		generatePsShaderText(graph, typeDefinitionLibrary, shaderTextLibrary, "color", "shader_fragColor", shaderOutputs.data(), shaderOutputs.size(), false, psWriter, nodeDepenencies);
		
		// register vs and ps shader text
		
		const std::string nameVs = String::FormatC("%s.vs", name.c_str());
		const std::string namePs = String::FormatC("%s.ps", name.c_str());
		
		const char * textVs = vsWriter.ToString();
		const char * textPs = psWriter.ToString();
		
		framework.registerShaderSource(nameVs.c_str(), textVs);
		framework.registerShaderSource(namePs.c_str(), textPs);
		
		shader = Shader(name.c_str(), shaderOutputNames);
	}
	
	void free()
	{
		const std::string nameVs = String::FormatC("%s.vs", name.c_str());
		const std::string namePs = String::FormatC("%s.ps", name.c_str());
		
		framework.unregisterShaderSource(nameVs.c_str());
		framework.unregisterShaderSource(namePs.c_str());
	}
	
	void reload(const Graph_TypeDefinitionLibrary & typeDefinitionLibrary, const ShaderTextLibrary & shaderTextLibrary)
	{
		free();
		
		load(typeDefinitionLibrary, shaderTextLibrary);
	}
};
	
class ShaderGraphCache : public ResourceCacheBase
{
public:
	class Key
	{
	public:
		std::string name;
		std::string outputs;
		
		inline bool operator<(const Key & other) const
		{
			if (name != other.name)
				return name < other.name;
			if (outputs != other.outputs)
				return outputs < other.outputs;
			return false;
		}
	};
	
	typedef std::map<Key, ShaderGraphElem*> Map;
	
	Graph_TypeDefinitionLibrary m_typeDefinitionLibrary;

	ShaderTextLibrary m_shaderTextLibrary;
	
	Map m_map;
	
	void addShaderNodePath(const char * path)
	{
		scanShaderNodes(path, m_typeDefinitionLibrary, m_shaderTextLibrary);
	}
	
	virtual void clear() override
	{
		for (Map::iterator i = m_map.begin(); i != m_map.end(); ++i)
		{
			i->second->free();
			
			delete i->second;
			i->second = nullptr;
		}
		
		m_map.clear();
	}
	
	virtual void reload() override
	{
		for (Map::iterator i = m_map.begin(); i != m_map.end(); ++i)
		{
			i->second->reload(m_typeDefinitionLibrary, m_shaderTextLibrary);
		}
	}
	
	virtual void handleFileChange(const std::string & filename, const std::string & extension) override
	{
		if (m_shaderTextLibrary.fileToShaderNode.count(filename) != 0)
		{
			auto & shaderNode = m_shaderTextLibrary.fileToShaderNode[filename];
			
			for (auto & i : m_map)
			{
				ShaderGraphElem * cacheElem = i.second;
				
				if (cacheElem->nodeDepenencies.count(shaderNode) != 0)
				{
					cacheElem->reload(m_typeDefinitionLibrary, m_shaderTextLibrary);
				}
			}
		}
	}
	
	ShaderGraphElem & findOrCreate(const char * name)
	{
		const char * outputs = globals.shaderOutputs;
		
		Key key;
		key.name = name;
		key.outputs = outputs;
		
		Map::iterator i = m_map.find(key);
		
		if (i != m_map.end())
		{
			return *i->second;
		}
		else
		{
			ShaderGraphElem * elem = new ShaderGraphElem(name, outputs);
			
			elem->load(m_typeDefinitionLibrary, m_shaderTextLibrary);
			
			i = m_map.insert(Map::value_type(key, elem)).first;
			
			return *i->second;
		}
	}
};

struct ShaderGraph_RealTimeConnection : GraphEdit_RealTimeConnection
{
	struct SocketPreview
	{
		GraphNodeId nodeId = kGraphNodeIdInvalid;
		int dstSocketIndex = -1;
		
		float timeLastReferenced = 0.f;
		
		Surface surface;
	};
	
	std::list<SocketPreview> socketPreviews;
	
	void previewDstSocket(const GraphNodeId nodeId, const int dstSocketIndex)
	{
		for (auto & socketPreview : socketPreviews)
		{
			if (socketPreview.nodeId == nodeId &&
				socketPreview.dstSocketIndex == dstSocketIndex)
			{
				socketPreview.timeLastReferenced = framework.time;
				return;
			}
		}
		
		socketPreviews.emplace_back(SocketPreview());
		auto & socketPreview = socketPreviews.back();
		socketPreview.nodeId = nodeId;
		socketPreview.dstSocketIndex = dstSocketIndex;
		socketPreview.timeLastReferenced = framework.time;
		socketPreview.surface.init(256, 256, SURFACE_RGBA8, true, false);
		
		pushSurface(&socketPreview.surface, true);
		pushBlend(BLEND_OPAQUE);
		{
			drawDstSocketPreview(socketPreview.nodeId, socketPreview.dstSocketIndex, 0xff);
		}
		popBlend();
		popSurface();
	}
	
	void tickVisualizers()
	{
		const float kTimeout = 1.f;
		
		for (auto socketPreview_itr = socketPreviews.begin(); socketPreview_itr != socketPreviews.end(); )
		{
			auto & socketPreview = *socketPreview_itr;
			
			if (socketPreview.timeLastReferenced + kTimeout < framework.time)
			{
				socketPreview.surface.free();
				
				socketPreview_itr = socketPreviews.erase(socketPreview_itr);
			}
			else
			{
				pushSurface(&socketPreview.surface, true);
				pushBlend(BLEND_OPAQUE);
				{
					drawDstSocketPreview(socketPreview.nodeId, socketPreview.dstSocketIndex, 0xff);
				}
				popBlend();
				popSurface();
				
				++socketPreview_itr;
			}
		}
	}
	
	//
	
	virtual bool getDstSocketValue(const GraphNodeId nodeId, const int dstSocketIndex, const std::string & dstSocketName, std::string & value) override
	{
		previewDstSocket(nodeId, dstSocketIndex);
		
		for (auto & socketPreview : socketPreviews)
		{
			if (socketPreview.nodeId == nodeId &&
				socketPreview.dstSocketIndex == dstSocketIndex)
			{
				value = String::FormatC("%d", socketPreview.surface.getTexture());
				return true;
			}
		}
		
		return false;
	}
};

static void registerBuiltinTypes(Graph_TypeDefinitionLibrary & typeDefinitionLibrary)
{
	auto & boolType = typeDefinitionLibrary.valueTypeDefinitions["bool"];
	boolType.typeName = "bool";
	boolType.editor = "checkbox";
	
	auto & intType = typeDefinitionLibrary.valueTypeDefinitions["int"];
	intType.typeName = "int";
	intType.editor = "textbox_int";
	
	auto & floatType = typeDefinitionLibrary.valueTypeDefinitions["float"];
	floatType.typeName = "float";
	floatType.editor = "textbox_float";
	floatType.visualizer = "gx-texture";
	
	auto & vec4Type = typeDefinitionLibrary.valueTypeDefinitions["vec4"];
	vec4Type.typeName = "vec4";
	vec4Type.editor = "textbox_float";
	vec4Type.visualizer = "gx-texture";
	
	auto & stringType = typeDefinitionLibrary.valueTypeDefinitions["string"];
	stringType.typeName = "string";
	stringType.editor = "textbox";
	
	auto & colorType = typeDefinitionLibrary.valueTypeDefinitions["vec4.color"];
	colorType.typeName = "vec4.color";
	colorType.editor = "colorpicker";
	colorType.visualizer = "gx-texture";
}

static ShaderGraphCache g_shaderGraphCache;

class ShaderGraph
{
	ShaderGraphElem & m_cacheElem;
	
public:
	ShaderGraph(const char * name)
		: m_cacheElem(g_shaderGraphCache.findOrCreate(name))
	{
	}
	
	Shader & getShader() const
	{
		return m_cacheElem.shader;
	}
};

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	Graph_TypeDefinitionLibrary typeDefinitionLibrary;
	registerBuiltinTypes(typeDefinitionLibrary);
	
	ShaderTextLibrary shaderTextLibrary;
	
	scanShaderNodes("100-nodes", typeDefinitionLibrary, shaderTextLibrary);
	
	framework.allowHighDpi = true;
	framework.enableSound = false;
	framework.enableDepthBuffer = true;
	
	framework.windowIsResizable = true;
	
	if (!framework.init(640, 480))
		return -1;
	
	initUi();
	
	ShaderGraph_RealTimeConnection realTimeConnection;
	
	GraphEdit graphEdit(640, 480, &typeDefinitionLibrary);
	graphEdit.realTimeConnection = &realTimeConnection;
	
	graphEdit.load("test-001.xml");
	
	framework.registerResourceCache(&g_shaderGraphCache);
	g_shaderGraphCache.addShaderNodePath("100-nodes");
	
	{
		auto & shaderGraphElem = g_shaderGraphCache.findOrCreate("test-001.xml");
		ShaderGraph shaderGraph("test-001.xml");
		auto & shader = shaderGraph.getShader();
		setShader(shader);
		shader.setImmediate("time", 0.f);
		clearShader();
	}
	
	std::set<std::string> nodeDependencies;
	std::map<int, int> usedVsInputsByNode;
	int allUsedVsInputs = 0;
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		// reload shader nodes on file change
		
		bool reloadShaderNodes = false;
		
		for (auto & file : framework.changedFiles)
		{
			if (shaderTextLibrary.fileToShaderNode.count(file) != 0)
				reloadShaderNodes = true;
		}
		
		if (reloadShaderNodes)
		{
			// reset libraries
			
			typeDefinitionLibrary = Graph_TypeDefinitionLibrary();
			registerBuiltinTypes(typeDefinitionLibrary);
			
			shaderTextLibrary = ShaderTextLibrary();
			
			// reload
			
			scanShaderNodes("100-nodes", typeDefinitionLibrary, shaderTextLibrary);
		}
		
		// tick graph editor
		
		framework.getCurrentViewportSize(
			graphEdit.displaySx,
			graphEdit.displaySy);
		graphEdit.tick(framework.timeStep, false);
		
		graphEdit.tickVisualizers(framework.timeStep);
		
		// tick visualization
		
		realTimeConnection.tickVisualizers();
		
		framework.beginDraw(0, 0, 0, 0);
		{
			projectPerspective3d(90.f, .01f, 100.f);
			pushDepthTest(true, DEPTH_LESS);
			
			gxPushMatrix();
			{
				gxTranslatef(0, 0, 2.5f);
				gxRotatef(framework.time * 20.f, 1, 1, 0);
				
				Shader shader("shader");
				
				if (shader.isValid())
				{
					setShader(shader);
					shader.setImmediate("time", framework.time);
					
					setColor(colorWhite);
				}
				else
				{
					setColor(255, 0, 255);
				}
				
				fillCylinder(Vec3(), 1.f, 1.f, 100);
			}
			gxPopMatrix();
			
			popDepthTest();
			
			//
			
			projectScreen2d();
			
			if (false)
			{
				// draw per-node preview for the first output socket
				
				Shader shader("shader");
			
				if (shader.isValid())
				{
					setShader(shader);
					shader.setImmediate("time", framework.time);
					
					pushBlend(BLEND_OPAQUE);
					gxPushMatrix();
					{
						gxMultMatrixf(graphEdit.dragAndZoom.transform.m_v);
						
						for (auto & node_itr : graphEdit.graph->nodes)
						{
							auto & node = node_itr.second;
							auto * nodeData = graphEdit.tryGetNodeData(node.id);
							
							shader.setImmediate("u_nodePreview_nodeId", node.id);
							shader.setImmediate("u_nodePreview_socketIndex", 0);
						
							const float kNodeSx = 100.f; // we don't know the actual node size.. so we take an 'educated' guess..
							
							drawRect(
								nodeData->x + kNodeSx/2.f - 40.f,
								nodeData->y - 20.f,
								nodeData->x + kNodeSx/2.f + 40.f,
								nodeData->y);
						}
					}
					gxPopMatrix();
					popBlend();
					
					shader.setImmediate("u_nodePreview_nodeId", kGraphNodeIdInvalid);
					shader.setImmediate("u_nodePreview_socketIndex", -1);
				}
			}
			
			graphEdit.draw();
		}
		framework.endDraw();
		
		std::set<std::string> usedVaryings;
		scanUsedPsVaryings(*graphEdit.graph, usedVaryings);
		
		ShaderTextWriter shaderVsBuilder;
		ShaderTextWriter shaderPsBuilder;
		
		const ShaderOutput * shaderOutputs[] =
			{
				findShaderOutput('c'),
				findShaderOutput('n')
			};
		
		nodeDependencies.clear();
		usedVsInputsByNode.clear();
		generateVsShaderText(*graphEdit.graph, typeDefinitionLibrary, shaderTextLibrary, usedVaryings, true, shaderVsBuilder, nodeDependencies, &usedVsInputsByNode);
		generatePsShaderText(*graphEdit.graph, typeDefinitionLibrary, shaderTextLibrary, "color", "shader_fragColor", shaderOutputs, sizeof(shaderOutputs) / sizeof(shaderOutputs[0]), true, shaderPsBuilder, nodeDependencies);
		{
			allUsedVsInputs = 0;
			
			for (auto usedVsInput_itr : usedVsInputsByNode)
				allUsedVsInputs |= usedVsInput_itr.second;
				
			const char * shaderVs = shaderVsBuilder.ToString();
			const char * shaderPs = shaderPsBuilder.ToString();
			
			const char * currentShaderPs;
			const char * currentShaderVs;
			if (!framework.tryGetShaderSource("shader.vs", currentShaderVs))
				currentShaderVs = "";
			if (!framework.tryGetShaderSource("shader.ps", currentShaderPs))
				currentShaderPs = "";
			
			if (strcmp(shaderVs, currentShaderVs) != 0 ||
				strcmp(shaderPs, currentShaderPs) != 0)
			{
				printf("-- vs:\n%s", shaderVs);
				printf("-- ps:\n%s", shaderPs);
				
				// check if shader compiles
				
				shaderSource("shader.vs", shaderVs);
				shaderSource("shader.ps", shaderPs);
				Shader shader("shader");
				setShader(shader);
				{
				
				}
				clearShader();
			}
		}
		
		for (auto & changedFile : framework.changedFiles)
		{
			logDebug("file changed: %s", changedFile.c_str());
			
			if (shaderTextLibrary.fileToShaderNode.count(changedFile) != 0)
			{
				auto & shaderNode = shaderTextLibrary.fileToShaderNode[changedFile];
				
				logDebug("node changed: %s", shaderNode.c_str());
			}
		}
	}
	
	shutUi();
	
	framework.shutdown();
	
	return 0;
}

