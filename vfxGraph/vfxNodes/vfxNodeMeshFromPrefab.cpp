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
#include "vfxNodeMeshFromPrefab.h"

VFX_ENUM_TYPE(meshPrefabType)
{
	elem("cube", VfxNodeMeshFromPrefab::kType_Cube);
	elem("cylinder", VfxNodeMeshFromPrefab::kType_Cylinder);
	elem("circle", VfxNodeMeshFromPrefab::kType_Circle);
	elem("rect", VfxNodeMeshFromPrefab::kType_Rect);
}

VFX_NODE_TYPE(VfxNodeMeshFromPrefab)
{
	typeName = "mesh.fromPrefab";
	
	inEnum("type", "meshPrefabType");
	in("resolution", "int", "10");
	in("scale", "float", "1");
	in("scale.x", "float", "1");
	in("scale.y", "float", "1");
	in("scale.z", "float", "1");
	out("mesh", "mesh");
}

VfxNodeMeshFromPrefab::VfxNodeMeshFromPrefab()
	: VfxNodeBase()
	, currentType(kType_None)
	, currentResolution(0)
	, currentScale(1, 1, 1)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Type, kVfxPlugType_Int);
	addInput(kInput_Resolution, kVfxPlugType_Int);
	addInput(kInput_Scale, kVfxPlugType_Float);
	addInput(kInput_ScaleX, kVfxPlugType_Float);
	addInput(kInput_ScaleY, kVfxPlugType_Float);
	addInput(kInput_ScaleZ, kVfxPlugType_Float);
	addOutput(kOutput_Mesh, kVfxPlugType_Mesh, &mesh);
}

VfxNodeMeshFromPrefab::~VfxNodeMeshFromPrefab()
{
}

void VfxNodeMeshFromPrefab::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeMeshFromPrefab);
	
	const Type type = (Type)getInputInt(kInput_Type, kType_Cylinder);
	const int resolution = getInputInt(kInput_Resolution, 10);
	const float scaleX = getInputFloat(kInput_ScaleX, 1.f);
	const float scaleY = getInputFloat(kInput_ScaleY, 1.f);
	const float scaleZ = getInputFloat(kInput_ScaleZ, 1.f);
	const Vec3 scale = Vec3(scaleX, scaleY, scaleZ) * getInputFloat(kInput_Scale, 1.f);

	if (isPassthrough || resolution <= 0)
	{
		currentType = kType_None,
		currentResolution = 0;
		currentScale.Set(1, 1, 1);
		
		//
		
		vb.free();
		ib.free();
		mesh.clear();
		
		return;
	}

	if (type != currentType || resolution != currentResolution || scale != currentScale)
	{
		currentType = type;
		currentResolution = resolution;
		currentScale = scale;

		// generate mesh
		
		vb.free();
		ib.free();
		mesh.clear();
		
		gxPushMatrix();
		gxScalef(scale[0], scale[1], scale[2]);
		
		gxCaptureMeshBegin(mesh, vb, ib);
		
		setColor(colorWhite);
		
		switch (type)
		{
		case kType_None:
			break;
		
		case kType_Cube:
			fillCube(Vec3(), Vec3(1, 1, 1));
			break;
			
		case kType_Cylinder:
			fillCylinder(Vec3(), 1, 1, resolution);
			break;
			
		case kType_Circle:
			gxRotatef(90.f, 1, 0, 0);
			fillCircle(0, 0, 1, resolution);
			break;
			
		case kType_Rect:
			drawGrid3d(1, 1, 0, 2);
			break;
		}
		
		gxCaptureMeshEnd();
		gxPopMatrix();
	}
}
