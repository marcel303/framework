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
#include "framework.h"
#include "osc/OscOutboundPacketStream.h"
#include "oscEndpointMgr.h"
#include "Parse.h"
#include "StringEx.h"
#include "vfxNodeOscSheet.h"

// todo : reset all inputs to their default values when the OSC sheet prefix changes

//

extern void splitString(const std::string & str, std::vector<std::string> & result, char c);

//

extern OscEndpointMgr g_oscEndpointMgr;

//

VFX_ENUM_TYPE(oscSheetSendMode)
{
	elem("onTick");
	elem("onChange");
	elem("onSync");
}

VFX_NODE_TYPE(VfxNodeOscSheet)
{
	typeName = "osc.sheet";
	
	in("oscEndpoint", "string");
	in("oscPrefix", "string");
	in("groupPrefix", "bool", "1");
	in("oscSheet", "string");
	in("synOnInit", "bool");
	inEnum("sendMode", "oscSheetSendMode", "1");
	in("sync!", "trigger");
}

VfxNodeOscSheet::VfxNodeOscSheet()
	: VfxNodeBase()
	, currentOscPrefix()
	, currentOscSheet()
	, currentGroupPrefix(false)
	, sync(false)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_OscEndpoint, kVfxPlugType_String);
	addInput(kInput_OscPrefix, kVfxPlugType_String);
	addInput(kInput_GroupPrefix, kVfxPlugType_Bool);
	addInput(kInput_OscSheet, kVfxPlugType_String);
	addInput(kInput_SyncOnInit, kVfxPlugType_Bool);
	addInput(kInput_SendMode, kVfxPlugType_Int);
	addInput(kInput_Sync, kVfxPlugType_Trigger);
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
			const int defaultValue_index = csvDocument.getColumnIndex("Default Value");
			const int enumValues_index = csvDocument.getColumnIndex("Enumeration Values");
			
			const size_t currentOscPrefixSize = currentOscPrefix.size();
			
			if (oscAddress_index < 0)
				logWarning("missing 'OSC Address' column");
			if (type_index < 0)
				logWarning("missing 'Type' column");
			if (defaultValue_index < 0)
				logWarning("missing 'Default Value' column");
			
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
						inputInfo.lastFloat = inputInfo.defaultFloat;
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
							inputInfo.lastFloat = inputInfo.defaultFloat;
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
							inputInfo.lastFloat = inputInfo.defaultFloat;
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
						inputInfo.lastInt = inputInfo.defaultInt;
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
						inputInfo.lastBool = inputInfo.defaultBool;
						inputInfos.push_back(inputInfo);
					}
					else if (strstr(type, "enum") != nullptr)
					{
						VfxNodeBase::DynamicInput input;
						input.type = kVfxPlugType_Int;
						input.name = name;
						input.defaultValue = defaultValue; // todo : look up default enum value from OSC sheet
						
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
						inputInfo.defaultInt = Parse::Int32(defaultValue); // todo : check default enum value from sheet
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
			setDynamicInputs(inputs.data(), inputs.size());
		}
		
		Assert(inputInfos.size() == dynamicInputs.size());
	}
}

void VfxNodeOscSheet::tick(const float dt)
{
	const char * endpointName = getInputString(kInput_OscEndpoint, "");
	
	updateOscSheet();
	
	auto endpoint = g_oscEndpointMgr.findSender(endpointName);
	
	const SendMode sendMode = (SendMode)getInputInt(kInput_SendMode, kSend_OnChange);
	
#define shouldSend(in_isChanged) ((sendMode == kSend_OnChange && (in_isChanged)) || (sendMode == kSend_OnTick) || sync)

	if (endpoint != nullptr)
	{
		char buffer[1 << 12];
		osc::OutboundPacketStream stream(buffer, sizeof(buffer));
		
		stream << osc::BeginBundleImmediate;
		
		bool isEmpty = true;
		
		for (size_t i = 0; i < dynamicInputs.size(); )
		{
			auto & input = dynamicInputs[i];
			
			auto & inputInfo = inputInfos[i];
			
			if (inputInfo.isVec2f)
			{
				const float value1 = getInputFloat(kInput_COUNT + i + 0, inputInfo.defaultFloat);
				const float value2 = getInputFloat(kInput_COUNT + i + 1, inputInfo.defaultFloat);
				
				const bool isChanged =
					value1 != inputInfos[i + 0].lastFloat ||
					value2 != inputInfos[i + 1].lastFloat;
				
				if (shouldSend(isChanged))
				{
					isEmpty = false;
					
					stream << osc::BeginMessage(inputInfo.oscAddress.c_str());
					{
						stream
							<< value1
							<< value2;
					}
					stream << osc::EndMessage;
					
					inputInfos[i + 0].lastFloat = value1;
					inputInfos[i + 1].lastFloat = value2;
				}
				
				i += 2;
			}
			else if (inputInfo.isVec3f)
			{
				const float value1 = getInputFloat(kInput_COUNT + i + 0, inputInfo.defaultFloat);
				const float value2 = getInputFloat(kInput_COUNT + i + 1, inputInfo.defaultFloat);
				const float value3 = getInputFloat(kInput_COUNT + i + 2, inputInfo.defaultFloat);
				
				const bool isChanged =
					value1 != inputInfos[i + 0].lastFloat ||
					value2 != inputInfos[i + 1].lastFloat ||
					value3 != inputInfos[i + 2].lastFloat;
				
				if (shouldSend(isChanged))
				{
					isEmpty = false;
					
					stream << osc::BeginMessage(inputInfo.oscAddress.c_str());
					{
						stream
							<< value1
							<< value2
							<< value3;
					}
					stream << osc::EndMessage;
					
					inputInfos[i + 0].lastFloat = value1;
					inputInfos[i + 1].lastFloat = value2;
					inputInfos[i + 2].lastFloat = value3;
				}
				
				i += 3;
			}
			else
			{
				if (input.type == kVfxPlugType_Float)
				{
					const float value = getInputFloat(kInput_COUNT + i, inputInfo.defaultFloat);
					
					if (shouldSend(value != inputInfo.lastFloat))
					{
						isEmpty = false;
						
						stream << osc::BeginMessage(inputInfo.oscAddress.c_str());
						{
							stream << value;
						}
						stream << osc::EndMessage;
						
						inputInfo.lastFloat = value;
					}
				}
				else if (input.type == kVfxPlugType_Int)
				{
					const int value = getInputInt(kInput_COUNT + i, inputInfo.defaultInt);
					
					if (shouldSend(value != inputInfo.lastInt))
					{
						isEmpty = false;
						
						stream << osc::BeginMessage(inputInfo.oscAddress.c_str());
						{
							stream << value;
						}
						stream << osc::EndMessage;
						
						inputInfo.lastInt = value;
					}
				}
				else if (input.type == kVfxPlugType_Bool)
				{
					const bool value = getInputBool(kInput_COUNT + i, inputInfo.defaultBool);
					
					if (shouldSend(value != inputInfo.lastBool))
					{
						isEmpty = false;
						
						stream << osc::BeginMessage(inputInfo.oscAddress.c_str());
						{
							stream << value;
						}
						stream << osc::EndMessage;
						
						inputInfo.lastBool = value;
					}
				}
				else
				{
					Assert(false);
				}
				
				i += 1;
			}
			
			// flush if the buffer is getting full
			
			if (stream.Size() >= 1000)
			{
				stream << osc::EndBundle;
				
				endpoint->send(stream.Data(), stream.Size());
				
				stream = osc::OutboundPacketStream(buffer, sizeof(buffer));
				
				stream << osc::BeginBundleImmediate;
				
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
	
#undef shouldSend
	
	sync = false;
}

void VfxNodeOscSheet::init(const GraphNode & node)
{
	updateOscSheet();
	
	sync = getInputBool(kInput_SyncOnInit, false);
}

void VfxNodeOscSheet::handleTrigger(const int socketIndex)
{
	if (socketIndex == kInput_Sync)
		sync = true;
}
