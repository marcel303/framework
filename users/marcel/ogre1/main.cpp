#include <assert.h>
#include <map>
#include <stdlib.h>
#include <string>
#include <vector>
#include "tinyxml2.h"
#include "Quaternion.h"

#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <string.h>

using namespace tinyxml2;

// forward declarations

class Anim;
class AnimKey;
class Bone;
class BoneTransform;
class Mesh;
class MeshSet;
class Model;
class Skeleton;
class Vertex;

//

class Vertex
{
public:
	float px, py, pz;
	float nx, ny, nz;
	float tx, ty;
	
	unsigned char boneIndices[4];
	unsigned char boneWeights[4];
};

class Mesh
{
public:
	Vertex * m_vertices;
	int m_numVertices;
	
	int * m_indices;
	int m_numIndices;
	
	Mesh()
	{
		m_vertices = 0;
		m_numVertices = 0;
		
		m_indices = 0;
		m_numIndices = 0;
	}
	
	~Mesh()
	{
		allocateVB(0);
		allocateIB(0);
	}
	
	void allocateVB(int numVertices)
	{
		if (m_numVertices)
		{
			delete [] m_vertices;
			m_vertices = 0;
			m_numVertices = 0;
		}
		
		if (numVertices)
		{
			m_vertices = new Vertex[numVertices];
			m_numVertices = numVertices;
			
			memset(m_vertices, 0, sizeof(m_vertices[0]) * numVertices);
		}
	}
	
	void allocateIB(int numIndices)
	{
		if (m_numIndices)
		{
			delete [] m_indices;
			m_indices = 0;
			m_numIndices = 0;
		}
		
		if (numIndices)
		{
			m_indices = new int[numIndices];
			m_numIndices = numIndices;
			
			memset(m_indices, 0, sizeof(m_indices[0]) * numIndices);
		}
	}
};

class MeshSet
{
public:
	Mesh ** m_meshes;
	int m_numMeshes;
	
	MeshSet()
	{
		m_meshes = 0;
		m_numMeshes = 0;
	}
	
	~MeshSet()
	{
		allocate(0);
	}
	
	void allocate(int numMeshes)
	{
		if (m_numMeshes)
		{
			for (int i = 0; i < m_numMeshes; ++i)
			{
				delete m_meshes[i];
				m_meshes[i] = 0;
			}
			delete [] m_meshes;
			m_meshes = 0;
			m_numMeshes = 0;
		}
		
		if (numMeshes)
		{
			m_meshes = new Mesh*[numMeshes];
			m_numMeshes = numMeshes;
			
			for (int i = 0; i < numMeshes; ++i)
				m_meshes[i] = 0;
		}
	}
};

class BoneTransform
{
public:
	Vec3 translation;
	Quaternion rotation;
	float scale;
	
	void clear()
	{
		translation.SetZero();
		rotation.makeIdentity();
	}
	
	static void add(BoneTransform & result, const BoneTransform & transform)
	{
		result.translation += transform.translation;
		result.rotation *= transform.rotation;
	}
	
	static void interpolate(BoneTransform & result, const BoneTransform & transform1, const BoneTransform & transform2, float time)
	{
		const float t1 = 1.f - time;
		const float t2 = time;
		
		result.translation += (transform1.translation * t1) + (transform2.translation * t2);
		result.rotation *= transform1.rotation.slerp(transform2.rotation, time);
	}
};

class Bone
{
public:
	Bone()
	{
		parent = -1;
	}
	
	BoneTransform transform;
	Mat4x4 poseMatrix;
	int parent;
};

class Skeleton
{
public:
	Bone * m_bones;
	int m_numBones;
	
	Skeleton()
	{
		m_bones = 0;
		m_numBones = 0;
	}
	
	~Skeleton()
	{
		allocate(0);
	}
	
	void allocate(int numBones)
	{
		if (m_numBones)
		{
			delete [] m_bones;
			m_bones = 0;
			m_numBones = 0;
		}
		
		if (numBones)
		{
			m_bones = new Bone[numBones];
			m_numBones = numBones;
		}
	}
	
	void calculatePoseMatrices()
	{
		for (int i = 0; i < m_numBones; ++i)
		{
			Mat4x4 & poseMatrix = m_bones[i].poseMatrix;
			
			poseMatrix.MakeIdentity();
			
			int boneIndex = i;
			
			while (boneIndex != -1)
			{
				Mat4x4 matrix;
				
				matrix = m_bones[boneIndex].transform.rotation.toMatrix();
				matrix.SetTranslation(m_bones[boneIndex].transform.translation);
				
				poseMatrix = poseMatrix * matrix;
				
				boneIndex = m_bones[boneIndex].parent;
			}
			
			poseMatrix = poseMatrix.CalcInv();
		}
	}
};

struct AnimKey
{
	float time;
	
	BoneTransform transform;
};

class Anim
{
public:
	int m_numBones;
	int * m_numKeys;
	AnimKey * m_keys;
	
	Anim()
	{
		m_numBones = 0;
		m_numKeys = 0;
		m_keys = 0;
	}
	
	~Anim()
	{
		allocate(0, 0);
	}
	
	void allocate(int numBones, int numKeys)
	{
		if (m_numBones)
		{
			m_numBones = 0;
			delete [] m_numKeys;
			m_numKeys = 0;
			delete [] m_keys;
			m_keys = 0;
		}
		
		if (numBones)
		{
			m_numBones = numBones;
			m_numKeys = new int[numBones];
			m_keys = new AnimKey[numKeys];
		}
	}
	
	bool evaluate(float time, BoneTransform * transforms)
	{
		bool isDone = true;
		
		const AnimKey * firstKey = m_keys;
		
		for (int i = 0; i < m_numBones; ++i)
		{
			const int numKeys = m_numKeys[i];
			
			if (numKeys)
			{
				const AnimKey * key = firstKey;
				const AnimKey * lastKey = firstKey + numKeys - 1;
				
				while (key != lastKey && time >= key[1].time)
				{
					key++;
				}
				
				if (key != lastKey && time >= key->time)
				{
					isDone = false;
					
					const AnimKey & key1 = key[0];
					const AnimKey & key2 = key[1];
					
					assert(time >= key1.time && time <= key2.time);
					
					const float t = (time - key1.time) / (key2.time - key1.time);
					
					assert(t >= 0.f && t <= 1.f);
					
					/*
					if (i == 0)
					{
						printf("found key. time=%f, key1=%f, key2=%f, t=%f!\n", time, key1.time, key2.time, t);
						printf("=> %f %f %f -> %f %f %f [%f %f %f %f]!\n",
							key1.transform.translation[0],
							key1.transform.translation[1],
							key1.transform.translation[2],
							key2.transform.translation[0],
							key2.transform.translation[1],
							key2.transform.translation[2],
							key1.transform.rotation[0],
							key1.transform.rotation[1],
							key1.transform.rotation[2],
							key1.transform.rotation[3]);
					}
					*/
					
					BoneTransform::interpolate(transforms[i], key1.transform, key2.transform, t);
				}
				else
				{
					// either the first or last key in the animation. copy value
					
					assert(key == firstKey || key == lastKey);
					
					BoneTransform::add(transforms[i], key->transform);
				}
				
				firstKey += numKeys;
			}
		}
		
		return isDone;
	}
};

class AnimSet
{
public:
	AnimSet()
	{
	}
	
	~AnimSet()
	{
		for (std::map<std::string, Anim*>::iterator i = m_animations.begin(); i != m_animations.end(); ++i)
		{
			Anim * anim = i->second;
			
			delete anim;
		}
		
		m_animations.clear();
	}
	
	std::map<std::string, Anim*> m_animations;
};

class Model
{
	Anim * m_currentAnim;
	
public:
	MeshSet * m_meshes;
	Skeleton * m_skeleton;
	AnimSet * m_animations;
	
	bool animIsDone;
	float animTime;
	int animLoop;
	float animSpeed;
	
	Model(MeshSet * meshes, Skeleton * skeleton, AnimSet * animations)
	{
		m_currentAnim = 0;
		
		m_meshes = meshes;
		m_skeleton = skeleton;
		m_animations = animations;
		
		animIsDone = true;
		animTime = 0.f;
		animLoop = 0;
		animSpeed = 1.f;
	}
	
	~Model()
	{
		delete m_meshes;
		m_meshes = 0;
		
		delete m_skeleton;
		m_skeleton = 0;
		
		delete m_animations;
		m_animations = 0;
	}
	
	void startAnim(const char * name, int loop = 1)
	{
		assert(loop != 0);
		
		std::map<std::string, Anim*>::iterator i = m_animations->m_animations.find(name);
		
		if (i != m_animations->m_animations.end())
		{
			m_currentAnim = i->second;
		}
		else
		{
			m_currentAnim = 0;
		}
		
		animIsDone = false;
		animTime = 0.f;
		animLoop = loop;
		
		if (animLoop > 0)
			animLoop--;
	}
	
	void process(float timeStep)
	{
		animTime += animSpeed * timeStep;
	}
	
	void draw()
	{
		// todo: build matrix
		
		Mat4x4 matrix;
		
		matrix.MakeTranslation(0.f, 0.f, 0.f);
		
		drawEx(matrix, true, false);
	}
	
	void drawEx(const Mat4x4 & matrix, bool drawMesh, bool drawBones)
	{
		// calculate transforms in local bone space
		
		BoneTransform * transforms = (BoneTransform*)alloca(sizeof(BoneTransform) * m_skeleton->m_numBones);
		
		for (int i = 0; i < m_skeleton->m_numBones; ++i)
		{
			transforms[i] = m_skeleton->m_bones[i].transform;
		}
		
		// apply animations
		
		if (m_currentAnim)
		{
			animIsDone = m_currentAnim->evaluate(animTime, transforms);
			
			if (animIsDone && (animLoop > 0 || animLoop < 0))
			{
				animIsDone = false;
				animTime = 0.f;
				if (animLoop > 0)
					animLoop--;
			}
		}
		
		/*
		if (m_currentAnim)
		{
			for (int i = 0; i < m_skeleton->m_numBones; ++i)
			{
				printf("[numKeys=%02d] (%+07.2f, %+07.2f, %+07.2f), %g, %+07.2f @ (%+05.2f, %+05.2f, %+05.2f)\n",
					m_currentAnim->m_numKeys[i],
					transforms[i].translation[0],
					transforms[i].translation[1],
					transforms[i].translation[2],
					transforms[i].scale,
					transforms[i].rotation[0],
					transforms[i].rotation[1],
					transforms[i].rotation[2],
					transforms[i].rotation[3]);
			}
		}
		*/
		
		// convert translation / rotation pairs into matrices
		
		Mat4x4 * localMatrices = (Mat4x4*)alloca(sizeof(Mat4x4) * m_skeleton->m_numBones);
		
		for (int i = 0; i < m_skeleton->m_numBones; ++i)
		{
			const BoneTransform & transform = transforms[i];
			
			localMatrices[i] = transform.rotation.toMatrix();			
			localMatrices[i].SetTranslation(transform.translation);
		}
		
		// calculate the bone hierarchy in world space
		
		Mat4x4 * worldMatrices = (Mat4x4*)alloca(sizeof(Mat4x4) * m_skeleton->m_numBones);
		
		for (int i = 0; i < m_skeleton->m_numBones; ++i)
		{
			// todo: calculate matrices for a given bone only once
			
			int boneIndex = i;
			
			Mat4x4 finalMatrix = localMatrices[boneIndex];
			
			boneIndex = m_skeleton->m_bones[boneIndex].parent;
			
			while (boneIndex != -1)
			{
				finalMatrix = localMatrices[boneIndex] * finalMatrix;
				
				boneIndex = m_skeleton->m_bones[boneIndex].parent;
			}
			
			worldMatrices[i] = matrix * finalMatrix;
		}
		
		// draw
		
		if (drawMesh)
		{
			for (int i = 0; i < m_meshes->m_numMeshes; ++i)
			{
				const Mesh * mesh = m_meshes->m_meshes[i];
				
				glBegin(GL_TRIANGLES);
				{
					for (int i = 0; i < mesh->m_numIndices; ++i)
					{
						const int vertexIndex = mesh->m_indices[i];
						
						const Vertex & vertex = mesh->m_vertices[vertexIndex];
						
						// todo: apply vertex transformation using a shader
						
						// -- software vertex blend (hard skinned) --
						const int boneIndex = vertex.boneIndices[0];
						const Mat4x4 & boneToWorld = worldMatrices[boneIndex];
						const Mat4x4 & worldToBone = m_skeleton->m_bones[boneIndex].poseMatrix;
						const Vec3 p = boneToWorld * worldToBone * Vec3(vertex.px, vertex.py, vertex.pz);
						// -- software vertex blend (hard skinned) --
						
						glColor3f(vertex.boneIndices[0] / 30.f, 1.f, 1.f);
						glTexCoord2f(vertex.tx, vertex.ty);
						glNormal3f(vertex.nx, vertex.ny, vertex.nz);
						glVertex3f(p[0], p[1], p[2]);
					}
				}
				glEnd();
			}
		}
		
		if (drawBones)
		{
			for (int i = 0; i < m_skeleton->m_numBones; ++i)
			{
				const int boneIndex1 = i;
				const int boneIndex2 = m_skeleton->m_bones[i].parent;
				
				if (boneIndex1 != -1)
				{
					const Mat4x4 & worldMatrix1 =                             worldMatrices[boneIndex1];
					const Mat4x4 & worldMatrix2 = boneIndex2 == -1 ? matrix : worldMatrices[boneIndex2];
					
					glBegin(GL_LINES);
					{
						const Vec3 p1 = worldMatrix1 * Vec3(0.f, 0.f, 0.f);
						const Vec3 p2 = worldMatrix2 * Vec3(0.f, 0.f, 0.f);
						
						glVertex3f(p1[0], p1[1], p1[2]);
						glVertex3f(p2[0], p2[1], p2[2]);
					}
					glEnd();
				}
			}
		}
	}
};

//

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

Model * loadModel(const char * meshFileName, const char * skeletonFileName)
{
	std::vector<Mesh*> meshes;
	
	XMLDocument xmlModelDoc;
	
	xmlModelDoc.LoadFile(meshFileName);
	
	XMLElement * xmlModel = xmlModelDoc.FirstChildElement(XML_MODEL);
	
	int maxBoneIndex = -1;
	
	if (xmlModel)
	{
		printf("model!\n");
		
		//Model * model = new Model();
		
		XMLElement * xmlMeshes = xmlModel->FirstChildElement(XML_MESHES);
		
		if (xmlMeshes)
		{
			printf("meshes!\n");
			
			XMLElement * xmlMesh = xmlMeshes->FirstChildElement(XML_MESH);
			
			while (xmlMesh)
			{
				printf("mesh!\n");
				
				Mesh * mesh = new Mesh();
				
				std::vector<int> numBones;
				
				// mesh (material, operationtype=triangle_list)
				
				XMLElement * xmlGeometry = xmlMesh->FirstChildElement(XML_GEOMETRY);
				
				if (xmlGeometry)
				{
					// geometry (vertexcount)
					
					const int vertexCount = xmlGeometry->IntAttribute("vertexcount");
					
					mesh->allocateVB(vertexCount);
					
					numBones.resize(vertexCount, 0);
					
					printf("geometry (vertexCount=%d)!\n", vertexCount);
					
					XMLElement * xmlVertexStream = xmlGeometry->FirstChildElement(XML_VERTEX_STREAM);
					
					while (xmlVertexStream)
					{
						// vertexbuffer (positions, normals, texture_coord=N)
						
						const bool positions = xmlVertexStream->BoolAttribute("positions");
						const bool normals = xmlVertexStream->BoolAttribute("normals");
						const int texcoords = xmlVertexStream->IntAttribute("texture_coords");
						
						printf("vertex stream: position=%d, normal=%d, texcoords=%d!\n", (int)positions, (int)normals, texcoords);
						
						XMLElement * xmlVertex = xmlVertexStream->FirstChildElement(XML_VERTEX);
						
						int vertexIndex = 0;
						
						while (xmlVertex)
						{
							assert(vertexIndex + 1 <= mesh->m_numVertices);
							
							if (positions)
							{
								// position (x, y, z)
								
								XMLElement * xmlPosition = xmlVertex->FirstChildElement("position");
								
								if (xmlPosition)
								{
									const float x = xmlPosition->FloatAttribute("x");
									const float y = xmlPosition->FloatAttribute("y");
									const float z = xmlPosition->FloatAttribute("z");
									
									//printf("position: %d: %g %g %g\n", vertexIndex, x, y, z);
									
									mesh->m_vertices[vertexIndex].px = x;
									mesh->m_vertices[vertexIndex].py = y;
									mesh->m_vertices[vertexIndex].pz = z;
								}
							}
							
							if (normals)
							{
								// normal (x, y, z)
								
								XMLElement * xmlNormal = xmlVertex->FirstChildElement("normal");
								
								if (xmlNormal)
								{
									const float x = xmlNormal->FloatAttribute("x");
									const float y = xmlNormal->FloatAttribute("y");
									const float z = xmlNormal->FloatAttribute("z");
									
									printf("normal: %g %g %g\n", x, y, z);
									
									mesh->m_vertices[vertexIndex].nx = x;
									mesh->m_vertices[vertexIndex].ny = y;
									mesh->m_vertices[vertexIndex].nz = z;
								}
							}
							
							if (texcoords)
							{
								// texcoord (u, v)
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
					
					printf("faces (count=%d)!\n", numFaces);
					
					XMLElement * xmlFace = xmlFaces->FirstChildElement(XML_FACE);
					
					int indexIndex = 0;
					
					while (xmlFace)
					{
						assert(indexIndex + 3 <= mesh->m_numIndices);
						
						//printf("face!\n");
						
						// v1, v2, v3
						
						const int index1 = xmlFace->IntAttribute("v1");
						const int index2 = xmlFace->IntAttribute("v2");
						const int index3 = xmlFace->IntAttribute("v3");
						
						//printf("indices: %d, %d, %d\n", index1, index2, index3);
						
						mesh->m_indices[indexIndex + 0] = index1;
						mesh->m_indices[indexIndex + 1] = index2;
						mesh->m_indices[indexIndex + 2] = index3;
						
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
						const int boneIndex = xmlBoneMapping->IntAttribute("boneindex");
						const float weight = xmlBoneMapping->FloatAttribute("weight");
						
						assert(vertexIndex + 1 <= mesh->m_numVertices);
				
						//printf("vertexIndex=%d, boneIndex=%d, weight=%g\n", vertexIndex, boneIndex, weight);
						
						if (boneIndex > maxBoneIndex)
							maxBoneIndex = boneIndex;
						
						if (numBones[vertexIndex] < 4)
						{
							const int boneIndex2 = numBones[vertexIndex]++;
							
							//printf("added: boneIndex2=%d\n", boneIndex2);
							
							mesh->m_vertices[vertexIndex].boneIndices[boneIndex2] = boneIndex;
							mesh->m_vertices[vertexIndex].boneWeights[boneIndex2] = weight * 255.f;
						}
						
						xmlBoneMapping = xmlBoneMapping->NextSiblingElement(XML_BONE_MAPPING);
					}
				}
				
				meshes.push_back(mesh);
				
				xmlMesh = xmlMesh->NextSiblingElement(XML_MESH);
			}
		}
	}
	
	XMLDocument xmlSkeletonDoc;
	
	xmlSkeletonDoc.LoadFile(skeletonFileName);
	
	XMLElement * xmlSkeleton = xmlSkeletonDoc.FirstChildElement(XML_SKELETON);
	
	{
		// update max bone index with data from skeleton
		
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
	}
	
	printf("maxBoneIndex=%d\n", maxBoneIndex);
	
	Skeleton * skeleton = new Skeleton();
	
	skeleton->allocate(maxBoneIndex + 1);
	
	std::map<std::string, Anim*> animations;
	
	if (xmlSkeleton)
	{
		printf("skeleton!\n");
		
		typedef std::map<std::string, int> BoneNameToBoneIndexMap;
		BoneNameToBoneIndexMap boneNameToBoneIndex;
		
		XMLElement * xmlBones = xmlSkeleton->FirstChildElement(XML_BONES);
		
		if (xmlBones)
		{
			printf("bones!\n");
			
			XMLElement * xmlBone = xmlBones->FirstChildElement(XML_BONE);
			
			while (xmlBone)
			{
				const int boneIndex = xmlBone->IntAttribute("id");
				const char * name = xmlBone->Attribute("name");
				
				if (!name)
					name = "";
				
				assert(boneIndex + 1 <= skeleton->m_numBones);
				
				//printf("bone! boneIndex=%d, name=%s\n", boneIndex, name);
				
				boneNameToBoneIndex[name] = boneIndex;
				
				BoneTransform & transform = skeleton->m_bones[boneIndex].transform;
				
				float px = 0.f;
				float py = 0.f;
				float pz = 0.f;
				
				float ra = 0.f;
				float rx = 1.f;
				float ry = 0.f;
				float rz = 0.f;
				
				XMLElement * xmlPosition = xmlBone->FirstChildElement("position");
				
				if (xmlPosition)
				{
					px = xmlPosition->FloatAttribute("x");
					py = xmlPosition->FloatAttribute("y");
					pz = xmlPosition->FloatAttribute("z");
				}
				
				transform.translation[0] = px;
				transform.translation[1] = py;
				transform.translation[2] = pz;
				
				XMLElement * xmlRotation = xmlBone->FirstChildElement("rotation");
				
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
				
				// todo: convert to quat-log
				
				transform.rotation.fromAxisAngle(Vec3(rx, ry, rz), ra);
				transform.scale = 1.f;
				
				//printf("position: %g %g %g\n", px, py, pz);
				//printf("rotation: %g @ %g %g %g\n", ra, rx, ry, rz);
				
				xmlBone = xmlBone->NextSiblingElement(XML_BONE);
			}
		}
		
		XMLElement * xmlBoneHierarchy = xmlSkeleton->FirstChildElement(XML_BONE_HIERARCHY);
		
		if (xmlBoneHierarchy)
		{
			printf("hierarchy!\n");
			
			XMLElement * xmlBoneLink = xmlBoneHierarchy->FirstChildElement(XML_BONE_LINK);
			
			while (xmlBoneLink)
			{
				const char * boneName = xmlBoneLink->Attribute("bone");
				const char * parentName = xmlBoneLink->Attribute("parent");
				
				if (boneName && parentName)
				{
					const int boneIndex = boneNameToBoneIndex[boneName];
					const int parentIndex = boneNameToBoneIndex[parentName];
					
					assert(boneIndex + 1 <= skeleton->m_numBones);
					assert(parentIndex + 1 <= skeleton->m_numBones);
					
					printf("bone link: %d -> %d\n", boneIndex, parentIndex);
					
					skeleton->m_bones[boneIndex].parent = parentIndex;
				}
				
				xmlBoneLink = xmlBoneLink->NextSiblingElement(XML_BONE_LINK);
			}
		}
		
		XMLElement * xmlAnimations = xmlSkeleton->FirstChildElement(XML_ANIMATIONS);
		
		if (xmlAnimations)
		{
			//printf("animations!\n");
			
			XMLElement * xmlAnimation = xmlAnimations->FirstChildElement(XML_ANIMATION);
			
			while (xmlAnimation)
			{
				// animation (name, length)
				
				const char * animName = xmlAnimation->Attribute("name");
				
				if (!animName)
					animName = "";
				
				printf("animation! %s\n", animName);
					
				XMLElement * xmlTracks = xmlAnimation->FirstChildElement(XML_TRACKS);
				
				if (xmlTracks)
				{
					//printf("tracks!\n");
					
					typedef std::map< int, std::vector<AnimKey> > AnimKeysByBoneIndexMap;
					AnimKeysByBoneIndexMap animKeysByBoneIndex;
					
					XMLElement * xmlTrack = xmlTracks->FirstChildElement(XML_TRACK);
					
					while (xmlTrack)
					{
						//printf("track!\n");
						
						// track (bone)
						
						const char * boneName = xmlTrack->Attribute("bone");
						
						if (!boneName)
							boneName = "";
						
						BoneNameToBoneIndexMap::iterator i = boneNameToBoneIndex.find(boneName);
						
						assert(i != boneNameToBoneIndex.end());
						
						const int boneIndex = i->second;
						
						assert(boneIndex + 1 <= skeleton->m_numBones);
				
						std::vector<AnimKey> & animKeys = animKeysByBoneIndex[boneIndex];
						
						XMLElement * xmlKeyFrames = xmlTrack->FirstChildElement(XML_KEYFRAMES);
						
						if (xmlKeyFrames)
						{
							XMLElement * xmlKeyFrame = xmlKeyFrames->FirstChildElement(XML_KEYFRAME);
							
							while (xmlKeyFrame)
							{
								// keyframe (time -> translate, rotate)
								
								const float time = xmlKeyFrame->FloatAttribute("time");
								
								float px = 0.f;
								float py = 0.f;
								float pz = 0.f;
								
								float ra = 0.f;
								float rx = 0.f;
								float ry = 0.f;
								float rz = 0.f;
								
								XMLElement * translation = xmlKeyFrame->FirstChildElement(XML_KEYFRAME_TRANSLATION);
								
								if (translation)
								{
									px = translation->FloatAttribute("x");
									py = translation->FloatAttribute("y");
									pz = translation->FloatAttribute("z");
								}
								
								XMLElement * rotation = xmlKeyFrame->FirstChildElement(XML_KEYFRAME_ROTATION);
								
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
								key.transform.translation[0] = px;
								key.transform.translation[1] = py;
								key.transform.translation[2] = pz;
								key.transform.rotation.fromAxisAngle(Vec3(rx, ry, rz), ra);
								key.transform.scale = 0.f;
								
								animKeys.push_back(key);
								
								xmlKeyFrame = xmlKeyFrame->NextSiblingElement(XML_KEYFRAME);
							}
						}
						
						xmlTrack = xmlTrack->NextSiblingElement(XML_TRACK);
					}
					
					// finalize animation
					
					Anim * animation = new Anim();
					
					int numAnimKeys = 0;
					
					for (int i = 0; i < skeleton->m_numBones; ++i)
					{
						const std::vector<AnimKey> & animKeys = animKeysByBoneIndex[i];
						
						numAnimKeys += int(animKeys.size());
					}
					
					//printf("animation allocate: %d, %d\n", skeleton->m_numBones, numAnimKeys);
					
					animation->allocate(skeleton->m_numBones, numAnimKeys);
					
					AnimKey * finalAnimKey = animation->m_keys;
					
					for (int i = 0; i < skeleton->m_numBones; ++i)
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
	
	MeshSet * meshSet = new MeshSet();
	meshSet->allocate(meshes.size());
	for (size_t i = 0; i < meshes.size(); ++i)
		meshSet->m_meshes[i] = meshes[i];
	
	skeleton->calculatePoseMatrices();
	
	AnimSet * animSet = new AnimSet();
	animSet->m_animations = animations;
	
	Model * model = new Model(meshSet, skeleton, animSet);
	
	return model;
}

int main(int argc, char * argv[])
{
	(void)argc;
	(void)argv;
	
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_SetVideoMode(640, 480, 32, SDL_OPENGL);
	
	float angle = 0.f;
	bool wireframe = true;
	bool rotate = false;
	bool drawBones = true;
	bool loop = false;
	bool autoPlay = false;
	
	for (int i = 0; i < 1; ++i)
	{
		Model * model = loadModel("mesh.xml", "skeleton.xml");
		
		//model->startAnim("Death2");
		
		bool stop = false;
		
		while (!stop)
		{
			bool startRandomAnimation = false;
			
			SDL_Event e;
			
			while (SDL_PollEvent(&e))
			{
				if (e.type == SDL_KEYDOWN)
				{
					if (e.key.keysym.sym == SDLK_a)
					{
						model->startAnim("Backflip");
					}
					else if (e.key.keysym.sym == SDLK_w)
					{
						wireframe = !wireframe;
					}
					else if (e.key.keysym.sym == SDLK_r)
					{
						rotate = !rotate;
					}
					else if (e.key.keysym.sym == SDLK_b)
					{
						drawBones = !drawBones;
					}
					else if (e.key.keysym.sym == SDLK_l)
					{
						loop = !loop;
					}
					else if (e.key.keysym.sym == SDLK_p)
					{
						autoPlay = !autoPlay;
					}
					else if (e.key.keysym.sym == SDLK_SPACE)
					{
						startRandomAnimation = true;
					}
					else if (e.key.keysym.sym == SDLK_UP)
					{
						model->animSpeed *= 2.f;
					}
					else if (e.key.keysym.sym == SDLK_DOWN)
					{
						model->animSpeed /= 2.f;
					}
					else if (e.key.keysym.sym == SDLK_ESCAPE)
					{
						stop = true;
					}
				}
			}
			
			if (autoPlay && model->animIsDone)
			{
				startRandomAnimation = true;
			}
			
			if (startRandomAnimation)
			{
				const char * names[] =
				{
					"Attack1",
					"Attack2",
					"Attack3",
					"Backflip",
					"Block",
					"Climb",
					"Crouch",
					"Death1",
					"Death2",
					"HighJump",
					"Idle1",
					"Idle2",
					"Idle3",
					"Jump",
					"JumpNoHeight",
					"Kick",
					"SideKick",
					"Spin",
					"Stealth",
					"Walk"
				};
				
				const int index = rand() % (sizeof(names) / sizeof(names[0]));
				const char * name = names[index];
				
				printf("anim: %s\n", name);					
				model->startAnim(name, loop ? -1 : 1);
			}
			
			const float timeStep = 1.f / 60.f / 1.f;
			
			if (rotate)
			{
				angle += 45.f * timeStep;
			}
			
			model->process(timeStep);
			
			glClearColor(0, 0, 0, 0);
			glClearDepth(0);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			
			glEnable(GL_DEPTH_TEST);
			glDepthFunc(GL_GREATER);
			
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			glMatrixMode(GL_MODELVIEW);
			glLoadIdentity();
			const float scale = 1.f / 200.f;
			glScalef(scale, scale, scale);
			
			glEnable(GL_LIGHTING);
			glEnable(GL_LIGHT0);
			GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 };
			glLightfv(GL_LIGHT0, GL_POSITION, light_position);
			glEnable(GL_NORMALIZE);
			glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
			
			glTranslatef(0.f, -100.f, 0.f);
			glRotatef(angle, 0.f, 1.f, 0.f);
			glRotatef(90, 0.f, 1.f, 0.f);
			
			Mat4x4 matrix;
			matrix.MakeIdentity();
			
			glColor3ub(255, 255, 255);
			model->drawEx(matrix, true, false);
			
			glDisable(GL_LIGHTING);
			glDisable(GL_DEPTH_TEST);
			
			if (drawBones)
			{
				glColor3ub(0, 255, 0);
				model->drawEx(matrix, false, true);
			}
			
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			
			SDL_GL_SwapBuffers();
		}
		
		delete model;
		model = 0;
	}
	
	SDL_Quit();
	
	return 0;
}
