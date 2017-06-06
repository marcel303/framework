#include "framework.h"
#include "vfxNodeTransform2D.h"

#include "Calc.h"

VfxNodeTransform2D::VfxNodeTransform2D()
	: VfxNodeBase()
	, transform()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Any, kVfxPlugType_DontCare);
	addInput(kInput_X, kVfxPlugType_Float);
	addInput(kInput_Y, kVfxPlugType_Float);
	addInput(kInput_Scale, kVfxPlugType_Float);
	addInput(kInput_ScaleX, kVfxPlugType_Float);
	addInput(kInput_ScaleY, kVfxPlugType_Float);
	addInput(kInput_Angle, kVfxPlugType_Float);
	addOutput(kOutput_Transform, kVfxPlugType_Transform, &transform);
}

void VfxNodeTransform2D::initSelf(const GraphNode & node)
{
	// todo : parse node.editorValue;
}

void VfxNodeTransform2D::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeTransform2D);
	
	const float x = getInputFloat(kInput_X, 0.f);
	const float y = getInputFloat(kInput_Y, 0.f);
	const float scale = getInputFloat(kInput_Scale, 1.f);
	const float scaleX = getInputFloat(kInput_ScaleX, 1.f);
	const float scaleY = getInputFloat(kInput_ScaleY, 1.f);
	const float angle = getInputFloat(kInput_Angle, 0.f);
	
	Mat4x4 t;
	Mat4x4 s;
	Mat4x4 r;
	
	t.MakeTranslation(x, y, 0.f);
	s.MakeScaling(scale * scaleX, scale * scaleY, 1.f);
	r.MakeRotationZ(Calc::DegToRad(angle));
	
	transform.matrix = t * r * s;
}

void VfxNodeTransform2D::beforeDraw() const
{
	gxPushMatrix();
	gxMultMatrixf(transform.matrix.m_v);
}

void VfxNodeTransform2D::afterDraw() const
{
	gxPopMatrix();
}
