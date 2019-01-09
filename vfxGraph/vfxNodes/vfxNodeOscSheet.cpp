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

#include "Csv.h"
#include "Parse.h"
#include "StringEx.h"
#include "vfxNodeOscSheet.h"

extern void splitString(const std::string & str, std::vector<std::string> & result, char c);

VFX_NODE_TYPE(VfxNodeOscSheet)
{
	typeName = "osc.sheet";
	
	in("oscEndpoint", "string");
	in("oscPrefix", "string");
	in("groupPrefix", "bool", "1");
	in("oscSheet", "string");
}

VfxNodeOscSheet::VfxNodeOscSheet()
	: VfxNodeBase()
	, currentOscPrefix()
	, currentOscSheet()
	, currentGroupPrefix(false)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_OscEndpoint, kVfxPlugType_String);
	addInput(kInput_OscPrefix, kVfxPlugType_String);
	addInput(kInput_GroupPrefix, kVfxPlugType_Bool);
	addInput(kInput_OscSheet, kVfxPlugType_String);
}

void VfxNodeOscSheet::updateOscSheet()
{
	const char * oscPrefix =
		isPassthrough
		? ""
		: getInputString(kInput_OscPrefix, "");
	
	const bool groupPrefix = getInputBool(kInput_GroupPrefix, true);
	
	const char * oscSheet = getInputString(kInput_OscSheet, "");
	
	if (framework.fileHasChanged(oscSheet))
	{
		currentOscSheet.clear();
	}
	
	if (oscPrefix == currentOscPrefix && oscSheet == currentOscSheet && groupPrefix == currentGroupPrefix)
	{
		return;
	}
	
	currentOscPrefix = oscPrefix;
	currentOscSheet = oscSheet;
	currentGroupPrefix = groupPrefix;
	
	inputInfos.clear();
	
	if (oscPrefix[0] == 0 || oscSheet[0] == 0)
	{
		setDynamicInputs(nullptr, 0);
	}
	else
	{
		// parse OSC sheet. detect which parameters belong to this sound object. create dynamic inputs
		
		std::vector<VfxNodeBase::DynamicInput> inputs;
		
		ReadOnlyCsvDocument csvDocument;
		
		if (csvDocument.load(oscSheet, true, '\t'))
		{
			const int oscAddress_index = csvDocument.getColumnIndex("OSC Address");
			const int type_index = csvDocument.getColumnIndex("Type");
			const int defaultValue_index = csvDocument.getColumnIndex("Default value");
			const int enumValues_index = csvDocument.getColumnIndex("Enumeration Values");
			
			const size_t currentOscPrefixSize = currentOscPrefix.size();
			
			if (oscAddress_index >= 0 && type_index >= 0 && defaultValue_index >= 0)
			{
				for (auto i = csvDocument.firstRow(); i != csvDocument.lastRow(); i = csvDocument.nextRow(i))
				{
					const char * oscAddress = i[oscAddress_index];
					const char * type = i[type_index];
					const char * defaultValue = i[defaultValue_index];
					
					if (strstr(oscAddress, oscPrefix) != oscAddress)
						continue;
					
					const char * name = oscAddress + currentOscPrefixSize;
					
					if (groupPrefix && *name != '/')
						continue;
					
					while (*name == '/')
						name++;
					
					bool skipped = false;
					
					if (strcmp(type, "f") == 0)
					{
						VfxNodeBase::DynamicInput input;
						input.type = kVfxPlugType_Float;
						input.name = name;
						input.defaultValue = defaultValue;
						inputs.push_back(input);
						
						InputInfo inputInfo;
						inputInfo.oscAddress = oscAddress;
						inputInfo.defaultFloat = Parse::Float(defaultValue);
						inputInfos.push_back(inputInfo);
					}
					else if (strcmp(type, "f f") == 0) // todo : not yet supported
					{
						for (int i = 0; i < 2; ++i)
						{
							const char elem[2] = { 'x', 'y' };
							
							VfxNodeBase::DynamicInput input;
							input.type = kVfxPlugType_Float;
							input.name = String::FormatC("%s.%c", name, elem[i]);
							input.defaultValue = defaultValue;
							inputs.push_back(input);
							
							InputInfo inputInfo;
							inputInfo.oscAddress = oscAddress;
							inputInfo.isVec2f = true;
							inputInfo.defaultFloat = Parse::Float(defaultValue);
							inputInfos.push_back(inputInfo);
						}
					}
					else if (strcmp(type, "f f f") == 0) // todo : not yet supported
					{
						for (int i = 0; i < 3; ++i)
						{
							const char elem[3] = { 'x', 'y', 'z' };
							
							VfxNodeBase::DynamicInput input;
							input.type = kVfxPlugType_Float;
							input.name = String::FormatC("%s.%c", name, elem[i]);
							input.defaultValue = defaultValue;
							inputs.push_back(input);
							
							InputInfo inputInfo;
							inputInfo.oscAddress = oscAddress;
							inputInfo.isVec3f = true;
							inputInfo.defaultFloat = Parse::Float(defaultValue);
							inputInfos.push_back(inputInfo);
						}
					}
					else if (strcmp(type, "i") == 0)
					{
						VfxNodeBase::DynamicInput input;
						input.type = kVfxPlugType_Int;
						input.name = name;
						input.defaultValue = defaultValue;
						inputs.push_back(input);
						
						InputInfo inputInfo;
						inputInfo.oscAddress = oscAddress;
						inputInfo.defaultInt = Parse::Int32(defaultValue);
						inputInfos.push_back(inputInfo);
					}
					else if (strstr(type, "boolean") != nullptr)
					{
						VfxNodeBase::DynamicInput input;
						input.type = kVfxPlugType_Bool;
						input.name = name;
						input.defaultValue = strcmp(defaultValue, "true") == 0 ? "1" : "0";
						inputs.push_back(input);
						
						InputInfo inputInfo;
						inputInfo.oscAddress = oscAddress;
						inputInfo.defaultBool = Parse::Bool(defaultValue);
						inputInfos.push_back(inputInfo);
					}
					else if (strstr(type, "enum") != nullptr)
					{
						VfxNodeBase::DynamicInput input;
						input.type = kVfxPlugType_Int;
						input.name = name;
						input.defaultValue = strcmp(defaultValue, "true") == 0 ? "1" : "0";
						
						if (enumValues_index >= 0)
						{
							std::vector<std::string> enumNames;
							splitString(i[enumValues_index], enumNames, ',');
							
							input.enumElems.resize(enumNames.size());
							
							for (size_t i = 0; i < enumNames.size(); ++i)
							{
								input.enumElems[i].name = enumNames[i];
								input.enumElems[i].valueText = String::FormatC("%d", i);
							}
						}
						
						inputs.push_back(input);
					 
						InputInfo inputInfo;
						inputInfo.oscAddress = oscAddress;
						inputInfo.defaultBool = Parse::Bool(defaultValue);
						inputInfos.push_back(inputInfo);
					}
					else
					{
						logDebug("unknown OSC data type: %s", type);
						
						skipped = true;
					}
				}
			}
		}
		
		if (inputs.empty())
		{
			setDynamicInputs(nullptr, 0);
		}
		else
		{
			setDynamicInputs(&inputs[0], inputs.size());
		}
		
		Assert(inputInfos.size() == dynamicInputs.size());
	}
}

void VfxNodeOscSheet::tick(const float dt)
{
	const char * endpointName = getInputString(kInput_OscEndpoint, "");
	
	updateOscSheet();
	
	auto endpoint = g_oscEndpointMgr.findSender(endpointName);
	
	if (endpoint != nullptr)
	{
		char buffer[1 << 12];
		osc::OutboundPacketStream stream(buffer, sizeof(buffer));
		
		bool isEmpty = true;
		
		for (size_t i = 0; i < dynamicInputs.size(); )
		{
			auto & input = dynamicInputs[i];
			
			auto & inputInfo = inputInfos[i];
			
			if (isEmpty)
				stream << osc::BeginBundleImmediate;
			
			stream << osc::BeginMessage(inputInfos[i].oscAddress.c_str());
			{
				if (inputInfo.isVec2f)
				{
					stream
						<< getInputFloat(kInput_COUNT + i + 0, inputInfo.defaultFloat)
						<< getInputFloat(kInput_COUNT + i + 1, inputInfo.defaultFloat);
					
					i += 2;
				}
				else if (inputInfo.isVec3f)
				{
					stream
						<< getInputFloat(kInput_COUNT + i + 0, inputInfo.defaultFloat)
						<< getInputFloat(kInput_COUNT + i + 1, inputInfo.defaultFloat)
						<< getInputFloat(kInput_COUNT + i + 2, inputInfo.defaultFloat);
					
					i += 3;
				}
				else
				{
					if (input.type == kVfxPlugType_Float)
						stream << getInputFloat(kInput_COUNT + i, inputInfo.defaultFloat);
					else if (input.type == kVfxPlugType_Int)
						stream << getInputInt(kInput_COUNT + i, inputInfo.defaultInt);
					else if (input.type == kVfxPlugType_Bool)
						stream << getInputBool(kInput_COUNT + i, inputInfo.defaultBool);
					else
					{
						Assert(false);
					}
					
					i += 1;
				}
			}
			stream << osc::EndMessage;
			
			isEmpty = false;
			
			// flush if the buffer is getting full
			
			if (stream.Size() >= 1000)
			{
				stream << osc::EndBundle;
				
				endpoint->send(stream.Data(), stream.Size());
				
				stream = osc::OutboundPacketStream(buffer, sizeof(buffer));
				isEmpty = true;
			}
		}
		
		// flush
		
		if (isEmpty == false)
		{
			stream << osc::EndBundle;
			
			endpoint->send(stream.Data(), stream.Size());
		}
	}
}

void VfxNodeOscSheet::init(const GraphNode & node)
{
	updateOscSheet();
}
