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

#include "framework.h"
#include "vfxNodeDrawModel.h"

VFX_NODE_TYPE(drawModel, VfxNodeDrawModel)
{
	typeName = "draw.model";
	
	in("file", "string");
	in("anim", "string");
	in("animSpeed", "float", "1");
	in("loopCount", "float", "-1");
	in("rootMotion", "bool");
	in("scale", "float", "1");
	out("any", "any");
}

VfxNodeDrawModel::VfxNodeDrawModel()
	: VfxNodeBase()
	, model(nullptr)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Filename, kVfxPlugType_String);
	addInput(kInput_Animation, kVfxPlugType_String);
	addInput(kInput_AnimationSpeed, kVfxPlugType_Float);
	addInput(kInput_AnimationLoopCount, kVfxPlugType_Float);
	addInput(kInput_AnimationRootMotionEnabled, kVfxPlugType_Bool);
	addInput(kInput_Scale, kVfxPlugType_Float);
	addOutput(kOutput_Any, kVfxPlugType_DontCare, this);
}

VfxNodeDrawModel::~VfxNodeDrawModel()
{
	delete model;
	model = nullptr;
}

void VfxNodeDrawModel::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeDrawModel);
	
	const char * filename = getInputString(kInput_Filename, nullptr);

	if (isPassthrough || filename == nullptr)
	{
		currentFilename.clear();
		
		delete model;
		model = nullptr;

		return;
	}

	if (filename != currentFilename)
	{
		currentFilename = filename;

		delete model;
		model = nullptr;

		model = new Model(filename);
	}

	const char * anim = getInputString(kInput_Animation, "");
	const float animSpeed = getInputFloat(kInput_AnimationSpeed, 1.f);
	const float animLoopCount = getInputFloat(kInput_AnimationLoopCount, -1.f);
	const bool animRootMotionEnabled = getInputBool(kInput_AnimationRootMotionEnabled, false);
	
	if (anim != currentAnim)
	{
		currentAnim = anim;
		
		model->stopAnim();
		
		model->startAnim(anim);
	}
	
	model->animSpeed = animSpeed;
	model->animLoop = int(std::round(animLoopCount));
	model->animRootMotionEnabled = animRootMotionEnabled;
	
	// todo : tick model
}

void VfxNodeDrawModel::draw() const
{
	if (isPassthrough || model == nullptr)
		return;
	
	const float scale = getInputFloat(kInput_Scale, 1.f);

	gxPushMatrix();
	{
		gxScalef(scale, scale, scale);
		
		const int drawFlags = DrawMesh | DrawBones | DrawNormals;
		
		setColor(colorWhite);
		model->draw(drawFlags);
		
		setColor(colorWhite);
		glEnable(GL_DEPTH_TEST);
	}
	gxPopMatrix();
}
