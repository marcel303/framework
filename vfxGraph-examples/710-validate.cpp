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

#include "framework.h"
#include "Path.h"
#include "tinyxml2.h"
#include "tinyxml2_helpers.h"
#include "vfxGraph.h"
#include "vfxNodeBase.h"

using namespace tinyxml2;

static void checkVfxGraphIntegrity(const char * filename, const GraphEdit_TypeDefinitionLibrary & tdl)
{
	XMLDocument d;
	
	if (d.LoadFile(filename) != XML_SUCCESS)
	{
		logError("%s: failed to load XML document", filename);
	}
	else
	{
		auto xmlGraph = d.FirstChildElement("graph");
		
		if (xmlGraph == nullptr)
		{
			logError("%s: failed to find graph element", filename);
		}
		else
		{
			std::map<GraphNodeId, std::string> nodes;
			
			for (auto xmlNode = xmlGraph->FirstChildElement("node"); xmlNode != nullptr; xmlNode = xmlNode->NextSiblingElement("node"))
			{
				const GraphNodeId nodeId = intAttrib(xmlNode, "id", kGraphNodeIdInvalid);
				const char * typeName = stringAttrib(xmlNode, "typeName", nullptr);
				
				if (nodeId == kGraphNodeIdInvalid)
				{
					logError("%s: found node without valid node id", filename);
				}
				else
				{
					auto i = nodes.find(nodeId);
					
					if (i != nodes.end())
					{
						logError("%s: found node with duplicate node id. nodeId=%d", filename, nodeId);
					}
					else
					{
						nodes.insert(std::make_pair(nodeId, typeName ? typeName : ""));
					}
				}
				
				if (typeName == nullptr)
				{
					logError("%s: found node without valid type name. nodeId=%d", filename, nodeId);
				}
				else
				{
					auto td = tdl.tryGetTypeDefinition(typeName);
					
					if (td == nullptr)
					{
						logError("%s: found node with type name that doesn't exist in type definition library. nodeId=%d, typeName=%s", filename, nodeId, typeName);
					}
				}
			}
			
			//
			
			std::set<GraphLinkId> linkIds;
			
			for (auto xmlLink = xmlGraph->FirstChildElement("link"); xmlLink != nullptr; xmlLink = xmlLink->NextSiblingElement("link"))
			{
				const int linkId = intAttrib(xmlLink, "id", kGraphLinkIdInvalid);
				const bool isDynamic = boolAttrib(xmlLink, "dynamic", false);
				
				if (linkId == kGraphLinkIdInvalid)
				{
					logError("%s: found link without valid link id", filename);
				}
				else
				{
					auto i = linkIds.find(linkId);
					
					if (i != linkIds.end())
					{
						logError("%s: found link with duplicate link id. linkId=%d", filename, linkId);
					}
					else
					{
						linkIds.insert(linkId);
					}
				}
				
				const int srcNodeId = intAttrib(xmlLink, "srcNodeId", kGraphNodeIdInvalid);
				const char * srcNodeSocketName = stringAttrib(xmlLink, "srcNodeSocketName", nullptr);
				const int dstNodeId = intAttrib(xmlLink, "dstNodeId", kGraphNodeIdInvalid);
				const char * dstNodeSocketName = stringAttrib(xmlLink, "dstNodeSocketName", nullptr);
				
				if (srcNodeId == kGraphNodeIdInvalid)
					logError("%s: found link with invalid src node id", filename);
				if (srcNodeSocketName == nullptr)
					logError("%s: found link with invalid src socket name", filename);
				if (dstNodeId == kGraphNodeIdInvalid)
					logError("%s: found link with invalid dst node id", filename);
				if (dstNodeSocketName == nullptr)
					logError("%s: found link with invalid dst socket name", filename);
				
				if (srcNodeId != kGraphNodeIdInvalid)
				{
					auto i = nodes.find(srcNodeId);
					
					if (i == nodes.end())
						logError("%s: found link with non-existing src node. linkId=%d, srcNodeId=%d", filename, linkId, srcNodeId);
					else
					{
						auto td = tdl.tryGetTypeDefinition(i->second);
						
						if (td != nullptr)
						{
							if (srcNodeSocketName != nullptr)
							{
								bool found = false;
								
								for (auto & input : td->inputSockets)
									if (input.name == srcNodeSocketName)
										found = true;
								
								if (found == false && isDynamic == false)
								{
									logError("%s: found link with non-existing src socket. linkId=%d, srcNodeId=%d, srcNodeSocketName=%s", filename, linkId, srcNodeId, srcNodeSocketName);
								}
							}
						}
					}
				}
				
				if (dstNodeId != kGraphNodeIdInvalid)
				{
					auto i = nodes.find(dstNodeId);
					
					if (i == nodes.end())
						logError("%s: found link with non-existing dst node. linkId=%d, dstNodeId=%d", filename, linkId, dstNodeId);
					else
					{
						auto td = tdl.tryGetTypeDefinition(i->second);
						
						if (td != nullptr)
						{
							if (dstNodeSocketName != nullptr)
							{
								bool found = false;
								
								for (auto & output : td->outputSockets)
									if (output.name == dstNodeSocketName)
										found = true;
								
								if (found == false && isDynamic == false)
								{
									logError("%s: found link with non-existing dst socket. linkId=%d, dstNodeId=%d, dstNodeSocketName=%s", filename, linkId, dstNodeId, dstNodeSocketName);
								}
							}
						}
					}
				}
			}
		}
	}
}

static void checkVfxGraphs(const GraphEdit_TypeDefinitionLibrary & tdl)
{
	std::vector<std::string> filenames = listFiles(".", true);
	
	for (auto & filename : filenames)
	{
		if (Path::GetExtension(filename, true) != "xml")
			continue;
		
		if (filename == "audioKey.xml" ||
			filename == "audioKey2.xml" ||
			filename == "ccl.xml" ||
			filename == "combTest5.xml" ||
			filename == "combTest5p.xml" ||
			filename == "mlworkshopA.xml" ||
			filename == "mlworkshopAp.xml" ||
			filename == "types.xml")
			continue;
		
		checkVfxGraphIntegrity(filename.c_str(), tdl);
	}
}

int main(int argc, char * argv[])
{
	GraphEdit_TypeDefinitionLibrary * typeDefinitionLibrary = new GraphEdit_TypeDefinitionLibrary();
	
	createVfxTypeDefinitionLibrary(*typeDefinitionLibrary, g_vfxEnumTypeRegistrationList, g_vfxNodeTypeRegistrationList);

	checkVfxGraphs(*typeDefinitionLibrary);
	
	return 0;
}
