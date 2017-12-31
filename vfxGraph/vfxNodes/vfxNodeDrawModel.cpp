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

template <typename T>
static inline T pad(T value, int align)
{
	return (value + align - 1) & (~(align - 1));
}

#define ALIGNED_ALLOCA(size, align) reinterpret_cast<void*>(pad(reinterpret_cast<uintptr_t>(alloca(size + align - 1)), align))

VFX_NODE_TYPE(VfxNodeDrawModel)
{
	typeName = "draw.model";
	
	in("file", "string");
	in("anim", "string");
	in("animSpeed", "float", "1");
	in("loopCount", "float", "-1");
	in("rootMotion", "bool");
	in("scale", "float", "1");
	out("any", "draw", "draw");
	out("position.x", "channel");
	out("position.y", "channel");
	out("position.z", "channel");
	out("normal.x", "channel");
	out("normal.y", "channel");
	out("normal.z", "channel");
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
	addOutput(kOutput_Draw, kVfxPlugType_Draw, this);
	addOutput(kOutput_PositionX, kVfxPlugType_Channel, &positionX);
	addOutput(kOutput_PositionY, kVfxPlugType_Channel, &positionY);
	addOutput(kOutput_PositionZ, kVfxPlugType_Channel, &positionZ);
	addOutput(kOutput_NormalX, kVfxPlugType_Channel, &normalX);
	addOutput(kOutput_NormalY, kVfxPlugType_Channel, &normalY);
	addOutput(kOutput_NormalZ, kVfxPlugType_Channel, &normalZ);
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
	const float scale = getInputFloat(kInput_Scale, 1.f);
	const bool wantsPositionChannels =
		outputs[kOutput_PositionX].isReferenced() ||
		outputs[kOutput_PositionY].isReferenced() ||
		outputs[kOutput_PositionZ].isReferenced();
	const bool wantsNormalChannels =
		outputs[kOutput_NormalX].isReferenced() ||
		outputs[kOutput_NormalY].isReferenced() ||
		outputs[kOutput_NormalZ].isReferenced();
	
	positionX.reset();
	positionY.reset();
	positionZ.reset();

	normalX.reset();
	normalY.reset();
	normalZ.reset();

	if (isPassthrough || filename == nullptr)
	{
		currentFilename.clear();
		
		delete model;
		model = nullptr;
		
		channelData.free();
		
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
	
	model->tick(dt);
	
	if (wantsPositionChannels || wantsNormalChannels)
	{
		Mat4x4 matrix;
		matrix.MakeScaling(scale, scale, scale);
		
		const int numMatrices = model->calculateBoneMatrices(matrix, nullptr, nullptr, nullptr, 0);
		
		Mat4x4 * localMatrices = (Mat4x4*)ALIGNED_ALLOCA(sizeof(Mat4x4) * numMatrices, 16);
		Mat4x4 * worldMatrices = (Mat4x4*)ALIGNED_ALLOCA(sizeof(Mat4x4) * numMatrices, 16);
		Mat4x4 * globalMatrices = (Mat4x4*)ALIGNED_ALLOCA(sizeof(Mat4x4) * numMatrices, 16);
		
		model->calculateBoneMatrices(matrix, localMatrices, worldMatrices, globalMatrices, numMatrices);
		
		const int numVertices = model->softBlend(
			matrix, nullptr, nullptr,  nullptr, numMatrices,
			false, nullptr, nullptr, nullptr,
			false, nullptr, nullptr, nullptr, 0);
		
		const int size =
			(wantsPositionChannels * numVertices * 3) +
			(wantsNormalChannels   * numVertices * 3);
		
		channelData.allocOnSizeChange(size);
		
		float * dataPtr = channelData.data;
		
		if (wantsPositionChannels)
		{
			positionX.setData(dataPtr, false, numVertices); dataPtr += numVertices;
			positionY.setData(dataPtr, false, numVertices); dataPtr += numVertices;
			positionZ.setData(dataPtr, false, numVertices); dataPtr += numVertices;
		}
		
		if (wantsNormalChannels)
		{
			normalX.setData(dataPtr, false, numVertices); dataPtr += numVertices;
			normalY.setData(dataPtr, false, numVertices); dataPtr += numVertices;
			normalZ.setData(dataPtr, false, numVertices); dataPtr += numVertices;
		}
		
		model->softBlend(
			matrix, localMatrices, worldMatrices, globalMatrices, numMatrices,
			wantsPositionChannels, positionX.dataRw(), positionY.dataRw(), positionZ.dataRw(),
			wantsNormalChannels, normalX.dataRw(), normalY.dataRw(), normalZ.dataRw(),
			numVertices);
	}
	else
	{
		channelData.free();
	}
}

void VfxNodeDrawModel::draw() const
{
	if (isPassthrough || model == nullptr || !tryGetOutput(kOutput_Draw)->isReferencedByLink)
		return;
	
	vfxCpuTimingBlock(VfxNodeDrawModel);
	vfxGpuTimingBlock(VfxNodeDrawModel);

	const float scale = getInputFloat(kInput_Scale, 1.f);

	gxPushMatrix();
	{
		gxScalef(scale, scale, scale);
		
		const int drawFlags = DrawMesh;
		
		setColor(colorWhite);
		model->draw(drawFlags);
	}
	gxPopMatrix();
}
