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

class Surface;

struct VfxNodeFsfxV2 : VfxNodeBase
{
	struct ShaderInput
	{
		int type;
		VfxPlugType socketType;
		int socketIndex;
		int uniformLocation;
	};
	
	enum Input
	{
		kInput_Draw,
		kInput_Shader,
		kInput_Image1,
		kInput_Image2,
		kInput_Color1,
		kInput_Param1,
		kInput_Param2,
		kInput_Time,
		kInput_Opacity,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Draw,
		kOutput_Image,
		kOutput_COUNT
	};
	
	std::string currentShader;
	int currentShaderVersion;
	Shader * shader;
	std::vector<ShaderInput> shaderInputs;
	
	ShaderBuffer * gaussianKernel;
	
	mutable Surface * imageSurface;
	mutable VfxImage_Texture imageOutput;
	
	VfxNodeFsfxV2();
	virtual ~VfxNodeFsfxV2() override;
	
	void loadShader(const char * filename);
	void freeShader();
	
	void updateImageOutput(Surface * source) const;
	
	virtual void tick(const float dt) override;
	virtual void draw() const override;
	
	virtual void getDescription(VfxNodeDescription & d) override;
};
