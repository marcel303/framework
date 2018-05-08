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

//#include "imageDelayLine.h" // todo
#include "vfxNodeImageDelayLine.h"

//

VFX_NODE_TYPE(VfxNodeImageDelayLine)
{
	typeName = "image.delay";
	
	in("image", "image");
	in("maxDelay", "float");
	in("delay1", "float", "-1");
	in("delay2", "float", "-1");
	in("delay3", "float", "-1");
	in("delay4", "float", "-1");
	out("image1", "image");
	out("image2", "image");
	out("image3", "image");
	out("image4", "image");
}

//

#include <deque>
#include "framework.h"

static GLuint allocateTexture(const int sx, const int sy, GLenum internalFormat, const bool filter, const bool clamp, const GLint * swizzleMask)
{
	GLuint newTexture;
	
	GLuint restoreTexture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
	checkErrorGL();
	
	glGenTextures(1, &newTexture);
	glBindTexture(GL_TEXTURE_2D, newTexture);
	glTexStorage2D(GL_TEXTURE_2D, 1, internalFormat, sx, sy);
	checkErrorGL();
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	checkErrorGL();
	
	glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
	checkErrorGL();

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, clamp ? GL_CLAMP_TO_EDGE : GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter ? GL_LINEAR : GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter ? GL_LINEAR : GL_NEAREST);
	
	glBindTexture(GL_TEXTURE_2D, restoreTexture);
	checkErrorGL();
	
	return newTexture;
}

static GLuint copyTexture(const GLuint texture)
{
	// update texture
	
	GLuint oldBuffer = 0;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (GLint*)&oldBuffer);
	checkErrorGL();
	
	GLuint restoreTexture;
	glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
	checkErrorGL();

	//
	
	glBindTexture(GL_TEXTURE_2D, texture);
	checkErrorGL();
	
	int sx;
	int sy;
	int internalFormat;
	
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &sx);
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &sy);
	checkErrorGL();
	glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);
	checkErrorGL();
	
	int magFilter;
	int wrapS;
	
	glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, &magFilter);
	checkErrorGL();
	glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, &wrapS);
	checkErrorGL();
	
	GLint swizzleMask[4];
	glGetTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
	checkErrorGL();
	
	const bool filter = magFilter != GL_POINT;
	const bool clamp = wrapS == GL_CLAMP_TO_EDGE;
	
	GLuint newTexture = allocateTexture(sx, sy, internalFormat, filter, clamp, swizzleMask);
	
	glBindTexture(GL_TEXTURE_2D, newTexture);
	checkErrorGL();
	
	GLuint frameBuffer = 0;
	
	glGenFramebuffers(1, &frameBuffer);
	checkErrorGL();
	
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);
	checkErrorGL();
	
	glReadBuffer(GL_COLOR_ATTACHMENT0);
	glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, sx, sy);
	checkErrorGL();
	
	//
	
	glBindTexture(GL_TEXTURE_2D, restoreTexture);
	checkErrorGL();
	
	glBindFramebuffer(GL_FRAMEBUFFER, oldBuffer);
	checkErrorGL();
	
	//
	
	glDeleteFramebuffers(1, &frameBuffer);
	frameBuffer = 0;
	
	return newTexture;
}

struct ImageDelayLine
{
	std::deque<GLuint> history;
	int historySize;
	int maxHistorySize;

	ImageDelayLine()
		: history(0)
		, historySize(0)
		, maxHistorySize(0)
	{
	}

	~ImageDelayLine()
	{
		shut();
	}

	void init(const int length)
	{
		shut();

		maxHistorySize = length;
	}

	void shut()
	{
		setLength(0);

		maxHistorySize = 0;
	}

	int getLength() const
	{
		return maxHistorySize;
	}

	void setLength(const int length)
	{
		maxHistorySize = length;

		while (historySize > maxHistorySize)
		{
			GLuint texture = history.back();
			
			glDeleteTextures(1, &texture);
			texture = 0;

			history.pop_back();
			historySize--;
		}
	}

	void add(const GLuint texture)
	{
		if (historySize == maxHistorySize)
		{
			GLuint texture = history.back();
			
			glDeleteTextures(1, &texture);
			texture = 0;
			checkErrorGL();

			history.pop_back();
		}
		else
		{
			historySize++;
		}
		
		const GLuint copy = copyTexture(texture);
		
		history.push_front(copy);
	}

	GLuint get(const int offset)
	{
		if (offset >= 0 && offset < historySize)
		{
			return history[offset];
		}
		else
		{
			return 0;
		}
	}
};

VfxNodeImageDelayLine::VfxNodeImageDelayLine()
	: VfxNodeBase()
	, outputImage()
	, delayLine(nullptr)
{
	delayLine = new ImageDelayLine();

	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Image, kVfxPlugType_Image);
	addInput(kInput_MaxDelay, kVfxPlugType_Float);
	addInput(kInput_Delay1, kVfxPlugType_Float);
	addInput(kInput_Delay2, kVfxPlugType_Float);
	addInput(kInput_Delay3, kVfxPlugType_Float);
	addInput(kInput_Delay4, kVfxPlugType_Float);
	addOutput(kOutput_Image1, kVfxPlugType_Image, &outputImage[0]);
	addOutput(kOutput_Image2, kVfxPlugType_Image, &outputImage[1]);
	addOutput(kOutput_Image3, kVfxPlugType_Image, &outputImage[2]);
	addOutput(kOutput_Image4, kVfxPlugType_Image, &outputImage[3]);
	
	delayLine->init(0);
}

VfxNodeImageDelayLine::~VfxNodeImageDelayLine()
{
	delayLine->shut();
	
	delete delayLine;
	delayLine = nullptr;
}

void VfxNodeImageDelayLine::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeDelayLine);
	
	const int maxDelay = 1 + std::max(0, (int)std::ceil(getInputFloat(kInput_MaxDelay, 0.f)));
	
	const int delay1 = (int)std::round(getInputFloat(kInput_Delay1, -1.f) * maxDelay);
	const int delay2 = (int)std::round(getInputFloat(kInput_Delay2, -1.f) * maxDelay);
	const int delay3 = (int)std::round(getInputFloat(kInput_Delay3, -1.f) * maxDelay);
	const int delay4 = (int)std::round(getInputFloat(kInput_Delay4, -1.f) * maxDelay);
	
	// set delay line length
	
	if (maxDelay != delayLine->getLength())
	{
		delayLine->setLength(maxDelay);
	}
	
	if (delayLine->getLength() > 0)
	{
		const VfxImageBase * image = getInputImage(kInput_Image, nullptr);

		if (image != nullptr)
		{
			delayLine->add(image->getTexture());
		}
		
		const int offset[4] =
		{
			std::min(delayLine->getLength() - 1, std::max(0, delay1)),
			std::min(delayLine->getLength() - 1, std::max(0, delay2)),
			std::min(delayLine->getLength() - 1, std::max(0, delay3)),
			std::min(delayLine->getLength() - 1, std::max(0, delay4))
		};

		for (int i = 0; i < 4; ++i)
		{
			if (tryGetInput(kInput_Delay1 + i)->isConnected())
				outputImage[i].texture = delayLine->get(offset[i]);
			else
				outputImage[i].texture = 0;
		}
	}
	else
	{
		for (int i = 0; i < 4; ++i)
		{
			outputImage[i].texture = 0;
		}
	}
}

void VfxNodeImageDelayLine::getDescription(VfxNodeDescription & d)
{
	d.add("history size: %d/%d", delayLine->historySize, delayLine->maxHistorySize);
	d.newline();
	
	for (int i = 0; i < 4; ++i)
	{
		char name[64];
		sprintf(name, "output image %d", i + 1);
		
		d.add(name, outputImage[i]);
		d.newline();
	}
}
