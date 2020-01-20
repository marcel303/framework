#include "gltf.h"
#include "gltf-loader.h"
#include "json.hpp"
#include "Path.h"
#include "TextIO.h"

using json = nlohmann::json;

static void from_json(const json & j, Vec3 & v)
{
	v[0] = j.at(0).get<float>();
	v[1] = j.at(1).get<float>();
	v[2] = j.at(2).get<float>();
}

static void from_json(const json & j, Color & c)
{
	c.r = j.at(0).get<float>();
	c.g = j.at(1).get<float>();
	c.b = j.at(2).get<float>();
	c.a = j.at(3).get<float>();
}

static void from_json(const json & j, Quat & q)
{
	q[0] = j.at(0).get<float>();
	q[1] = j.at(1).get<float>();
	q[2] = j.at(2).get<float>();
	q[3] = j.at(3).get<float>();
}

static void from_json(const json & j, Mat4x4 & m)
{
	for (int i = 0; i < 16; ++i)
		m.m_v[i] = j.at(i).get<float>();
}

namespace gltf
{
	bool loadScene(const char * path, gltf::Scene & scene)
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
				
				scene.accessors.resize(accessors.size());
				
				int idx = 0;
				
				for (auto & json_accessor : accessors)
				{
					gltf::Accessor & accessor = scene.accessors[idx++];
					
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
				
				scene.buffers.resize(buffers.size());
				
				int idx = 0;
				
				for (auto & buffer_json : buffers)
				{
					gltf::Buffer & buffer = scene.buffers[idx++];
					
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
					
					buffer.data = (uint8_t*)malloc(buffer.byteLength);
					
					if (buffer.byteLength > 0)
					{
						if (fread(buffer.data, buffer.byteLength, 1, file) != 1)
						{
							fclose(file);
							file = nullptr;

							logDebug("failed to read buffer from file");
							return false;
						}
					}
					
					fclose(file);
					file = nullptr;
				}
			}
			else if (member_name == "bufferViews")
			{
				auto & bufferViews = member;
				
				scene.bufferViews.resize(bufferViews.size());
				
				int idx = 0;
				
				for (auto & json_bufferView : bufferViews)
				{
					gltf::BufferView & bufferView = scene.bufferViews[idx++];
					
					bufferView.buffer = json_bufferView.value("buffer", -1);
					bufferView.byteOffset = json_bufferView.value("byteOffset", 0);
					bufferView.byteLength = json_bufferView.value("byteLength", -1);
					bufferView.byteStride = json_bufferView.value("byteStride", 0);
					bufferView.target = json_bufferView.value("target", -1);
					bufferView.name = json_bufferView.value("name", "");
					
					if (!bufferView.isValid())
					{
						logDebug("bufferView is invalid");
						return false;
					}
				}
			}
			else if (member_name == "meshes")
			{
				auto & meshes = member;
				
				scene.meshes.resize(meshes.size());
				
				int idx = 0;
				
				for (auto & mesh_json : meshes)
				{
					gltf::Mesh & mesh = scene.meshes[idx++];
					
					for (auto mesh_member_itr = mesh_json.begin(); mesh_member_itr != mesh_json.end(); ++mesh_member_itr)
					{
						if (mesh_member_itr.key() == "primitives")
						{
							auto & mesh_member = mesh_member_itr.value();
							
							mesh.primitives.resize(mesh_member.size());
							
							int prim_idx = 0;
							
							for (auto & primitive_json : mesh_member)
							{
								gltf::MeshPrimitive & primitive = mesh.primitives[prim_idx++];
								
								primitive.indices = primitive_json.value("indices", -1);
								primitive.material = primitive_json.value("material", -1);
								primitive.mode = primitive_json.value("mode", 4);
								
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
							}
						}
					}
				}
			}
			else if (member_name == "images")
			{
				auto & images = member;
				
				scene.images.resize(images.size());
				
				int idx = 0;
				
				for (auto & image_json : images)
				{
					gltf::Image & image = scene.images[idx++];
					
					image.uri = image_json.value("uri", "");
					image.name = image_json.value("name", "");
					
					image.path = dir + "/" + image.uri;
					
					// todo : add support for bufferView property ?
					
					if (!image.isValid())
					{
						logDebug("image is invalid");
						return false;
					}
				}
			}
			else if (member_name == "textures")
			{
				auto & textures = member;
				
				scene.textures.resize(textures.size());
				
				int idx = 0;
				
				for (auto & texture_json : textures)
				{
					gltf::Texture & texture = scene.textures[idx++];
					
					texture.sampler = texture_json.value("sampler", -1);
					texture.source = texture_json.value("source", -1);
					
					if (!texture.isValid())
					{
						logDebug("texture is invalid");
						return false;
					}
				}
			}
			else if (member_name == "samplers")
			{
				auto & samplers = member;
				
				scene.samplers.resize(samplers.size());
				
				int idx = 0;
				
				for (auto & sampler_json : samplers)
				{
					gltf::Sampler & sampler = scene.samplers[idx++];
					
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
					{
						logDebug("sampler is invalid");
						return false;
					}
				}
			}
			else if (member_name == "materials")
			{
				auto & materials = member;
				
				scene.materials.resize(materials.size());
				
				int idx = 0;
				
				for (auto & material_json : materials)
				{
					gltf::Material & material = scene.materials[idx++];
					
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
							
							material.pbrSpecularGlossiness.isSet = true;
							
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
				}
			}
			else if (member_name == "nodes")
			{
				auto & nodes_json = member;
				
				scene.nodes.resize(nodes_json.size());
				
				int idx = 0;
				
				for (auto & node_json : nodes_json)
				{
					gltf::Node & node = scene.nodes[idx++];
					
					node.name = node_json.value("name", "");
					node.mesh = node_json.value("mesh", -1);
					node.children = node_json.value("children", std::vector<int>());
					node.translation = node_json.value("translation", Vec3(0, 0, 0));
					node.rotation = node_json.value("rotation", Quat(0, 0, 0, 1));
					node.scale = node_json.value("scale", Vec3(1, 1, 1));
					node.matrix = node_json.value("matrix", Mat4x4(true));
				}
			}
			else if (member_name == "scene")
			{
				scene.activeScene = member_itr.value();
			}
			else if (member_name == "scenes")
			{
				auto & scenes_json = member;
				
				scene.sceneRoots.resize(scenes_json.size());
				
				int idx = 0;
				
				for (auto & scene_json : scenes_json)
				{
					gltf::SceneRoot & sceneRoot = scene.sceneRoots[idx++];
					
					sceneRoot.name = scene_json.value("name", "");
					sceneRoot.nodes = scene_json.value("nodes", std::vector<int>());
				}
			}
			else
			{
				logDebug("unknown key: %s", member_name.c_str());
			}
		}
		
		return true;
	}
}
