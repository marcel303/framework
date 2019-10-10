#include "framework.h"
#include "gx_mesh.h"
#include "json.hpp"
#include "Path.h"
#include "TextIO.h"
#include "Quat.h"

#include "data/engine/ShaderCommon.txt"

#define VIEW_SX 1000
#define VIEW_SY 600

#define ANIMATED_CAMERA 0 // todo : remove option and use hybrid

#define LOW_LATENCY_HACK_TEST 0 // todo : remove

#if LOW_LATENCY_HACK_TEST
	#include <unistd.h>
#endif


#define SHADER_METALLIC_ROUGHNESS 1
#define SHADER_SPECULAR_GLOSSINESS 0

using json = nlohmann::json;

namespace gltf
{
	enum PrimitiveType
	{
		kPrimitiveType_Points = 0,
		kPrimitiveType_Lines = 1,
		kPrimitiveType_LineStrip = 2,
		kPrimitiveType_Triangles = 4,
		kPrimitiveType_TriangleStrip = 5,
		kPrimitiveType_TriangleFan = 6
	};

	enum ElementType
	{
		kElementType_S8 = 5120,
		kElementType_U8 = 5121,
		kElementType_S16 = 5122,
		kElementType_U16 = 5123,
		kElementType_U32 = 0x1405,
		kElementType_Float32 = 5126
	};
	
	struct Asset
	{
		struct Extras
		{
			std::string author;
			std::string license;
			std::string source;
			std::string title;
		};
		
		std::string copyright;
		std::string generator;
		std::string version;
		std::string minVersion;
		
		Extras extras;
	};
	
	struct Buffer
	{
		std::string uri;
		int byteLength = -1;
		std::string name;
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
		int byteOffset = -1;
		int byteLength = -1;
		int byteStride = 0;
		int target = -1; // 34962 = ARRAY_BUFFER, 34963 = ELEMENT_ARRAY_BUFFER
		std::string name;
		
		bool isValid() const
		{
			return
				buffer >= 0 &&
				byteOffset >= 0 &&
				byteLength >= 0 &&
				byteStride >= 0/* &&
				target >= 0*/;
		}
	};
	
	struct Accessor
	{
		int bufferView = -1;
		int byteOffset = -1;
		int componentType = -1;
		bool normalized = false;
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
		std::string name;
		
		std::string path;
		
		bool isValid() const
		{
			return
				!uri.empty();
		}
	};
	
	struct Sampler
	{
		int minFilter = -1; // todo : add enums
		int magFilter = -1;
		int wrapS = -1;
		int wrapT = -1;
		
		bool isValid() const
		{
			// todo
			return true;
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
	
	// todo : samplers
	
	struct Material
	{
		struct NormalTexture
		{
			int index = -1;
			int texCoord = 0;
			float scale = 1.f; // multiplier for the normal fetched from the texture
			
			bool isValid() const
			{
				return
					index >= 0 &&
					texCoord >= 0;
			}
		};
		
		struct OcclusionTexture
		{
			int index = -1;
			int texCoord = 0;
			float strength = 1.f;
			
			bool isValid() const
			{
				return
					index >= 0 &&
					texCoord >= 0;
			}
		};
		
		struct EmissiveTexture
		{
			int index = -1;
			int texCoord = 0;
			
			bool isValid() const
			{
				return
					index >= 0 &&
					texCoord >= 0;
			}
		};
		
		struct PbrMetallicRoughness
		{
			struct BaseColorTexture
			{
				int index = -1;
				int texCoord = 0;
			};
			
			struct MetallicRoughnessTexture
			{
				int index = -1;
				int texCoord = 0;
			};
			
			Color baseColorFactor = colorWhite;
			BaseColorTexture baseColorTexture;
			float metallicFactor = 1.f;
			float roughnessFactor = 1.f;
			MetallicRoughnessTexture metallicRoughnessTexture;
		};
		
		struct PbrSpecularGlossiness
		{
			struct DiffuseTexture
			{
				int index = -1;
				int texCoord = 0;
			};
			
			struct SpecularGlossinessTexture
			{
				int index = -1;
				int texCoord = 0;
			};
			
			Color diffuseFactor = colorWhite;
			DiffuseTexture diffuseTexture;
			Vec3 specularFactor = Vec3(1, 1, 1);
			float glossinessFactor = 1.f;
			SpecularGlossinessTexture specularGlossinessTexture;
		};
		
		std::string name;
		std::string alphaMode = "OPAQUE"; // OPAQUE, MASK (alpha test) or BLEND
		float alphaCutoff = .5f;
		bool doubleSided = false;
		
		// material types
		
		PbrMetallicRoughness pbrMetallicRoughness;
		PbrSpecularGlossiness pbrSpecularGlossiness;
		
		// additional maps
		
		NormalTexture normalTexture;
		OcclusionTexture occlusionTexture;
		EmissiveTexture emissiveTexture;
		Vec3 emissiveFactor;
		
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
	
	struct Node
	{
		std::string name;
		int mesh = -1;
		std::vector<int> children;
		Vec3 translation;
		Quat rotation;
		Vec3 scale;
		Mat4x4 matrix = Mat4x4(true);
	};
	
	struct SceneRoot
	{
		std::string name;
		std::vector<int> nodes;
	};
	
	struct Scene
	{
		Asset asset;
		
		std::vector<Buffer> buffers;
		std::vector<BufferView> bufferViews;
		std::vector<Accessor> accessors;
		std::vector<Image> images;
		std::vector<Sampler> samplers;
		std::vector<Texture> textures;
		std::vector<Material> materials;
		std::vector<Mesh> meshes;
		std::vector<Node> nodes;
		std::vector<SceneRoot> sceneRoots;
		
		int activeScene = -1;
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

void from_json(const json & j, Quat & q)
{
	q[0] = j.at(0).get<float>();
	q[1] = j.at(1).get<float>();
	q[2] = j.at(2).get<float>();
	q[3] = j.at(3).get<float>();
}

void from_json(const json & j, Mat4x4 & m)
{
	for (int i = 0; i < 16; ++i)
		m.m_v[i] = j.at(i).get<float>();
}

static bool loadGltf(const char * path, gltf::Scene & scene)
{
	auto dir = Path::GetDirectory(path);
	auto filename = Path::GetFileName(path);
	
	char * text = nullptr;
	size_t size = 0;
	
	if (!TextIO::loadFileContents(path, text, size))
	{
		logDebug("failed to load GLTF file contents");
		return false;
	}
	
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
				accessor.componentType = json_accessor.value("componentType", -1);
				accessor.normalized = json_accessor.value("normalized", false);
				accessor.count = json_accessor.value("count", -1);
				accessor.min = json_accessor.value("min", std::vector<float>());
				accessor.max = json_accessor.value("max", std::vector<float>());
				accessor.type = json_accessor.value("type", "");
				
				if (!accessor.isValid())
				{
					logDebug("accessor is invalid");
					return false;
				}
		
				scene.accessors.push_back(accessor);
			}
		}
		else if (member_name == "asset")
		{
			auto & asset_json = member;
			
			auto & asset = scene.asset;
			
			asset.copyright = asset_json.value("copyright", "");
			asset.generator = asset_json.value("generator", "");
			asset.version = asset_json.value("version", "");
			asset.minVersion = asset_json.value("minVersion", "");
			
			auto extras_json_itr = asset_json.find("extras");
			
			if (extras_json_itr != asset_json.end())
			{
				auto & extras_json = extras_json_itr.value();
				
				asset.extras.author = extras_json.value("author", "");
				asset.extras.license = extras_json.value("license", "");
				asset.extras.source = extras_json.value("source", "");
				asset.extras.title = extras_json.value("title", "");
			}
		}
		else if (member_name == "buffers")
		{
			auto & buffers = member;
			
			for (auto & buffer_json : buffers)
			{
				gltf::Buffer buffer;
				
				buffer.uri = buffer_json.value("uri", "");
				buffer.byteLength = buffer_json.value("byteLength", 0);
				buffer.name = buffer_json.value("name", "");
				
				if (!buffer.isValid())
				{
					logDebug("buffer is invalid");
					return false;
				}
				
				auto path = dir + "/" + buffer.uri;
				
				FILE * file = fopen(path.c_str(), "rb");
				
				if (file == nullptr)
				{
					logDebug("failed to open buffer file");
					return false;
				}
				
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
				bufferView.byteOffset = json_bufferView.value("byteOffset", -1);
				bufferView.byteLength = json_bufferView.value("byteLength", -1);
				bufferView.byteStride = json_bufferView.value("byteStride", 0);
				bufferView.target = json_bufferView.value("target", -1);
				bufferView.name = json_bufferView.value("name", "");
				
				if (!bufferView.isValid())
				{
					logDebug("bufferView is invalid");
					return false;
				}
				
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
							{
								logDebug("primitive is invalid");
								return false;
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
				image.name = image_json.value("name", "");
				
				image.path = dir + "/" + image.uri;
				
				// todo : add support for bufferView property ?
				
				if (!image.isValid())
				{
					logDebug("image is invalid");
					return false;
				}
				
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
				{
					logDebug("texture is invalid");
					return false;
				}
				
				scene.textures.push_back(texture);
			}
		}
		else if (member_name == "samplers")
		{
			auto & samplers = member;
			
			for (auto & sampler_json : samplers)
			{
				gltf::Sampler sampler;
				
				sampler.minFilter = sampler_json.value("minFilter", -1);
				sampler.magFilter = sampler_json.value("magFilter", -1);
				sampler.wrapS = sampler_json.value("wrapS", -1);
				sampler.wrapT = sampler_json.value("wrapT", -1);
				
				/*
				minFilter:
				9728 = NEAREST,
				9729 = LINEAR,
				9984 = NEAREST_MIPMAP_NEAREST,
				9985 = LINEAR_MIPMAP_NEAREST,
				9986 = NEAREST_MIPMAP_LINEAR,
				9987 = LINEAR_MIPMAP_LINEAR
				*/
				
				/*
				magFilter:
				9728 = NEAREST,
				9729 = LINEAR
				*/
				
				/*
				wrap:
				10497 = REPEAT,
				33071 = CLAMP_TO_EDGE,
				33648 = MIRRORED_REPEAT
				
				*/
				
				if (!sampler.isValid())
					return false;
				
				scene.samplers.push_back(sampler);
			}
		}
		else if (member_name == "materials")
		{
			auto & materials = member;
			
			for (auto & material_json : materials)
			{
				gltf::Material material;
				
				material.name = material_json.value("name", "");
				material.alphaMode = material_json.value("alphaMode", "OPAQUE");
				material.alphaCutoff = material_json.value("alphaCutoff", .5f);
				material.doubleSided = material_json.value("doubleSided", false);
				
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
						material.pbrMetallicRoughness.baseColorTexture.texCoord = baseColorTexture.value("texCoord", 0);
					}
					
					material.pbrMetallicRoughness.metallicFactor = pbrMetallicRoughness.value("metallicFactor", 1.f);
					material.pbrMetallicRoughness.roughnessFactor = pbrMetallicRoughness.value("roughnessFactor", 1.f);
					
					// metallicRoughnessTexture
					
					auto metallicRoughnessTexture_itr = pbrMetallicRoughness.find("metallicRoughnessTexture");
				
					if (metallicRoughnessTexture_itr != pbrMetallicRoughness.end())
					{
						auto & metallicRoughnessTexture = metallicRoughnessTexture_itr.value();
						
						material.pbrMetallicRoughness.metallicRoughnessTexture.index = metallicRoughnessTexture.value("index", -1);
						material.pbrMetallicRoughness.metallicRoughnessTexture.texCoord = metallicRoughnessTexture.value("texCoord", 0);
					}
				}
				
				auto extensions_itr = material_json.find("extensions");
				
				if (extensions_itr != material_json.end())
				{
					auto & extensions = extensions_itr.value();
					
					auto pbrSpecularGlossiness_itr = extensions.find("KHR_materials_pbrSpecularGlossiness");
					
					if (pbrSpecularGlossiness_itr != extensions.end())
					{
						auto & pbrSpecularGlossiness = pbrSpecularGlossiness_itr.value();
						
						material.pbrSpecularGlossiness.diffuseFactor = pbrSpecularGlossiness.value("diffuseFactor", colorWhite);
						
						auto diffuseTexture_itr = pbrSpecularGlossiness.find("diffuseTexture");
					
						if (diffuseTexture_itr != pbrSpecularGlossiness.end())
						{
							auto & diffuseTexture = diffuseTexture_itr.value();
							
							material.pbrSpecularGlossiness.diffuseTexture.index = diffuseTexture.value("index", -1);
							material.pbrSpecularGlossiness.diffuseTexture.texCoord = diffuseTexture.value("texCoord", 0);
						}
						
						material.pbrSpecularGlossiness.specularFactor = pbrSpecularGlossiness.value("specularFactor", Vec3(1, 1, 1));
						material.pbrSpecularGlossiness.glossinessFactor = pbrSpecularGlossiness.value("glossinessFactor", 1.f);
						
						auto specularGlossinessTexture_itr = pbrSpecularGlossiness.find("specularGlossinessTexture");
					
						if (specularGlossinessTexture_itr != pbrSpecularGlossiness.end())
						{
							auto & specularGlossinessTexture = specularGlossinessTexture_itr.value();
							
							material.pbrSpecularGlossiness.specularGlossinessTexture.index = specularGlossinessTexture.value("index", -1);
							material.pbrSpecularGlossiness.specularGlossinessTexture.texCoord = specularGlossinessTexture.value("texCoord", 0);
						}
					}
				}
				
				// normalTexture
				
				auto normalTexture_itr = material_json.find("normalTexture");
				
				if (normalTexture_itr != material_json.end())
				{
					auto & normalTexture = *normalTexture_itr;
					
					material.normalTexture.index = normalTexture.value("index", -1);
					material.normalTexture.texCoord = normalTexture.value("texCoord", 0);
					material.normalTexture.scale = normalTexture.value("scale", 1.f);
					
					if (material.normalTexture.scale != 1.f)
						logWarning("normalTexture.scale isn't 1.0. rendering won't be as expected");
				}
				
				// occlusionTexture
				
				auto occlusionTexture_itr = material_json.find("occlusionTexture");
				
				if (occlusionTexture_itr != material_json.end())
				{
					auto & occlusionTexture = *occlusionTexture_itr;
					
					material.occlusionTexture.index = occlusionTexture.value("index", -1);
					material.occlusionTexture.texCoord = occlusionTexture.value("texCoord", 0);
					material.occlusionTexture.strength = occlusionTexture.value("strength", 1.f);
				}
				
				auto emissiveTexture = material_json.find("emissiveTexture");
				
				if (emissiveTexture != material_json.end())
				{
					material.emissiveTexture.index = emissiveTexture.value().value("index", -1);
					material.emissiveTexture.texCoord = emissiveTexture.value().value("texCoord", 0);
				}
				
				material.emissiveFactor = material_json.value("emissiveFactor", Vec3());
				
				if (!material.isValid())
				{
					logDebug("material is invalid");
					return false;
				}
				
				scene.materials.push_back(material);
			}
		}
		else if (member_name == "nodes")
		{
			auto & nodes_json = member;
			
			for (auto & node_json : nodes_json)
			{
				gltf::Node node;
				node.name = node_json.value("name", "");
				node.mesh = node_json.value("mesh", -1);
				node.children = node_json.value("children", std::vector<int>());
				node.translation = node_json.value("translation", Vec3(0, 0, 0));
				node.rotation = node_json.value("rotation", Quat(0, 0, 0, 1));
				node.scale = node_json.value("scale", Vec3(1, 1, 1));
				node.matrix = node_json.value("matrix", Mat4x4(true));
				
				// todo : matrix
				
				scene.nodes.push_back(node);
			}
		}
		else if (member_name == "scene")
		{
			scene.activeScene = member_itr.value();
		}
		else if (member_name == "scenes")
		{
			auto & scenes_json = member;
			
			for (auto & scene_json : scenes_json)
			{
				gltf::SceneRoot sceneRoot;
				sceneRoot.name = scene_json.value("name", "");
				sceneRoot.nodes = scene_json.value("nodes", std::vector<int>());
				
				scene.sceneRoots.push_back(sceneRoot);
			}
		}
		else
		{
			logDebug("unknown key: %s", member_name.c_str());
		}
	}
	
	return true;
}

#if ANIMATED_CAMERA

struct AnimatedCamera3d
{
	Vec3 position;
	Quat orientation;
	
	Vec3 desiredPosition;
	Quat desiredOrientation;
	bool animate = false;
	float animationSpeed = 1.f;
	
	void tick(const float dt, const bool inputIsCaptured)
	{
		if (inputIsCaptured == false)
		{
		
		}
		
		if (animate)
		{
			const float retain = powf(1.f - animationSpeed, dt);
			const float attain = 1.f - retain;
			
			position = lerp(position, desiredPosition, attain);
			orientation = orientation.slerp(desiredOrientation, attain);
			
			//position = desiredPosition;
			//orientation = desiredOrientation;
		}
	}
	
	Mat4x4 getWorldMatrix() const
	{
		return Mat4x4(true).Translate(position).Rotate(orientation);
	}

	Mat4x4 getViewMatrix() const
	{
		return getWorldMatrix().CalcInv();
	}

	void pushViewMatrix() const
	{
		const Mat4x4 matrix = getViewMatrix();
		
		const GX_MATRIX restoreMatrixMode = gxGetMatrixMode();
		{
			gxMatrixMode(GX_PROJECTION);
			gxPushMatrix();
			gxMultMatrixf(matrix.m_v);
		}
		gxMatrixMode(restoreMatrixMode);
	}

	void popViewMatrix() const
	{
		const GX_MATRIX restoreMatrixMode = gxGetMatrixMode();
		{
			gxMatrixMode(GX_PROJECTION);
			gxPopMatrix();
		}
		gxMatrixMode(restoreMatrixMode);
	}
};

#endif

int main(int argc, char * argv[])
{
	changeDirectory(CHIBI_RESOURCE_PATH);
	
	framework.enableDepthBuffer = true;
	framework.enableRealTimeEditing = true;
	
	if (!framework.init(VIEW_SX, VIEW_SY))
		return -1;

	//const char * path = "van_gogh_room/scene.gltf";
	//const char * path = "littlest_tokyo/scene.gltf";
	const char * path = "ftm/scene.gltf";
	//const char * path = "nara_the_desert_dancer_free_download/scene.gltf";
	//const char * path = "halloween_little_witch/scene.gltf";

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
	std::map<const gltf::Mesh*, GxMesh*> meshes;
	
	// create vertex buffers from buffer objects
	
	int bufferIndex = 0;
	
	for (auto & buffer : scene.buffers)
	{
		GxVertexBuffer * vertexBuffer = new GxVertexBuffer();
		vertexBuffer->alloc(&buffer.data.front(), buffer.byteLength);
		
		Assert(vertexBuffers[bufferIndex] == nullptr);
		vertexBuffers[bufferIndex++] = vertexBuffer;
	}
	
	// create index buffers and meshes for mesh primitives
	
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
				
				if (!resolveBufferView(primitive.indices, accessor, bufferView, buffer))
				{
					logWarning("failed to resolve buffer view");
				}
				else
				{
					if (accessor->componentType != gltf::kElementType_U16 &&
						accessor->componentType != gltf::kElementType_U32)
					{
						logWarning("index element type not supported");
						continue;
					}
					
					indexBuffer = new GxIndexBuffer();
					const uint8_t * index_mem = &buffer->data.front() + bufferView->byteOffset + accessor->byteOffset;
					
					const GX_INDEX_FORMAT format =
						accessor->componentType == gltf::kElementType_U16
						? GX_INDEX_16
						: GX_INDEX_32;
					
					indexBuffer->alloc(index_mem, accessor->count, format);
					
					Assert(indexBuffers[primitive.indices] == nullptr);
					indexBuffers[primitive.indices] = indexBuffer;
				}
			}
			
			// create mapping between vertex buffer and vertex shader
			
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
				{
					logWarning("failed to resolve buffer view");
					continue;
				}
				
				if (vertexBufferIndex == -1)
					vertexBufferIndex = bufferView->buffer;
				else if (bufferView->buffer != vertexBufferIndex)
					vertexBufferIndex = -2;
	
				/*
				POSITION,
				NORMAL,
				TANGENT,
				TEXCOORD_0,
				TEXCOORD_1,
				COLOR_0,
				JOINS_0, (bone indices)
				WEIGHTS_0
				
				note : bitangent = cross(normal, tangent.xyz) * tangent.w
				*/
				
				const int id =
					attributeName == "POSITION" ? VS_POSITION :
					attributeName == "NORMAL" ? VS_NORMAL :
					attributeName == "COLOR_0" ? VS_COLOR :
					attributeName == "TEXCOORD_0" ? VS_TEXCOORD0 :
					attributeName == "TEXCOORD_1" ? VS_TEXCOORD1 :
					attributeName == "JOINTS_0" ? VS_BLEND_INDICES :
					attributeName == "WEIGHTS_0" ? VS_BLEND_WEIGHTS :
					-1;
				
				if (id == -1)
				{
					//logDebug("unknown attribute: %s", attributeName.c_str());
					continue;
				}
				
				const int numComponents =
					accessor->type == "SCALAR" ? 1 :
					accessor->type == "VEC2" ? 2 :
					accessor->type == "VEC3" ? 3 :
					accessor->type == "VEC4" ? 4 :
					-1;
				
				if (numComponents == -1)
				{
					logWarning("number of components not supported");
					continue;
				}
				
				const GX_ELEMENT_TYPE type =
					accessor->type == "SCALAR" ? GX_ELEMENT_FLOAT32 :
					accessor->type == "VEC2" ? GX_ELEMENT_FLOAT32 :
					accessor->type == "VEC3" ? GX_ELEMENT_FLOAT32 :
					accessor->type == "VEC4" ? GX_ELEMENT_FLOAT32 :
					(GX_ELEMENT_TYPE)-1;
				
				if (type == (GX_ELEMENT_TYPE)-1)
				{
					logWarning("element type not supported");
					continue;
				}
				
				GxVertexInput v;
				v.id = id;
				v.numComponents = numComponents;
				v.type = type;
				v.normalize = accessor->normalized;
				v.offset = bufferView->byteOffset + accessor->byteOffset;
				v.stride = bufferView->byteStride;
				
				vertexInputs.push_back(v);
			}
			
			if (vertexBufferIndex < 0)
			{
				logWarning("invalid vertex buffer index");
				continue;
			}
			
			// create mesh
			
			Assert(vertexBuffers[vertexBufferIndex] != nullptr);
			
			GxVertexBuffer * vertexBuffer = vertexBuffers[vertexBufferIndex];
			
			GxMesh * gxMesh = new GxMesh();
			gxMesh->setVertexBuffer(vertexBuffer, &vertexInputs.front(), vertexInputs.size(), 0);
			gxMesh->setIndexBuffer(indexBuffer);
			
			Assert(meshes[&mesh] == nullptr);
			meshes[&mesh] = gxMesh;
		}
	}
	
#if ANIMATED_CAMERA
	AnimatedCamera3d camera;
#else
	Camera3d camera;
#endif
	
	camera.position = Vec3(0, 0, -2);
	
	bool centimeters = true;
	
	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;
		
		if (keyboard.wentDown(SDLK_t))
			centimeters = !centimeters;
		
		camera.tick(framework.timeStep, true);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			projectPerspective3d(60.f, .1f, 100.f);
			pushDepthTest(true, DEPTH_LESS);
			pushBlend(BLEND_OPAQUE);
			camera.pushViewMatrix();
			
			if (centimeters)
				gxScalef(-.01f, .01f, .01f);
			else
				gxScalef(-1, 1, 1);
			
			auto tryGetTextureId = [&](const int textureIndex) -> GxTextureId
			{
				GxTextureId result = 0;
				
				if (textureIndex >= 0 && textureIndex < scene.textures.size())
				{
					auto & texture = scene.textures[textureIndex];
					
					if (texture.source >= 0 && texture.source < scene.images.size())
					{
						auto & image = scene.images[texture.source];
						
						result = getTexture(image.path.c_str());
					}
				}
				
				return result;
			};
			
			auto drawMesh = [&](const gltf::Mesh & mesh, const bool isOpaquePass)
			{
				for (auto & primitive : mesh.primitives)
				{
					if (primitive.mode != gltf::kPrimitiveType_Triangles)
					{
						logWarning("primitive type not supported");
						continue;
					}
					
					if (primitive.material < 0 || primitive.material >= scene.materials.size())
					{
						logWarning("invalid material index");
						continue;
					}
					
					auto & material = scene.materials[primitive.material];
					
					//Assert(material.alphaMode != "MASK"); // todo : implement !
					
					const BLEND_MODE blendMode =
						material.alphaMode == "OPAQUE" || material.alphaMode == "MASK"
						? BLEND_OPAQUE
						: BLEND_ALPHA;
					
				#if SHADER_METALLIC_ROUGHNESS
					// PBR metallic roughness material
					const GxTextureId textureId = keyboard.isDown(SDLK_1) ? 0 : tryGetTextureId(material.pbrMetallicRoughness.baseColorTexture.index);
					const GxTextureId metallicRoughnessTextureId = keyboard.isDown(SDLK_2) ? 0 : tryGetTextureId(material.pbrMetallicRoughness.metallicRoughnessTexture.index);
					const GxTextureId normalTextureId = keyboard.isDown(SDLK_3) ? 0 : tryGetTextureId(material.normalTexture.index);
					const GxTextureId occlusionTextureId = keyboard.isDown(SDLK_4) ? 0 : tryGetTextureId(material.occlusionTexture.index);
					const GxTextureId emissiveTextureId = keyboard.isDown(SDLK_5) ? 0 : tryGetTextureId(material.emissiveTexture.index);
				#endif
				
				#if SHADER_SPECULAR_GLOSSINESS
					// PBR specular glossiness material
					const GxTextureId diffuseTextureId = keyboard.isDown(SDLK_1) ? 0 : tryGetTextureId(material.pbrSpecularGlossiness.diffuseTexture.index);
					const GxTextureId specularGlossinessTextureId = keyboard.isDown(SDLK_2) ? 0 : tryGetTextureId(material.pbrSpecularGlossiness.specularGlossinessTexture.index);
					const GxTextureId normalTextureId = keyboard.isDown(SDLK_3) ? 0 : tryGetTextureId(material.normalTexture.index);
					const GxTextureId occlusionTextureId = keyboard.isDown(SDLK_4) ? 0 : tryGetTextureId(material.occlusionTexture.index);
					const GxTextureId emissiveTextureId = keyboard.isDown(SDLK_5) ? 0 : tryGetTextureId(material.emissiveTexture.index);
				#endif
				
					Color color = colorWhite;
					
					if (!keyboard.isDown(SDLK_u))
						color = material.pbrMetallicRoughness.baseColorFactor;
					
					const bool isOpaqueMaterial = (blendMode == BLEND_OPAQUE);
					
					if (isOpaquePass != isOpaqueMaterial)
						continue;
					
					setBlend(blendMode);
					
				#if 1
					if (!keyboard.isDown(SDLK_m))
					{
						auto gxMesh_itr = meshes.find(&mesh);
						
						if (gxMesh_itr != meshes.end())
						{
							GxMesh * gxMesh = gxMesh_itr->second;
							
						#if 0
							Shader shader("shader");
							setShader(shader);
							
							shader.setTexture("source", 0, textureId);
							shader.setImmediate("color", color.r, color.g, color.b, color.a);
							shader.setImmediate("params", textureId != 0, 0.f, 0.f, 0.f);
							shader.setImmediate("time", framework.time);
						#endif
						
						#if SHADER_SPECULAR_GLOSSINESS
							Shader shader("shader-pbr-specularGlossiness");
							setShader(shader);
							
							shader.setImmediate("scene_camPos",
								camera.position[0],
								camera.position[1],
								camera.position[2]);
							
							const float dx = cosf(framework.time);
							const float dz = sinf(framework.time);
							
							shader.setImmediate("scene_lightDir",
								dx,
								-1.f,
								dz);
							
							shader.setTexture("diffuseTexture", 0, diffuseTextureId);
							shader.setTexture("normalTexture", 1, normalTextureId);
							shader.setTexture("occlusionTexture", 2, occlusionTextureId);
							shader.setTexture("specularGlossinessTexture", 3, specularGlossinessTextureId);
							shader.setTexture("emissiveTexture", 4, emissiveTextureId);

							shader.setImmediate("material_diffuseFactor",
								material.pbrSpecularGlossiness.diffuseFactor.r,
								material.pbrSpecularGlossiness.diffuseFactor.g,
								material.pbrSpecularGlossiness.diffuseFactor.b,
								material.pbrSpecularGlossiness.diffuseFactor.a);
							shader.setImmediate("material_hasDiffuseTexture", diffuseTextureId != 0);
							shader.setImmediate("material_hasSpecularGlossinessTexture", specularGlossinessTextureId != 0);
							shader.setImmediate("material_glossinessFactor",
								material.pbrSpecularGlossiness.glossinessFactor);
							shader.setImmediate("material_specularFactor",
								material.pbrSpecularGlossiness.specularFactor[0],
								material.pbrSpecularGlossiness.specularFactor[1],
								material.pbrSpecularGlossiness.specularFactor[2]);
							
							shader.setImmediate("material_hasNormalTexture", normalTextureId != 0);
							shader.setImmediate("material_occlusionStrength", material.occlusionTexture.strength);
							shader.setImmediate("material_hasOcclusionTexture", occlusionTextureId != 0);
							shader.setImmediate("material_hasEmissiveTexture", emissiveTextureId != 0);
							shader.setImmediate("material_emissiveFactor",
								material.emissiveFactor[0],
								material.emissiveFactor[1],
								material.emissiveFactor[2]);
							shader.setImmediate("material_alphaMask", false);
							shader.setImmediate("material_alphaMaskCutoff", 0.f);
						#endif
						
						#if SHADER_METALLIC_ROUGHNESS
							Shader shader("shader-pbr");
							setShader(shader);
							
							shader.setImmediate("scene_camPos",
								camera.position[0],
								camera.position[1],
								camera.position[2]);
							
							shader.setImmediate("scene_lightDir",
								.5f,
								1.f,
								.5f);
							
							shader.setTexture("baseColorTexture", 0, textureId);
							shader.setTexture("normalTexture", 1, normalTextureId);
							shader.setTexture("occlusionTexture", 2, occlusionTextureId);
							shader.setTexture("metallicRoughnessTexture", 3, metallicRoughnessTextureId);
							shader.setTexture("emissiveTexture", 4, emissiveTextureId);

							shader.setImmediate("material_baseColorFactor",
								material.pbrMetallicRoughness.baseColorFactor.r,
								material.pbrMetallicRoughness.baseColorFactor.g,
								material.pbrMetallicRoughness.baseColorFactor.b,
								material.pbrMetallicRoughness.baseColorFactor.a);
							shader.setImmediate("material_hasBaseColorTexture", textureId != 0);
							shader.setImmediate("material_hasMetallicRoughnessTexture", metallicRoughnessTextureId != 0);
							shader.setImmediate("material_hasNormalTexture", normalTextureId != 0);
							shader.setImmediate("material_occlusionStrength", material.occlusionTexture.strength);
							shader.setImmediate("material_hasOcclusionTexture", occlusionTextureId != 0);
							shader.setImmediate("material_hasEmissiveTexture", emissiveTextureId != 0);
							shader.setImmediate("material_metallicFactor",
								material.pbrMetallicRoughness.metallicFactor);
							shader.setImmediate("material_roughnessFactor",
								material.pbrMetallicRoughness.roughnessFactor);
							shader.setImmediate("material_emissiveFactor",
								material.emissiveFactor[0],
								material.emissiveFactor[1],
								material.emissiveFactor[2]);
							shader.setImmediate("material_alphaMask", material.alphaMode == "MASK");
							shader.setImmediate("material_alphaMaskCutoff", material.alphaCutoff);
						#endif
					
							setShader(shader);
							{
								pushWireframe(keyboard.isDown(SDLK_w));
								pushCullMode(material.doubleSided ? CULL_NONE : CULL_BACK, CULL_CCW);
								gxMesh->draw(GX_TRIANGLES);
								popCullMode();
								popWireframe();
							}
							clearShader();
						}
						
						continue;
					}
				#endif
				
				#if SHADER_METALLIC_ROUGHNESS
					gxSetTexture(textureId);
				#endif
				#if SHADER_SPECULAR_GLOSSINESS
					gxSetTexture(diffuseTextureId);
				#endif
					
					setColor(color);
					
					// draw mesh, without the use of vertex and index buffers
					
					gltf::Accessor * indexAccessor;
					gltf::BufferView * indexBufferView;
					gltf::Buffer * indexBuffer;
					
					if (!resolveBufferView(primitive.indices, indexAccessor, indexBufferView, indexBuffer))
					{
						logWarning("failed to resolve buffer view");
						continue;
					}
					
					if (indexAccessor->componentType != gltf::kElementType_U32)
					{
						logWarning("component type not supported");
						continue;
					}
					
					//
					
					gltf::Accessor * positionAccessor;
					gltf::BufferView * positionBufferView;
					gltf::Buffer * positionBuffer;
					
					auto position_itr = primitive.attributes.find("POSITION");
					
					if (position_itr == primitive.attributes.end())
					{
						logWarning("position attribute not found");
						continue;
					}
					
					const int positionAccessorIndex = position_itr->second;
					
					if (!resolveBufferView(positionAccessorIndex, positionAccessor, positionBufferView, positionBuffer))
					{
						logWarning("failed to resolve buffer view");
						continue;
					}
					
					if (positionAccessor->type != "VEC3")
					{
						logWarning("position element type not supported");
						continue;
					}
					
					//
					
					gltf::Accessor * texcoord0Accessor = nullptr;
					gltf::BufferView * texcoord0BufferView;
					gltf::Buffer * texcoord0Buffer;
					
					auto texcoord0_itr = primitive.attributes.find("TEXCOORD_0");
					
					if (texcoord0_itr != primitive.attributes.end())
					{
						const int texcoord0AccessorIndex = texcoord0_itr->second;
						
						if (!resolveBufferView(texcoord0AccessorIndex, texcoord0Accessor, texcoord0BufferView, texcoord0Buffer))
						{
							logWarning("failed to resolve buffer view");
							continue;
						}
						
						if (texcoord0Accessor->type != "VEC2")
						{
							logWarning("texcoord element type not supported");
							continue;
						}
					}
					
					pushWireframe(keyboard.isDown(SDLK_w));
					pushCullMode(material.doubleSided ? CULL_NONE : CULL_BACK, CULL_CCW);
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
					popCullMode();
					popWireframe();
					
					gxSetTexture(0);
				}
			};
			
			std::function<void(const gltf::Node & node, const bool isOpaquePass)> drawNodeTraverse = [&](const gltf::Node & node, const bool isOpaquePass)
			{
				gxPushMatrix();
				gxTranslatef(node.translation[0], node.translation[1], node.translation[2]);
				Mat4x4 rotationMatrix = node.rotation.toMatrix();
				gxMultMatrixf(rotationMatrix.m_v);
				gxScalef(node.scale[0], node.scale[1], node.scale[2]);
				gxMultMatrixf(node.matrix.m_v);
				
				for (auto child_index : node.children)
				{
					if (child_index < 0 || child_index >= scene.nodes.size())
					{
						logWarning("invalid child index");
						continue;
					}
					
					auto & child = scene.nodes[child_index];
					
					drawNodeTraverse(child, isOpaquePass);
				}
				
				if (node.mesh >= 0 && node.mesh < scene.meshes.size())
				{
					auto & mesh = scene.meshes[node.mesh];
					
					drawMesh(mesh, isOpaquePass);
				}
				
				gxPopMatrix();
			};
			
			std::function<void(const gltf::Node & node)> drawNodeMinMaxTraverse = [&](const gltf::Node & node)
			{
				gxPushMatrix();
				gxTranslatef(node.translation[0], node.translation[1], node.translation[2]);
				Mat4x4 rotationMatrix = node.rotation.toMatrix();
				gxMultMatrixf(rotationMatrix.m_v);
				gxScalef(node.scale[0], node.scale[1], node.scale[2]);
				gxMultMatrixf(node.matrix.m_v);
				
				for (auto child_index : node.children)
				{
					if (child_index < 0 || child_index >= scene.nodes.size())
					{
						logWarning("invalid child index");
						continue;
					}
					
					auto & child = scene.nodes[child_index];
					
					drawNodeMinMaxTraverse(child);
				}
				
				if (node.mesh >= 0 && node.mesh < scene.meshes.size())
				{
					auto & mesh = scene.meshes[node.mesh];
					
					for (auto & primitive : mesh.primitives)
					{
						gltf::Accessor * positionAccessor;
						gltf::BufferView * positionBufferView;
						gltf::Buffer * positionBuffer;
						
						auto position_itr = primitive.attributes.find("POSITION");
						
						if (position_itr == primitive.attributes.end())
						{
							logWarning("position attribute not found");
							continue;
						}
						
						const int positionAccessorIndex = position_itr->second;
						
						if (!resolveBufferView(positionAccessorIndex, positionAccessor, positionBufferView, positionBuffer))
						{
							logWarning("failed to resolve buffer view");
							continue;
						}
						
						if (positionAccessor->type != "VEC3")
						{
							logWarning("position element type not supported");
							continue;
						}
						
						const Vec3 min(
							positionAccessor->min[0],
							positionAccessor->min[1],
							positionAccessor->min[2]);
						const Vec3 max(
							positionAccessor->max[0],
							positionAccessor->max[1],
							positionAccessor->max[2]);
						
						const Vec3 mid = (min + max) / 2.f;
						const Vec3 size = (max - min) / 2.f;
						
						setColor(127, 63, 255);
						lineCube(mid, size);
					}
				}
				
				gxPopMatrix();
			};
			
			struct BoundingBox
			{
				Vec3 min;
				Vec3 max;
				bool hasMinMax = false;
			};
			
			std::function<void(const gltf::Node & node, BoundingBox & boundingBox)> calculateNodeMinMaxTraverse = [&](const gltf::Node & node, BoundingBox & boundingBox)
			{
				gxPushMatrix();
				gxTranslatef(node.translation[0], node.translation[1], node.translation[2]);
				Mat4x4 rotationMatrix = node.rotation.toMatrix();
				gxMultMatrixf(rotationMatrix.m_v);
				gxScalef(node.scale[0], node.scale[1], node.scale[2]);
				gxMultMatrixf(node.matrix.m_v);
				
				for (auto child_index : node.children)
				{
					if (child_index < 0 || child_index >= scene.nodes.size())
					{
						logWarning("invalid child index");
						continue;
					}
					
					auto & child = scene.nodes[child_index];
					
					calculateNodeMinMaxTraverse(child, boundingBox);
				}
				
				if (node.mesh >= 0 && node.mesh < scene.meshes.size())
				{
					auto & mesh = scene.meshes[node.mesh];
					
					for (auto & primitive : mesh.primitives)
					{
						gltf::Accessor * positionAccessor;
						gltf::BufferView * positionBufferView;
						gltf::Buffer * positionBuffer;
						
						auto position_itr = primitive.attributes.find("POSITION");
						
						if (position_itr == primitive.attributes.end())
						{
							logWarning("position attribute not found");
							continue;
						}
						
						const int positionAccessorIndex = position_itr->second;
						
						if (!resolveBufferView(positionAccessorIndex, positionAccessor, positionBufferView, positionBuffer))
						{
							logWarning("failed to resolve buffer view");
							continue;
						}
						
						if (positionAccessor->type != "VEC3")
						{
							logWarning("position element type not supported");
							continue;
						}
						
						const Vec3 min(
							positionAccessor->min[0],
							positionAccessor->min[1],
							positionAccessor->min[2]);
						const Vec3 max(
							positionAccessor->max[0],
							positionAccessor->max[1],
							positionAccessor->max[2]);
						
						const Vec3 minMax[2] = { min, max };
						
						Mat4x4 nodeToWorld;
						
						gxGetMatrixf(GX_MODELVIEW, nodeToWorld.m_v);
						
						// 1) construct all eight vertices of the bounding box giving by min, max
						// 2) transform the min, max from node space into world-space
						// 3) compare with bounding box min, max and expand bounding box
						
						for (int x = 0; x < 2; ++x)
						{
							for (int y = 0; y < 2; ++y)
							{
								for (int z = 0; z < 2; ++z)
								{
									const Vec3 point_node(
										minMax[x][0],
										minMax[y][1],
										minMax[z][2]);
									
									const Vec3 point_world = nodeToWorld.Mul4(point_node);
									
									if (boundingBox.hasMinMax)
									{
										for (int i = 0; i < 3; ++i)
										{
											boundingBox.min[i] = fminf(boundingBox.min[i], point_world[i]);
											boundingBox.max[i] = fmaxf(boundingBox.max[i], point_world[i]);
										}
									}
									else
									{
										boundingBox.hasMinMax = true;
										boundingBox.min = point_world;
										boundingBox.max = point_world;
									}
								}
							}
						}
					}
				}
				
				gxPopMatrix();
			};
			
			BoundingBox boundingBox;
			
			if (scene.activeScene >= 0 && scene.activeScene < scene.sceneRoots.size())
			{
				auto & sceneRoot = scene.sceneRoots[scene.activeScene];
			
				for (auto & node_index : sceneRoot.nodes)
				{
					if (node_index >= 0 && node_index < scene.nodes.size())
					{
						auto & node = scene.nodes[node_index];
						
						calculateNodeMinMaxTraverse(node, boundingBox);
					}
				}
			}
			
			if (keyboard.wentDown(SDLK_p))
			{
				const float distance = (boundingBox.max - boundingBox.min).CalcSize() / 2.f * .9f;
				const Vec3 target = (boundingBox.min + boundingBox.max) / 2.f;
				
			#if ANIMATED_CAMERA
				camera.desiredOrientation.fromAxisAngle(Vec3(0, 1, 0), random(0.f, float(M_PI) * 2.f));
				const Mat4x4 orientationMatrix = camera.desiredOrientation.toMatrix();
				const Vec3 orientationVector = orientationMatrix.GetAxis(2);
				camera.desiredPosition = target - orientationVector * distance;
				camera.animate = true;
				camera.animationSpeed = .9f;
			#else
				camera.pitch = 8.f;
				camera.yaw = random(0.f, 360.f);
				
				camera.position = target - camera.getWorldMatrix().GetAxis(2) * distance;
			#endif
			}
			
			for (int i = 0; i < 2; ++i)
			{
				const bool isOpaquePass = (i == 0);
				
				if (scene.activeScene < 0 || scene.activeScene >= scene.sceneRoots.size())
				{
					logWarning("invalid scene index");
					continue;
				}
				
				pushDepthWrite(keyboard.isDown(SDLK_z) ? true : isOpaquePass ? true : false);
				{
					auto & sceneRoot = scene.sceneRoots[scene.activeScene];
					
					for (auto & node_index : sceneRoot.nodes)
					{
						if (node_index >= 0 && node_index < scene.nodes.size())
						{
							auto & node = scene.nodes[node_index];
							
							drawNodeTraverse(node, isOpaquePass);
						}
					}
				}
				popDepthWrite();
			}
			
			if (keyboard.isDown(SDLK_b))
			{
				pushBlend(BLEND_ADD);
				pushDepthWrite(false);
				{
					if (scene.activeScene >= 0 && scene.activeScene < scene.sceneRoots.size())
					{
						auto & sceneRoot = scene.sceneRoots[scene.activeScene];
						
						for (auto & node_index : sceneRoot.nodes)
						{
							if (node_index >= 0 && node_index < scene.nodes.size())
							{
								auto & node = scene.nodes[node_index];
								
								drawNodeMinMaxTraverse(node);
							}
						}
					}
				}
				popDepthWrite();
				popBlend();
			}
			
			camera.popViewMatrix();
			popBlend();
			popDepthTest();
			
			projectScreen2d();
			
			setColor(0, 0, 0, 127);
			hqBegin(HQ_FILLED_ROUNDED_RECTS);
			hqFillRoundedRect(4, 4, VIEW_SX - 4, 90, 10.f);
			hqEnd();
			
			setFont("calibri.ttf");
			setColor(colorWhite);
			setLumi(170);
			
			drawText(10, 30, 16, +1, -1, "(Extra) Author: %s", scene.asset.extras.author.c_str());
			drawText(10, 50, 16, +1, -1, "(Extra) License: %s", scene.asset.extras.license.c_str());
			drawText(10, 70, 16, +1, -1, "(Extra) Title: %s", scene.asset.extras.title.c_str());
		}
		framework.endDraw();
		
	#if LOW_LATENCY_HACK_TEST
		static uint64_t t1 = 0;
		static uint64_t t2 = 0;
		
		static int x = 0;
		x++;
		printf("frame: %d\n", x);
		
		t2 = SDL_GetTicks();
		
		const uint64_t time_ms = t2 - t1 + 1;
		
		if (time_ms < 16 && !keyboard.isDown(SDLK_RSHIFT))
		{
			const uint64_t delay_ms = 16 - time_ms;
			
			usleep(delay_ms * 1000);
		}
		
		t1 = SDL_GetTicks();
	#endif
	}
	
	Font("calibri.ttf").saveCache();
	
	framework.shutdown();

	return 0;
}
