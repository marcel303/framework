#include <assert.h>
#include <map>
#include <stdlib.h>
#include <string>
#include <vector>
#include "tinyxml2.h"

#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>

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

// todo: 4x4 matrix class

class Matrix
{
public:
	float values[16];
	
	Matrix()
	{
		memset(values, 0, sizeof(values)); // fixme, undesirable
	}
	
	void buildIdentity()
	{
		// todo
		
		values[0] = values[1] = values[2] = 0.f;
	}
	
	void buildTranslation(float x, float y, float z)
	{
		// todo
		
		values[0] = x;
		values[1] = y;
		values[2] = z;
	}
};

Matrix operator*(const Matrix & m1, const Matrix & m2)
{
	Matrix r;
	r.values[0] = m1.values[0] + m2.values[0];
	r.values[1] = m1.values[1] + m2.values[1];
	r.values[2] = m1.values[2] + m2.values[2];
	return r;
}

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
	union
	{
		struct
		{
			float translation[3];
			float scale;
			float rotation[4];
		} data;
		
		float values[8];
	};
	
	void clear()
	{
		memset(values, 0, sizeof(values));
	}
	
	static void add(BoneTransform & result, const BoneTransform & transform)
	{
		const int numValues = sizeof(transform.values) / sizeof(transform.values[0]);
		
		for (int i = 0; i < numValues; ++i)
		{
			result.values[i] += transform.values[i];
		}
	}
	
	static void interpolate(BoneTransform & result, const BoneTransform & transform1, const BoneTransform & transform2, float time)
	{
		const int numValues = sizeof(transform1.values) / sizeof(transform1.values[0]);
		
		const float t1 = 1.f - time;
		const float t2 = time;
		
		for (int i = 0; i < numValues; ++i)
		{
			result.values[i] += transform1.values[i] * t1 + transform2.values[i] * t2;
		}
	}
};

class Bone
{
public:
	Bone()
	{
		parent = -1;
	}
	
	BoneTransform m_transform;
	
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
	
	void evaluate(float time, BoneTransform * transforms)
	{
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
					const AnimKey & key1 = key[0];
					const AnimKey & key2 = key[1];
					
					assert(time >= key1.time && time <= key2.time);
					
					const float t = (time - key1.time) / (key2.time - key1.time);
					
					assert(t >= 0.f && t <= 1.f);
					
					//printf("found key. time=%f, key1=%f, key2=%f, t=%f!\n", time, key1.time, key2.time, t);
					
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
	float m_animTime;
	
public:
	MeshSet * m_meshes;
	Skeleton * m_skeleton;
	AnimSet * m_animations;
	
	Model(MeshSet * meshes, Skeleton * skeleton, AnimSet * animations)
	{
		m_currentAnim = 0;
		m_animTime = 0;
		
		m_meshes = meshes;
		m_skeleton = skeleton;
		m_animations = animations;
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
	
	void startAnim(const char * name)
	{
		std::map<std::string, Anim*>::iterator i = m_animations->m_animations.find(name);
		
		if (i != m_animations->m_animations.end())
		{
			m_currentAnim = i->second;
		}
		else
		{
			m_currentAnim = 0;
		}
		
		m_animTime = 0.f;
	}
	
	void process(float timeStep)
	{
		m_animTime += timeStep;
	}
	
	void draw()
	{
		// todo: build matrix
		
		Matrix matrix;
		
		matrix.buildTranslation(0.f, 0.f, 0.f);
		
		drawEx(matrix, true, false);
	}
	
	void drawEx(const Matrix & matrix, bool drawMesh, bool drawBones)
	{
		// calculate transforms in local bone space
		
		BoneTransform * transforms = (BoneTransform*)alloca(sizeof(BoneTransform) * m_skeleton->m_numBones);
		
		for (int i = 0; i < m_skeleton->m_numBones; ++i)
		{
			transforms[i] = m_skeleton->m_bones[i].m_transform;
		}
		
		if (m_currentAnim)
		{
			m_currentAnim->evaluate(m_animTime, transforms);
		}
		
		/*
		for (int i = 0; i < m_skeleton->m_numBones; ++i)
		{
			printf("(%+07.2f, %+07.2f, %+07.2f), %g, %+07.2f @ (%+05.2f, %+05.2f, %+05.2f)\n",
				transforms[i].data.translation[0],
				transforms[i].data.translation[1],
				transforms[i].data.translation[2],
				transforms[i].data.scale,
				transforms[i].data.rotation[0],
				transforms[i].data.rotation[1],
				transforms[i].data.rotation[2],
				transforms[i].data.rotation[3]);
		}
		*/
		
		// todo: convert translation / rotation pairs into matrices
		
		Matrix * localMatrices = (Matrix*)alloca(sizeof(Matrix) * m_skeleton->m_numBones);
		
		for (int i = 0; i < m_skeleton->m_numBones; ++i)
		{
			//const BoneTransform & transform = m_skeleton->m_bones[i].m_transform;
			const BoneTransform & transform = transforms[i];
			
			localMatrices[i].buildTranslation(
				transform.data.translation[0],
				transform.data.translation[1],
				transform.data.translation[2]);
		}
		
		// todo: calculate the bone hierarchy in world space
		
		Matrix * worldMatrices = (Matrix*)alloca(sizeof(Matrix) * m_skeleton->m_numBones);
		
		for (int i = 0; i < m_skeleton->m_numBones; ++i)
		{
			Matrix finalMatrix;
			finalMatrix.buildIdentity();
			
			int boneIndex = i;
			
			while (boneIndex != -1)
			{
				finalMatrix = finalMatrix * localMatrices[boneIndex];
				
				boneIndex = m_skeleton->m_bones[boneIndex].parent;
			}
			
			worldMatrices[i] = finalMatrix * matrix;
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
						
						glColor3f(vertex.boneIndices[0] / 30.f, 1.f, 1.f);
						glTexCoord2f(vertex.tx, vertex.ty);
						glNormal3f(vertex.nx, vertex.ny, vertex.nz);
						glVertex3f(vertex.px, vertex.py, vertex.pz);
						
						//printf("%g, %g, %g\n", vertex.px, vertex.py, vertex.pz);
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
				
				if (boneIndex2 != -1)
				{
					const Matrix & worldMatrix1 = worldMatrices[boneIndex1];
					const Matrix & worldMatrix2 = worldMatrices[boneIndex2];
					
					glBegin(GL_LINES);
					{
						glVertex3fv(&worldMatrix1.values[0]);
						glVertex3fv(&worldMatrix2.values[0]);
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
		
		std::map<std::string, int> boneNameToBoneIndex;
		
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
				
				BoneTransform & transform = skeleton->m_bones[boneIndex].m_transform;
				
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
				
				transform.data.translation[0] = px;
				transform.data.translation[1] = py;
				transform.data.translation[2] = pz;
				
				transform.data.scale = 1.f;
				
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
				
				transform.data.rotation[0] = ra;
				transform.data.rotation[1] = rx;
				transform.data.rotation[2] = ry;
				transform.data.rotation[3] = rz;
				
				//printf("position: %g %g %g\n", px, py, pz);
				//printf("rotation: %g @ %g %g %g\n", ra, rx, ry, rz);
				
				// todo: calculate transform (position, quat rotation)
				//skeleton->m_bones[boneIndex].m_transform;
				
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
					
					std::map< int, std::vector<AnimKey> > animKeysByBoneIndex;
					
					XMLElement * xmlTrack = xmlTracks->FirstChildElement(XML_TRACK);
					
					while (xmlTrack)
					{
						//printf("track!\n");
						
						// track (bone)
						
						const char * boneName = xmlTrack->Attribute("bone");
						
						if (!boneName)
							boneName = "";
						
						const int boneIndex = boneNameToBoneIndex[boneName];
						
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
								key.transform.data.translation[0] = px;
								key.transform.data.translation[1] = py;
								key.transform.data.translation[2] = pz;
								key.transform.data.scale = 0.f;
								key.transform.data.rotation[0] = ra;
								key.transform.data.rotation[1] = rx;
								key.transform.data.rotation[2] = ry;
								key.transform.data.rotation[3] = rz;
								
								/*
								printf("keyframe! time=%g, translation=%g, %g, %g, rotation = %g @ %g, %g, %g\n",
									time,
									px, py, pz,
									ra, rx, ry, rz);
								*/
								
								animKeys.push_back(key);
								
								xmlKeyFrame = xmlKeyFrame->NextSiblingElement(XML_KEYFRAME);
							}
						}
						
						xmlTrack = xmlTrack->NextSiblingElement(XML_TRACK);
					}
					
					// finalize animation
					
					Anim * animation = new Anim();
					
					int numAnimKeys = 0;
					
					for (std::map< int, std::vector<AnimKey> >::iterator i = animKeysByBoneIndex.begin(); i != animKeysByBoneIndex.end(); ++i)
					{
						const std::vector<AnimKey> & animKeys = i->second;
						
						numAnimKeys += int(animKeys.size());
					}
					
					//printf("animation allocate: %d, %d\n", skeleton->m_numBones, numAnimKeys);
					
					animation->allocate(skeleton->m_numBones, numAnimKeys);
					
					AnimKey * finalAnimKey = animation->m_keys;
					
					for (int i = 0; i < animation->m_numBones; ++i)
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
	
	for (int i = 0; i < 1; ++i)
	{
		Model * model = loadModel("mesh.xml", "skeleton.xml");
		
		//model->startAnim("Death2");
		
		bool stop = false;
		
		while (!stop)
		{
			SDL_Event e;
			
			while (SDL_PollEvent(&e))
			{
				if (e.type == SDL_KEYDOWN)
				{
					if (e.key.keysym.sym == SDLK_SPACE)
					{
						const int index = 1 + (rand() % 3);
						char name[32];
						sprintf(name, "Attack%d", index);
						model->startAnim(name);
					}
					else
					{
						stop = true;
					}
				}
			}
			
			const float timeStep = 1.f / 60.f;
			
			angle += 45.f * timeStep;
			
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
			glScalef(0.01f, 0.01f, 0.01f);
			
			glEnable(GL_LIGHTING);
			glEnable(GL_LIGHT0);
			GLfloat light_position[] = { 1.0, 1.0, 1.0, 0.0 };
			glLightfv(GL_LIGHT0, GL_POSITION, light_position);
			glEnable(GL_NORMALIZE);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			
			glTranslatef(0.f, -100.f, 0.f);
			glRotatef(angle, 0.f, 1.f, 0.f);
			glRotatef(90, 0.f, 1.f, 0.f);
			
			Matrix matrix;
			matrix.buildIdentity();
			
			glColor3ub(255, 255, 255);
			model->drawEx(matrix, true, false);
			
			glDisable(GL_LIGHTING);
			glDisable(GL_DEPTH_TEST);
			
			glColor3ub(0, 255, 0);
			model->drawEx(matrix, false, true);
			
			SDL_GL_SwapBuffers();
		}
		
		delete model;
		model = 0;
	}
	
	SDL_Quit();
	
	return 0;
}
