#pragma once

#include "Mat4x4.h"
#include <map> // for MagicaDict
#include <stdint.h>
#include <vector>

struct MagicaDict
{
	std::map<std::string, std::string> items;
};

struct MagicaVoxel
{
	uint8_t colorIndex;
};

struct MagicaModel
{
	int id = -1;
	
	MagicaVoxel * voxels = nullptr;
	int sx = 0;
	int sy = 0;
	int sz = 0;
	
	~MagicaModel();

	void alloc(int sx, int sy, int sz);
	void free();
	
	MagicaVoxel * getVoxel(int x, int y, int z) const
	{
		int index = x + y * sx + z * sy * sx;
		
		return voxels + index;
	}
	
	MagicaVoxel * getVoxelWithBorder(int x, int y, int z, MagicaVoxel * border) const
	{
		bool inside =
			(x >= 0 & x < sx) &
			(y >= 0 & y < sy) &
			(z >= 0 & z < sz);
		
		if (inside)
		{
			int index = x + y * sx + z * sy * sx;
			
			return voxels + index;
		}
		else
		{
			return border;
		}
	}
};

struct MagicaMaterial_V2
{
	int id = -1;
	
	MagicaDict dict;
};

struct MagicaRotation
{
	int matrix[3][3];
	
	MagicaRotation();
	
	void decode(int v);
};

enum struct MagicaSceneNodeType
{
	None,
	Transform,
	Group,
	Shape
};

struct MagicaSceneNodeBase
{
	MagicaSceneNodeType type = MagicaSceneNodeType::None;
	
	int id = -1;
	
	MagicaDict attributes;
};

struct MagicaSceneNodeTransform : MagicaSceneNodeBase
{
	int childNodeId = -1;
	int layerId = -1;
	
	std::vector<Mat4x4> frames;
};

struct MagicaSceneNodeGroup : MagicaSceneNodeBase
{
	std::vector<int> childNodeIds;
};

struct MagicaSceneNodeShape : MagicaSceneNodeBase
{
	std::vector<int> modelIds;
};

struct MagicaWorld
{
	uint8_t palette[256][4];
	
	std::vector<MagicaModel*> models;
	
	std::vector<MagicaMaterial_V2*> materials;
	
	std::vector<MagicaSceneNodeBase*> nodes;
	
	MagicaWorld();
	~MagicaWorld();
	
	void free();
	
	const MagicaSceneNodeBase * tryGetNode(int id) const;
	const MagicaModel * tryGetModel(int id) const;
};

// -- io --

class StreamReader;

bool readMagicaWorld(StreamReader & r, MagicaWorld & world);
bool readMagicaWorld(const char * filename, MagicaWorld & world);
