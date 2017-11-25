/*
	Copyright (C) 2017 Marcel Smit
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

#include "model_ogre.h"
#include "tinyxml2.h"

using namespace tinyxml2;

#define XML_MODEL "mesh"
#define XML_MESHES "submeshes"
#define XML_MESH "submesh"
#define XML_FACES "faces"
#define XML_FACE "face"
#define XML_GEOMETRY "geometry"
#define XML_VERTEX_STREAM "vertexbuffer"
#define XML_VERTEX "vertex"
#define XML_BONE_MAPPINGS "boneassignments"
#define XML_BONE_MAPPING "vertexboneassignment"

#define XML_SKELETON "skeleton"
#define XML_BONES "bones"
#define XML_BONE "bone"
#define XML_BONE_HIERARCHY "bonehierarchy"
#define XML_BONE_LINK "boneparent"
#define XML_ANIMATIONS "animations"
#define XML_ANIMATION "animation"
#define XML_TRACKS "tracks"
#define XML_TRACK "track"
#define XML_KEYFRAMES "keyframes"
#define XML_KEYFRAME "keyframe"
#define XML_KEYFRAME_TRANSLATION "translate"
#define XML_KEYFRAME_ROTATION "rotate"
#define XML_KEYFRAME_ROTATION_AXIS "axis"

namespace AnimModel
{
	MeshSet * LoaderOgreXML::loadMeshSet(const char * filename, const BoneSet * boneSet)
	{
		std::map<int, int> boneIndexRemappingTable;
		for (int i = 0; i < boneSet->m_numBones; ++i)
			boneIndexRemappingTable[boneSet->m_bones[i].originalIndex] = i;
		
		//
		
		std::vector<Mesh*> meshes;
		
		XMLDocument xmlModelDoc;
		
		xmlModelDoc.LoadFile(filename);
		
		XMLElement * xmlModel = xmlModelDoc.FirstChildElement(XML_MODEL);
		
		if (xmlModel)
		{
			XMLElement * xmlMeshes = xmlModel->FirstChildElement(XML_MESHES);
			
			if (xmlMeshes)
			{
				XMLElement * xmlMesh = xmlMeshes->FirstChildElement(XML_MESH);
				
				while (xmlMesh)
				{
					Mesh * mesh = new Mesh();
					
					std::vector<int> numVertexBones;
					
					// mesh (material, operationtype=triangle_list)
					
					XMLElement * xmlGeometry = xmlMesh->FirstChildElement(XML_GEOMETRY);
					
					if (xmlGeometry)
					{
						// geometry (vertexcount)
						
						const int vertexCount = xmlGeometry->IntAttribute("vertexcount");
						
						mesh->allocateVB(vertexCount);
						
						numVertexBones.resize(vertexCount, 0);
						
						XMLElement * xmlVertexStream = xmlGeometry->FirstChildElement(XML_VERTEX_STREAM);
						
						while (xmlVertexStream)
						{
							// vertexbuffer (positions, normals, texture_coord=N)
							
							const bool positions = xmlVertexStream->BoolAttribute("positions");
							const bool normals = xmlVertexStream->BoolAttribute("normals");
							const int texcoords = xmlVertexStream->IntAttribute("texture_coords");
							
							//printf("vertex stream: position=%d, normal=%d, texcoords=%d!\n", (int)positions, (int)normals, texcoords);
							
							XMLElement * xmlVertex = xmlVertexStream->FirstChildElement(XML_VERTEX);
							
							int vertexIndex = 0;
							
							while (xmlVertex)
							{
								fassert(vertexIndex + 1 <= mesh->m_numVertices);
								
								if (vertexIndex + 1 <= mesh->m_numVertices)
								{
									if (positions)
									{
										// position (x, y, z)
										
										XMLElement * xmlPosition = xmlVertex->FirstChildElement("position");
										
										if (xmlPosition)
										{
											mesh->m_vertices[vertexIndex].px = xmlPosition->FloatAttribute("x");
											mesh->m_vertices[vertexIndex].py = xmlPosition->FloatAttribute("y");
											mesh->m_vertices[vertexIndex].pz = xmlPosition->FloatAttribute("z");
										}
									}
									
									if (normals)
									{
										// normal (x, y, z)
										
										XMLElement * xmlNormal = xmlVertex->FirstChildElement("normal");
										
										if (xmlNormal)
										{
											mesh->m_vertices[vertexIndex].nx = xmlNormal->FloatAttribute("x");
											mesh->m_vertices[vertexIndex].ny = xmlNormal->FloatAttribute("y");
											mesh->m_vertices[vertexIndex].nz = xmlNormal->FloatAttribute("z");
										}
									}
									
									if (texcoords)
									{
										// texcoord (u, v)
										
										XMLElement * xmlTexcoord = xmlVertex->FirstChildElement("texcoord");
										
										if (xmlTexcoord)
										{
											mesh->m_vertices[vertexIndex].tx = xmlTexcoord->FloatAttribute("u");
											mesh->m_vertices[vertexIndex].ty = xmlTexcoord->FloatAttribute("v");
										}
									}
								}
								
								vertexIndex++;
								
								xmlVertex = xmlVertex->NextSiblingElement(XML_VERTEX);
							}
							
							xmlVertexStream = xmlVertexStream->NextSiblingElement(XML_VERTEX_STREAM);
						}
					}
					
					XMLElement * xmlFaces = xmlMesh->FirstChildElement(XML_FACES);
					
					if (xmlFaces)
					{
						// faces (count)
						
						const int numFaces = xmlFaces->IntAttribute("count");
						
						mesh->allocateIB(numFaces * 3);
						
						//printf("faces! count=%d\n", numFaces);
						
						XMLElement * xmlFace = xmlFaces->FirstChildElement(XML_FACE);
						
						int indexIndex = 0;
						
						while (xmlFace)
						{
							fassert(indexIndex + 3 <= mesh->m_numIndices);
							
							if (indexIndex + 3 <= mesh->m_numIndices)
							{
								// v1, v2, v3
								
								mesh->m_indices[indexIndex + 0] = xmlFace->IntAttribute("v1");
								mesh->m_indices[indexIndex + 1] = xmlFace->IntAttribute("v2");
								mesh->m_indices[indexIndex + 2] = xmlFace->IntAttribute("v3");
							}
							
							indexIndex += 3;
							
							xmlFace = xmlFace->NextSiblingElement(XML_FACE);
						}
					}
									
					XMLElement * xmlBoneMappings = xmlMesh->FirstChildElement(XML_BONE_MAPPINGS);
					
					if (xmlBoneMappings)
					{
						XMLElement * xmlBoneMapping = xmlBoneMappings->FirstChildElement(XML_BONE_MAPPING);
						
						while (xmlBoneMapping)
						{
							// vertexindex, boneindex, weight
							
							const int vertexIndex = xmlBoneMapping->IntAttribute("vertexindex");
							const int boneIndex = boneIndexRemappingTable[xmlBoneMapping->IntAttribute("boneindex")];
							const float weight = xmlBoneMapping->FloatAttribute("weight");
							
							//printf("vertexIndex=%d, boneIndex=%d, weight=%g\n", vertexIndex, boneIndex, weight);
							
							fassert(vertexIndex + 1 <= mesh->m_numVertices);
							
							if (vertexIndex + 1 <= mesh->m_numVertices)
							{
								// todo: replace lowest weight element smaller than the current weight if there's no more space
								
								if (numVertexBones[vertexIndex] < 4)
								{
									const int vertexBoneIndex = numVertexBones[vertexIndex]++;
									
									mesh->m_vertices[vertexIndex].boneIndices[vertexBoneIndex] = boneIndex;
									mesh->m_vertices[vertexIndex].boneWeights[vertexBoneIndex] = weight * 255.f;
								}
							}
							
							xmlBoneMapping = xmlBoneMapping->NextSiblingElement(XML_BONE_MAPPING);
						}
					}
					
					// todo: improve weighting, so it always adds up to 255
					
					for (int i = 0; i < mesh->m_numVertices; ++i)
					{
						Vertex & vertex = mesh->m_vertices[i];
						
						const float totalWeight =
							(
								vertex.boneWeights[0] +
								vertex.boneWeights[1] +
								vertex.boneWeights[2] +
								vertex.boneWeights[3]
							) / 255.f;
						
						if (totalWeight == 0.f)
						{
							vertex.boneWeights[0] = 255;
						}
						else
						{
							for (int j = 0; j < 4; ++j)
							{
								vertex.boneWeights[j] /= totalWeight;
							}
						}
					}
					
					meshes.push_back(mesh);
					
					xmlMesh = xmlMesh->NextSiblingElement(XML_MESH);
				}
			}
		}
		
		MeshSet * meshSet = new MeshSet();
		meshSet->allocate(meshes.size());
		for (size_t i = 0; i < meshes.size(); ++i)
		{
			meshes[i]->finalize();
			
			meshSet->m_meshes[i] = meshes[i];
		}
		
		return meshSet;
	}
	
	BoneSet * LoaderOgreXML::loadBoneSet(const char * filename)
	{
		XMLDocument xmlSkeletonDoc;
		
		xmlSkeletonDoc.LoadFile(filename);
		
		XMLElement * xmlSkeleton = xmlSkeletonDoc.FirstChildElement(XML_SKELETON);
		
		// calculate max bone index with data from skeleton
		
		int maxBoneIndex = -1;
		
		XMLElement * xmlBones = xmlSkeleton->FirstChildElement(XML_BONES);
		
		if (xmlBones)
		{
			XMLElement * xmlBone = xmlBones->FirstChildElement(XML_BONE);
			
			while (xmlBone)
			{
				const int boneIndex = xmlBone->IntAttribute("id");
				
				if (boneIndex > maxBoneIndex)
					maxBoneIndex = boneIndex;
				
				xmlBone = xmlBone->NextSiblingElement(XML_BONE);
			}
		}
		
		//printf("maxBoneIndex=%d\n", maxBoneIndex);
		
		// read bone data
		
		BoneSet * boneSet = new BoneSet();
		
		boneSet->allocate(maxBoneIndex + 1);
		
		if (xmlSkeleton)
		{
			//printf("skeleton!\n");
			
			typedef std::map<std::string, int> BoneNameToBoneIndexMap;
			BoneNameToBoneIndexMap boneNameToBoneIndex;
			
			XMLElement * xmlBones = xmlSkeleton->FirstChildElement(XML_BONES);
			
			if (xmlBones)
			{
				//printf("bones!\n");
				
				XMLElement * xmlBone = xmlBones->FirstChildElement(XML_BONE);
				
				while (xmlBone)
				{
					const int boneIndex = xmlBone->IntAttribute("id");
					const char * name = xmlBone->Attribute("name");
					
					if (!name)
						name = "";
					
					fassert(boneIndex + 1 <= boneSet->m_numBones);
					
					if (boneIndex + 1 <= boneSet->m_numBones)
					{
						//printf("bone! boneIndex=%d, name=%s\n", boneIndex, name);
						
						boneNameToBoneIndex[name] = boneIndex;
						
						boneSet->m_bones[boneIndex].name = name;
						
						// read transform
						
						BoneTransform & transform = boneSet->m_bones[boneIndex].transform;
						
						XMLElement * xmlPosition = xmlBone->FirstChildElement("position");
						
						float px = 0.f;
						float py = 0.f;
						float pz = 0.f;
						
						if (xmlPosition)
						{
							px = xmlPosition->FloatAttribute("x");
							py = xmlPosition->FloatAttribute("y");
							pz = xmlPosition->FloatAttribute("z");
						}
						
						XMLElement * xmlRotation = xmlBone->FirstChildElement("rotation");
						
						float ra = 0.f;
						float rx = 1.f;
						float ry = 0.f;
						float rz = 0.f;
						
						if (xmlRotation)
						{
							ra = xmlRotation->FloatAttribute("angle");
							
							XMLElement * xmlRotationAxis = xmlRotation->FirstChildElement("axis");
							
							if (xmlRotationAxis)
							{
								rx = xmlRotationAxis->FloatAttribute("x");
								ry = xmlRotationAxis->FloatAttribute("y");
								rz = xmlRotationAxis->FloatAttribute("z");
							}
						}
						
						// todo: convert rotation to quat-log ?
						
						transform.translation = Vec3(px, py, pz);
						transform.rotation.fromAxisAngle(Vec3(rx, ry, rz), ra);
						transform.scale = Vec3(1.f, 1.f, 1.f);
					}
					
					xmlBone = xmlBone->NextSiblingElement(XML_BONE);
				}
			}
			
			// read hierarchy information
			
			XMLElement * xmlBoneHierarchy = xmlSkeleton->FirstChildElement(XML_BONE_HIERARCHY);
			
			if (xmlBoneHierarchy)
			{
				//printf("hierarchy!\n");
				
				XMLElement * xmlBoneLink = xmlBoneHierarchy->FirstChildElement(XML_BONE_LINK);
				
				while (xmlBoneLink)
				{
					const char * boneName = xmlBoneLink->Attribute("bone");
					const char * parentName = xmlBoneLink->Attribute("parent");
					
					if (boneName && parentName)
					{
						const int boneIndex = boneNameToBoneIndex[boneName];
						const int parentIndex = boneNameToBoneIndex[parentName];
						
						//printf("bone link: %d -> %d\n", boneIndex, parentIndex);
						
						fassert(boneIndex + 1 <= boneSet->m_numBones);
						fassert(parentIndex + 1 <= boneSet->m_numBones);
						
						if ((boneIndex + 1 <= boneSet->m_numBones) && (parentIndex + 1 <= boneSet->m_numBones))
						{
							boneSet->m_bones[boneIndex].parent = parentIndex;
						}
					}
					
					xmlBoneLink = xmlBoneLink->NextSiblingElement(XML_BONE_LINK);
				}
			}
		}
		
		boneSet->calculatePoseMatrices();
		boneSet->sortBoneIndices();
		
		return boneSet;
	}
	
	AnimSet * LoaderOgreXML::loadAnimSet(const char * filename, const BoneSet * boneSet)
	{
		// set up bone name -> bone index map
		
		typedef std::map<std::string, int> BoneNameToBoneIndexMap;
		BoneNameToBoneIndexMap boneNameToBoneIndex;
		
		for (int i = 0; i < boneSet->m_numBones; ++i)
			boneNameToBoneIndex[boneSet->m_bones[i].name] = i;
		
		//
		
		XMLDocument xmlSkeletonDoc;
		
		xmlSkeletonDoc.LoadFile(filename);
		
		std::map<std::string, Anim*> animations;
		
		XMLElement * xmlSkeleton = xmlSkeletonDoc.FirstChildElement(XML_SKELETON);
		
		if (xmlSkeleton)
		{
			XMLElement * xmlAnimations = xmlSkeleton->FirstChildElement(XML_ANIMATIONS);
			
			if (xmlAnimations)
			{
				XMLElement * xmlAnimation = xmlAnimations->FirstChildElement(XML_ANIMATION);
				
				while (xmlAnimation)
				{
					// animation (name, length)
					
					const char * animName = xmlAnimation->Attribute("name");
					
					if (!animName)
						animName = "";
					
					//printf("animation! %s\n", animName);
						
					XMLElement * xmlTracks = xmlAnimation->FirstChildElement(XML_TRACKS);
					
					if (xmlTracks)
					{
						typedef std::map< int, std::vector<AnimKey> > AnimKeysByBoneIndexMap;
						AnimKeysByBoneIndexMap animKeysByBoneIndex;
						
						XMLElement * xmlTrack = xmlTracks->FirstChildElement(XML_TRACK);
						
						while (xmlTrack)
						{
							// track (bone)
							
							const char * boneName = xmlTrack->Attribute("bone");
							
							if (!boneName)
								boneName = "";
							
							BoneNameToBoneIndexMap::iterator i = boneNameToBoneIndex.find(boneName);
							
							fassert(i != boneNameToBoneIndex.end());
							
							if (i != boneNameToBoneIndex.end())
							{
								const int boneIndex = i->second;
								
								fassert(boneIndex + 1 <= boneSet->m_numBones);
								
								if (boneIndex + 1 <= boneSet->m_numBones)
								{
									std::vector<AnimKey> & animKeys = animKeysByBoneIndex[boneIndex];
									
									XMLElement * xmlKeyFrames = xmlTrack->FirstChildElement(XML_KEYFRAMES);
									
									if (xmlKeyFrames)
									{
										XMLElement * xmlKeyFrame = xmlKeyFrames->FirstChildElement(XML_KEYFRAME);
										
										while (xmlKeyFrame)
										{
											// keyframe (time -> translate, rotate)
											
											const float time = xmlKeyFrame->FloatAttribute("time");
											
											XMLElement * translation = xmlKeyFrame->FirstChildElement(XML_KEYFRAME_TRANSLATION);
											
											float px = 0.f;
											float py = 0.f;
											float pz = 0.f;
											
											if (translation)
											{
												px = translation->FloatAttribute("x");
												py = translation->FloatAttribute("y");
												pz = translation->FloatAttribute("z");
											}
											
											XMLElement * rotation = xmlKeyFrame->FirstChildElement(XML_KEYFRAME_ROTATION);
											
											float ra = 0.f;
											float rx = 0.f;
											float ry = 0.f;
											float rz = 0.f;
											
											if (rotation)
											{
												ra = rotation->FloatAttribute("angle");
												
												XMLElement * rotationAxis = rotation->FirstChildElement(XML_KEYFRAME_ROTATION_AXIS);
												
												if (rotationAxis)
												{
													rx = rotationAxis->FloatAttribute("x");
													ry = rotationAxis->FloatAttribute("y");
													rz = rotationAxis->FloatAttribute("z");
												}
											}
											
											AnimKey key;
											
											key.time = time;
											key.translation = Vec3(px, py, pz);
											Quat quat;
											quat.fromAxisAngle(Vec3(rx, ry, rz), ra);
											key.rotation[0] = quat[0];
											key.rotation[1] = quat[1];
											key.rotation[2] = quat[2];
											key.rotation[3] = quat[3];
											key.scale = Vec3(0.f, 0.f, 0.f);
											
											animKeys.push_back(key);
											
											xmlKeyFrame = xmlKeyFrame->NextSiblingElement(XML_KEYFRAME);
										}
									}
								}
							}
							
							xmlTrack = xmlTrack->NextSiblingElement(XML_TRACK);
						}
						
						// finalize animation
						
						Anim * animation = new Anim();
						
						int numAnimKeys = 0;
						
						for (int i = 0; i < boneSet->m_numBones; ++i)
						{
							const std::vector<AnimKey> & animKeys = animKeysByBoneIndex[i];
							
							numAnimKeys += int(animKeys.size());
						}
						
						animation->allocate(boneSet->m_numBones, numAnimKeys, RotationType_Quat, true);
						
						AnimKey * finalAnimKey = animation->m_keys;
						
						for (int i = 0; i < boneSet->m_numBones; ++i)
						{
							const std::vector<AnimKey> & animKeys = animKeysByBoneIndex[i];
							
							for (size_t j = 0; j < animKeys.size(); ++j)
							{
								*finalAnimKey++ = animKeys[j];
							}
							
							animation->m_numKeys[i] = animKeys.size();
						}
						
						animations[animName] = animation;
					}
					
					xmlAnimation = xmlAnimation->NextSiblingElement(XML_ANIMATION);
				}
			}
		}
		
		AnimSet * animSet = new AnimSet();
		animSet->m_animations = animations;
		
		return animSet;
	}
}
