/*
	Copyright (C) 2020 Marcel Smit
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
#include "framework.h" // fileHasChanged
#include "Log.h"
#include "Path.h"
#include "vfxNodeDatatable.h"

VFX_NODE_TYPE(VfxNodeDatatable)
{
	typeName = "channel.fromFile";
	
	in("filename", "string");
	in("hasHeader", "bool");
	in("transpose", "bool");
	out("channel", "channel");
}

VfxNodeDatatable::VfxNodeDatatable()
	: VfxNodeBase()
	, filename()
	, hasHeader(false)
	, transpose(false)
	, outputChannel()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Filename, kVfxPlugType_String);
	addInput(kInput_HasHeader, kVfxPlugType_Bool);
	addInput(kInput_Transpose, kVfxPlugType_Bool);
	addOutput(kOutput_Channel, kVfxPlugType_Channel, &outputChannel);
}

void VfxNodeDatatable::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeDatatable);
	
	if (isPassthrough)
	{
		filename.clear();
		channelData.free();
		outputChannel.reset();
		setDynamicOutputs(nullptr, 0);
		outputChannels.clear();
		return;
	}
	
	const char * newFilename = getInputString(kInput_Filename, "");
	const bool newHasHeader = getInputBool(kInput_HasHeader, false);
	const bool newTranspose = getInputBool(kInput_Transpose, false);
	
	// reload data when filename changes or when we're asked to interpret the data differently
	
	if (newFilename != filename || newHasHeader != hasHeader || newTranspose != transpose || framework.fileHasChanged(filename.c_str()))
	{
		LOG_DBG("reloading data table!");
		
		filename = newFilename;
		hasHeader = newHasHeader;
		transpose = newTranspose;
		
		channelData.free();
		outputChannel.reset();
		setDynamicOutputs(nullptr, 0);
		outputChannels.clear();
		
		if (Path::GetExtension(filename, true) == "csv")
		{
			CsvDocument document;
			
			if (document.load(filename.c_str(), hasHeader, ',') == true)
			{
				const int numColumns = document.m_header.size();
				const int numRows = document.m_rows.size();
				
				channelData.alloc(numColumns * numRows);
				
				float * data = channelData.data;
				
				if (transpose)
				{
					for (auto & row : document.m_rows)
					{
						for (auto & header : document.m_header)
						{
							*data++ = row.getFloat(header.c_str(), 0.f);
						}
					}
					
					// todo : add option to output 1D data to dynamic outputs or 2D data
					
					outputChannel.setData2D(channelData.data, false, numColumns, numRows);
				}
				else
				{
					for (auto & header : document.m_header)
					{
						for (auto & row : document.m_rows)
						{
							*data++ = row.getFloat(header.c_str(), 0.f);
						}
					}
					
					outputChannel.setData2D(channelData.data, false, numRows, numColumns);
				}
				
			// todo : add transpose support to CSV document reader. let it handle headers correctly so we can use those here to give channels a name, when the data is transposed
				if (hasHeader && transpose == false)
				{
					const int channelCount = transpose == false ? numColumns : numRows;
					const int channelSize = transpose == false ? numRows : numColumns;
					
					std::vector<DynamicOutput> dynamicOutputs;
					dynamicOutputs.resize(channelCount);
					outputChannels.resize(channelCount);
					
					for (size_t i = 0; i < document.m_header.size(); ++i)
					{
						dynamicOutputs[i].name = document.m_header[i];
						dynamicOutputs[i].type = kVfxPlugType_Channel;
						dynamicOutputs[i].mem = &outputChannels[i];
						
						outputChannels[i].setData(channelData.data + channelSize * i, false, channelSize);
					}
					
					setDynamicOutputs(dynamicOutputs.data(), dynamicOutputs.size());
				}
			}
		}
	}
}
