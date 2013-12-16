#include <map>
#include "ExceptionLogger.h"
#include "ImageDescription.h"
#include "MemoryStream.h"
#include "StreamReader.h"
#include "StringEx.h"
#include "XmlNode.h"
#include "XmlReader.h"

kdImageDescription::kdImageDescription()
{
	sx = 0;
	sy = 0;
	layerCount = 0;
	activeLayer = 0;
	commandStream.position = 0;
	dataStream.position = 0;
	hasLastColorCommand = false;
	hasLastToolSelectCommand = false;
}

void kdImageDescription::Read(Stream* stream)
{
	XmlReader reader;

	reader.Load(stream);

	// parse XML file

	std::map<int, std::string> layerFileMap;
	std::map<int, bool> layerVisibilityMap;
	std::map<int, float> layerOpacityMap;
	std::map<int, int> layerOrderMap;

	for (size_t i = 0; i < reader.RootNode_get()->mChildNodes.size(); ++i)
	{
		XmlNode* node = reader.RootNode_get()->mChildNodes[i];

		if (node->mName == "format")
		{
			sx = node->GetAttribute_Int32("sx", 0);
			sy = node->GetAttribute_Int32("sy", 0);
		}
		else if (node->mName == "streams")
		{
			for (size_t j = 0; j < node->mChildNodes.size(); ++j)
			{
				XmlNode* streamNode = node->mChildNodes[j];
				
				if (streamNode->mName == "commandStream")
				{
					commandStream.position = streamNode->GetAttribute_Int32("position", 0);
				}
				else if (streamNode->mName == "dataStream")
				{
					dataStream.position = streamNode->GetAttribute_Int32("position", 0);
				}
				else
				{
					LOG_ERR("unknown XML element: %s", streamNode->mName.c_str());
					
					throw ExceptionVA("unknown data stream. load aborted. continue might cause damage");
				}
			}
		}
		else if (node->mName == "layers")
		{
			layerCount = node->GetAttribute_Int32("count", 0);
			activeLayer = node->GetAttribute_Int32("active", 0);

			for (size_t j = 0; j < node->mChildNodes.size(); ++j)
			{
				XmlNode* layerNode = node->mChildNodes[j];

				if (layerNode->mName == "layer")
				{
					const int index = layerNode->GetAttribute_Int32("index", -1);
					const int order = layerNode->GetAttribute_Int32("order", -1);
					const bool visibility = layerNode->GetAttribute_Int32("visibility", 1) != 0;
					const float opacity = layerNode->GetAttribute_Int32("opacity", 100) / 100.0f;

					const std::string file = layerNode->GetAttribute("file", "");

					if (index == -1)
						throw ExceptionVA("layer index not set");
					if (order == -1)
						throw ExceptionVA("layer order not set");

					if (file == String::Empty)
						throw ExceptionVA("layer file not empty");

					layerOpacityMap[index] = opacity;
					layerVisibilityMap[index] = visibility;
					layerFileMap[index] = file;
					layerOrderMap[order] = index;
				}
				else
				{
					LOG_WRN("unknown XML element: %s", node->mName.c_str());
				}
			}
		}
		else if (node->mName == "restore")
		{
			for (size_t j = 0; j < node->mChildNodes.size(); ++j)
			{
				XmlNode* restoreNode = node->mChildNodes[j];
				
				if (restoreNode->mName == "color")
				{
					MemoryStream packetStream;
					if (restoreNode->GetAttribute_Bytes("data", &packetStream))
					{
						try
						{
							packetStream.Seek(0, SeekMode_Begin);
							StreamReader streamReader(&packetStream, false);
							CommandPacket packet;
							packet.Read(streamReader);
							lastColorCommand = packet;
							hasLastColorCommand = true;
						}
						catch (std::exception& e)
						{
							ExceptionLogger::Log(e);
						}
					}
				}
				else if (restoreNode->mName == "tool")
				{
					MemoryStream packetStream;
					if (restoreNode->GetAttribute_Bytes("data", &packetStream))
					{
						try
						{
							packetStream.Seek(0, SeekMode_Begin);
							StreamReader streamReader(&packetStream, false);
							CommandPacket packet;
							packet.Read(streamReader);
							lastToolSelectCommand = packet;
							hasLastToolSelectCommand = true;
						}
						catch (std::exception& e)
						{
							ExceptionLogger::Log(e);
						}
					}
				}
				else if (restoreNode->mName == "swatches")
				{
					MemoryStream swatchesStream;
					if (restoreNode->GetAttribute_Bytes("data", &swatchesStream))
					{
						try
						{
							swatchesStream.Seek(0, SeekMode_Begin);
							swatches.Read(&swatchesStream);
						}
						catch (std::exception& e)
						{
							ExceptionLogger::Log(e);
						}
					}
				}
				else
				{
					LOG_WRN("unknown XML element: %s", node->mName.c_str());
				}
			}
		}
		else
		{
			LOG_WRN("unknown XML element: %s", node->mName.c_str());
		}
	}
	
	for (int i = 0; i < layerCount; ++i)
	{
		if (layerFileMap.count(i) == 0)
			throw ExceptionVA("missing layer file %d", i);
		
		dataLayerFile.push_back(layerFileMap[i]);
	}
	
	for (int i = 0; i < layerCount; ++i)
	{
		if (layerVisibilityMap.count(i) == 0)
			dataLayerVisibility.push_back(true);
		else
			dataLayerVisibility.push_back(layerVisibilityMap[i]);
	}
	
	for (int i = 0; i < layerCount; ++i)
	{
		if (layerOpacityMap.count(i) == 0)
			dataLayerOpacity.push_back(1.0f);
		else
			dataLayerOpacity.push_back(layerOpacityMap[i]);
	}
	
	for (int i = 0; i < layerCount; ++i)
	{
		if (layerOrderMap.count(i) == 0)
			throw ExceptionVA("missing layer order %d", i);

		layerOrder.push_back(layerOrderMap[i]);
	}
}

void kdImageDescription::Validate()
{
	if (sx == 0 || sy == 0)
		throw ExceptionVA("invalid size: %dx%d", sx, sy);
	if (layerCount != 3)
		throw ExceptionVA("layer count must be 3");
}
