// viewer for MagicaVoxel .vox files
// https://github.com/ephtracy/voxel-model/blob/master/MagicaVoxel-file-format-vox.txt
// https://github.com/ephtracy/voxel-model/blob/master/MagicaVoxel-file-format-vox-extension.txt
// https://ephtracy.github.io/

#include "framework.h"
#include "gx_mesh.h"

#include "FileStream.h"
#include "StreamReader.h"

#include "Parse.h" // for VoxDict to transform

#include <map> // for VoxDict
#include <stdint.h>
#include <string.h> // memcmp
#include <vector>

struct VoxDict
{
	std::map<std::string, std::string> items;
};

struct VoxModel
{
	struct Voxel
	{
		uint8_t colorIndex;
	};
	
	int id;
	
	Voxel * voxels = nullptr;
	int sx;
	int sy;
	int sz;
	
	void alloc(int in_sx, int in_sy, int in_sz)
	{
		free();
		
		//
		
		sx = in_sx;
		sy = in_sy;
		sz = in_sz;
		
		int numVoxels = sx * sy * sz;
		
		voxels = new Voxel[numVoxels];
		
		for (int i = 0; i < numVoxels; ++i)
			voxels[i].colorIndex = 0xff;
	}
	
	void free()
	{
		delete [] voxels;
		voxels = nullptr;
		
		sx = 0;
		sy = 0;
		sz = 0;
	}
	
	Voxel * getVoxel(int x, int y, int z) const
	{
		int index = x + y * sx + z * sy * sx;
		
		return voxels + index;
	}
	
	Voxel * getVoxelWithBorder(int x, int y, int z, Voxel * border) const
	{
		bool inside =
			x >= 0 & x < sx &
			y >= 0 & y < sy &
			z >= 0 & z < sz;
		
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

struct VoxMaterial_V2
{
	int id;
	VoxDict dict;
};

struct VoxRotation
{
	int matrix[3][3];
	
	void decode(int v)
	{
		int index1 = (v >> 0) & 0x3;
		int index2 = (v >> 2) & 0x3;
		int index3 = 3 - index1 - index2;
		
		int sign1 = ((v >> 4) & 0x1) ? -1 : +1;
		int sign2 = ((v >> 5) & 0x1) ? -1 : +1;
		int sign3 = ((v >> 6) & 0x1) ? -1 : +1;
		
		memset(matrix, 0, sizeof(matrix));
		matrix[0][index1] = sign1;
		matrix[1][index2] = sign2;
		matrix[2][index3] = sign3;
	}
};

enum struct VoxSceneNodeType
{
	Transform,
	Group,
	Shape
};

struct VoxSceneNodeBase
{
	VoxSceneNodeType type;
	int id;
	VoxDict attributes;
};

struct VoxSceneNodeTransform : VoxSceneNodeBase
{
	int childNodeId;
	int layerId;
	std::vector<Mat4x4> frames;
};

struct VoxSceneNodeGroup : VoxSceneNodeBase
{
	std::vector<int> childNodeIds;
};

struct VoxSceneNodeShape : VoxSceneNodeBase
{
	std::vector<int> modelIds;
};

static const uint32_t default_palette[256] =
{
	0x00000000, 0xffffffff, 0xffccffff, 0xff99ffff, 0xff66ffff, 0xff33ffff, 0xff00ffff, 0xffffccff, 0xffccccff, 0xff99ccff, 0xff66ccff, 0xff33ccff, 0xff00ccff, 0xffff99ff, 0xffcc99ff, 0xff9999ff,
	0xff6699ff, 0xff3399ff, 0xff0099ff, 0xffff66ff, 0xffcc66ff, 0xff9966ff, 0xff6666ff, 0xff3366ff, 0xff0066ff, 0xffff33ff, 0xffcc33ff, 0xff9933ff, 0xff6633ff, 0xff3333ff, 0xff0033ff, 0xffff00ff,
	0xffcc00ff, 0xff9900ff, 0xff6600ff, 0xff3300ff, 0xff0000ff, 0xffffffcc, 0xffccffcc, 0xff99ffcc, 0xff66ffcc, 0xff33ffcc, 0xff00ffcc, 0xffffcccc, 0xffcccccc, 0xff99cccc, 0xff66cccc, 0xff33cccc,
	0xff00cccc, 0xffff99cc, 0xffcc99cc, 0xff9999cc, 0xff6699cc, 0xff3399cc, 0xff0099cc, 0xffff66cc, 0xffcc66cc, 0xff9966cc, 0xff6666cc, 0xff3366cc, 0xff0066cc, 0xffff33cc, 0xffcc33cc, 0xff9933cc,
	0xff6633cc, 0xff3333cc, 0xff0033cc, 0xffff00cc, 0xffcc00cc, 0xff9900cc, 0xff6600cc, 0xff3300cc, 0xff0000cc, 0xffffff99, 0xffccff99, 0xff99ff99, 0xff66ff99, 0xff33ff99, 0xff00ff99, 0xffffcc99,
	0xffcccc99, 0xff99cc99, 0xff66cc99, 0xff33cc99, 0xff00cc99, 0xffff9999, 0xffcc9999, 0xff999999, 0xff669999, 0xff339999, 0xff009999, 0xffff6699, 0xffcc6699, 0xff996699, 0xff666699, 0xff336699,
	0xff006699, 0xffff3399, 0xffcc3399, 0xff993399, 0xff663399, 0xff333399, 0xff003399, 0xffff0099, 0xffcc0099, 0xff990099, 0xff660099, 0xff330099, 0xff000099, 0xffffff66, 0xffccff66, 0xff99ff66,
	0xff66ff66, 0xff33ff66, 0xff00ff66, 0xffffcc66, 0xffcccc66, 0xff99cc66, 0xff66cc66, 0xff33cc66, 0xff00cc66, 0xffff9966, 0xffcc9966, 0xff999966, 0xff669966, 0xff339966, 0xff009966, 0xffff6666,
	0xffcc6666, 0xff996666, 0xff666666, 0xff336666, 0xff006666, 0xffff3366, 0xffcc3366, 0xff993366, 0xff663366, 0xff333366, 0xff003366, 0xffff0066, 0xffcc0066, 0xff990066, 0xff660066, 0xff330066,
	0xff000066, 0xffffff33, 0xffccff33, 0xff99ff33, 0xff66ff33, 0xff33ff33, 0xff00ff33, 0xffffcc33, 0xffcccc33, 0xff99cc33, 0xff66cc33, 0xff33cc33, 0xff00cc33, 0xffff9933, 0xffcc9933, 0xff999933,
	0xff669933, 0xff339933, 0xff009933, 0xffff6633, 0xffcc6633, 0xff996633, 0xff666633, 0xff336633, 0xff006633, 0xffff3333, 0xffcc3333, 0xff993333, 0xff663333, 0xff333333, 0xff003333, 0xffff0033,
	0xffcc0033, 0xff990033, 0xff660033, 0xff330033, 0xff000033, 0xffffff00, 0xffccff00, 0xff99ff00, 0xff66ff00, 0xff33ff00, 0xff00ff00, 0xffffcc00, 0xffcccc00, 0xff99cc00, 0xff66cc00, 0xff33cc00,
	0xff00cc00, 0xffff9900, 0xffcc9900, 0xff999900, 0xff669900, 0xff339900, 0xff009900, 0xffff6600, 0xffcc6600, 0xff996600, 0xff666600, 0xff336600, 0xff006600, 0xffff3300, 0xffcc3300, 0xff993300,
	0xff663300, 0xff333300, 0xff003300, 0xffff0000, 0xffcc0000, 0xff990000, 0xff660000, 0xff330000, 0xff0000ee, 0xff0000dd, 0xff0000bb, 0xff0000aa, 0xff000088, 0xff000077, 0xff000055, 0xff000044,
	0xff000022, 0xff000011, 0xff00ee00, 0xff00dd00, 0xff00bb00, 0xff00aa00, 0xff008800, 0xff007700, 0xff005500, 0xff004400, 0xff002200, 0xff001100, 0xffee0000, 0xffdd0000, 0xffbb0000, 0xffaa0000,
	0xff880000, 0xff770000, 0xff550000, 0xff440000, 0xff220000, 0xff110000, 0xffeeeeee, 0xffdddddd, 0xffbbbbbb, 0xffaaaaaa, 0xff888888, 0xff777777, 0xff555555, 0xff444444, 0xff222222, 0xff111111
};

struct VoxWorld
{
	uint8_t palette[256][4];
	
	std::vector<VoxModel> models;
	
	std::vector<VoxMaterial_V2*> materials;
	
	std::vector<VoxSceneNodeBase*> nodes;
	
	VoxWorld()
	{
		for (int i = 0; i < 256; ++i)
		{
			uint32_t c = default_palette[i];
			uint8_t r = (c >>  0) & 0xff;
			uint8_t g = (c >>  8) & 0xff;
			uint8_t b = (c >> 16) & 0xff;
			uint8_t a = (c >> 24) & 0xff;
			palette[i][0] = r;
			palette[i][1] = g;
			palette[i][2] = b;
			palette[i][3] = a;
		}
	}
	
	~VoxWorld()
	{
		free();
	}
	
	void free()
	{
		for (auto & model : models)
		{
			model.free();
		}
		
		models.clear();
		
		//
		
		for (auto *& material : materials)
		{
			delete material;
			material = nullptr;
		}
		
		materials.clear();
		
		//
		
		for (auto *& node : nodes)
		{
			delete node;
			node = nullptr;
		}
		
		nodes.clear();
	}
	
	const VoxSceneNodeBase * tryGetNode(int id) const
	{
		for (auto * node : nodes)
			if (node->id == id)
				return node;
		
		return nullptr;
	}
	
	const VoxModel * tryGetModel(int id) const
	{
		for (auto & model : models)
			if (model.id == id)
				return &model;
		
		return nullptr;
	}
};

struct vox_chunk_t
{
	char id[4];
	int content_length;
	int children_length;
};

static void read_chunk(StreamReader & r, vox_chunk_t & c)
{
	r.ReadBytes(c.id, 4);
	c.content_length = r.ReadInt32();
	c.children_length = r.ReadInt32();
}

static void skip_chunk(StreamReader & r, vox_chunk_t & c)
{
	r.Stream_get()->Seek(c.content_length + c.children_length, SeekMode_Offset);
}

static bool is_chunk(vox_chunk_t & c, const char * id)
{
	return memcmp(c.id, id, 4) == 0;
}

static bool read_string(StreamReader & r, std::string & s)
{
	int length = r.ReadInt32();
	
	if (length < 0 || length > 0xffff)
		return false;

	s.resize(length);
	r.ReadBytes((void*)s.data(), length);
	
	return true;
}

static bool read_dict(StreamReader & r, VoxDict & dict)
{
	int count = r.ReadInt32();
	
	for (int i = 0; i < count; ++i)
	{
		std::string key;
		std::string value;
		
		if (!read_string(r, key) ||
			!read_string(r, value))
			return false;
		
		dict.items[key] = value;
	}
	
	return true;
}

extern void splitString(const std::string & str, std::vector<std::string> & result, char c);

static bool read_vox_world(StreamReader & r, VoxWorld & world)
{
	try
	{
		char id[4];
		r.ReadBytes(id, 4);
		
		int version = r.ReadInt32();

		if (memcmp(id, "VOX ", 4) != 0)
			return false;
		
		if (version != 150)
			logWarning("warning: VOX file format version %d may not be supported", version);

		vox_chunk_t c;
		read_chunk(r, c);
		
		if (!is_chunk(c, "MAIN"))
			return false;
		
		for (;;)
		{
			if (r.Stream_get()->EOF_get())
				break;
			
			read_chunk(r, c);
			
			if (is_chunk(c, "PACK"))
			{
				int numModels = r.ReadInt32();
				(void)numModels;
			}
			else if (is_chunk(c, "SIZE"))
			{
				int sx = r.ReadInt32();
				int sy = r.ReadInt32();
				int sz = r.ReadInt32();
				
				read_chunk(r, c);
				
				if (!is_chunk(c, "XYZI"))
					return false;
				
				VoxModel model;
				model.id = world.models.size();
				model.alloc(sx, sy, sz);
				
				int numVoxels = r.ReadInt32();
				
				logDebug("model %d: numVoxels=%d", model.id, numVoxels);
				
				for (int i = 0; i < numVoxels; ++i)
				{
					uint8_t x = r.ReadUInt8();
					uint8_t y = r.ReadUInt8();
					uint8_t z = r.ReadUInt8();
					uint8_t colorIndex = r.ReadUInt8();
					
					VoxModel::Voxel * voxel = model.getVoxel(x, y, z);
					
					voxel->colorIndex = colorIndex;
				}
				
				world.models.push_back(model);
			}
			else if (is_chunk(c, "RGBA"))
			{
				for (int i = 0; i < 255; ++i)
					r.ReadBytes(world.palette[i + 1], 4);
				
				uint8_t dummy[4];
				r.ReadBytes(dummy, 4);
			}
			else if (is_chunk(c, "MATL"))
			{
				// material v2 chunk
				// see: https://github.com/ephtracy/voxel-model/issues/19
				
				int id = r.ReadInt32();

				VoxDict attributes;
				
				if (!read_dict(r, attributes))
					return false;
				
				VoxMaterial_V2 * m = new VoxMaterial_V2();
				m->id = id;
				m->dict = attributes;
				
				world.materials.push_back(m);
			}
			else if (is_chunk(c, "nTRN"))
			{
				int id = r.ReadInt32();
				
				VoxDict attributes;
				
				if (!read_dict(r, attributes))
					return false;
				
				int childNodeId = r.ReadInt32();
				int reserved = r.ReadInt32();
				(void)reserved;
				int layerId = r.ReadInt32();
				int numFrames = r.ReadInt32();
				
				std::vector<VoxDict> frames;
				
				for (int i = 0; i < numFrames; ++i)
				{
					VoxDict frameAttributes;
					
					if (!read_dict(r, frameAttributes))
						return false;
					
					frames.push_back(frameAttributes);
				}
				
				VoxSceneNodeTransform * node = new VoxSceneNodeTransform();
				node->type = VoxSceneNodeType::Transform;
				node->id = id;
				node->attributes = attributes;
				node->childNodeId = childNodeId;
				node->layerId = layerId;
				for (auto & frame : frames)
				{
					auto & r_text = frame.items["_r"];
					auto & t_text = frame.items["_t"];
					std::vector<std::string> t_elems;
					splitString(t_text, t_elems, ' ');
					int r_encoded = Parse::Int32(r_text);
					Mat4x4 m(true);
					if (r_encoded != 0)
					{
						VoxRotation r;
						r.decode(r_encoded);
						for (int i = 0; i < 3; ++i)
							for (int j = 0; j < 3; ++j)
								m(j, i) = r.matrix[i][j];
					}
					float t[3];
					t[0] = t_elems.size() >= 3 ? Parse::Float(t_elems[0].c_str()) : 0.f;
					t[1] = t_elems.size() >= 3 ? Parse::Float(t_elems[1].c_str()) : 0.f;
					t[2] = t_elems.size() >= 3 ? Parse::Float(t_elems[2].c_str()) : 0.f;
					m.SetTranslation(t[0], t[1], t[2]);
					node->frames.push_back(m);
				}
				
				world.nodes.push_back(node);
			}
			else if (is_chunk(c, "nGRP"))
			{
				int id = r.ReadInt32();
				
				VoxDict attributes;
				
				if (!read_dict(r, attributes))
					return false;
				
				int numChildren = r.ReadInt32();
				
				std::vector<int> childNodeIds;
				
				for (int i = 0; i < numChildren; ++i)
				{
					int childNodeId = r.ReadInt32();
					childNodeIds.push_back(childNodeId);
				}
				
				VoxSceneNodeGroup * node = new VoxSceneNodeGroup();
				node->type =VoxSceneNodeType::Group;
				node->id = id;
				node->attributes = attributes;
				node->childNodeIds = childNodeIds;
				
				world.nodes.push_back(node);
			}
			else if (is_chunk(c, "nSHP"))
			{
				int id = r.ReadInt32();
				
				VoxDict attributes;
				
				if (!read_dict(r, attributes))
					return false;
				
				int numModels = r.ReadInt32();
				
				std::vector<int> modelIds;
				
				for (int i = 0; i < numModels; ++i)
				{
					int modelId = r.ReadInt32();
					
					VoxDict attributes;
					
					if (!read_dict(r, attributes))
						return false;
					
					modelIds.push_back(modelId);
				}
				
				VoxSceneNodeShape * node = new VoxSceneNodeShape();
				node->type = VoxSceneNodeType::Shape;
				node->id = id;
				node->attributes = attributes;
				node->modelIds = modelIds;
				
				world.nodes.push_back(node);
			}
			else
			{
				logDebug("unknown chunk id: %.4s", c.id);
				
				skip_chunk(r, c);
			}
		}
		
		return true;
	}
	catch (std::exception & e)
	{
		logError("failed to read VOX file: %s", e.what());
		
		return false;
	}
}

static void drawVoxModel(const VoxWorld & world, const VoxModel & model)
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

	beginCubeBatch();
	{
		VoxModel::Voxel emptyVoxel;
		emptyVoxel.colorIndex = 0xff;
		
		for (int x = 0; x < model.sx; ++x)
		{
			for (int y = 0; y < model.sy; ++y)
			{
				for (int z = 0; z < model.sz; ++z)
				{
					const VoxModel::Voxel * voxel = model.getVoxel(x, y, z);
					
					if (voxel->colorIndex == 0xff)
						continue;
					
				#if 1
					// optimize : determine whether one of the neighboring cells is empty
					//            if so: draw the voxel
					//            otherwise: skip it, since it won't be visible
					
					const VoxModel::Voxel * neighbors[6] =
					{
						model.getVoxelWithBorder(x - 1, y, z, &emptyVoxel),
						model.getVoxelWithBorder(x + 1, y, z, &emptyVoxel),
						model.getVoxelWithBorder(x, y - 1, z, &emptyVoxel),
						model.getVoxelWithBorder(x, y + 1, z, &emptyVoxel),
						model.getVoxelWithBorder(x, y, z - 1, &emptyVoxel),
						model.getVoxelWithBorder(x, y, z + 1, &emptyVoxel)
					};
					
					bool visible =
						neighbors[0]->colorIndex == 0xff |
						neighbors[1]->colorIndex == 0xff |
						neighbors[2]->colorIndex == 0xff |
						neighbors[3]->colorIndex == 0xff |
						neighbors[4]->colorIndex == 0xff |
						neighbors[5]->colorIndex == 0xff;
					
					if (visible == false)
						continue;
				#endif
				
					const uint8_t * color = world.palette[voxel->colorIndex];
					
					setColor(color[0], color[1], color[2], color[3]);
					
					fillCube(
						Vec3(
							x - (model.sx - 1) / 2.f,
							y - (model.sy - 1) / 2.f,
							z - (model.sz - 1) / 2.f), Vec3(.5f, .5f, .5f));
				}
			}
		}
	}
	endCubeBatch();
}

static void drawVoxSceneNode(const VoxWorld & world, const VoxSceneNodeBase & node)
{
	switch (node.type)
	{
	case VoxSceneNodeType::Transform:
		{
			auto & transform = static_cast<const VoxSceneNodeTransform&>(node);
			
			gxPushMatrix();
			{
				if (transform.frames.empty() == false)
				{
					auto & frame = transform.frames[0];
					gxMultMatrixf(frame.m_v);
					
				}
				auto * childNode = world.tryGetNode(transform.childNodeId);
				if (childNode != nullptr)
					drawVoxSceneNode(world, *childNode);
			}
			gxPopMatrix();
		}
		break;
	
	case VoxSceneNodeType::Group:
		{
			auto & group = static_cast<const VoxSceneNodeGroup&>(node);
			
			for (auto & childNodeId : group.childNodeIds)
			{
				auto * childNode = world.tryGetNode(childNodeId);
				if (childNode != nullptr)
					drawVoxSceneNode(world, *childNode);
			}
		}
		break;
		
	case VoxSceneNodeType::Shape:
		{
			auto & shape = static_cast<const VoxSceneNodeShape&>(node);
			
			for (auto & modelId : shape.modelIds)
			{
				auto * model = world.tryGetModel(modelId);
				if (model != nullptr)
					drawVoxModel(world, *model);
			}
		}
		break;
	}
}

static void drawVoxWorld(const VoxWorld & world)
{
	if (world.nodes.empty())
	{
		for (auto & model : world.models)
		{
			drawVoxModel(world, model);
		}
	}
	else
	{
		auto * rootNode = world.nodes[0];
		drawVoxSceneNode(world, *rootNode);
	}
}

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableDepthBuffer = true;
	framework.enableRealTimeEditing = true;
	framework.filedrop = true;
	
	if (!framework.init(800, 600))
		return 0;

	VoxWorld world;
	
	try
	{
		//FileStream stream("marcel-01.vox", OpenMode_Read);
		//FileStream stream("monu7.vox", OpenMode_Read);
		FileStream stream("room.vox", OpenMode_Read);
		StreamReader reader(&stream, false);
		
		read_vox_world(reader, world);
	}
	catch (std::exception & e)
	{
		logError("error: %s", e.what());
	}

	Camera3d camera;
	
	GxMesh drawMesh;
	GxVertexBuffer vb;
	GxIndexBuffer ib;
	
	gxCaptureMeshBegin(drawMesh, vb, ib);
	{
		drawVoxWorld(world);
	}
	gxCaptureMeshEnd();
	
	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;

		for (auto & file : framework.droppedFiles)
		{
			try
			{
				world.free();
				
				FileStream stream(file.c_str(), OpenMode_Read);
				StreamReader reader(&stream, false);
				
				read_vox_world(reader, world);
			}
			catch (std::exception & e)
			{
				logError("error: %s", e.what());
			}
			
			gxCaptureMeshBegin(drawMesh, vb, ib);
			{
				drawVoxWorld(world);
			}
			gxCaptureMeshEnd();
		}
		
		camera.tick(framework.timeStep, true);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			projectPerspective3d(90.f, .01f, 100.f);
			pushDepthTest(true, DEPTH_LESS);
			
			camera.pushViewMatrix();
			{
				Shader shader("shader");
				setShader(shader);
				
				const Vec4 lightDirection_world(1.f, -2.f, .5f, 0);
				const Vec4 lightDirection_view = transformToWorld(lightDirection_world).CalcNormalized();
				shader.setImmediate("lightDirection", lightDirection_view[0], lightDirection_view[1], lightDirection_view[2]);
			
				gxScalef(-1, 1, 1);
				gxRotatef(-90, 1, 0, 0);
				gxScalef(.1f, .1f, .1f);
				
			#if 1
				drawMesh.draw();
			#else
				drawVoxWorld(world);
			#endif
				
				clearShader();
			}
			camera.popViewMatrix();
			
			popDepthTest();
		}
		framework.endDraw();
	}

	return 0;
}
