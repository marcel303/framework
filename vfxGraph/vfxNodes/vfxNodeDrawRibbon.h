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

#pragma once

#include "vfxNodeBase.h"

class Model;

struct VfxNodeDrawRibbon : VfxNodeBase
{
	const static int kMaxLength = 100;
	
	enum Input
	{
		kInput_Draw,
		kInput_X1,
		kInput_Y1,
		kInput_Z1,
		kInput_X2,
		kInput_Y2,
		kInput_Z2,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Draw,
		kOutput_COUNT
	};
	
	// todo : add objects/ribbon.cpp/h source files
	
	struct Ribbon
	{
		float x1[kMaxLength];
		float y1[kMaxLength];
		float z1[kMaxLength];
		
		float x2[kMaxLength];
		float y2[kMaxLength];
		float z2[kMaxLength];
		
		int length;
		int maxLength;
		
		int nextWritePos;
		int startPos;
		
		Ribbon()
			: length(0)
			, maxLength(kMaxLength)
			, nextWritePos(0)
			, startPos(0)
		{
		}
		
		void add1(const float _x, const float _y, const float _z)
		{
			x1[nextWritePos] = _x;
			y1[nextWritePos] = _y;
			z1[nextWritePos] = _z;
			
			x2[nextWritePos] = _x;
			y2[nextWritePos] = _y;
			z2[nextWritePos] = _z;
			
			nextWritePos = (nextWritePos + 1) % maxLength;
			
			if (length < maxLength)
				length++;
			else
				startPos = (startPos + 1) % maxLength;
		}
		
		void add2(
			const float _x1, const float _y1, const float _z1,
			const float _x2, const float _y2, const float _z2)
		{
			x1[nextWritePos] = _x1;
			y1[nextWritePos] = _y1;
			z1[nextWritePos] = _z1;
			
			x2[nextWritePos] = _x2;
			y2[nextWritePos] = _y2;
			z2[nextWritePos] = _z2;
			
			nextWritePos = (nextWritePos + 1) % maxLength;
			
			if (length < maxLength)
				length++;
			else
				startPos = (startPos + 1) % maxLength;
		}
	};
	
	Ribbon ribbon;

	VfxNodeDrawRibbon();
	
	virtual void tick(const float dt) override;
	virtual void draw() const override;
};
