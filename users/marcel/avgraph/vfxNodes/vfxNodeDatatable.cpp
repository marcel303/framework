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
#include "Log.h"
#include "Path.h"
#include "vfxNodeDatatable.h"

VFX_NODE_TYPE(datatable, VfxNodeDatatable)
{
	typeName = "channels.fromFile";
	
	in("filename", "string");
	in("hasHeader", "bool");
	in("transpose", "bool");
	out("channels", "channels");
}

VfxNodeDatatable::VfxNodeDatatable()
	: VfxNodeBase()
	, filename()
	, hasHeader(false)
	, transpose(false)
	, outputChannels()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Filename, kVfxPlugType_String);
	addInput(kInput_HasHeader, kVfxPlugType_Bool);
	addInput(kInput_Transpose, kVfxPlugType_Bool);
	addOutput(kOutput_Channels, kVfxPlugType_Channels, &outputChannels);
}

void VfxNodeDatatable::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeDatatable);
	
	if (isPassthrough)
	{
		filename.clear();
		channelData.free();
		outputChannels.reset();
		return;
	}
	
	const char * newFilename = getInputString(kInput_Filename, "");
	const bool newHasHeader = getInputBool(kInput_HasHeader, false);
	const bool newTranspose = getInputBool(kInput_Transpose, false);

	// todo : reloud data when filename changes
	
	if (newFilename != filename || newHasHeader != hasHeader || newTranspose != transpose)
	{
		LOG_DBG("reloading data table!", 0);
		
		filename = newFilename;
		hasHeader = newHasHeader;
		transpose = newTranspose;
		
		channelData.free();
		outputChannels.reset();
		
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
					
					outputChannels.setDataContiguous(channelData.data, false, numColumns, numRows);
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
					
					outputChannels.setDataContiguous(channelData.data, false, numRows, numColumns);
				}
			}
		}
	}
}
