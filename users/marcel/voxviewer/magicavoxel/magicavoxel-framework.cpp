#include "framework.h"
#include "magicavoxel-framework.h"

void drawMagicaModel(const MagicaWorld & world, const MagicaModel & model)
{
/*
	todo : determine parity sign, and use it to invert cubes. I tried multiplying the size of the cubes by the parity sign, but this leaves the normals untouched. we need both the winding and normals to be flipped
 
	Mat4x4 modelView;
	gxGetMatrixf(GX_MODELVIEW, modelView.m_v);
	const Vec3 xAxis = modelView.GetAxis(0);
	const Vec3 yAxis = modelView.GetAxis(1);
	const Vec3 zAxis = modelView.GetAxis(2);
	const float paritySign = ((xAxis % yAxis) * zAxis) < 0 ? -1.f : +1.f;
*/

	gxBegin(GX_QUADS);
	{
		MagicaVoxel emptyVoxel;
		emptyVoxel.colorIndex = 0xff;
		
		const float vertices[8][3] =
		{
			{ -.5f, -.5f, -.5f },
			{ +.5f, -.5f, -.5f },
			{ +.5f, +.5f, -.5f },
			{ -.5f, +.5f, -.5f },
			{ -.5f, -.5f, +.5f },
			{ +.5f, -.5f, +.5f },
			{ +.5f, +.5f, +.5f },
			{ -.5f, +.5f, +.5f }
		};

		const int faces[6][4] =
		{
			{ 0, 4, 7, 3 },
			{ 2, 6, 5, 1 },
			{ 0, 1, 5, 4 },
			{ 7, 6, 2, 3 },
			{ 3, 2, 1, 0 },
			{ 4, 5, 6, 7 }
		};

		const float normals[6][3] =
		{
			{ -1,  0,  0 },
			{ +1,  0,  0 },
			{  0, -1,  0 },
			{  0, +1,  0 },
			{  0,  0, -1 },
			{  0,  0, +1 }
		};
		
		const float center_x = (model.sx - 1) / 2.f;
		const float center_y = (model.sy - 1) / 2.f;
		const float center_z = (model.sz - 1) / 2.f;
		
		for (int z = 0; z < model.sz; ++z)
		{
			for (int y = 0; y < model.sy; ++y)
			{
				const MagicaVoxel * __restrict voxel_line = model.getVoxel(0, y, z);
				
				for (int x = 0; x < model.sx; ++x)
				{
					const MagicaVoxel * __restrict voxel = voxel_line + x;
					
					if (voxel->colorIndex == 0xff)
						continue;
					
					const uint8_t * __restrict color = world.palette[voxel->colorIndex];
					
					setColor(color[0], color[1], color[2], color[3]);
					
					const float position_x = x - center_x;
					const float position_y = y - center_y;
					const float position_z = z - center_z;
					
				#if 1
					// optimize : determine whether one of the neighboring cells is empty
					//            if so: draw the face
					//            otherwise: skip it, since it won't be visible
					
					const MagicaVoxel * __restrict neighbors[6] =
					{
						model.getVoxelWithBorder(x - 1, y, z, &emptyVoxel),
						model.getVoxelWithBorder(x + 1, y, z, &emptyVoxel),
						model.getVoxelWithBorder(x, y - 1, z, &emptyVoxel),
						model.getVoxelWithBorder(x, y + 1, z, &emptyVoxel),
						model.getVoxelWithBorder(x, y, z - 1, &emptyVoxel),
						model.getVoxelWithBorder(x, y, z + 1, &emptyVoxel)
					};
				#endif
				
					for (int face_idx = 0; face_idx < 6; ++face_idx)
					{
					#if 1
						if (neighbors[face_idx]->colorIndex != 0xff)
							continue;
					#endif
						
						const float * __restrict normal = normals[face_idx];
						
						gxNormal3fv(normal);
						
						const int * __restrict face = faces[face_idx];
						
						for (int vertex_idx = 0; vertex_idx < 4; ++vertex_idx)
						{
							const float * __restrict vertex = vertices[face[vertex_idx]];
							
							gxVertex3f(
								position_x + vertex[0],
								position_y + vertex[1],
								position_z + vertex[2]);
						}
					}
				}
			}
		}
	}
	gxEnd();
}

void drawMagicaSceneNode(const MagicaWorld & world, const MagicaSceneNodeBase & node)
{
	switch (node.type)
	{
	case MagicaSceneNodeType::Transform:
		{
			auto & transform = static_cast<const MagicaSceneNodeTransform&>(node);
			
			gxPushMatrix();
			{
				if (transform.frames.empty() == false)
				{
					auto & frame = transform.frames[0];
					gxMultMatrixf(frame.m_v);
					
				}
				auto * childNode = world.tryGetNode(transform.childNodeId);
				if (childNode != nullptr)
					drawMagicaSceneNode(world, *childNode);
			}
			gxPopMatrix();
		}
		break;
	
	case MagicaSceneNodeType::Group:
		{
			auto & group = static_cast<const MagicaSceneNodeGroup&>(node);
			
			for (auto & childNodeId : group.childNodeIds)
			{
				auto * childNode = world.tryGetNode(childNodeId);
				if (childNode != nullptr)
					drawMagicaSceneNode(world, *childNode);
			}
		}
		break;
		
	case MagicaSceneNodeType::Shape:
		{
			auto & shape = static_cast<const MagicaSceneNodeShape&>(node);
			
			for (auto & modelId : shape.modelIds)
			{
				auto * model = world.tryGetModel(modelId);
				if (model != nullptr)
					drawMagicaModel(world, *model);
			}
		}
		break;
	}
}

void drawMagicaWorld(const MagicaWorld & world)
{
	if (world.nodes.empty())
	{
		for (auto & model : world.models)
		{
			drawMagicaModel(world, *model);
		}
	}
	else
	{
		auto * rootNode = world.nodes[0];
		drawMagicaSceneNode(world, *rootNode);
	}
}
