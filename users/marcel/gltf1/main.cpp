#include "framework.h"
#include "json.hpp"
#include "Path.h"
#include "TextIO.h"

#include <SDL2/SDL_opengl.h> // GL_CULL_FACE. todo : add functions to control culling mode to Framework

using json = nlohmann::json;

namespace gltf
{
	struct Buffer
	{
		std::string uri;
		int byteLength = -1;
		uint8_t * data = nullptr;
	};
	
	struct BufferView
	{
		int buffer = -1;
		int byteLength = -1;
		int byteOffset = -1;
		std::string name;
		int target = -1;
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
	};
	
	struct Image
	{
		std::string uri;
		
		std::string path;
	};
	
	struct Texture
	{
		int sampler = -1;
		int source = -1;
	};
	
	struct Material
	{
		struct TextureReference
		{
			int index = -1;
			int texCoord = -1;
		};
		
		struct PbrMetallicRoughness
		{
			TextureReference baseColorTexture;
		};
		
		std::string alphaMode; // OPAQUE, MASK (alpha test) or BLEND
		bool doubleSided = true;
		
		PbrMetallicRoughness pbrMetallicRoughness;
		
		TextureReference emissiveTexture;
	};
	
	struct MeshPrimitive
	{
		/*
          "attributes": {
            "NORMAL": 1,
            "POSITION": 0,
            "TANGENT": 2,
            "TEXCOORD_0": 3
          },
		*/
		
		std::map<std::string, int> attributes;
		
		int indices = -1;
		int material = -1;
		int mode = -1;
	};
	
	struct Mesh
	{
		std::vector<MeshPrimitive> primitives;
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
				
				auto path = dir + "/" + buffer.uri;
				
				FILE * file = fopen(path.c_str(), "rb");
				
				if (file == nullptr)
					return false;
				
				buffer.data = (uint8_t*)malloc(buffer.byteLength);
				
				fread(buffer.data, buffer.byteLength, 1, file);
				
				fclose(file);
				file = nullptr;
				
				//logDebug("buffer.byteLength: %d", buffer.byteLength);
				//logDebug("buffer.uri: %s", buffer.uri.c_str());
				
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
									
									primitive.attributes["POSITION"] = attributes_json.value("POSITION", -1);
									primitive.attributes["TEXCOORD_0"] = attributes_json.value("TEXCOORD_0", -1);
								}
							}
							
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
				
				scene.materials.push_back(material);
			}
		}
		else
		{
			logDebug("unknown key: %s", member_name.c_str());
		}
	}
	
	
	return false;
}

int main(int argc, char * argv[])
{
	changeDirectory(CHIBI_RESOURCE_PATH);
	
	framework.enableDepthBuffer = true;
	
	if (!framework.init(800, 600))
		return -1;

	//const char * path = "van_gogh_room/scene.gltf";
	//const char * path = "littlest_tokyo/scene.gltf";
	const char * path = "ftm/scene.gltf";

	gltf::Scene scene;
	
	if (!loadGltf(path, scene))
	{
		logError("failed to load GLTF file");
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
			
			setColor(colorWhite);
			
			//gxScalef(.01f, .01f, .01f);
			
			for (int i = 0; i < 2; ++i)
			{
				const bool isOpaquePass = (i == 0);
				
				for (auto & mesh : scene.meshes)
				{
					for (auto & primitive : mesh.primitives)
					{
						const int kMode_Triangles = 0x0004;
						const int kElementType_U32 = 0x1405;
						
						if (primitive.mode != kMode_Triangles)
							continue;
						
						BLEND_MODE blendMode;
						
						if (primitive.material < 0 || primitive.material >= scene.materials.size())
						{
							gxSetTexture(0);
							
							blendMode = BLEND_OPAQUE;
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
							
							if (textureIndex < 0 || textureIndex >= scene.textures.size())
								gxSetTexture(0);
							else
							{
								auto & texture = scene.textures[textureIndex];
								
								if (texture.source < 0 || texture.source >= scene.images.size())
									gxSetTexture(0);
								else
								{
									auto & image = scene.images[texture.source];
									
									gxSetTexture(getTexture(image.path.c_str()));
								}
							}
						}
						
						const bool isOpaqueMaterial = (blendMode == BLEND_OPAQUE);
						
						if (isOpaquePass != isOpaqueMaterial)
							continue;
						
						setBlend(blendMode);
						
						gltf::Accessor * indexAccessor;
						gltf::BufferView * indexBufferView;
						gltf::Buffer * indexBuffer;
						
						if (!resolveBufferView(primitive.indices, indexAccessor, indexBufferView, indexBuffer))
							continue;
						
						if (indexAccessor->componentType != kElementType_U32)
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
						
						gltf::Accessor * texcoord0Accessor;
						gltf::BufferView * texcoord0BufferView;
						gltf::Buffer * texcoord0Buffer;
						
						const int texcoord0AccessorIndex = primitive.attributes["TEXCOORD_0"];
						
						if (!resolveBufferView(texcoord0AccessorIndex, texcoord0Accessor, texcoord0BufferView, texcoord0Buffer))
							continue;
						
						if (texcoord0Accessor->type != "VEC2")
							continue;
						
						pushWireframe(false);
						gxBegin(GX_TRIANGLES);
						{
							for (int i = 0; i < indexAccessor->count; ++i)
							{
								const uint8_t * index_mem = indexBuffer->data + indexBufferView->byteOffset + indexAccessor->byteOffset;
								Assert(index_mem < indexBuffer->data + indexBuffer->byteLength);
								const uint32_t * index_ptr = (uint32_t*)index_mem;
								const uint32_t index = index_ptr[i];
								
								//
								
								const uint8_t * position_mem = positionBuffer->data + positionBufferView->byteOffset + positionAccessor->byteOffset;
								position_mem += index * 3 * sizeof(float);
								Assert(position_mem < positionBuffer->data + positionBuffer->byteLength);
								const float * position_ptr = (float*)position_mem;
								
								const float position_x = position_ptr[0];
								const float position_y = position_ptr[1];
								const float position_z = position_ptr[2];
								
								//
								
								const uint8_t * texcoord0_mem = texcoord0Buffer->data + texcoord0BufferView->byteOffset + texcoord0Accessor->byteOffset;
								texcoord0_mem += index * 2 * sizeof(float);
								Assert(texcoord0_mem < texcoord0Buffer->data + texcoord0Buffer->byteLength);
								const float * texcoord0_ptr = (float*)texcoord0_mem;
								
								const float texcoord0_x = texcoord0_ptr[0];
								const float texcoord0_y = texcoord0_ptr[1];
								
								//
								
								gxTexCoord2f(texcoord0_x, texcoord0_y);
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
