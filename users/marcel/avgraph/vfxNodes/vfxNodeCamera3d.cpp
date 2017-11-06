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
#include "vfxNodeCamera3d.h"

VFX_NODE_TYPE(VfxNodeCamera3d)
{
	typeName = "draw.camera3d";
	
	in("any", "any");
	in("control", "bool");
	in("x", "float");
	in("y", "float");
	in("z", "float");
	in("yaw", "float");
	in("pitch", "float");
	in("roll", "float");
	out("any", "any");
	out("mat.world", "channels");
}

VfxNodeCamera3d::VfxNodeCamera3d()
	: VfxNodeBase()
	, camera(nullptr)
	, cameraWorldMatrix(true)
	, cameraWorldMatrixOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Any, kVfxPlugType_DontCare);
	addInput(kInput_Interactive, kVfxPlugType_Bool);
	addInput(kInput_X, kVfxPlugType_Float);
	addInput(kInput_Y, kVfxPlugType_Float);
	addInput(kInput_Z, kVfxPlugType_Float);
	addInput(kInput_Yaw, kVfxPlugType_Float);
	addInput(kInput_Pitch, kVfxPlugType_Float);
	addInput(kInput_Roll, kVfxPlugType_Float);
	addOutput(kOutput_Any, kVfxPlugType_DontCare, this);
	addOutput(kOutput_WorldMatrix, kVfxPlugType_Channels, &cameraWorldMatrixOutput);
	
	camera = new Camera3d();
}

VfxNodeCamera3d::~VfxNodeCamera3d()
{
	delete camera;
	camera = nullptr;
}

void VfxNodeCamera3d::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeCamera3d);
	
	if (isPassthrough)
	{
		cameraWorldMatrix.MakeIdentity();
		cameraWorldMatrixOutput.reset();
		return;
	}
	
	const bool interactive = getInputBool(kInput_Interactive, false);

	const float x = getInputFloat(kInput_X, 0.f);
	const float y = getInputFloat(kInput_Y, 0.f);
	const float z = getInputFloat(kInput_Z, 0.f);

	const float yaw = getInputFloat(kInput_Yaw, 0.f);
	const float pitch = getInputFloat(kInput_Pitch, 0.f);

	if (interactive == false)
	{
		camera->position[0] = x;
		camera->position[1] = y;
		camera->position[2] = z;

		camera->yaw = yaw;
		camera->pitch = pitch;
	}

	camera->tick(dt, interactive);
	
	cameraWorldMatrix = camera->getWorldMatrix();
	cameraWorldMatrixOutput.setData2DContiguous(cameraWorldMatrix.m_v, false, 4, 4, 1);
}

void VfxNodeCamera3d::beforeDraw() const
{
	if (isPassthrough)
		return;
	
	camera->pushViewMatrix();
}

void VfxNodeCamera3d::afterDraw() const
{
	if (isPassthrough)
		return;
	
	camera->popViewMatrix();
}
