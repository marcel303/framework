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

//

#include "Calc.h"
#include "gx_mesh.h"
#include "vfxChannelZipper.h"

VFX_ENUM_TYPE(drawMeshPositionMode)
{
	elem("regular", VfxNodeDrawMesh::kPositionMode_Regular);
	elem("cartesian", VfxNodeDrawMesh::kPositionMode_CartesianProduct);
}

VFX_NODE_TYPE(VfxNodeDrawMesh)
{
	typeName = "draw.mesh";
	
	in("mesh", "mesh");
	inEnum("pos.mode", "drawMeshPositionMode", "regular");
	in("pos.x", "channel");
	in("pos.y", "channel");
	in("pos.z", "channel");
	in("rot.angle", "channel", "0");
	in("rot.axis.x", "channel", "1");
	in("rot.axis.y", "channel", "0");
	in("rot.axis.z", "channel", "0");
	in("scale", "channel");
	in("scale.x", "channel");
	in("scale.y", "channel");
	in("scale.z", "channel");
	in("shader", "string");
	in("instanced", "bool", "0");
	out("draw", "draw");
}

VfxNodeDrawMesh::VfxNodeDrawMesh()
	: VfxNodeBase()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Mesh, kVfxPlugType_Mesh);
	addInput(kInput_PositionMode, kVfxPlugType_Int);
	addInput(kInput_PositionX, kVfxPlugType_Channel);
	addInput(kInput_PositionY, kVfxPlugType_Channel);
	addInput(kInput_PositionZ, kVfxPlugType_Channel);
	addInput(kInput_RotationAngle, kVfxPlugType_Channel);
	addInput(kInput_RotationAxisX, kVfxPlugType_Channel);
	addInput(kInput_RotationAxisY, kVfxPlugType_Channel);
	addInput(kInput_RotationAxisZ, kVfxPlugType_Channel);
	addInput(kInput_Scale, kVfxPlugType_Channel);
	addInput(kInput_ScaleX, kVfxPlugType_Channel);
	addInput(kInput_ScaleY, kVfxPlugType_Channel);
	addInput(kInput_ScaleZ, kVfxPlugType_Channel);
	addInput(kInput_Shader, kVfxPlugType_String);
	addInput(kInput_Instanced, kVfxPlugType_Bool);
	addOutput(kOutput_Draw, kVfxPlugType_Draw, this);
}

VfxNodeDrawMesh::~VfxNodeDrawMesh()
{
	shaderBuffer.free();
}

void VfxNodeDrawMesh::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeDrawMesh);
	
	if (isPassthrough)
	{
		shaderBuffer.free();
		stats = Stats();
		return;
	}
}

void VfxNodeDrawMesh::draw() const
{
	if (isPassthrough)
		return;
	
	vfxGpuTimingBlock(VfxNodeDrawMesh);
	
	const GxMesh * mesh = getInputMesh(kInput_Mesh, nullptr);
	
	if (mesh != nullptr)
	{
		const PositionMode positionMode = (PositionMode)getInputInt(kInput_PositionMode, kPositionMode_Regular);
		
		const VfxChannel * positionX = getInputChannel(kInput_PositionX, nullptr);
		const VfxChannel * positionY = getInputChannel(kInput_PositionY, nullptr);
		const VfxChannel * positionZ = getInputChannel(kInput_PositionZ, nullptr);
		
		const VfxChannel * rotationAngle = getInputChannel(kInput_RotationAngle, nullptr);
		const VfxChannel * rotationAxisX = getInputChannel(kInput_RotationAxisX, nullptr);
		const VfxChannel * rotationAxisY = getInputChannel(kInput_RotationAxisY, nullptr);
		const VfxChannel * rotationAxisZ = getInputChannel(kInput_RotationAxisZ, nullptr);
		
		const VfxChannel * scale = getInputChannel(kInput_Scale, nullptr);
		const VfxChannel * scaleX = getInputChannel(kInput_ScaleX, nullptr);
		const VfxChannel * scaleY = getInputChannel(kInput_ScaleY, nullptr);
		const VfxChannel * scaleZ = getInputChannel(kInput_ScaleZ, nullptr);
		
		const char * shaderName = getInputString(kInput_Shader, nullptr);
		const bool instanced = getInputBool(kInput_Instanced, false) && shaderName != nullptr;
	
		gxPushMatrix();
		{
			VfxChannelData positionData;
			VfxChannel positionX_zipped;
			VfxChannel positionY_zipped;
			VfxChannel positionZ_zipped;
			
			if (positionMode == kPositionMode_CartesianProduct)
			{
				// zip the position channels into its cartesian product
				
				VfxChannelZipper_Cartesian zipper_position(
					{
						positionX,
						positionY,
						positionZ
					});
				
				const int numPositions = zipper_position.size();
				positionData.alloc(numPositions * 3);
				
				positionX_zipped.setData(positionData.data + 0 * numPositions, false, numPositions);
				positionY_zipped.setData(positionData.data + 1 * numPositions, false, numPositions);
				positionZ_zipped.setData(positionData.data + 2 * numPositions, false, numPositions);

				int positionIndex = 0;

				while (!zipper_position.done())
				{
					positionX_zipped.dataRw()[positionIndex] = zipper_position.read(0, 0.f);
					positionY_zipped.dataRw()[positionIndex] = zipper_position.read(1, 0.f);
					positionZ_zipped.dataRw()[positionIndex] = zipper_position.read(2, 0.f);
					
					positionIndex++;
					
					zipper_position.next();
				}
				
				Assert(positionIndex == numPositions);
				
				// zip the cartesian product with the rest of the channels
				
				positionX = &positionX_zipped;
				positionY = &positionY_zipped;
				positionZ = &positionZ_zipped;
			}
			
			VfxChannelZipper zipper(
				{
					positionX,
					positionY,
					positionZ,
					rotationAngle,
					rotationAxisX,
					rotationAxisY,
					rotationAxisZ,
					scale,
					scaleX,
					scaleY,
					scaleZ
				});
			
			// compute transforms
			
			const int numTransforms = (zipper.size() == 0) ? 1 : zipper.size();
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
					
					const float rotation_angle = zipper.read(3, 0.f);
					const float rorationAxis_x = zipper.read(4, 1.f);
					const float rorationAxis_y = zipper.read(5, 0.f);
					const float rorationAxis_z = zipper.read(6, 0.f);
					
					const float scale = zipper.read(7, 1.f);
					const float scale_x = zipper.read(8, 1.f) * scale;
					const float scale_y = zipper.read(9, 1.f) * scale;
					const float scale_z = zipper.read(10, 1.f) * scale;
					
					Assert(index < numTransforms);
					transforms[index++] =
						Mat4x4(true)
						.Translate(x, y, z)
						.Rotate(
							Calc::DegToRad(rotation_angle),
							Vec3(
								rorationAxis_x,
								rorationAxis_y,
								rorationAxis_z))
						.Scale(scale_x, scale_y, scale_z);
					
					zipper.next();
				}
				
				Assert(index == numTransforms);
			}
			
			if (shaderName != nullptr)
			{
				Shader shader(shaderName);
				setShader(shader);
				{
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
							gxPushMatrix();
							{
								gxMultMatrixf(transforms[i].m_v);
								
								mesh->draw();
							}
							gxPopMatrix();
						}
					}
				}
				clearShader();
			}
			else
			{
				for (int i = 0; i < numTransforms; ++i)
				{
					gxPushMatrix();
					{
						gxMultMatrixf(transforms[i].m_v);
					
						mesh->draw();
					}
					gxPopMatrix();
				}
			}
			
			delete [] transforms;
			transforms = nullptr;
			
			stats.numInstancesDrawn = numTransforms;
		}
		gxPopMatrix();
	}
}

void VfxNodeDrawMesh::getDescription(VfxNodeDescription & d)
{
	d.add("numInstancesDrawn: %d", stats.numInstancesDrawn);
}
