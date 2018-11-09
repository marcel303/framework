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

#include <algorithm>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include "data/engine/ShaderCommon.txt"
#include "framework.h"
#include "internal.h"
#include "Mat4x4.h"
#include "model.h"
#include "model_fbx.h"
#include "model_ogre.h"
#include "Path.h"

#define DEBUG_TRS 0

template <typename T>
static inline T pad(T value, int align)
{
	return (value + align - 1) & (~(align - 1));
}

#define ALIGNED_ALLOCA(size, align) reinterpret_cast<void*>(pad(reinterpret_cast<uintptr_t>(alloca(size + align - 1)), align))

ModelCache g_modelCache;

namespace AnimModel
{
	static const VsInput vsInputs[] =
	{
		{ VS_POSITION,      3, GL_FLOAT,         0, offsetof(Vertex, px)          },
		{ VS_NORMAL,        3, GL_FLOAT,         0, offsetof(Vertex, nx)          },
		{ VS_COLOR,         4, GL_FLOAT,         0, offsetof(Vertex, cx)          },
		{ VS_TEXCOORD,      2, GL_FLOAT,         0, offsetof(Vertex, tx)          },
		{ VS_BLEND_INDICES, 4, GL_UNSIGNED_BYTE, 0, offsetof(Vertex, boneIndices) },
		{ VS_BLEND_WEIGHTS, 4, GL_UNSIGNED_BYTE, 1, offsetof(Vertex, boneWeights) }
	};
	const int numVsInputs = sizeof(vsInputs) / sizeof(vsInputs[0]);
	
	//
	
	Mesh::Mesh()
	{
		m_vertices = 0;
		m_numVertices = 0;
		
		m_indices = 0;
		m_numIndices = 0;
		
		m_vertexArray = 0;
		m_indexArray = 0;
		
		m_vertexArrayObject = 0;
	}
	
	Mesh::~Mesh()
	{
		if (m_vertexArrayObject)
		{
			glDeleteVertexArrays(1, &m_vertexArrayObject);
			m_vertexArrayObject = 0;
		}
		
		if (m_vertexArray)
		{
			glDeleteBuffers(1, &m_vertexArray);
			m_vertexArray = 0;
		}
		
		if (m_indexArray)
		{
			glDeleteBuffers(1, &m_indexArray);
			m_indexArray = 0;
		}
		
		allocateVB(0);
		allocateIB(0);
	}
	
	void Mesh::allocateVB(int numVertices)
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
	
	void Mesh::allocateIB(int numIndices)
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
	
	void Mesh::finalize()
	{
		fassert(!m_vertexArray && !m_indexArray);
		
		// create vertex buffer
		glGenBuffers(1, &m_vertexArray);
		glBindBuffer(GL_ARRAY_BUFFER, m_vertexArray);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * m_numVertices, m_vertices, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		checkErrorGL();
		
		// create index buffer
		glGenBuffers(1, &m_indexArray);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexArray);
		checkErrorGL();
		if (m_numVertices < 65536)
		{
			unsigned short * indices = new unsigned short[m_numIndices];
			for (int i = 0; i < m_numIndices; ++i)
				indices[i] = m_indices[i];
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(short) * m_numIndices, indices, GL_STATIC_DRAW);
			checkErrorGL();
			delete [] indices;
		}
		else
		{
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * m_numIndices, m_indices, GL_STATIC_DRAW);
			checkErrorGL();
		}
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		checkErrorGL();
		
		// create vertex array object
		glGenVertexArrays(1, &m_vertexArrayObject);
		checkErrorGL();
		glBindVertexArray(m_vertexArrayObject);
		checkErrorGL();
		{
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexArray);
			checkErrorGL();
			glBindBuffer(GL_ARRAY_BUFFER, m_vertexArray);
			checkErrorGL();
			bindVsInputs(vsInputs, numVsInputs, sizeof(Vertex));
		}
		glBindVertexArray(0);
		checkErrorGL();
		
		if (!m_material.shader.isValid())
		{
			m_material.shader = Shader("engine/BasicSkinned");
		}
	}
	
	//
	
	MeshSet::MeshSet()
	{
		m_meshes = 0;
		m_numMeshes = 0;
	}
	
	MeshSet::~MeshSet()
	{
		allocate(0);
	}
	
	void MeshSet::allocate(int numMeshes)
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
	
	//
	
	void BoneTransform::clear()
	{
		translation.SetZero();
		rotation.makeIdentity();
		scale = Vec3(1.f, 1.f, 1.f);
	}
	
	void BoneTransform::add(BoneTransform & result, const BoneTransform & transform)
	{
		result.translation += transform.translation;
		result.rotation *= transform.rotation;
		result.scale += transform.scale;
	}
	
	//
	
	BoneSet::BoneSet()
	{
		m_bones = 0;
		m_numBones = 0;
		m_bonesAreSorted = false;
	}
	
	BoneSet::~BoneSet()
	{
		allocate(0);
	}
	
	void BoneSet::allocate(int numBones)
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
	
	void BoneSet::calculatePoseMatrices()
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
				
				poseMatrix = matrix * poseMatrix;
				
				boneIndex = m_bones[boneIndex].parent;
			}
			
			poseMatrix = poseMatrix.CalcInv();
		}
	}
	
#if DEBUG_TRS
	static void dumpMatrix(const Mat4x4 & m)
	{
		for (int i = 0; i < 4; ++i)
		{
			printf("\t%+6.2f %+6.2f %+6.2f %+6.2f\n",
				m(i, 0), m(i, 1), m(i, 2), m(i, 3));
		}
	}
#endif
	
	void BoneSet::calculateBoneMatrices()
	{
		for (int i = 0; i < m_numBones; ++i)
		{
			Mat4x4 boneMatrix;
			
			if (m_bones[i].parent != -1)
			{
				const Mat4x4 & poseMatrix1 = m_bones[m_bones[i].parent].poseMatrix;
				const Mat4x4 & poseMatrix2 = m_bones[i].poseMatrix;
				
				boneMatrix = poseMatrix1 * poseMatrix2.CalcInv();
			}
			else
			{
				boneMatrix = m_bones[i].poseMatrix.CalcInv();
			}
			
			Vec3 translation = boneMatrix.GetTranslation();
			
			Quat rotation;
			rotation.fromMatrix(boneMatrix);
			
			// todo: calculate scale
			Vec3 scale = Vec3(1.f, 1.f, 1.f);
			
			m_bones[i].transform.translation = translation;
			m_bones[i].transform.rotation = rotation;
			m_bones[i].transform.scale = scale;
		}
		
	#if DEBUG_TRS
		for (int i = 0; i < m_numBones; ++i)
		{
			Mat4x4 m;
			m.MakeIdentity();
			
			int index = i;
			
			while (index != -1)
			{
				const BoneTransform & transform = m_bones[index].transform;
				
				Mat4x4 t;
				Mat4x4 r;
				Mat4x4 s;
				
				t.MakeTranslation(transform.translation);
				r = transform.rotation.toMatrix();
				s.MakeScaling(transform.scale);
				
				const Mat4x4 trs = t * r * s;
				
				m = trs * m;
				
				index = m_bones[index].parent;
			}
			
			Mat4x4 p = m_bones[i].poseMatrix.CalcInv();
			
			printf("trs:\n");
			dumpMatrix(m);
			dumpMatrix(p);
			
			Mat4x4 f = m * m_bones[i].poseMatrix;
			dumpMatrix(f);
		}
	#endif
	}
	
	static bool sortByDistance(const std::pair<int, int> & v1, const std::pair<int, int> & v2)
	{
		if (v1.second != v2.second)
			return v1.second < v2.second;
		return v1.first < v2.first;
	}
	
	void BoneSet::sortBoneIndices()
	{
		// sort the bones in such a way that the bones with the smallest distance to the root are sorted first.
		// this makes it possible to quickly calculate the global bone transforms, by simply iterating the list
		// of bones once, instead of using some sort of recursive algoritm. since the sorted state ensures the
		// parent bone for any bone has already been processed, we know that the parent transform has already
		// been calculated, so we can directly access it instead of having to recurse
		
		m_bonesAreSorted = true;
		
		// compute distances to the root node
		
		std::vector< std::pair<int, int> > boneIndexToDistance;
		boneIndexToDistance.resize(m_numBones);
		
		for (int i = 0; i < m_numBones; ++i)
		{
			int distance = 0;
			int index = i;
			
			while (index != -1)
			{
				distance++;
				index = m_bones[index].parent;
			}
			
			boneIndexToDistance[i] = std::pair<int, int>(i, distance);
		}
		
		// sort by distance to the root node. nodes with a smaller distance are sorted first
		
		std::sort(boneIndexToDistance.begin(), boneIndexToDistance.end(), sortByDistance);
		
		// remap bones and parent indices
		
		std::vector<Bone> newBones;
		newBones.resize(m_numBones);
		
		for (int i = 0; i < m_numBones; ++i)
		{
			newBones[i] = m_bones[boneIndexToDistance[i].first];
			newBones[i].originalIndex = boneIndexToDistance[i].first;
			for (int j = 0; j < m_numBones; ++j)
			{
				if (boneIndexToDistance[j].first == newBones[i].parent)
				{
					newBones[i].parent = j;
					break;
				}
			}
		}
		
		for (int i = 0; i < m_numBones; ++i)
			m_bones[i] = newBones[i];
	}
	
	int BoneSet::findBone(const std::string & name)
	{
		for (int i = 0; i < m_numBones; ++i)
			if (m_bones[i].name == name)
				return i;
		
		return -1;
	}
	
	//
	
	void AnimKey::interpolate(BoneTransform & result, const AnimKey & key1, const AnimKey & key2, float t, RotationType rotationType)
	{
		const float t1 = 1.f - t;
		const float t2 = t;
		
		Quat quat;
		
		if (rotationType == RotationType_Quat)
		{
			Quat quat1(key1.rotation[0], key1.rotation[1], key1.rotation[2], key1.rotation[3]);
			Quat quat2(key2.rotation[0], key2.rotation[1], key2.rotation[2], key2.rotation[3]);
			
			quat = quat1.slerp(quat2, t);
		}
		else
		{
			const float rotation[3] =
			{
				key1.rotation[0] * t1 + key2.rotation[0] * t2,
				key1.rotation[1] * t1 + key2.rotation[1] * t2,
				key1.rotation[2] * t1 + key2.rotation[2] * t2
			};
			
			Quat quatX;
			Quat quatY;
			Quat quatZ;
			
			quatX.fromAxisAngle(Vec3(1.f, 0.f, 0.f), rotation[0]);
			quatY.fromAxisAngle(Vec3(0.f, 1.f, 0.f), rotation[1]);
			quatZ.fromAxisAngle(Vec3(0.f, 0.f, 1.f), rotation[2]);
			
			if (rotationType == RotationType_EulerXYZ)
			{
				quat = quatZ * quatY * quatX;
			}
			else
			{
				fassert(false);
				quat.makeIdentity();
			}
		}
		
		result.translation = (key1.translation * t1) + (key2.translation * t2);
		result.rotation = quat;
		result.scale = (key1.scale * t1) + (key2.scale * t2);
	}
	
	//
	
	Anim::Anim()
	{
		m_numBones = 0;
		m_numKeys = 0;
		m_keys = 0;
		m_rotationType = RotationType_Quat;
		m_isAdditive = false;
		
		m_rootMotion = false;
		m_loop = 1;
	}
	
	Anim::~Anim()
	{
		allocate(0, 0, m_rotationType, false);
	}
	
	void Anim::allocate(int numBones, int numKeys, RotationType rotationType, bool isAdditive)
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
		
		m_rotationType = rotationType;
		m_isAdditive = isAdditive;
	}
	
	bool Anim::evaluate(float time, BoneTransform * transforms, int boneIndex)
	{
		bool isDone = true;
		
		const AnimKey * firstKey = m_keys;
		
		for (int i = 0; i < m_numBones; ++i)
		{
			const int numKeys = m_numKeys[i];
			
			if (numKeys)
			{
				if (i == boneIndex || boneIndex == -1)
				{
					const AnimKey * key = firstKey;
					const AnimKey * lastKey = firstKey + numKeys - 1;
					
				#if 0
					while (key != lastKey && time >= key[1].time)
					{
						key++;
					}
				#else
					int startIndex = 0;
					int count = numKeys;
					
					while (count >= 2)
					{
						const int step = count / 2;
						const int midIndex = startIndex + step;
						
						const AnimKey * midKey = key + midIndex;
						
						if (time >= midKey->time)
						{
							startIndex += step;
							
							count -= step;
						}
						else
						{
							count = step;
						}
					}
					
					key = key + startIndex;
				#endif
					
				#if 0
					const AnimKey * testKey = firstKey;
					
					while (testKey != lastKey && time >= testKey[1].time)
					{
						testKey++;
					}
					
					Assert(key == testKey);
				#endif
					
					BoneTransform transform;
					
					if (key != lastKey && time >= key->time)
					{
						const AnimKey & key1 = key[0];
						const AnimKey & key2 = key[1];
						
						fassert(time >= key1.time && time <= key2.time);
						
						const float t = (time - key1.time) / (key2.time - key1.time);
						
						fassert(t >= 0.f && t <= 1.f);
						
						AnimKey::interpolate(transform, key1, key2, t, m_rotationType);
					}
					else
					{
						// either the first or last key in the animation. copy value
						
						fassert(key == firstKey || key == lastKey);
						
						AnimKey::interpolate(transform, *key, *key, 0.f, m_rotationType);
					}
					
					if (m_isAdditive)
						BoneTransform::add(*transforms, transform);
					else
						*transforms = transform;
					
					if (key != lastKey)
					{
						isDone = false;
					}
				}
				
				firstKey += numKeys;
			}
			
			if (i == boneIndex || boneIndex == -1)
				transforms++;
		}
		
		return isDone;
	}
	
	void Anim::triggerActions(float oldTime, float newTime, int loop)
	{
		for (size_t i = 0; i < m_triggers.size(); ++i)
		{
			const AnimTrigger & trigger = m_triggers[i];
			
			if (oldTime <= trigger.time && newTime > trigger.time && (loop == trigger.loop || (trigger.loop == -1)))
			{
				framework.processActions(trigger.actions, trigger.args);
			}
		}
	}
	
	//
	
	AnimSet::AnimSet()
	{
	}
	
	AnimSet::~AnimSet()
	{
		for (std::map<std::string, Anim*>::iterator i = m_animations.begin(); i != m_animations.end(); ++i)
		{
			Anim * anim = i->second;
			
			delete anim;
		}
		
		m_animations.clear();
	}
	
	void AnimSet::rename(const std::string & name)
	{
		std::map<std::string, Anim*> newAnimations;
		
		for (std::map<std::string, Anim*>::iterator i = m_animations.begin(); i != m_animations.end(); ++i)
		{
			const std::string newName = name;
			
			Anim * anim = i->second;
			
			if (newAnimations.count(newName) == 0)
				newAnimations[newName] = anim;
			else
			{
				logWarning("duplicate animation name: %s", newName.c_str());
				delete anim;
			}
		}
		
		m_animations = newAnimations;
	}
	
	void AnimSet::mergeFromAndFree(AnimSet * animSet)
	{
		for (std::map<std::string, Anim*>::iterator i = animSet->m_animations.begin(); i != animSet->m_animations.end(); ++i)
		{
			const std::string & name = i->first;
			Anim * anim = i->second;
			
			if (m_animations.count(name) == 0)
				m_animations[name] = anim;
			else
			{
				logWarning("duplicate animation name: %s", name.c_str());
				delete anim;
			}
		}
		
		animSet->m_animations.clear();
		
		delete animSet;
	}
}

//

using namespace AnimModel;

Model::Model(const char * filename, const bool autoUpdate)
	: m_autoUpdate(autoUpdate)
{
	ctor();
	
	m_model = &g_modelCache.findOrCreate(filename);
	
	ctorEnd();
	
}

Model::Model(ModelCacheElem & cacheElem, const bool autoUpdate)
	: m_autoUpdate(autoUpdate)
{
	ctor();
	
	m_model = &cacheElem;
	
	ctorEnd();
}

void Model::ctor()
{
	// book keeping
	m_prev = 0;
	m_next = 0;
	
	// drawing
	x = y = z = 0.f;
	axis = Vec3(1.f, 0.f, 0.f);
	angle = 0.f;
	scale = 1.f;
	overrideShader = nullptr;
	
	// animation
	m_animSegment = 0;
	animIsActive = false;
	animIsPaused = false;
	m_isAnimStarted = false;
	animTime = 0.f;
	animLoop = 0;
	animLoopCount = 0;
	animSpeed = 1.f;
	animRootMotionEnabled = true;
	m_boneTransforms = nullptr;
	
	if (m_autoUpdate)
		framework.registerModel(this);
}

void Model::ctorEnd()
{
	const int numBones = m_model->boneSet->m_numBones;
	
	m_boneTransforms = new BoneTransform[numBones];
	
	for (int i = 0; i < numBones; ++i)
		m_boneTransforms[i] = m_model->boneSet->m_bones[i].transform;
}

Model::~Model()
{
	delete [] m_boneTransforms;
	m_boneTransforms = nullptr;
	
	if (m_autoUpdate)
		framework.unregisterModel(this);
}

void Model::startAnim(const char * name, int loop)
{
	fassert(loop != 0);
	
	// todo: apply loop count from animation
	
	animIsPaused = false;
	m_animSegmentName = name;
	m_isAnimStarted = true;
	animTime = 0.f;
	animLoop = loop;
	animLoopCount = 0;
	if (animLoop > 0)
		animLoop--;
	
	updateAnimationSegment();
}

void Model::stopAnim()
{
	animIsActive = false;
	m_isAnimStarted = false;
}

std::vector<std::string> Model::getAnimList() const
{
	std::vector<std::string> result;
	
	const std::map<std::string, Anim*> & anims = m_model->animSet->m_animations;
	
	for (std::map<std::string, Anim*>::const_iterator i = anims.begin(); i != anims.end(); ++i)
	{
		const std::string & name = i->first;
		
		result.push_back(name);
	}
	
	return result;
}

const char * Model::getAnimName() const
{
	return m_animSegmentName.c_str();
}

void Model::tick(const float dt)
{
	updateAnimation(dt);
}

void Model::draw(const int drawFlags) const
{
	drawEx(Vec3(x, y, z), axis, angle, scale, drawFlags);
}

void Model::drawEx(Vec3Arg position, Vec3Arg axis, const float angle, const float scale, const int drawFlags) const
{
	// build transformation matrix
	 
	Mat4x4 matrix;

	calculateTransform(position, axis, angle, scale, matrix);

	drawEx(matrix, drawFlags);
}

void Model::drawEx(const Mat4x4 & matrix, const int drawFlags) const
{
	const int numBones = calculateBoneMatrices(matrix, nullptr, nullptr, nullptr, 0);
	
	Mat4x4 * localMatrices = (Mat4x4*)ALIGNED_ALLOCA(sizeof(Mat4x4) * numBones, 16);
	Mat4x4 * worldMatrices = (Mat4x4*)ALIGNED_ALLOCA(sizeof(Mat4x4) * numBones, 16);
	Mat4x4 * globalMatrices = (Mat4x4*)ALIGNED_ALLOCA(sizeof(Mat4x4) * numBones, 16);
	
	calculateBoneMatrices(matrix, localMatrices, worldMatrices, globalMatrices, numBones);
	
	if (drawFlags & DrawUnSkinned)
	{
		gxPushMatrix();
		gxMultMatrixf(matrix.m_v);
	}
	
	// draw
	
	if (drawFlags & DrawMesh)
	{
		const Shader * previousShader = nullptr;
		
		for (int i = 0; i < m_model->meshSet->m_numMeshes; ++i)
		{
			// todo: hide meshes that shouldn't render
			//if (i != 0)
			//	continue;
			
			Mesh * mesh = m_model->meshSet->m_meshes[i];
			Shader & shader = (overrideShader != nullptr) ? *overrideShader : mesh->m_material.shader;
			
			setShader(shader);
			
			gxValidateMatrices();
			
			if (&shader != previousShader)
			{
				previousShader = &shader;
				
				// todo: use constant locations for skinningMatrices and drawColor, and move this setting to outside of the loop
				// set uniform constants for skinning matrices
				
				const GLint boneMatrices = shader.getImmediate("skinningMatrices");
				
				if (boneMatrices != -1)
				{
					glUniformMatrix4fv(boneMatrices, numBones, GL_FALSE, (GLfloat*)globalMatrices);
					checkErrorGL();
				}
				
				const GLint drawColor = shader.getImmediate("drawColor");
				
				if (drawColor != -1)
				{
					glUniform4f(drawColor,
						(drawFlags & DrawColorTexCoords)    ? 1.f : 0.f,
						(drawFlags & DrawColorNormals)      ? 1.f : 0.f,
						(drawFlags & DrawColorBlendIndices) ? 1.f : 0.f,
						(drawFlags & DrawColorBlendWeights) ? 1.f : 0.f);
					checkErrorGL();
				}
				
				const GLint drawSkin = shader.getImmediate("drawSkin");
				
				if (drawSkin != -1)
				{
					glUniform4f(drawSkin,
						(drawFlags & DrawUnSkinned)   ? 1.f : 0.f,
						(drawFlags & DrawHardSkinned) ? 1.f : 0.f,
						0.f,
						0.f);
					checkErrorGL();
				}
			}
			
			// bind vertex arrays
			
			fassert(mesh->m_vertexArray);
			fassert(mesh->m_indexArray);
			glBindVertexArray(mesh->m_vertexArrayObject);
			checkErrorGL();
			
			GLenum indexType = mesh->m_numVertices < 65536 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
			glDrawElements(GL_TRIANGLES, mesh->m_numIndices, indexType, 0);
			checkErrorGL();
			
			glBindVertexArray(0);
			checkErrorGL();
		}
		
		clearShader();
	}
	
	if (drawFlags & DrawUnSkinned)
	{
		gxPopMatrix();
	}
	
	if (drawFlags & DrawNormals)
	{
		for (int i = 0; i < m_model->meshSet->m_numMeshes; ++i)
		{
			const Mesh * mesh = m_model->meshSet->m_meshes[i];
			
			gxBegin(GL_LINES);
			{
				for (int j = 0; j < mesh->m_numVertices; ++j)
				{
					const Vertex & vertex = mesh->m_vertices[j];
					
					Vec3 p(0.f, 0.f, 0.f);
					Vec3 n(0.f, 0.f, 0.f);
					
					if (drawFlags & DrawUnSkinned)
					{
						// -- software vertex blend (unskinned) --
						p = matrix.Mul4(Vec3(vertex.px, vertex.py, vertex.pz));
						n = matrix.Mul3(Vec3(vertex.nx, vertex.ny, vertex.nz));
						// -- software vertex blend (unskinned) --
					}
					else if (drawFlags & DrawHardSkinned)
					{
						// -- software vertex blend (hard skinned) --
						const int boneIndex = vertex.boneIndices[0];
						const Mat4x4 & globalMatrix = globalMatrices[boneIndex];
						p = globalMatrix.Mul4(Vec3(vertex.px, vertex.py, vertex.pz));
						n = globalMatrix.Mul3(Vec3(vertex.nx, vertex.ny, vertex.nz));
						// -- software vertex blend (hard skinned) --
					}
					else
					{
						// -- software vertex blend (soft skinned) --
						
						for (int b = 0; b < 4; ++b)
						{
							if (vertex.boneWeights[b] == 0)
								continue;
							const int boneIndex = vertex.boneIndices[b];
							const float boneWeight = vertex.boneWeights[b] / 255.f;
							const Mat4x4 & globalMatrix = globalMatrices[boneIndex];
							p += globalMatrix.Mul4(Vec3(vertex.px, vertex.py, vertex.pz)) * boneWeight;
							n += globalMatrix.Mul3(Vec3(vertex.nx, vertex.ny, vertex.nz)) * boneWeight;
						}
						// -- software vertex blend (soft skinned) --
					}
					
					const float scale = 3.f;
					
					const Vec3 & p1 = p;
					const Vec3   p2 = p + n * scale;
					
					gxColor3ub(127, 127, 127);
					gxNormal3f(n[0], n[1], n[2]);
					gxVertex3f(p1[0], p1[1], p1[2]);
					gxVertex3f(p2[0], p2[1], p2[2]);
				}
			}
			gxEnd();
		}
	}
	
	if (drawFlags & DrawBones)
	{
		GLint restoreDepthTest;
		glGetIntegerv(GL_DEPTH_TEST, &restoreDepthTest);
		checkErrorGL();
		glDisable(GL_DEPTH_TEST);
		checkErrorGL();

		// bone to object matrix translation
		gxColor3ub(127, 127, 127);
		gxBegin(GL_LINES);
		{
			for (int boneIndex = 0; boneIndex < m_model->boneSet->m_numBones; ++boneIndex)
			{
				const int parentBoneIndex = m_model->boneSet->m_bones[boneIndex].parent;
				if (parentBoneIndex != -1)
				{
					const Mat4x4 & m1 = worldMatrices[boneIndex];
					const Mat4x4 & m2 = worldMatrices[parentBoneIndex];
					gxVertex3f(m1(3, 0), m1(3, 1), m1(3, 2));
					gxVertex3f(m2(3, 0), m2(3, 1), m2(3, 2));
				}
			}
		}
		gxEnd();
		gxColor3ub(0, 255, 0);
		glPointSize(5.f);
		gxBegin(GL_POINTS);
		{
			for (int boneIndex = 0; boneIndex < m_model->boneSet->m_numBones; ++boneIndex)
			{
				const Mat4x4 & m = worldMatrices[boneIndex];
				gxVertex3f(m(3, 0), m(3, 1), m(3, 2));
			}
		}
		gxEnd();
		
		if (restoreDepthTest)
		{
			glEnable(GL_DEPTH_TEST);
			checkErrorGL();
		}
	}
	
	if (drawFlags & DrawPoseMatrices)
	{
		GLint restoreDepthTest;
		glGetIntegerv(GL_DEPTH_TEST, &restoreDepthTest);
		checkErrorGL();
		glDisable(GL_DEPTH_TEST);
		checkErrorGL();

		// object to bone matrix translation
		gxColor3ub(127, 127, 127);
		gxBegin(GL_LINES);
		{
			for (int boneIndex = 0; boneIndex < m_model->boneSet->m_numBones; ++boneIndex)
			{
				const int parentBoneIndex = m_model->boneSet->m_bones[boneIndex].parent;
				if (parentBoneIndex != -1)
				{
					const Mat4x4 m1 = matrix * m_model->meshToObject * m_model->boneSet->m_bones[boneIndex].poseMatrix.CalcInv();
					const Mat4x4 m2 = matrix * m_model->meshToObject * m_model->boneSet->m_bones[parentBoneIndex].poseMatrix.CalcInv();
					gxVertex3f(m1(3, 0), m1(3, 1), m1(3, 2));
					gxVertex3f(m2(3, 0), m2(3, 1), m2(3, 2));
				}
			}
		}
		gxEnd();
		gxColor3ub(255, 0, 0);
		glPointSize(7.f);
		checkErrorGL();
		gxBegin(GL_POINTS);
		{
			for (int boneIndex = 0; boneIndex < m_model->boneSet->m_numBones; ++boneIndex)
			{
				const Mat4x4 m = matrix * m_model->meshToObject * m_model->boneSet->m_bones[boneIndex].poseMatrix.CalcInv();
				gxVertex3f(m(3, 0), m(3, 1), m(3, 2));
			}
		}
		gxEnd();
		
		if (restoreDepthTest)
		{
			glEnable(GL_DEPTH_TEST);
			checkErrorGL();
		}
	}
}

void Model::calculateTransform(Mat4x4 & matrix) const
{
	calculateTransform(Vec3(x, y, z), axis, angle, scale, matrix);
}

void Model::calculateTransform(Vec3Arg position, Vec3Arg axis, const float angle, const float scale, Mat4x4 & matrix)
{
	Quat rotation;
	rotation.fromAxisAngle(axis, angle);

	//Mat4x4 matrix;
	rotation.toMatrix3x3(matrix);

	matrix(0, 3) = 0.f;
	matrix(1, 3) = 0.f;
	matrix(2, 3) = 0.f;

	matrix(3, 0) = position[0];
	matrix(3, 1) = position[1];
	matrix(3, 2) = position[2];
	matrix(3, 3) = 1.f;

	if (scale != 1.f)
	{
		for (int i = 0; i < 3; ++i)
			for (int j = 0; j < 3; ++j)
				matrix(i, j) *= scale;
	}
}

int Model::calculateBoneMatrices(
	const Mat4x4 & matrix,
	Mat4x4 * localMatrices,
	Mat4x4 * worldMatrices,
	Mat4x4 * globalMatrices,
	const int numMatrices) const
{
	if (numMatrices == 0)
	{
		// todo : add flag if static mesh or not ?
		
		if (m_model->boneSet->m_numBones == 0)
			return 1;
		else
			return m_model->boneSet->m_numBones;
	}
	
	if (m_model->boneSet->m_numBones == 0)
	{
		Assert(numMatrices == 1);
		
		localMatrices[0].MakeIdentity();
		worldMatrices[0] = matrix;
		globalMatrices[0] = worldMatrices[0];
		
		return 1;
	}
	
	Assert(numMatrices == m_model->boneSet->m_numBones);
	
	// convert translation / rotation pairs into matrices
	
	for (int i = 0; i < m_model->boneSet->m_numBones; ++i)
	{
		// todo: scale?
		
		const BoneTransform & transform = m_boneTransforms[i];
		
		Mat4x4 & boneMatrix = localMatrices[i];
		
		transform.rotation.toMatrix3x3(boneMatrix);
		
		boneMatrix(0, 3) = 0.f;
		boneMatrix(1, 3) = 0.f;
		boneMatrix(2, 3) = 0.f;
		
		boneMatrix(3, 0) = transform.translation[0];
		boneMatrix(3, 1) = transform.translation[1];
		boneMatrix(3, 2) = transform.translation[2];
		boneMatrix(3, 3) = 1.f;
	}
	
	// calculate the bone hierarchy in world space
	
	if (m_model->boneSet->m_bonesAreSorted)
	{
		for (int i = 0; i < m_model->boneSet->m_numBones; ++i)
		{
			const int parent = m_model->boneSet->m_bones[i].parent;
			
			if (parent == -1)
				worldMatrices[i] = matrix * m_model->meshToObject * localMatrices[i];
			else
				worldMatrices[i] = worldMatrices[parent] * localMatrices[i];
		}
	}
	else
	{
		for (int i = 0; i < m_model->boneSet->m_numBones; ++i)
		{
			int boneIndex = i;
			
			Mat4x4 finalMatrix = localMatrices[boneIndex];
			
			boneIndex = m_model->boneSet->m_bones[boneIndex].parent;
			
			while (boneIndex != -1)
			{
				finalMatrix = localMatrices[boneIndex] * finalMatrix;
				
				boneIndex = m_model->boneSet->m_bones[boneIndex].parent;
			}
			
			worldMatrices[i] = matrix * m_model->meshToObject * finalMatrix;
		}
	}
	
	for (int i = 0; i < m_model->boneSet->m_numBones; ++i)
	{
		const Mat4x4 & worldToBone = m_model->boneSet->m_bones[i].poseMatrix;
		
		globalMatrices[i] = worldMatrices[i] * worldToBone;
	}
	
	return 0;
}

int Model::softBlend(const Mat4x4 & matrix, Mat4x4 * localMatrices, Mat4x4 * worldMatrices, Mat4x4 * globalMatrices, const int numMatrices,
	const bool wantsPosition,
	float * __restrict positionX,
	float * __restrict positionY,
	float * __restrict positionZ,
	const bool wantsNormal,
	float * __restrict normalX,
	float * __restrict normalY,
	float * __restrict normalZ,
	const int numVertices) const
{
	Assert(numMatrices == m_model->boneSet->m_numBones);
	
	if (numVertices == 0)
	{
		int result = 0;
		
		for (int i = 0; i < m_model->meshSet->m_numMeshes; ++i)
			result += m_model->meshSet->m_meshes[i]->m_numVertices;
		
		return result;
	}
	
	calculateBoneMatrices(matrix, localMatrices, worldMatrices, globalMatrices, m_model->boneSet->m_numBones);
	
	// todo : write a fast SIMD version of this. write out streams of px, py, pz, nx, ny, nz
	
	const float boneWeightScale = 1.f / 255.f;
	
	int outputIndex = 0;
	
	for (int i = 0; i < m_model->meshSet->m_numMeshes; ++i)
	{
		const Mesh * mesh = m_model->meshSet->m_meshes[i];
		
		for (int j = 0; j < mesh->m_numVertices; ++j)
		{
			const Vertex & vertex = mesh->m_vertices[j];
			
			const Vec3 vp(vertex.px, vertex.py, vertex.pz);
			const Vec3 vn(vertex.nx, vertex.ny, vertex.nz);
			
			// -- software vertex blend (soft skinned) --
			
			Vec3 p(0.f, 0.f, 0.f);
			Vec3 n(0.f, 0.f, 0.f);
			
			for (int b = 0; b < 4; ++b)
			{
				if (vertex.boneWeights[b] == 0)
					continue;
				
				const int boneIndex = vertex.boneIndices[b];
				const float boneWeight = vertex.boneWeights[b];
				
				const Mat4x4 & globalMatrix = globalMatrices[boneIndex];
				
				if (wantsPosition)
				{
					p += globalMatrix.Mul4(vp) * boneWeight;
				}
				
				if (wantsNormal)
				{
					n += globalMatrix.Mul3(vn) * boneWeight;
				}
			}
			
			if (wantsPosition)
			{
				positionX[outputIndex] = p[0] * boneWeightScale;
				positionY[outputIndex] = p[1] * boneWeightScale;
				positionZ[outputIndex] = p[2] * boneWeightScale;
			}
			
			if (wantsNormal)
			{
				n.Normalize();
				
				normalX[outputIndex] = n[0];
				normalY[outputIndex] = n[1];
				normalZ[outputIndex] = n[2];
			}
			
			outputIndex++;
		}
	}
	
	return 0;
}

void Model::calculateAABB(Vec3 & min, Vec3 & max, const bool applyAnimation) const
{
	const float minFloat = -std::numeric_limits<float>::max();
	const float maxFloat = +std::numeric_limits<float>::max();

	min = Vec3(maxFloat, maxFloat, maxFloat);
	max = Vec3(minFloat, minFloat, minFloat);
	
	//
	
	if (applyAnimation)
	{
		Mat4x4 matrix;
		matrix.MakeScaling(scale, scale, scale);
		
		const int numMatrices = calculateBoneMatrices(matrix, nullptr, nullptr, nullptr, 0);
		
		Mat4x4 * localMatrices = (Mat4x4*)ALIGNED_ALLOCA(sizeof(Mat4x4) * numMatrices, 16);
		Mat4x4 * worldMatrices = (Mat4x4*)ALIGNED_ALLOCA(sizeof(Mat4x4) * numMatrices, 16);
		Mat4x4 * globalMatrices = (Mat4x4*)ALIGNED_ALLOCA(sizeof(Mat4x4) * numMatrices, 16);
		
		calculateBoneMatrices(matrix, localMatrices, worldMatrices, globalMatrices, numMatrices);
		
		const int numVertices = softBlend(
			matrix, nullptr, nullptr,  nullptr, numMatrices,
			false, nullptr, nullptr, nullptr,
			false, nullptr, nullptr, nullptr, 0);
		
		float * __restrict positionX = new float[numVertices];
		float * __restrict positionY = new float[numVertices];
		float * __restrict positionZ = new float[numVertices];
		
		softBlend(
			matrix, localMatrices, worldMatrices, globalMatrices, numMatrices,
			true, positionX, positionY, positionZ,
			false, nullptr, nullptr, nullptr, numVertices);
		
		for (int i = 0; i < numVertices; ++i)
		{
			min[0] = fminf(min[0], positionX[i]);
			min[1] = fminf(min[1], positionY[i]);
			min[2] = fminf(min[2], positionZ[i]);
			
			max[0] = fmaxf(max[0], positionX[i]);
			max[1] = fmaxf(max[1], positionY[i]);
			max[2] = fmaxf(max[2], positionZ[i]);
		}
		
		delete [] positionX;
		delete [] positionY;
		delete [] positionZ;
	}
	else
	{
		for (int i = 0; i < m_model->meshSet->m_numMeshes; ++i)
		{
			const Mesh * mesh = m_model->meshSet->m_meshes[i];
			
			for (int j = 0; j < mesh->m_numVertices; ++j)
			{
				const Vertex & vertex = mesh->m_vertices[j];
				
				min[0] = fminf(min[0], vertex.px);
				min[1] = fminf(min[1], vertex.py);
				min[2] = fminf(min[2], vertex.pz);
				
				max[0] = fmaxf(max[0], vertex.px);
				max[1] = fmaxf(max[1], vertex.py);
				max[2] = fmaxf(max[2], vertex.pz);
			}
		}
	}
}

void Model::updateAnimationSegment()
{
	if (m_isAnimStarted && !m_animSegmentName.empty())
	{
		std::map<std::string, Anim*>::iterator i = m_model->animSet->m_animations.find(m_animSegmentName);
		
		if (i != m_model->animSet->m_animations.end())
			m_animSegment = i->second;
		else
			m_animSegment = 0;
		
		if (!m_animSegment)
		{
			logInfo("unable to find animation: %s", m_animSegmentName.c_str());
			animIsActive = false;
			animTime = 0.f;
		}
		else
		{
			animIsActive = true;
		}
	}
}

void Model::updateAnimation(float timeStep)
{
	animRootMotion.SetZero();
	
	const float oldTime = animTime;
	if (!animIsPaused)
		animTime += animSpeed * timeStep;
	const float newTime = animTime;
	
	// calculate transforms in local bone space
	
	for (int i = 0; i < m_model->boneSet->m_numBones; ++i)
	{
		m_boneTransforms[i] = m_model->boneSet->m_bones[i].transform;
	}
	
	// apply animations
	
	if (m_isAnimStarted && m_animSegment)
	{
		Anim * anim = reinterpret_cast<Anim*>(m_animSegment);
		
		const bool isDone = anim->evaluate(animTime, m_boneTransforms);
		
		if (anim->m_rootMotion)
		{
			int boneIndex = m_model->boneSet->findBone(m_model->rootNode);
			
			if (boneIndex != -1)
			{
				m_boneTransforms[boneIndex].translation = m_model->boneSet->m_bones[boneIndex].transform.translation;
			}
		}
		
		if (isDone)
		{
			if (animLoop > 0 || animLoop < 0)
			{
				animTime = 0.f;
				animLoopCount++;
				if (animLoop > 0)
					animLoop--;
			}
			else
			{
				animIsActive = false;
			}
		}
	}
	
	// update root motion
	
	Anim * anim = static_cast<Anim*>(m_animSegment);
	
	if (anim)
	{
		if (animRootMotionEnabled && anim->m_rootMotion)
		{
			int boneIndex = m_model->boneSet->findBone(m_model->rootNode);
			
			if (boneIndex != -1)
			{
				BoneTransform oldTransform;
				BoneTransform newTransform;
				
				anim->evaluate(oldTime, &oldTransform, boneIndex);
				anim->evaluate(newTime, &newTransform, boneIndex);
				
				animRootMotion = m_model->meshToObject * (newTransform.translation - oldTransform.translation);
				
				x += animRootMotion[0];
				y += animRootMotion[1];
				z += animRootMotion[2];
			}
		}
		
		anim->triggerActions(oldTime, newTime, animLoopCount);
	}
}

//

static Loader * createLoader(const char * filename)
{
	if (strstr(filename, ".fbx") != 0)
		return new LoaderFbxBinary();
	if (strstr(filename, ".xml") != 0)
		return new LoaderOgreXML();
	return 0;
}

ModelCacheElem::ModelCacheElem()
{
	meshSet = 0;
	boneSet = 0;
	animSet = 0;
}

void ModelCacheElem::free()
{
	delete meshSet;
	meshSet = 0;
	
	delete boneSet;
	boneSet = 0;
	
	delete animSet;
	animSet = 0;
	
	meshToObject.MakeIdentity();
	
	rootNode.clear();
}

struct SectionRecord
{
	SectionRecord()
	{
		consumed = false;
	}
	
	std::string line;
	std::string name;
	Dictionary args;
	bool consumed;
};

static bool parseSectionRecord(const char * filename, const std::string & line, SectionRecord & record)
{
	// format: <name> <key>:<value> <key:value> <key..
	
	std::vector<std::string> parts;
	splitString(line, parts);
	
	if (parts.size() == 0 || parts[0][0] == '#')
	{
		// empty line or comment
		return false;
	}
	
	if (parts.size() == 1)
	{
		logError("%s: missing parameters: %s (%s)", filename, line.c_str(), parts[0].c_str());
		return false;
	}
	
	record.line = line;
	record.name = parts[0];
	
	for (size_t i = 1; i < parts.size(); ++i)
	{
		const size_t separator = parts[i].find(':');
		
		if (separator == std::string::npos)
		{
			logError("%s: incorrect key:value syntax: %s (%s)", filename, line.c_str(), parts[i].c_str());
			continue;
		}
		
		const std::string key = parts[i].substr(0, separator);
		const std::string value = parts[i].substr(separator + 1, parts[i].size() - separator - 1);
		
		if (key.size() == 0 || value.size() == 0)
		{
			logError("%s: incorrect key:value syntax: %s (%s)", filename, line.c_str(), parts[i].c_str());
			continue;
		}
		
		if (record.args.contains(key.c_str()))
		{
			logError("%s: duplicate key: %s (%s)", filename, line.c_str(), key.c_str());
			continue;
		}
		
		record.args.setString(key.c_str(), value.c_str());
	}
	
	return true;
}

void ModelCacheElem::load(const char * filename)
{
	free();
	
	// meshset file:<filename>
	// boneset file:<filename>
	// animset file:<filename> name:<override-name>
	// model rootnode:<tnode>
	// animation name:walk loop:<loopcount> rootmotion:<enabled>
	//     trigger time:<second> loop:<loop> actions:<action,action,..> [params]
	
	float scale = 1.f;
	
	int right   = +1;
	int up      = +2;
	int forward = +3;
	
	// 1) read file contents
	std::vector<SectionRecord> records;
	
	if (Path::GetExtension(filename, true) == "txt")
	{
		FileReader r;
		
		if (r.open(filename, true))
		{
			std::string line;
			
			while (r.read(line))
			{
				SectionRecord record;
				
				if (parseSectionRecord(filename, line, record))
					records.push_back(record);
			}
		}
		else
		{
			logError("failed to open %s", filename);
		}
	}
	else
	{
		std::vector<std::string> lines;
		lines.push_back(std::string("boneset file:") + filename);
		lines.push_back(std::string("meshset file:") + filename);
		
		for (const std::string & line : lines)
		{
			SectionRecord record;
		
			if (parseSectionRecord(filename, line, record))
				records.push_back(record);
		}
	}
	
	// 2) load the bone set
	
	for (size_t i = 0; i < records.size(); ++i)
	{
		SectionRecord & record = records[i];
		
		if (record.name == "boneset")
		{
			record.consumed = true;
			
			if (boneSet)
				logError("%s: more than one bone set specified: %s (%s)", filename, record.line.c_str(), record.name.c_str());
			else
			{
				const std::string file = record.args.getString("file", "");
				
				if (file.empty())
					logError("%s: mandatory property 'file' not specified: %s (%s)", filename, record.line.c_str(), record.name.c_str());
				else
				{
					logDebug("using bone set from %s", file.c_str());
					
					Loader * loader = createLoader(file.c_str());
					
					if (loader)
					{
						boneSet = loader->loadBoneSet(file.c_str());
						delete loader;
					}
				}
			}
		}
	}
	
	if (boneSet == 0)
	{
		boneSet = new BoneSet();
		boneSet->allocate(1);
		
		Bone & bone = boneSet->m_bones[0];
		bone.name = "root";
		bone.poseMatrix.MakeIdentity();
		bone.parent = -1;
		bone.originalIndex = 0;
	}
	
	// 3) load mesh set and anim sets
	
	for (size_t i = 0; i < records.size(); ++i)
	{
		SectionRecord & record = records[i];
		
		if (record.name == "meshset")
		{
			record.consumed = true;
			
			if (meshSet)
				logError("%s: more than one mesh set specified: %s (%s)", filename, record.line.c_str(), record.name.c_str());
			else
			{
				const std::string file = record.args.getString("file", "");
				
				if (file.empty())
					logError("%s: mandatory property 'file' not specified: %s (%s)", filename, record.line.c_str(), record.name.c_str());
				else
				{
					logDebug("using mesh set from %s", file.c_str());
					
					Loader * loader = createLoader(file.c_str());
					
					if (loader)
					{
						meshSet = loader->loadMeshSet(file.c_str(), boneSet);
						delete loader;
					}
				}
			}
		}
		
		if (record.name == "animset")
		{
			record.consumed = true;
			
			const std::string file = record.args.getString("file", "");
			
			if (file.empty())
				logError("%s: mandatory property 'file' not specified: %s (%s)", filename, record.line.c_str(), record.name.c_str());
			else
			{
				logDebug("using anim set from %s", file.c_str());
				
				Loader * loader = createLoader(file.c_str());
				
				if (loader)
				{
					AnimSet * temp = loader->loadAnimSet(file.c_str(), boneSet);
					
					// todo: apply name when not merging
					
					const std::string name = record.args.getString("name", "");
					
					if (!name.empty())
						temp->rename(name);
					
					if (!animSet)
						animSet = temp;
					else
						animSet->mergeFromAndFree(temp);
						
					delete loader;
				}
			}
		}
	}
	
	if (meshSet == 0)
	{
		meshSet = new MeshSet();
	}
	
	if (!animSet)
	{
		animSet = new AnimSet();
	}
	
	// 4) load model info and animations & triggers
	
	Anim * currentAnim = 0;
	
	for (size_t i = 0; i < records.size(); ++i)
	{
		SectionRecord & record = records[i];
		
		if (record.name == "model")
		{
			record.consumed = true;
			
			// rootnode, scale, up, right, forward
			
			rootNode = record.args.getString("rootnode", "");
			
			scale = record.args.getFloat("scale", 1.f);
			
			const std::string directionStr[3] =
			{
				record.args.getString("right",   "+x"),
				record.args.getString("up",      "+y"),
				record.args.getString("forward", "+z")
			};
			
			int * directionInt[3] = { &right, &up, &forward };
			
			for (int j = 0; j < 3; ++j)
			{
				const std::string & str = directionStr[j];
				
				if (str.size() != 2 || (str[0] != '+' && str[0] != '-') || (str[1] != 'x' && str[1] != 'y' && str[1] != 'z'))
					logError("%s: invalid right, up, or forward string: %s (%d): %s (%s)", filename, str.c_str(), (int)str.size(), record.line.c_str(), record.name.c_str());
				else
				{
					const char directionChr[3] = { 'x', 'y', 'z' };
					const int sign = str[0] == '+' ? +1 : -1;
					
					for (int k = 0; k < 3; ++k)
					{
						if (str[1] == directionChr[k])
						{
							*directionInt[j] = sign * (k + 1);
							logDebug("mapping %c to %+d", directionChr[k], *directionInt[j]);
						}
					}
				}
			}
		}
		
		if (record.name == "animation")
		{
			record.consumed = true;
			
			const std::string name = record.args.getString("name", "");
			
			if (name.empty())
				logError("%s: mandatory property 'name' not specified: %s (%s)", filename, record.line.c_str(), record.name.c_str());
			else
			{
				std::map<std::string, Anim*>::iterator a = animSet->m_animations.find(name);
				
				if (a == animSet->m_animations.end())
					logError("%s: animation '%s' does not exist: %s (%s)", filename, name.c_str(), record.line.c_str(), record.name.c_str());
				else
				{
					currentAnim = a->second;
					
					currentAnim->m_rootMotion = record.args.getBool("rootmotion", currentAnim->m_rootMotion);
					currentAnim->m_loop = record.args.getInt("loop", currentAnim->m_loop);
				}
			}
		}
		
		if (record.name == "trigger")
		{
			record.consumed = true;
			
			if (!currentAnim)
			{
				logError("%s: must first define an animation before adding triggers to it! %s", filename, record.line.c_str());
				continue;
			}
			
			const float time = record.args.getFloat("time", 0.f);
			const int loop = record.args.getInt("loop", -1);
			
			// todo: calculate end time for animations
			
			if (time < 0.f/* || time > currentAnim->endTime*/)
			{
				logWarning("%s: time is not an interval within the animation: %s", filename, record.line.c_str());
				continue;
			}
			
			const std::string actions = record.args.getString("actions", "");
			
			//logInfo("added anim trigger. time=%g, actions=%s", time, actions.c_str());
			
			AnimTrigger trigger;
			trigger.time = time;
			trigger.loop = loop;
			trigger.actions = actions;
			trigger.args = record.args;
			currentAnim->m_triggers.push_back(trigger);
		}
		
		if (!record.consumed)
		{
			logError("%s: unknown section: %s (%s)", filename, record.line.c_str(), record.name.c_str());
		}
	}
	
	// set to 'object' space transform
	
	memset(&meshToObject, 0, sizeof(meshToObject));
	meshToObject((right   < 0 ? -right   : +right  ) - 1, 0) = right   < 0 ? -scale : +scale;
	meshToObject((up      < 0 ? -up      : +up     ) - 1, 1) = up      < 0 ? -scale : +scale;
	meshToObject((forward < 0 ? -forward : +forward) - 1, 2) = forward < 0 ? -scale : +scale;
	meshToObject(3, 3) = 1.f;
	
	//dumpMatrix(meshToObject);
}

//

void ModelCache::clear()
{
	for (Map::iterator i = m_map.begin(); i != m_map.end(); ++i)
	{
		i->second.free();
	}
	
	m_map.clear();
}

void ModelCache::reload()
{
	for (Map::iterator i = m_map.begin(); i != m_map.end(); ++i)
	{
		i->second.load(i->first.c_str());
	}
}

ModelCacheElem & ModelCache::findOrCreate(const char * name)
{
	Key key = name;
	
	Map::iterator i = m_map.find(key);
	
	if (i != m_map.end())
	{
		return i->second;
	}
	else
	{
		ModelCacheElem elem;
		
		elem.load(name);
		
		i = m_map.insert(Map::value_type(key, elem)).first;
		
		return i->second;
	}
}
