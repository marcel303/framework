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

#pragma once

#include "audioFloat.h"
#include "Debugging.h"
#include <string>

struct PcmData;

enum AudioPlugType
{
	kAudioPlugType_None,
	kAudioPlugType_Bool,
	kAudioPlugType_Int,
	kAudioPlugType_Float,
	kAudioPlugType_String,
	kAudioPlugType_FloatVec,
	kAudioPlugType_PcmData,
	kAudioPlugType_Trigger
};

struct AudioPlug
{
	AudioPlugType type : 8;
	bool isTriggered;
	void * mem;
	void * immediateMem;
	
	mutable AudioFloatArray floatArray;

	AudioPlug()
		: type(kAudioPlugType_None)
		, isTriggered(false)
		, mem(nullptr)
		, immediateMem(nullptr)
		, floatArray()
	{
	}
	
	void connectTo(AudioPlug & dst);
	void connectToImmediate(void * dstMem, const AudioPlugType dstType);
	
	void disconnect()
	{
		mem = immediateMem;
	}
	
	void disconnect(const void * dstMem)
	{
		Assert(dstMem != nullptr && dstMem != immediateMem);
		
		if (type == kAudioPlugType_FloatVec)
		{
			bool removed = false;
			
			for (auto elemItr = floatArray.elems.begin(); elemItr != floatArray.elems.end(); )
			{
				if (elemItr->audioFloat == dstMem)
				{
					elemItr = floatArray.elems.erase(elemItr);
					removed = true;
					break;
				}
				
				++elemItr;
			}
			
			Assert(removed);
			
			Assert(mem == immediateMem);
		}
		else
		{
			Assert(mem == dstMem);
			
			disconnect();
		}
	}
	
	bool isConnected() const
	{
		if (mem != nullptr)
			return true;
		
		if (floatArray.elems.empty() == false)
			return true;
	
		Assert(immediateMem == nullptr); // mem should be equal to immediateMem when we get here
		
		return false;
	}
	
	bool getBool() const
	{
		Assert(type == kAudioPlugType_Bool);
		return *((bool*)mem);
	}
	
	int getInt() const
	{
		Assert(type == kAudioPlugType_Int);
		return *((int*)mem);
	}
	
	float getFloat() const
	{
		Assert(type == kAudioPlugType_Float);
		return *((float*)mem);
	}
	
	const std::string & getString() const
	{
		Assert(type == kAudioPlugType_String);
		return *((std::string*)mem);
	}
	
	const AudioFloat & getAudioFloat() const
	{
		Assert(type == kAudioPlugType_FloatVec);
	
		if (!floatArray.elems.empty())
			return *floatArray.get();
	
		return *((AudioFloat*)mem);
	}
	
	const PcmData & getPcmData() const
	{
		Assert(type == kAudioPlugType_PcmData);
		return *((PcmData*)mem);
	}
	
	//
	
	bool & getRwBool()
	{
		Assert(type == kAudioPlugType_Bool);
		return *((bool*)mem);
	}
	
	int & getRwInt()
	{
		Assert(type == kAudioPlugType_Int);
		return *((int*)mem);
	}
	
	float & getRwFloat()
	{
		Assert(type == kAudioPlugType_Float);
		return *((float*)mem);
	}
	
	std::string & getRwString()
	{
		Assert(type == kAudioPlugType_String);
		return *((std::string*)mem);
	}
	
	AudioFloat & getRwAudioFloat()
	{
		Assert(type == kAudioPlugType_FloatVec);
		return *((AudioFloat*)mem);
	}
};
