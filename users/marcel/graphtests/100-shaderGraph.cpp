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

// todo : add support for true literal inputs : generate static branching, don't allow inputs to be connected

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
static std::map<std::string, std::string> s_shaderNodeToShaderText;

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
static void scanShaderNodes(const char * path, Graph_TypeDefinitionLibrary & types)
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
		
		std::string shaderText;
		
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
					
					if (parts.size() >= 4)
					{
						const auto & defaultValue = parts[3];
						
						inputSocket.defaultValue = defaultValue;
					}
					
					for (size_t i = 4; i < parts.size(); ++i)
					{
						if (false)
						{
							// todo : handle additional options here
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
							// todo : handle additional options here
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
			else if (String::StartsWithC(line.c_str(), "--"))
			{
				// -- [options..]
				
				std::vector<std::string> parts;
				splitString(line, parts);
				
				// todo : handle options
				
				for (size_t i = 1; i < parts.size(); ++i)
				{
					if (false)
					{
						// todo : handle options here
					}
					else
					{
						logError("unknown option(s): %s", parts[i].c_str());
						failed = true;
					}
				}
			}
			else
			{
				shaderText.append(line);
				shaderText.push_back('\n');
			}
		}
		
		//logDebug("shaderText: %s", shaderText.c_str());
		
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
			s_shaderNodeToShaderText[typeDefinition.typeName] = shaderText;
			
			types.typeDefinitions[typeDefinition.typeName] = typeDefinition;
		}
	}
}

static std::string generateScopeName(const GraphNodeId nodeId)
{
	char name[32];
	sprintf_s(name, sizeof(name), "node_%d_", nodeId);
	
	return name;
}

static bool generateShaderText_traverse(
	const Graph & graph,
	const GraphNode & node,
	const Graph_TypeDefinitionLibrary & typeDefinitionLibrary,
	StringBuilder & shaderText,
	std::set<GraphNodeId> & traversedNodes)
{
	auto * srcNode = &node;
	
	// generate shader text for dependencies
	
	for (auto & link_itr : graph.links)
	{
		auto & link = link_itr.second;
		
		if (link.srcNodeId == srcNode->id)
		{
			if (traversedNodes.count(link.dstNodeId) == 0)
			{
				traversedNodes.insert(link.dstNodeId);
				
				auto * dstNode = graph.tryGetNode(link.dstNodeId);
				
				generateShaderText_traverse(
					graph,
					*dstNode,
					typeDefinitionLibrary,
					shaderText,
					traversedNodes);
			}
		}
	}
	
	// generate shader text for node
	
	auto * srcType = typeDefinitionLibrary.tryGetTypeDefinition(srcNode->typeName);
	
	Assert(srcType != nullptr);
	
	// generate outputs (global scope)
	
	auto srcScopeName = generateScopeName(srcNode->id);
	
	for (auto & output : srcType->outputSockets)
	{
		shaderText.AppendFormat("%s %s%s;\n",
			output.typeName.c_str(),
			srcScopeName.c_str(),
			output.name.c_str());
	}
		
	shaderText.Append("{\n");
	{
		// generate inputs
		
		for (auto & input : srcType->inputSockets)
		{
			if (input.typeName == "string")
				continue;
				
			const std::string * inputValue = &input.defaultValue;
			
			auto inputValue_itr = srcNode->inputValues.find(input.name);
			if (inputValue_itr != srcNode->inputValues.end())
				inputValue = &inputValue_itr->second;
				
			shaderText.AppendFormat("%s %s = %s(%s);\n",
				input.typeName.c_str(),
				input.name.c_str(),
				input.typeName.c_str(),
				inputValue->empty() ? "0" : inputValue->c_str());
		}
		
		// assign inputs
		
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
			}
		}
		
		// generate outputs (local scope)
		
		auto srcScopeName = generateScopeName(srcNode->id);
		
		for (auto & output : srcType->outputSockets)
		{
			shaderText.AppendFormat("%s %s;\n",
				output.typeName.c_str(),
				output.name.c_str());
		}
		
		// generate flags
		
		shaderText.AppendFormat("bool isPassthrough = %s;\n",
			node.isPassthrough ? "true" : "false");
			
		// emit node shader text
		
		auto text_itr = s_shaderNodeToShaderText.find(srcNode->typeName);
	
		Assert(text_itr != s_shaderNodeToShaderText.end());
	
		const auto & text = text_itr->second;
		
		shaderText.Append(text.c_str());
		
		// assign outputs
		
		if (srcType->typeName == "immediate.float")
		{
			// uniform -> global scope output
			
			auto name_itr = node.inputValues.find("name");
			
			if (name_itr != node.inputValues.end())
			{
				auto & name = name_itr->second;
				
				if (srcNode->isPassthrough)
				{
					shaderText.AppendFormat("%s%s = 0.0;\n",
						srcScopeName.c_str(),
						"result");
				}
				else
				{
					shaderText.AppendFormat("%s%s = %s;\n",
						srcScopeName.c_str(),
						"result",
						name.c_str());
				}
			}
		}
		else if (srcType->typeName == "vs.input")
		{
			// buffer -> global scope output
			
			auto name_itr = node.inputValues.find("name");
			
			if (name_itr != node.inputValues.end())
			{
				auto & name = name_itr->second;
				
				if (srcNode->isPassthrough)
				{
					shaderText.AppendFormat("%s%s = vec4(0.0);\n",
						srcScopeName.c_str(),
						"result");
				}
				else
				{
					std::string loadCode;
					
					if (name == "position")
						loadCode = "unpackPosition()";
					else if (name == "normal")
						loadCode = "unpackNormal()";
					else if (name == "color")
						loadCode = "unpackColor()";
					else if (name == "texcoord1")
						loadCode = "vec4(unpackTexcoord(0), 0.0, 0.0)";
					else if (name == "texcoord2")
						loadCode = "vec4(unpackTexcoord(1), 0.0, 0.0)";
					
					if (!loadCode.empty())
					{
						shaderText.AppendFormat("%s%s = %s;\n",
							srcScopeName.c_str(),
							"result",
							loadCode.c_str());
						
						const auto viewSpace_itr = node.inputValues.find("viewSpace");
				
						const bool viewSpace =
							viewSpace_itr == node.inputValues.end() ? false :
							Parse::Bool(viewSpace_itr->second);
							
						if (viewSpace)
						{
							shaderText.AppendFormat("%s%s = objectToView(%s%s);\n",
								srcScopeName.c_str(),
								"result",
								srcScopeName.c_str(),
								"result");
						}
					}
				}
			}
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
			
			auto name_itr = node.inputValues.find("name");
			
			if (name_itr != node.inputValues.end())
			{
				auto & name = name_itr->second;
				
				if (srcNode->isPassthrough)
				{
					shaderText.AppendFormat("%s%s = vec4(0.0);\n",
						srcScopeName.c_str(),
						"result");
				}
				else
				{
					shaderText.AppendFormat("%s%s = v_%s;\n",
						srcScopeName.c_str(),
						"result",
						name.c_str());
						
					const auto normalize_itr = node.inputValues.find("_normalize");
			
					const bool normalize =
						normalize_itr == node.inputValues.end() ? false :
						Parse::Bool(normalize_itr->second);
						
					if (normalize)
					{
						shaderText.AppendFormat("%s%s = normalize(%s%s);\n",
							srcScopeName.c_str(),
							"result",
							srcScopeName.c_str(),
							"result");
					}
				}
			}
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
	shaderText.Append("}\n");
	
	return true;
}

static bool generateVsShaderText(
	const Graph & graph,
	const Graph_TypeDefinitionLibrary & typeDefinitionLibrary,
	const std::set<std::string> & usedVaryings,
	StringBuilder & shaderText)
{
	bool result = true;
	
	// generate header
	
	shaderText.Append("include engine/ShaderVS.txt\n");
	
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
		
		if (usedVaryings.count(name) != 0)
			continue;
		
		if (generatedVsOutputs.count(name) != 0)
			continue;
		
		generatedVsOutputs.insert(name);
		
		shaderText.AppendFormat("vec4 v_%s;\n",
			name.c_str());
	}
	
	// generate vertex outputs (used -> shader_out)
	
	for (auto & varying : usedVaryings)
	{
		if (generatedVsOutputs.count(varying) != 0)
			continue;
		
		generatedVsOutputs.insert(varying);
		
		shaderText.AppendFormat("shader_out vec4 v_%s;\n",
			varying.c_str());
	}
	
	// generate main function
	
	shaderText.Append("void main() {\n"); // main : begin
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
					shaderText,
					traversedNodes);
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
					shaderText,
					traversedNodes);
			}
		}
		
		// generated code for vertex outputs (referenced but not generated by graph)
		
		for (auto & varying : usedVaryings)
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
				logError("unknown varying: %s", varying.c_str());
				result = false;
			#endif
			}
		}

		// emit position output, if not output explicitly by the shader
		
		if (hasPositionOutput == false)
		{
			shaderText.Append("gl_Position = objectToProjection(unpackPosition());\n");
		}
	}
	shaderText.Append("}\n"); // main : end

	return result;
}

static bool generatePsShaderText(
	const Graph & graph,
	const Graph_TypeDefinitionLibrary & typeDefinitionLibrary,
	const char * forOutput,
	const char * toInput,
	StringBuilder & shaderText)
{
	bool result = true;
	
	// find output
	
	const GraphNode * outputNode = nullptr;
	
	for (auto & node_itr : graph.nodes)
	{
		auto & node = node_itr.second;
		
		if (node.isPassthrough)
			continue;
				
		if (node.typeName == "ps.output")
		{
			auto name_itr = node.inputValues.find("name");
			
			if (name_itr != node.inputValues.end() && name_itr->second == forOutput)
				outputNode = &node;
		}
	}
	
	if (outputNode == nullptr)
	{
		result = false;
	}
	else
	{
		// generate header
	
		shaderText.Append("include engine/ShaderPS.txt\n");
	
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
		
		// generate main function
	
		shaderText.Append("void main() {\n"); // main : begin
		{
			std::set<GraphNodeId> traversedNodes;
			
			result &= generateShaderText_traverse(
				graph,
				*outputNode,
				typeDefinitionLibrary,
				shaderText,
				traversedNodes);

			shaderText.AppendFormat("%s = %s%s;\n",
				toInput,
				generateScopeName(outputNode->id).c_str(),
				"result");
		}
		shaderText.Append("}\n"); // main : end
	}
	
	return result;
}

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	Graph_TypeDefinitionLibrary typeDefinitionLibrary;
	
	{
		auto & boolType = typeDefinitionLibrary.valueTypeDefinitions["bool"];
		boolType.typeName = "bool";
		boolType.editor = "checkbox";
		
		auto & floatType = typeDefinitionLibrary.valueTypeDefinitions["float"];
		floatType.typeName = "float";
		floatType.editor = "textbox_float";
		
		auto & vec4Type = typeDefinitionLibrary.valueTypeDefinitions["vec4"];
		vec4Type.typeName = "vec4";
		vec4Type.editor = "textbox_float";
		
		auto & stringType = typeDefinitionLibrary.valueTypeDefinitions["string"];
		stringType.typeName = "string";
		stringType.editor = "textbox";
	}
	
	scanShaderNodes("100-nodes", typeDefinitionLibrary);
	
	framework.allowHighDpi = true;
	framework.enableSound = false;
	framework.enableDepthBuffer = true;
	
	if (!framework.init(640, 480))
		return -1;
	
	initUi();
	
	GraphEdit graphEdit(640, 480, &typeDefinitionLibrary);
	
	graphEdit.load("test-001.xml");
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		graphEdit.tick(framework.timeStep, false);
		
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
				//fillCube(Vec3(), Vec3(1.f));
			}
			gxPopMatrix();
			
			popDepthTest();
			
			//
			
			projectScreen2d();
			
			graphEdit.draw();
		}
		framework.endDraw();
		
		std::set<std::string> usedVaryings;
		for (auto & node_itr : graphEdit.graph->nodes)
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
		
		StringBuilder shaderVsBuilder;
		StringBuilder shaderPsBuilder;
		
		//if (generateVsShaderText(*graphEdit.graph, typeDefinitionLibrary, usedVaryings, shaderVsBuilder) &&
		//	generatePsShaderText(*graphEdit.graph, typeDefinitionLibrary, "color", "shader_fragColor", shaderPsBuilder))
		generateVsShaderText(*graphEdit.graph, typeDefinitionLibrary, usedVaryings, shaderVsBuilder);
		generatePsShaderText(*graphEdit.graph, typeDefinitionLibrary, "color", "shader_fragColor", shaderPsBuilder);
		{
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
	}
	
	shutUi();
	
	framework.shutdown();
	
	return 0;
}

