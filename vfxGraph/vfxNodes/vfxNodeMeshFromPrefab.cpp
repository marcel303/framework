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
	elem("cylinder");
}

VFX_NODE_TYPE(VfxNodeMeshFromPrefab)
{
	typeName = "mesh.fromPrefab";
	
	inEnum("type", "meshPrefabType");
	in("resolution", "int", "10");
	in("scale", "float", "1");
	out("mesh", "mesh");
}

VfxNodeMeshFromPrefab::VfxNodeMeshFromPrefab()
	: VfxNodeBase()
	, currentType(kType_None)
	, currentResolution(0)
	, currentScale(1.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Type, kVfxPlugType_Int);
	addInput(kInput_Resolution, kVfxPlugType_Int);
	addInput(kInput_Scale, kVfxPlugType_Float);
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
	const float scale = getInputFloat(kInput_Scale, 1.f);

	if (isPassthrough || resolution <= 0)
	{
		currentType = kType_None,
		currentResolution = 0;
		currentScale = 1.f;
		
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
		
		gxCaptureMeshBegin(mesh, vb, ib);
		
		switch (type)
		{
		case kType_None:
			break;
			
		case kType_Cylinder:
			setColor(colorWhite);
			fillCylinder(Vec3(), scale, scale, resolution);
			break;
		}
		
		gxCaptureMeshEnd();
	}
}

//

#include "gx_mesh.h"
#include "data/engine/ShaderCommon.txt"

VFX_NODE_TYPE(VfxNodeDrawMesh)
{
	typeName = "draw.mesh";
	
	in("mesh", "mesh");
	in("pos.x", "channel");
	in("pos.y", "channel");
	in("pos.z", "channel");
	in("shader", "string");
	in("instanced", "bool", "0");
	out("draw", "draw");
}

VfxNodeDrawMesh::VfxNodeDrawMesh()
	: VfxNodeBase()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Mesh, kVfxPlugType_Mesh);
	addInput(kInput_PositionX, kVfxPlugType_Channel);
	addInput(kInput_PositionY, kVfxPlugType_Channel);
	addInput(kInput_PositionZ, kVfxPlugType_Channel);
	addInput(kInput_Shader, kVfxPlugType_String);
	addInput(kInput_Instanced, kVfxPlugType_Bool);
	addOutput(kOutput_Draw, kVfxPlugType_Draw, this);
}

VfxNodeDrawMesh::~VfxNodeDrawMesh()
{
}

void VfxNodeDrawMesh::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeDrawMesh);
	
	if (isPassthrough)
	{
		return;
	}
}

void VfxNodeDrawMesh::draw() const
{
	vfxGpuTimingBlock(VfxNodeDrawMesh);
	
	const GxMesh * mesh = getInputMesh(kInput_Mesh, nullptr);
	
	if (mesh != nullptr)
	{
		const VfxChannel * positionX = getInputChannel(kInput_PositionX, nullptr);
		const VfxChannel * positionY = getInputChannel(kInput_PositionY, nullptr);
		const VfxChannel * positionZ = getInputChannel(kInput_PositionZ, nullptr);
		
		const char * shaderName = getInputString(kInput_Shader, nullptr);
		const bool instanced = getInputBool(kInput_Instanced, false) && shaderName != nullptr;
	
		gxPushMatrix();
		{
			VfxChannelZipper zipper({ positionX, positionY, positionZ });
			
			// compute transforms
			
			const int numTransforms = zipper.size() == 0 ? 1 : zipper.size();
			Mat4x4 * transforms = new Mat4x4[numTransforms];
			
			if (zipper.done())
			{
				transforms[0].MakeIdentity();
			}
			else
			{
				int index = 0;
				
				while (!zipper.done())
				{
					const float x = zipper.read(0, 0.f);
					const float y = zipper.read(1, 0.f);
					const float z = zipper.read(2, 0.f);
					
					Assert(index < numTransforms);
					transforms[index++].MakeTranslation(x, y, z);
					
					zipper.next();
				}
			}
			
			if (shaderName != nullptr)
			{
				Shader shader(shaderName);
				setShader(shader);
				
				ShaderBuffer shaderBuffer;
				
				if (instanced)
				{
					shaderBuffer.setData(transforms, numTransforms * sizeof(transforms[0]));
					shader.setBuffer("transforms", shaderBuffer);
					
					mesh->drawInstanced(numTransforms);
				}
				else
				{
					for (int i = 0; i < numTransforms; ++i)
					{
						gxLoadMatrixf(transforms[i].m_v);
						
						mesh->draw();
					}
				}
				
				clearShader();
			}
			else
			{
				for (int i = 0; i < numTransforms; ++i)
				{
					gxLoadMatrixf(transforms[i].m_v);
					
					mesh->draw();
				}
			}
			
			delete [] transforms;
			transforms = nullptr;
		}
		gxPopMatrix();
	}
}
