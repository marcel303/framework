#pragma once

#include "framework.h" // Color
#include "Quat.h"
#include "Vec3.h"
#include <map>
#include <string>
#include <vector>

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

	//

	bool resolveBufferView(const Scene & scene, const int index, const gltf::Accessor *& accessor, const gltf::BufferView *& bufferView, const gltf::Buffer *& buffer);
}
