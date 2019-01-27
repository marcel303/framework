#include <GL/glew.h> // GL_CULL_FACE. todo : add functions to control culling mode to Framework
#include "framework.h"
#include "json.hpp"
#include "Path.h"
#include "TextIO.h"

#include "data/engine/ShaderCommon.txt"

#include <SDL2/SDL_opengl.h>

class GxVertexBuffer
{
	friend class GxMesh;
	
	GxShaderBufferId m_vertexArray;
	
public:
	GxVertexBuffer()
		: m_vertexArray(0)
	{
		// create buffer
		glGenBuffers(1, &m_vertexArray);
		checkErrorGL();
	}
	
	~GxVertexBuffer()
	{
		// free buffer
		glDeleteBuffers(1, &m_vertexArray);
		m_vertexArray = 0;
		checkErrorGL();
	}
	
	void setData(const void * bytes, const int numBytes)
	{
		// fill the buffer with data
		glBindBuffer(GL_ARRAY_BUFFER, m_vertexArray);
		glBufferData(GL_ARRAY_BUFFER, numBytes, bytes, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		checkErrorGL();
	}
};

enum GX_INDEX_FORMAT
{
	GX_INDEX_16, // indices are 16-bits (uint16_t)
	GX_INDEX_32  // indices are 32-bits (uint32_t)
};

class GxIndexBuffer
{
	friend class GxMesh;
	
	GxShaderBufferId m_indexArray;
	int m_numIndices;
	GX_INDEX_FORMAT m_format;
	
public:
	GxIndexBuffer()
		: m_indexArray(0)
		, m_numIndices(0)
		, m_format(GX_INDEX_16)
	{
		// create buffer
		glGenBuffers(1, &m_indexArray);
		checkErrorGL();
	}
	
	~GxIndexBuffer()
	{
		// free buffer
		glDeleteBuffers(1, &m_indexArray);
		m_indexArray = 0;
		checkErrorGL();
	}
	
	void setData(const void * bytes, const int numIndices, const GX_INDEX_FORMAT format)
	{
		// fill the buffer with data
		
		const int numBytes =
			format == GX_INDEX_16
			? numIndices * 2
			: numIndices * 4;
		
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexArray);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, numBytes, bytes, GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		checkErrorGL();
		
		m_numIndices = numIndices;
		m_format = format;
	}
	
	int getNumIndices() const
	{
		return m_numIndices;
	}
	
	GX_INDEX_FORMAT getFormat() const
	{
		return m_format;
	}
};

struct GxVertexInput
{
	int id;
	int components;
	int type;
	bool normalize;
	int offset;
	int stride;
};

class GxMesh
{
	uint32_t m_vertexArrayObject;
	
	const GxVertexBuffer * m_vertexBuffer;
	const GxIndexBuffer * m_indexBuffer;
	
	void bindVsInputs(const GxVertexInput * vsInputs, const int numVsInputs);
	
public:
	GxMesh()
		: m_vertexArrayObject(0)
		, m_vertexBuffer(nullptr)
		, m_indexBuffer(nullptr)
	{
		// create vertex array object
		glGenVertexArrays(1, &m_vertexArrayObject);
		checkErrorGL();
	}
	
	~GxMesh()
	{
		// free vertex array object
		glDeleteVertexArrays(1, &m_vertexArrayObject);
		m_vertexArrayObject = 0;
		checkErrorGL();
	}
	
	void setVertexBuffer(const GxVertexBuffer * buffer, const GxVertexInput * vertexInputs, const int numVertexInputs);
	void setIndexBuffer(const GxIndexBuffer * buffer);
	
	void draw() const;
};

void GxMesh::bindVsInputs(const GxVertexInput * vsInputs, const int numVsInputs)
{
	checkErrorGL();

	for (int i = 0; i < numVsInputs; ++i)
	{
		//logDebug("i=%d, id=%d, num=%d, type=%d, norm=%d, stride=%d, offset=%p\n", i, vsInputs[i].id, vsInputs[i].components, vsInputs[i].type, vsInputs[i].normalize, stride, (void*)vsInputs[i].offset);
		
		glEnableVertexAttribArray(vsInputs[i].id);
		checkErrorGL();
		
		glVertexAttribPointer(vsInputs[i].id, vsInputs[i].components, vsInputs[i].type, vsInputs[i].normalize, vsInputs[i].stride, (void*)(intptr_t)vsInputs[i].offset);
		checkErrorGL();
	}
}

void GxMesh::setVertexBuffer(const GxVertexBuffer * buffer, const GxVertexInput * vertexInputs, const int numVertexInputs)
{
	m_vertexBuffer = buffer;
	
	Assert(m_vertexArrayObject != 0);
	glBindVertexArray(m_vertexArrayObject);
	checkErrorGL();
	{
		Assert(m_vertexBuffer->m_vertexArray != 0);
		glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer->m_vertexArray);
		checkErrorGL();
		
		bindVsInputs(vertexInputs, numVertexInputs);
	}
	glBindVertexArray(0);
	checkErrorGL();
}

void GxMesh::setIndexBuffer(const GxIndexBuffer * buffer)
{
	m_indexBuffer = buffer;
	
	Assert(m_vertexArrayObject != 0);
	glBindVertexArray(m_vertexArrayObject);
	checkErrorGL();
	{
		Assert(m_indexBuffer->m_indexArray != 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer->m_indexArray);
		checkErrorGL();
	}
	glBindVertexArray(0);
	checkErrorGL();
}

void GxMesh::draw() const
{
	gxValidateMatrices();
	
	const int numIndices = m_indexBuffer->getNumIndices();
	
	// bind vertex arrays

	fassert(m_vertexBuffer && m_vertexBuffer->m_vertexArray);
	fassert(m_indexBuffer && m_indexBuffer->m_indexArray);
	glBindVertexArray(m_vertexArrayObject);
	checkErrorGL();

	GLenum indexType =
		m_indexBuffer->getFormat() == GX_INDEX_16
		? GL_UNSIGNED_SHORT
		: GL_UNSIGNED_INT;
	
	glDrawElements(GL_TRIANGLES, numIndices, indexType, 0);
	checkErrorGL();

	glBindVertexArray(0);
	checkErrorGL();
}

using json = nlohmann::json;

namespace gltf
{
	const int kMode_Triangles = 0x0004;
	const int kElementType_U32 = 0x1405;
	
	struct Buffer
	{
		std::string uri;
		int byteLength = -1;
		std::vector<uint8_t> data;
		
		bool isValid() const
		{
			return
				!uri.empty() &&
				byteLength >= 0;
		}
	};
	
	struct BufferView
	{
		int buffer = -1;
		int byteLength = -1;
		int byteOffset = -1;
		std::string name;
		int target = -1;
		
		bool isValid() const
		{
			return
				buffer >= 0 &&
				byteLength >= 0 &&
				byteOffset >= 0 &&
				target >= 0;
		}
	};
	
	struct Accessor
	{
		int bufferView = -1;
		int byteOffset = -1;
		int componentType = -1;
		int count = -1;
		std::vector<float> min;
		std::vector<float> max;
		std::string type;
		
		bool isValid() const
		{
			return
				bufferView >= 0 &&
				byteOffset >= 0 &&
				componentType >= 0 &&
				count >= 0 &&
				!type.empty();
		}
	};
	
	struct Image
	{
		std::string uri;
		
		std::string path;
		
		bool isValid() const
		{
			return
				!uri.empty();
		}
	};
	
	struct Texture
	{
		int sampler = -1;
		int source = -1;
		
		bool isValid() const
		{
			return
				sampler >= 0 &&
				source >= 0;
		}
	};
	
	struct Material
	{
		struct TextureReference
		{
			int index = -1;
			int texCoord = -1;
			
			bool isValid() const
			{
				return
					index >= 0 &&
					texCoord >= 0;
			}
		};
		
		struct PbrMetallicRoughness
		{
			Color baseColorFactor;
			TextureReference baseColorTexture;
		};
		
		std::string alphaMode; // OPAQUE, MASK (alpha test) or BLEND
		bool doubleSided = true;
		
		PbrMetallicRoughness pbrMetallicRoughness;
		
		TextureReference emissiveTexture;
		
		bool isValid() const
		{
			return
				(alphaMode == "OPAQUE" || alphaMode == "MASK" || alphaMode == "BLEND");
		}
	};
	
	struct MeshPrimitive
	{
		std::map<std::string, int> attributes;
		
		int indices = -1;
		int material = -1;
		int mode = -1;
		
		bool isValid() const
		{
			return
				indices >= 0 &&
				material >= 0 &&
				mode >= 0;
		}
	};
	
	struct Mesh
	{
		std::vector<MeshPrimitive> primitives;
		
		bool isValid() const
		{
			for (auto & primitive : primitives)
				if (!primitive.isValid())
					return false;
			
			return true;
		}
	};
	
	struct Scene
	{
		std::vector<Buffer> buffers;
		std::vector<BufferView> bufferViews;
		std::vector<Accessor> accessors;
		std::vector<Image> images;
		std::vector<Texture> textures;
		std::vector<Material> materials;
		std::vector<Mesh> meshes;
	};
}

void from_json(const json & j, Vec3 & v)
{
	v[0] = j.at(0).get<float>();
	v[1] = j.at(1).get<float>();
	v[2] = j.at(2).get<float>();
}

void from_json(const json & j, Vec4 & v)
{
	v[0] = j.at(0).get<float>();
	v[1] = j.at(1).get<float>();
	v[2] = j.at(2).get<float>();
	v[3] = j.at(3).get<float>();
}

void from_json(const json & j, Color & c)
{
	c.r = j.at(0).get<float>();
	c.g = j.at(1).get<float>();
	c.b = j.at(2).get<float>();
	c.a = j.at(3).get<float>();
}

static bool loadGltf(const char * path, gltf::Scene & scene)
{
	auto dir = Path::GetDirectory(path);
	auto filename = Path::GetFileName(path);
	
	char * text = nullptr;
	size_t size = 0;
	
	if (!TextIO::loadFileContents(path, text, size))
		return false;
	
	json j;
	
	try
	{
		j = json::parse(text);
	}
	catch (std::exception & e)
	{
		logError("failed to parse JSON: %s", e.what());
		return false;
	}
	
	for (auto member_itr = j.begin(); member_itr != j.end(); ++member_itr)
	{
		auto member_name = member_itr.key();
		auto & member = member_itr.value();
		
		if (member_name == "accessors")
		{
			auto & accessors = member;
			
			for (auto & json_accessor : accessors)
			{
				gltf::Accessor accessor;
				
				accessor.bufferView = json_accessor.value("bufferView", -1);
				accessor.byteOffset = json_accessor.value("byteOffset", 0);
				accessor.componentType = json_accessor.value("componentType", -1);;
				accessor.count = json_accessor.value("count", -1);
				accessor.min = json_accessor.value("min", std::vector<float>());
				accessor.max = json_accessor.value("max", std::vector<float>());
				accessor.type = json_accessor.value("type", "");
				
				if (!accessor.isValid())
					return false;
		
				scene.accessors.push_back(accessor);
			}
		}
		else if (member_name == "buffers")
		{
			auto & buffers = member;
			
			for (auto & buffer_json : buffers)
			{
				gltf::Buffer buffer;
				
				buffer.byteLength = buffer_json.value("byteLength", 0);
				buffer.uri = buffer_json.value("uri", "");
				
				if (!buffer.isValid())
					return false;
				
				auto path = dir + "/" + buffer.uri;
				
				FILE * file = fopen(path.c_str(), "rb");
				
				if (file == nullptr)
					return false;
				
				buffer.data.resize(buffer.byteLength);
				
				if (buffer.byteLength > 0)
				{
					fread(&buffer.data.front(), buffer.byteLength, 1, file);
				}
				
				fclose(file);
				file = nullptr;
				
				scene.buffers.push_back(buffer);
			}
		}
		else if (member_name == "bufferViews")
		{
			auto & bufferViews = member;
			
			for (auto & json_bufferView : bufferViews)
			{
				gltf::BufferView bufferView;
				
				bufferView.buffer = json_bufferView.value("buffer", -1);
				bufferView.byteLength = json_bufferView.value("byteLength", -1);
				bufferView.byteOffset = json_bufferView.value("byteOffset", -1);
				bufferView.name = json_bufferView.value("name", "");
				bufferView.target = json_bufferView.value("target", -1);
				
				if (!bufferView.isValid())
					return false;
				
				scene.bufferViews.push_back(bufferView);
			}
		}
		else if (member_name == "meshes")
		{
			auto & meshes = member;
			
			for (auto & mesh_json : meshes)
			{
				gltf::Mesh mesh;
				
				for (auto mesh_member_itr = mesh_json.begin(); mesh_member_itr != mesh_json.end(); ++mesh_member_itr)
				{
					if (mesh_member_itr.key() == "primitives")
					{
						auto & mesh_member = mesh_member_itr.value();
						
						for (auto & primitive_json : mesh_member)
						{
							gltf::MeshPrimitive primitive;
							
							primitive.indices = primitive_json.value("indices", -1);
							primitive.material = primitive_json.value("material", -1);
							primitive.mode = primitive_json.value("mode", -1);
							
							for (auto primitive_member_itr = primitive_json.begin(); primitive_member_itr != primitive_json.end(); ++primitive_member_itr)
							{
								if (primitive_member_itr.key() == "attributes")
								{
									auto & attributes_json = primitive_member_itr.value();
									
									for (auto attribute_json_itr = attributes_json.begin(); attribute_json_itr != attributes_json.end(); ++attribute_json_itr)
									{
										primitive.attributes[attribute_json_itr.key()] = attribute_json_itr.value();
									}
								}
							}
							
							if (!primitive.isValid())
								return false;
							
							mesh.primitives.push_back(primitive);
						}
					}
				}
				
				scene.meshes.push_back(mesh);
			}
		}
		else if (member_name == "images")
		{
			auto & images = member;
			
			for (auto & image_json : images)
			{
				gltf::Image image;
				
				image.uri = image_json.value("uri", "");
				
				image.path = dir + "/" + image.uri;
				
				if (!image.isValid())
					return false;
				
				scene.images.push_back(image);
			}
		}
		else if (member_name == "textures")
		{
			auto & textures = member;
			
			for (auto & texture_json : textures)
			{
				gltf::Texture texture;
				
				texture.sampler = texture_json.value("sampler", -1);
				texture.source = texture_json.value("source", -1);
				
				if (!texture.isValid())
					return false;
				
				scene.textures.push_back(texture);
			}
		}
		else if (member_name == "materials")
		{
			auto & materials = member;
			
			for (auto & material_json : materials)
			{
				gltf::Material material;
				
				material.alphaMode = material_json.value("alphaMode", "OPAQUE");
				material.doubleSided = material_json.value("doubleSided", true);
				
				auto pbrMetallicRoughness_itr = material_json.find("pbrMetallicRoughness");
				
				if (pbrMetallicRoughness_itr != material_json.end())
				{
					auto & pbrMetallicRoughness = pbrMetallicRoughness_itr.value();
					
					material.pbrMetallicRoughness.baseColorFactor = pbrMetallicRoughness.value("baseColorFactor", colorWhite);
					
					auto baseColorTexture_itr = pbrMetallicRoughness.find("baseColorTexture");
				
					if (baseColorTexture_itr != pbrMetallicRoughness.end())
					{
						auto & baseColorTexture = baseColorTexture_itr.value();
						
						material.pbrMetallicRoughness.baseColorTexture.index = baseColorTexture.value("index", -1);
					}
				}
				
				auto emissiveTexture = material_json.find("emissiveTexture");
				
				if (emissiveTexture != material_json.end())
				{
					material.emissiveTexture.index = emissiveTexture.value().value("index", -1);
				}
				
				if (!material.isValid())
					return false;
				
				scene.materials.push_back(material);
			}
		}
		else
		{
			logDebug("unknown key: %s", member_name.c_str());
		}
	}
	
	return true;
}

int main(int argc, char * argv[])
{
	changeDirectory(CHIBI_RESOURCE_PATH);
	
	framework.enableDepthBuffer = true;
	framework.enableRealTimeEditing = true;
	
	if (!framework.init(800, 600))
		return -1;

	//const char * path = "van_gogh_room/scene.gltf";
	//const char * path = "littlest_tokyo/scene.gltf";
	//const char * path = "ftm/scene.gltf";
	//const char * path = "nara_the_desert_dancer_free_download/scene.gltf";
	const char * path = "halloween_little_witch/scene.gltf";

	gltf::Scene scene;
	
	if (!loadGltf(path, scene))
	{
		logError("failed to load GLTF file");
	}
	
	auto resolveBufferView = [&](const int index, gltf::Accessor *& accessor, gltf::BufferView *& bufferView, gltf::Buffer *& buffer) -> bool
	{
		if (index < 0 || index >= scene.accessors.size())
			return false;
	
		accessor = &scene.accessors[index];
	
		if (accessor->bufferView < 0 || accessor->bufferView >= scene.bufferViews.size())
			return false;
	
		bufferView = &scene.bufferViews[accessor->bufferView];
	
		if (bufferView->buffer < 0 || bufferView->buffer >= scene.buffers.size())
			return false;
	
		buffer = &scene.buffers[bufferView->buffer];
		
		return true;
	};

	std::map<int, GxVertexBuffer*> vertexBuffers;
	std::map<int, GxIndexBuffer*> indexBuffers;
	std::map<gltf::Mesh*, GxMesh*> meshes;
	
	int bufferIndex = 0;
	
	for (auto & buffer : scene.buffers)
	{
		GxVertexBuffer * vertexBuffer = new GxVertexBuffer();
		
		vertexBuffer->setData(&buffer.data.front(), buffer.byteLength);
		
		Assert(vertexBuffers[bufferIndex] == nullptr);
		vertexBuffers[bufferIndex++] = vertexBuffer;
	}
	
	for (auto & mesh : scene.meshes)
	{
		for (auto & primitive : mesh.primitives)
		{
			GxIndexBuffer * indexBuffer = nullptr;
			
			//
			
			auto indexBuffer_itr = indexBuffers.find(primitive.indices);
			
			if (indexBuffer_itr != indexBuffers.end())
			{
				indexBuffer = indexBuffer_itr->second;
			}
			else
			{
				gltf::Accessor * accessor;
				gltf::BufferView * bufferView;
				gltf::Buffer * buffer;
				
				if (resolveBufferView(primitive.indices, accessor, bufferView, buffer))
				{
					if (accessor->componentType != gltf::kElementType_U32)
						continue;
					
					indexBuffer = new GxIndexBuffer();
					const uint8_t * index_mem = &buffer->data.front() + bufferView->byteOffset + accessor->byteOffset;
					
					indexBuffer->setData(index_mem, accessor->count, GX_INDEX_32);
					
					Assert(indexBuffers[primitive.indices] == nullptr);
					indexBuffers[primitive.indices] = indexBuffer;
				}
			}
			
			//
			
			std::vector<GxVertexInput> vertexInputs;
			
			int vertexBufferIndex = -1;
			
			for (auto & attribute : primitive.attributes)
			{
				gltf::Accessor * accessor;
				gltf::BufferView * bufferView;
				gltf::Buffer * buffer;
	
				const std::string & attributeName = attribute.first;
				const int accessorIndex = attribute.second;
	
				if (!resolveBufferView(accessorIndex, accessor, bufferView, buffer))
					continue;
				
				if (vertexBufferIndex == -1)
					vertexBufferIndex = bufferView->buffer;
				else if (bufferView->buffer != vertexBufferIndex)
					vertexBufferIndex = -2;
	
				const int id =
					attributeName == "POSITION" ? VS_POSITION :
					attributeName == "NORMAL" ? VS_NORMAL :
					attributeName == "TEXCOORD_0" ? VS_TEXCOORD :
					-1;
				
				if (id == -1)
					continue;
				
				const int components =
					accessor->type == "VEC2" ? 2 :
					accessor->type == "VEC3" ? 3 :
					-1;
				
				if (components == -1)
					continue;
				
				const int type =
					accessor->type == "VEC2" ? GL_FLOAT :
					accessor->type == "VEC3" ? GL_FLOAT :
					-1;
				
				if (type == -1)
					continue;
				
				GxVertexInput v;
				v.id = id;
				v.components = components;
				v.type = type;
				v.normalize = false;
				v.offset = bufferView->byteOffset + accessor->byteOffset;
				v.stride = 0;
				
				vertexInputs.push_back(v);
			}
			
			if (vertexBufferIndex < 0)
				continue;
			
			Assert(vertexBuffers[vertexBufferIndex] != nullptr);
			
			GxVertexBuffer * vertexBuffer = vertexBuffers[vertexBufferIndex];
			
			GxMesh * gxMesh = new GxMesh();
			gxMesh->setVertexBuffer(vertexBuffer, &vertexInputs.front(), vertexInputs.size());
			gxMesh->setIndexBuffer(indexBuffer);
			
			Assert(meshes[&mesh] == nullptr);
			meshes[&mesh] = gxMesh;
		}
	}
	
	Camera3d camera;
	
	camera.position = Vec3(0, 0, -2);
	
	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;

		camera.tick(framework.timeStep, true);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			projectPerspective3d(60.f, .1f, 100.f);
			pushDepthTest(true, DEPTH_LESS);
			camera.pushViewMatrix();
			
			gxRotatef(-90, 1, 0, 0);
			
			gxScalef(-.01f, .01f, .01f);
			//gxScalef(100.f, 100.f, 100.f);
			
			for (int i = 0; i < 2; ++i)
			{
				const bool isOpaquePass = (i == 0);
				
				for (auto & mesh : scene.meshes)
				{
					for (auto & primitive : mesh.primitives)
					{
						if (primitive.mode != gltf::kMode_Triangles)
							continue;
						
						BLEND_MODE blendMode;
						
						GxTextureId textureId = 0;
						
						if (primitive.material < 0 || primitive.material >= scene.materials.size())
						{
							blendMode = BLEND_OPAQUE;
							
							gxSetTexture(0);
							
							setColor(colorWhite);
						}
						else
						{
							auto & material = scene.materials[primitive.material];
							
							blendMode = material.alphaMode == "OPAQUE" ? BLEND_OPAQUE : BLEND_ALPHA;
							
							if (material.doubleSided)
							{
								glDisable(GL_CULL_FACE);
								checkErrorGL();
							}
							else
							{
								glFrontFace(GL_CCW);
								glCullFace(GL_FRONT); // weird
								glEnable(GL_CULL_FACE);
								checkErrorGL();
							}
							
							const int textureIndex = material.pbrMetallicRoughness.baseColorTexture.index;
							
							if (textureIndex >= 0 && textureIndex < scene.textures.size())
							{
								auto & texture = scene.textures[textureIndex];
								
								if (texture.source >= 0 || texture.source < scene.images.size())
								{
									auto & image = scene.images[texture.source];
									
									textureId = getTexture(image.path.c_str());
								}
							}
							
							if (keyboard.isDown(SDLK_u))
								setColor(colorWhite);
							else
								setColor(material.pbrMetallicRoughness.baseColorFactor);
						}
						
						const bool isOpaqueMaterial = (blendMode == BLEND_OPAQUE);
						
						if (isOpaquePass != isOpaqueMaterial)
							continue;
						
						setBlend(blendMode);
						
						gxSetTexture(textureId);
						
					#if 1
						if (!keyboard.isDown(SDLK_m))
						{
							auto gxMesh_itr = meshes.find(&mesh);
							
							if (gxMesh_itr != meshes.end())
							{
								GxMesh * gxMesh = gxMesh_itr->second;
								
								Shader shader("shader");
								shader.setTexture("source", 0, textureId);
								
								setShader(shader);
								{
									gxMesh->draw();
								}
								clearShader();
								
								continue;
							}
						}
					#endif
						
						gltf::Accessor * indexAccessor;
						gltf::BufferView * indexBufferView;
						gltf::Buffer * indexBuffer;
						
						if (!resolveBufferView(primitive.indices, indexAccessor, indexBufferView, indexBuffer))
							continue;
						
						if (indexAccessor->componentType != gltf::kElementType_U32)
							continue;
						
						//
						
						gltf::Accessor * positionAccessor;
						gltf::BufferView * positionBufferView;
						gltf::Buffer * positionBuffer;
						
						const int positionAccessorIndex = primitive.attributes["POSITION"];
						
						if (!resolveBufferView(positionAccessorIndex, positionAccessor, positionBufferView, positionBuffer))
							continue;
						
						if (positionAccessor->type != "VEC3")
							continue;
						
						//
						
						gltf::Accessor * texcoord0Accessor = nullptr;
						gltf::BufferView * texcoord0BufferView;
						gltf::Buffer * texcoord0Buffer;
						
						auto texcoord0_itr = primitive.attributes.find("TEXCOORD_0");
						
						if (texcoord0_itr != primitive.attributes.end())
						{
							const int texcoord0AccessorIndex = primitive.attributes["TEXCOORD_0"];
							
							if (!resolveBufferView(texcoord0AccessorIndex, texcoord0Accessor, texcoord0BufferView, texcoord0Buffer))
								continue;
							
							if (texcoord0Accessor->type != "VEC2")
								continue;
						}
						
						pushWireframe(keyboard.isDown(SDLK_w));
						gxBegin(GX_TRIANGLES);
						{
							for (int i = 0; i < indexAccessor->count; ++i)
							{
								const uint8_t * index_mem = &indexBuffer->data.front() + indexBufferView->byteOffset + indexAccessor->byteOffset;
								Assert(index_mem < &indexBuffer->data.front() + indexBuffer->byteLength);
								const uint32_t * index_ptr = (uint32_t*)index_mem;
								const uint32_t index = index_ptr[i];
								
								//
								
								const uint8_t * position_mem = &positionBuffer->data.front() + positionBufferView->byteOffset + positionAccessor->byteOffset;
								position_mem += index * 3 * sizeof(float);
								Assert(position_mem < &positionBuffer->data.front() + positionBuffer->byteLength);
								const float * position_ptr = (float*)position_mem;
								
								const float position_x = position_ptr[0];
								const float position_y = position_ptr[1];
								const float position_z = position_ptr[2];
								
								//
								
								if (texcoord0Accessor != nullptr)
								{
									const uint8_t * texcoord0_mem = &texcoord0Buffer->data.front() + texcoord0BufferView->byteOffset + texcoord0Accessor->byteOffset;
									texcoord0_mem += index * 2 * sizeof(float);
									Assert(texcoord0_mem < &texcoord0Buffer->data.front() + texcoord0Buffer->byteLength);
									const float * texcoord0_ptr = (float*)texcoord0_mem;
									
									const float texcoord0_x = texcoord0_ptr[0];
									const float texcoord0_y = texcoord0_ptr[1];
									
									gxTexCoord2f(texcoord0_x, texcoord0_y);
								}
								
								//
								
								gxVertex3f(position_x, position_y, position_z);
							}
						}
						gxEnd();
						popWireframe();
					}
				}
			}
			
			camera.popViewMatrix();
			popDepthTest();
		}
		framework.endDraw();
	}
	
	framework.shutdown();

	return 0;
}
