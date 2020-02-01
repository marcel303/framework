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

#include "framework.h"
#include "vfxNodeTransform2D.h"

VFX_NODE_TYPE(VfxNodeTransform2D)
{
	typeName = "draw.transform2d";
	
	in("any", "draw", "", "draw");
	in("x", "float");
	in("y", "float");
	in("scale", "float", "1");
	in("scaleX", "float", "1");
	in("scaleY", "float", "1");
	in("angle", "float");
	in("angle_norm", "float");
	out("transform", "draw", "draw");
	out("matrix", "channel");
}

VfxNodeTransform2D::VfxNodeTransform2D()
	: VfxNodeBase()
	, matrix()
	, matrixOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Draw, kVfxPlugType_Draw);
	addInput(kInput_X, kVfxPlugType_Float);
	addInput(kInput_Y, kVfxPlugType_Float);
	addInput(kInput_Scale, kVfxPlugType_Float);
	addInput(kInput_ScaleX, kVfxPlugType_Float);
	addInput(kInput_ScaleY, kVfxPlugType_Float);
	addInput(kInput_Angle, kVfxPlugType_Float);
	addInput(kInput_AngleNorm, kVfxPlugType_Float);
	addOutput(kOutput_Draw, kVfxPlugType_Draw, this);
	addOutput(kOutput_Matrix, kVfxPlugType_Channel, &matrixOutput);
}

void VfxNodeTransform2D::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeTransform2D);
	
	if (isPassthrough)
	{
		matrix.MakeIdentity();
		matrixOutput.setData(matrix.m_v, false, 16);
		return;
	}

	const float x = getInputFloat(kInput_X, 0.f);
	const float y = getInputFloat(kInput_Y, 0.f);
	const float scale = getInputFloat(kInput_Scale, 1.f);
	const float scaleX = getInputFloat(kInput_ScaleX, 1.f);
	const float scaleY = getInputFloat(kInput_ScaleY, 1.f);
	const float angle =
		tryGetInput(kInput_AngleNorm)->isConnected()
			? getInputFloat(kInput_AngleNorm, 0.f) * 360.f
			: getInputFloat(kInput_Angle, 0.f);
	
	Mat4x4 t;
	Mat4x4 s;
	Mat4x4 r;
	
	t.MakeTranslation(x, y, 0.f);
	s.MakeScaling(scale * scaleX, scale * scaleY, 1.f);
	r.MakeRotationZ(angle * M_PI / 180.f);
	
	matrix = t * r * s;
	matrixOutput.setData(matrix.m_v, false, 16);
}

void VfxNodeTransform2D::beforeDraw() const
{
	if (isPassthrough)
		return;
	
	gxPushMatrix();
	gxMultMatrixf(matrix.m_v);
}

void VfxNodeTransform2D::afterDraw() const
{
	if (isPassthrough)
		return;
		
	gxPopMatrix();
}
