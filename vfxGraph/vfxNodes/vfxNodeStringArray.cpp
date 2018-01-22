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

#include "vfxNodeStringArray.h"
#include <math.h>

extern void splitString(const std::string & str, std::vector<std::string> & result, char c);

VFX_NODE_TYPE(VfxNodeStringArray)
{
	typeName = "text.array";
	
	in("text", "string");
	in("index", "float");
	in("index_norm", "float");
	in("next!", "trigger");
	in("prev!", "trigger");
	in("random!", "trigger");
	out("text", "string");
}

VfxNodeStringArray::VfxNodeStringArray()
	: VfxNodeBase()
	, textOutput()
	, currentText()
	, currentIndex(0)
	, textList()
	, selectedIndex(0)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_TextArray, kVfxPlugType_String);
	addInput(kInput_Index, kVfxPlugType_Float);
	addInput(kInput_IndexNorm, kVfxPlugType_Float);
	addInput(kInput_SelectNext, kVfxPlugType_Trigger);
	addInput(kInput_SelectPrev, kVfxPlugType_Trigger);
	addInput(kInput_SelectRandom, kVfxPlugType_Trigger);
	addOutput(kOutput_Text, kVfxPlugType_String, &textOutput);
}

void VfxNodeStringArray::tick(const float dt)
{
	if (isPassthrough)
	{
		textOutput.clear();
		
		textList.clear();
		
		currentText.clear();
		
		return;
	}
	
	//
	
	bool isDirty = false;
	
	const char * text = getInputString(kInput_TextArray, nullptr);

	if (text == nullptr)
	{
		if (!currentText.empty())
		{
			isDirty = true;
			
			textList.clear();
			
			currentText.clear();
		}
	}
	else if (currentText != text)
	{
		isDirty = true;
		
		currentText = text;
		
		textList.clear();
		splitString(text, textList, ' ');
	}
	
	if (textList.empty())
		selectedIndex = 0;
	else if (selectedIndex >= textList.size())
		selectedIndex = textList.size() - 1;
	
	if (textList.empty())
	{
		textOutput.clear();
	}
	else
	{
		auto indexNormInput = tryGetInput(kInput_IndexNorm);
		auto indexInput = tryGetInput(kInput_Index);
		
		int index = -1;
		
		if (indexNormInput->isConnected())
			index = (int)floorf(indexNormInput->getFloat() * (textList.size() - .0001f));
		else if (indexInput->isConnected())
			index = int(floorf(indexInput->getFloat())) % textList.size();
		else
			index = selectedIndex;
		
		if (index < 0)
		{
			index = (index + textList.size()) % textList.size();
		}
		
		Assert(index >= 0 && index < textList.size());
		
		if (index != currentIndex)
		{
			isDirty = true;
			
			currentIndex = index;
		}
		
		if (isDirty)
		{
			textOutput = textList[index];
		}
	}
}

void VfxNodeStringArray::handleTrigger(const int index)
{
	if (!textList.empty())
	{
		if (index == kInput_SelectNext)
			selectedIndex = (selectedIndex + 1) % textList.size();
		if (index == kInput_SelectPrev)
			selectedIndex = (selectedIndex - 1 + textList.size()) % textList.size();
		if (index == kInput_SelectRandom)
			selectedIndex = rand() % textList.size();
	}
}
