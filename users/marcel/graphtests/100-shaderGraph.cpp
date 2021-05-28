#include "framework.h"
#include "graph.h"
#include "graphEdit.h"
#include "internal.h"
#include "Path.h"
#include "ui.h"
#include "StringEx.h"
#include "TextIO.h"
#include <map>
#include <string>

// todo : add support for true literal inputs : generate static branching, don't allow inputs to be connected

static std::map<std::string, std::string> s_shaderNodeToShaderText;

static void scanShaderNodes(const char * path, Graph_TypeDefinitionLibrary & types)
{
	auto files = listFiles(path, true);
	
	for (auto & file : files)
	{
		std::vector<std::string> lines;
		TextIO::LineEndings lineEndings;
		if (TextIO::load(file.c_str(), lines, lineEndings) == false)
			continue;
		
		bool failed = false;
		
		Graph_TypeDefinition typeDefinition;
		typeDefinition.typeName = String::Replace(Path::GetBaseName(file), '-', '.');
		
		std::string shaderText;
		
		int inputIndex = 0;
		int outputIndex = 0;
		
		for (auto & line : lines)
		{
			if (String::StartsWithC(line.c_str(), "in "))
			{
				std::vector<std::string> parts;
				splitString(line, parts);
				
				if (parts.size() >= 3)
				{
					const auto & typeName = parts[1];
					const auto & name = parts[2];
					
					Graph_TypeDefinition::InputSocket inputSocket;
					inputSocket.typeName = typeName;
					inputSocket.name = name;
					inputSocket.displayName = name[0] == '_' ? name.c_str() + 1 : name;
					inputSocket.index = inputIndex++;
					
					if (parts.size() >= 4)
					{
						const auto & defaultValue = parts[3];
						
						inputSocket.defaultValue = defaultValue;
					}
					
					typeDefinition.inputSockets.push_back(inputSocket);
				}
			}
			else if (String::StartsWithC(line.c_str(), "out "))
			{
				std::vector<std::string> parts;
				splitString(line, parts);
				
				if (parts.size() >= 3)
				{
					const auto & typeName = parts[1];
					const auto & name = parts[2];
					
					Graph_TypeDefinition::OutputSocket outputSocket;
					outputSocket.typeName = typeName;
					outputSocket.name = name;
					outputSocket.displayName = name[0] == '_' ? name.c_str() + 1 : name;
					outputSocket.index = outputIndex++;
					
					typeDefinition.outputSockets.push_back(outputSocket);
				}
			}
			else if (String::StartsWithC(line.c_str(), "--"))
			{
				continue;
			}
			else
			{
				shaderText.append(line);
				shaderText.push_back('\n');
			}
		}
		
	// todo : check type definition is not a duplicate
		logDebug("shaderText: %s", shaderText.c_str());
		
		s_shaderNodeToShaderText[typeDefinition.typeName] = shaderText;
		
		types.typeDefinitions[typeDefinition.typeName] = typeDefinition;
	}
}

static std::string generateScopeName(const GraphNodeId nodeId)
{
	char name[32];
	sprintf_s(name, sizeof(name), "node_%d_", nodeId);
	
	return name;
}

static bool generateShaderText_traverse(Graph & graph, const GraphNode & node, const Graph_TypeDefinitionLibrary & tdl, std::string & shaderText, std::set<GraphNodeId> & traversedNodes)
{
	auto * srcNode = &node;
	
	// generate shader text for dependencies
	
	for (auto & link_itr : graph.links)
	{
		auto & link = link_itr.second;
		
		if (link.srcNodeId == srcNode->id && traversedNodes.count(link.dstNodeId) == 0)
		{
			traversedNodes.insert(link.dstNodeId);
			
			const auto * dstNode = graph.tryGetNode(link.dstNodeId);
			
			generateShaderText_traverse(graph, *dstNode, tdl, shaderText, traversedNodes);
		}
	}
	
	// generate shader text for node
	
	auto * srcType = tdl.tryGetTypeDefinition(srcNode->typeName);
	
	Assert(srcType != nullptr);
		
	// generate outputs (global scope)
	
	auto srcScopeName = generateScopeName(srcNode->id);
	
	for (auto & output : srcType->outputSockets)
	{
		char line[1024];
		sprintf_s(line, sizeof(line), "%s %s%s;\n",
			output.typeName.c_str(),
			srcScopeName.c_str(),
			output.name.c_str());
		
		shaderText += line;
	}
		
	shaderText += "{\n";
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
				
			char line[1024];
			sprintf_s(line, sizeof(line), "%s %s = %s(%s);\n",
				input.typeName.c_str(),
				input.name.c_str(),
				input.typeName.c_str(),
				inputValue->empty() ? "0" : inputValue->c_str());
			
			shaderText += line;
		}
		
		// assign inputs
		
		for (auto & link_itr : graph.links)
		{
			auto & link = link_itr.second;
			
			if (link.srcNodeId == srcNode->id)
			{
				auto * dstNode = graph.tryGetNode(link.dstNodeId);
				auto * dstType = tdl.tryGetTypeDefinition(dstNode->typeName);
				
				auto & dstSocket = dstType->outputSockets[link.dstNodeSocketIndex];
				auto & srcSocket = srcType->inputSockets[link.srcNodeSocketIndex];
				
				auto dstScopeName = generateScopeName(dstNode->id);
				
				char line[1024];
				sprintf_s(line, sizeof(line), "%s = %s%s;\n",
					srcSocket.name.c_str(),
					dstScopeName.c_str(),
					dstSocket.name.c_str());
				
				shaderText += line;
			}
		}
		
		// generate outputs (local scope)
		
		auto srcScopeName = generateScopeName(srcNode->id);
		
		for (auto & output : srcType->outputSockets)
		{
			char line[1024];
			sprintf_s(line, sizeof(line), "%s %s;\n",
				output.typeName.c_str(),
				output.name.c_str());
			
			shaderText += line;
		}
			
		// emit node shader text
		
		auto text_itr = s_shaderNodeToShaderText.find(srcNode->typeName);
	
		Assert(text_itr != s_shaderNodeToShaderText.end());
	
		shaderText += text_itr->second;
		
		// assign outputs
		
		if (srcType->typeName == "io.input.float")
		{
			// uniform -> global scope output
			
			auto name_itr = node.inputValues.find("name");
			
			if (name_itr != node.inputValues.end())
			{
				auto & name = name_itr->second;
				char line[1024];
				sprintf_s(line, sizeof(line), "%s%s = %s;\n",
					srcScopeName.c_str(),
					"result",
					name.c_str());
				
				shaderText += line;
			}
		}
		else
		{
			// local variable -> global scope output
			
			for (auto & output : srcType->outputSockets)
			{
				char line[1024];
				sprintf_s(line, sizeof(line), "%s%s = %s;\n",
					srcScopeName.c_str(),
					output.name.c_str(),
					output.name.c_str());
				
				shaderText += line;
			}
		}
	}
	shaderText += "}\n";
	
	return true;
}

static bool generateShaderText(Graph & graph, const Graph_TypeDefinitionLibrary & tdl, const char * forOutput, const char * toInput, std::string & shaderText)
{
	bool result = true;
	
	// find output
	
	const GraphNode * outputNode = nullptr;
	
	for (auto & node_itr : graph.nodes)
	{
		auto & node = node_itr.second;
		
		if (node.typeName == "io.output")
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
		// generate uniforms
		
		for (auto & node_itr : graph.nodes)
		{
			auto & node = node_itr.second;
			
			if (node.typeName == "io.input.float")
			{
				auto name_itr = node.inputValues.find("name");
				
				if (name_itr != node.inputValues.end())
				{
					auto & name = name_itr->second;
				
					char line[1024];
					sprintf_s(line, sizeof(line), "uniform float %s;\n",
						name.c_str());
					
					shaderText += line;
				}
			}
		}
		
		std::set<GraphNodeId> traversedNodes;
		
		if (generateShaderText_traverse(graph, *outputNode, tdl, shaderText, traversedNodes) == false)
		{
			result = false;
		}
		else
		{
			char line[1024];
			sprintf_s(line, sizeof(line), "%s = %s%s;\n",
				toInput,
				generateScopeName(outputNode->id).c_str(),
				"result");
			
			shaderText += line;
		}
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
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		graphEdit.tick(framework.timeStep, false);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			projectPerspective3d(90.f, .01f, 100.f);
			
			gxPushMatrix();
			{
				gxTranslatef(0, 0, 50);
				gxRotatef(framework.time * 20.f, 1, 1, 0);
				
			#if true
				Shader shader("shader");
				if (shader.isValid())
				{
					setShader(shader);
					{
						shader.setImmediate("time", framework.time);
						
						fillCube(Vec3(), Vec3(20.f));
					}
					clearShader();
				}
			#else
				setColor(colorWhite);
				fillCube(Vec3(), Vec3(20.f));
			#endif
			}
			gxPopMatrix();
			
			//
			
			projectScreen2d();
			
			graphEdit.draw();
		}
		framework.endDraw();
		
		std::string shaderPs;
		shaderPs += "include engine/ShaderPS.txt\n";
		shaderPs += "void main() {\n";
		
		if (generateShaderText(*graphEdit.graph, typeDefinitionLibrary, "color", "shader_fragColor", shaderPs))
		{
			shaderPs += "}\n";
			
			static std::string oldText;
			
			if (shaderPs != oldText)
			{
				logDebug("%s", shaderPs.c_str());
				
				oldText = shaderPs;
				
				// check if shader compiles
				
				const std::string shaderVs = R"SHADER(
					include engine/ShaderVS.txt
					//shader_out vec4 v_color;
					//shader_out vec3 v_normal;
					void main()
					{
						vec4 position = unpackPosition();
						
						//v_color = unpackColor(); // todo : generate these from shader graph as well
						//v_normal = unpackNormal().xyz;
						
						gl_Position = objectToProjection(position);
					}
				)SHADER";
			
				shaderSource("shader.vs", shaderVs.c_str());
				shaderSource("shader.ps", shaderPs.c_str());
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

