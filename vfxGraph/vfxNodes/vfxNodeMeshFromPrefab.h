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

#pragma once

#include "gx_mesh.h"
#include "vfxNodeBase.h"

// todo : let DrawMesh node support instancing. channels for position, orientation, scale
// todo : mesh.fromPrefab: add sphere type

struct VfxNodeMeshFromPrefab : VfxNodeBase
{
	enum Type
	{
		kType_Cube,
		kType_Cylinder,
		kType_Circle,
		kType_Rect,
		kType_None
	};

	enum Input
	{
		kInput_Type,
		kInput_Resolution,
		kInput_Scale,
		kInput_ScaleX,
		kInput_ScaleY,
		kInput_ScaleZ,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Mesh,
		kOutput_COUNT
	};

	Type currentType;
	int currentResolution;
	Vec3 currentScale;

	GxVertexBuffer vb;
	GxIndexBuffer ib;
	GxMesh mesh;
	
	VfxNodeMeshFromPrefab();
	virtual ~VfxNodeMeshFromPrefab() override;
	
	virtual void tick(const float dt) override;
};

#include "framework.h"

struct VfxNodeDrawMesh : VfxNodeBase
{
	enum PositionMode
	{
		kPositionMode_Regular,
		kPositionMode_CartesianProduct
	};
	
	enum Input
	{
		kInput_Mesh,
		kInput_PositionMode,
		kInput_PositionX,
		kInput_PositionY,
		kInput_PositionZ,
		kInput_RotationAngle,
		kInput_RotationAxisX,
		kInput_RotationAxisY,
		kInput_RotationAxisZ,
		kInput_Scale,
		kInput_ScaleX,
		kInput_ScaleY,
		kInput_ScaleZ,
		kInput_Shader,
		kInput_Instanced,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Draw,
		kOutput_COUNT
	};
	
	struct Stats
	{
		int numInstancesDrawn = 0;
	};
	
	mutable ShaderBuffer shaderBuffer;
	
	mutable Stats stats;
	
	VfxNodeDrawMesh();
	virtual ~VfxNodeDrawMesh() override;
	
	virtual void tick(const float dt) override;
	
	virtual void draw() const override;
	
	virtual void getDescription(VfxNodeDescription & d) override;
};