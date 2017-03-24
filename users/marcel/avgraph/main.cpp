#include "framework.h"
#include "graph.h"
#include "tinyxml2.h"

using namespace tinyxml2;

#define GFX_SX 1024
#define GFX_SY 768

int main(int argc, char * argv[])
{
	framework.waitForEvents = true;
	
	if (framework.init(0, nullptr, GFX_SX, GFX_SY))
	{
		GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary = new GraphEdit_TypeDefinitionLibrary();
		
		{
			XMLDocument * document = new XMLDocument();
			
			if (document->LoadFile("types.xml") == XML_SUCCESS)
			{
				const XMLElement * xmlLibrary = document->FirstChildElement("library");
				
				if (xmlLibrary != nullptr)
				{
					typeDefinitionLibrary->loadXml(xmlLibrary);
				}
			}
			
			delete document;
			document = nullptr;
		}
		
		{
			const std::string typeName = "intLiteral";
			
			GraphEdit_TypeDefinition definition;
			definition.typeName = typeName;
			definition.outputSockets.resize(1);
			definition.outputSockets[0].typeName = "intLiteral";
			GraphEdit_Editor editor;
			editor.typeName = "int";
			editor.outputSocketIndex = 0;
			definition.editors.push_back(editor);
			definition.createUi();
			
			typeDefinitionLibrary->typeDefinitions[definition.typeName] = definition;
		}
		
		for (int i = 0; i < 5; ++i)
		{
			char typeName[256];
			sprintf(typeName, "effect_%03d", i);
			
			GraphEdit_TypeDefinition definition;
			definition.typeName = typeName;
			definition.inputSockets.resize(random(1, 8));
			for (auto & s : definition.inputSockets)
			{
				const int t = (rand() % 4);
				s.typeName = t == 0 ? "int" : t == 1 ? "intLiteral" : t == 2 ? "float" : "floatLiteral";
			}
			definition.outputSockets.resize(1);
			for (auto & s : definition.outputSockets)
			{
				const int t = (rand() % 4);
				s.typeName = t == 0 ? "int" : t == 1 ? "intLiteral" : t == 2 ? "float" : "floatLiteral";
			}
			definition.createUi();
			
			typeDefinitionLibrary->typeDefinitions[definition.typeName] = definition;
		}
		
		//
		
		GraphEdit * graphEdit = new GraphEdit();
		
		graphEdit->typeDefinitionLibrary = typeDefinitionLibrary;
		
		while (!framework.quitRequested)
		{
			framework.process();
			
			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;
			
			const float dt = framework.timeStep;
			
			graphEdit->tick(dt);
			
			if (keyboard.wentDown(SDLK_s))
			{
				XMLPrinter xmlGraph;
				
				xmlGraph.OpenElement("graph");
				{
					graphEdit->graph->saveXml(xmlGraph);
				}
				xmlGraph.CloseElement();
				
				const char * xml = xmlGraph.CStr();
				
				logDebug(xml);
				
				{
					delete graphEdit->graph;
					graphEdit->graph = nullptr;
					
					//
					
					graphEdit->graph = new Graph();
					
					//
					
					XMLDocument document;
					document.Parse(xml);
					const XMLElement * xmlGraph = document.FirstChildElement("graph");
					if (xmlGraph != nullptr)
						graphEdit->graph->loadXml(xmlGraph);
				}
			}
			
			framework.beginDraw(31, 31, 31, 255);
			{
				graphEdit->draw();
			}
			framework.endDraw();
		}
		
		delete graphEdit;
		graphEdit = nullptr;
		
		framework.shutdown();
	}

	return 0;
}
