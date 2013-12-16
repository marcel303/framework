#pragma once

#include <vector>
#include "CommandPacket.h"
#include "libgg_forward.h"
#include "Swatch.h"

class kdImageDescription
{
public:
	kdImageDescription();
	
	void Read(Stream* stream);
	
	void Validate();
	
	int sx;
	int sy;
	int layerCount;
	int activeLayer;
	std::vector<std::string> dataLayerFile;
	std::vector<bool> dataLayerVisibility;
	std::vector<float> dataLayerOpacity;
	std::vector<int> layerOrder;
	struct
	{
		//std::string fileName
		int position;
	} commandStream;
	struct
	{
		//std::string fileName
		int position;
	} dataStream;
	bool hasLastColorCommand;
	CommandPacket lastColorCommand;
	bool hasLastToolSelectCommand;
	CommandPacket lastToolSelectCommand;
	SwatchMgr swatches;
};
