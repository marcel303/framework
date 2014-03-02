#include <algorithm>
#include <assert.h>
#include <GL/glew.h>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include "framework.h"
#include "internal.h"
#include "Mat4x4.h"
#include "model.h"
#include "model_fbx.h"
#include "model_ogre.h"

#define DEBUG_TRS 0

ModelCache g_modelCache;

namespace AnimModel
{
	Mesh::Mesh()
	{
		m_vertices = 0;
		m_numVertices = 0;
		
		m_indices = 0;
		m_numIndices = 0;
		
		m_vertexArray = 0;
		m_indexArray = 0;
	}
	
	Mesh::~Mesh()
	{
		allocateVB(0);
		allocateIB(0);
		
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
		
		glGenBuffers(1, &m_vertexArray);
		glBindBuffer(GL_ARRAY_BUFFER, m_vertexArray);
		glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * m_numVertices, m_vertices, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		
		glGenBuffers(1, &m_indexArray);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexArray);
		if (m_numVertices < 65536)
		{
			unsigned short * indices = new unsigned short[m_numIndices];
			for (int i = 0; i < m_numIndices; ++i)
				indices[i] = m_indices[i];
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(short) * m_numIndices, indices, GL_STATIC_DRAW);
			delete [] indices;
		}
		else
		{
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(int) * m_numIndices, m_indices, GL_STATIC_DRAW);
		}
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
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
				assert(false);
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
	
	bool Anim::evaluate(float time, BoneTransform * transforms)
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
				
				BoneTransform transform;
				
				if (key != lastKey && time >= key->time)
				{
					const AnimKey & key1 = key[0];
					const AnimKey & key2 = key[1];
					
					assert(time >= key1.time && time <= key2.time);
					
					const float t = (time - key1.time) / (key2.time - key1.time);
					
					assert(t >= 0.f && t <= 1.f);
					
					AnimKey::interpolate(transform, key1, key2, t, m_rotationType);
				}
				else
				{
					// either the first or last key in the animation. copy value
					
					assert(key == firstKey || key == lastKey);
					
					AnimKey::interpolate(transform, *key, *key, 0.f, m_rotationType);
				}
				
				if (m_isAdditive)
					BoneTransform::add(transforms[i], transform);
				else
					transforms[i] = transform;
				
				if (key != lastKey)
				{
					isDone = false;
				}
				
				firstKey += numKeys;
			}
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

Model::Model(const char * filename)
{
	ctor();
	
	m_model = &g_modelCache.findOrCreate(filename);
}

Model::Model(ModelCacheElem & cacheElem)
{
	ctor();
	
	m_model = &cacheElem;
}

void Model::ctor()
{
	// drawing
	x = y = z = 0.f;
	axis = Vec3(1.f, 0.f, 0.f);
	angle = 0.f;
	scale = 1.f;
	
	// animation
	m_animSegment = 0;
	animIsActive = false;
	animIsPaused = false;
	m_isAnimStarted = false;
	animTime = 0.f;
	animLoop = 0;
	animLoopCount = 0;
	animSpeed = 1.f;
	
	framework.registerModel(this);
}

Model::~Model()
{
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

void Model::draw(int drawFlags)
{
	drawEx(Vec3(x, y, z), axis, angle, scale, drawFlags);
}

void Model::drawEx(Vec3 position, Vec3 axis, float angle, float scale, int drawFlags)
{
	// build transformation matrix
	 
	Quat rotation;
	rotation.fromAxisAngle(axis, angle);
	
	Mat4x4 matrix;
	rotation.toMatrix3x3(matrix);
	
	matrix(0, 3) = 0.f;
	matrix(1, 3) = 0.f;
	matrix(2, 3) = 0.f;
	
	matrix(3, 0) = x;
	matrix(3, 1) = y;
	matrix(3, 2) = z;
	matrix(3, 3) = 1.f;
	
	for (int i = 0; i < 3; ++i)
		for (int j = 0; j < 3; ++j)
			matrix(i, j) *= scale;
	
	drawEx(matrix, drawFlags);
}

void Model::drawEx(const Mat4x4 & matrix, int drawFlags)
{
	// calculate transforms in local bone space
	
	BoneTransform * transforms = (BoneTransform*)alloca(sizeof(BoneTransform) * m_model->boneSet->m_numBones);
	
	for (int i = 0; i < m_model->boneSet->m_numBones; ++i)
	{
		transforms[i] = m_model->boneSet->m_bones[i].transform;
	}
	
	// apply animations
	
	// todo: move to updateAnimation
	if (m_isAnimStarted && m_animSegment && !animIsPaused)
	{
		Anim * anim = reinterpret_cast<Anim*>(m_animSegment);
		
		const bool isDone = anim->evaluate(animTime, transforms);
		
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
	
	// convert translation / rotation pairs into matrices
	
	Mat4x4 * localMatrices = (Mat4x4*)alloca(sizeof(Mat4x4) * m_model->boneSet->m_numBones);
	
	for (int i = 0; i < m_model->boneSet->m_numBones; ++i)
	{
		// todo: scale?
		
		const BoneTransform & transform = transforms[i];
		
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
	
	Mat4x4 * worldMatrices = (Mat4x4*)alloca(sizeof(Mat4x4) * m_model->boneSet->m_numBones);
	
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
	
	Mat4x4 * globalMatrices = (Mat4x4*)alloca(sizeof(Mat4x4) * m_model->boneSet->m_numBones);
	
	for (int i = 0; i < m_model->boneSet->m_numBones; ++i)
	{
		const Mat4x4 & worldToBone = m_model->boneSet->m_bones[i].poseMatrix;
		
		globalMatrices[i] = worldMatrices[i] * worldToBone;
	}
	
	// draw
	
	if (drawFlags & DrawMesh)
	{
		// set uniform constants for skinning matrices
		
		Shader shader("engine/BasicSkinned"); // fixme!
		setShader(shader);
		
		const GLint boneMatrices = shader.getImmediate("skinningMatrices");
		
		if (boneMatrices != -1)
		{
			glUniformMatrix4fv(boneMatrices, m_model->boneSet->m_numBones, GL_FALSE, (GLfloat*)globalMatrices);
			checkErrorGL();
		}
		
		const GLint drawColor = shader.getImmediate("drawColor");
		
		if (drawColor != -1)
		{
			glUniform4f(drawColor,
				drawFlags & DrawColorTexCoords,
				drawFlags & DrawColorNormals,
				drawFlags & DrawColorBlendIndices,
				drawFlags & DrawColorBlendWeights);
		}
		
		for (int i = 0; i < m_model->meshSet->m_numMeshes; ++i)
		{
			const Mesh * mesh = m_model->meshSet->m_meshes[i];
			
			const GLint position = shader.getAttribute("in_position");
			const GLint normal = shader.getAttribute("in_normal");
			const GLint color = shader.getAttribute("in_color");
			const GLint texcoord = shader.getAttribute("in_texcoord");
			const GLint boneIndices = shader.getAttribute("in_skinningBlendIndices");
			const GLint boneWeights = shader.getAttribute("in_skinningBlendWeights");
			
			// bind vertex arrays
			
			fassert(mesh->m_vertexArray);
			fassert(mesh->m_indexArray);
			glBindBuffer(GL_ARRAY_BUFFER, mesh->m_vertexArray);
			checkErrorGL();
			
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->m_indexArray);
			checkErrorGL();
			
			if (position != -1)
			{
				glVertexAttribPointer(position, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, px));
				glEnableVertexAttribArray(position);
				checkErrorGL();
			}
			
			if (normal != -1)
			{
				glVertexAttribPointer(normal, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, nx));
				glEnableVertexAttribArray(normal);
				checkErrorGL();
			}
			
			if (color != -1)
			{
				glVertexAttribPointer(color, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, cx));
				glEnableVertexAttribArray(color);
				checkErrorGL();
			}
			
			if (texcoord != -1)
			{
				glVertexAttribPointer(texcoord, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tx));
				glEnableVertexAttribArray(texcoord);
				checkErrorGL();
			}
			
			if (boneIndices != -1)
			{
				glVertexAttribPointer(boneIndices, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, boneIndices));
				glEnableVertexAttribArray(boneIndices);
				checkErrorGL();
			}
			
			if (boneWeights != -1)
			{
				glVertexAttribPointer(boneWeights, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(Vertex), (void*)offsetof(Vertex, boneWeights));
				glEnableVertexAttribArray(boneWeights);
				checkErrorGL();
			}
			
			GLenum indexType = mesh->m_numVertices < 65536 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;
			glDrawElements(GL_TRIANGLES, mesh->m_numIndices, indexType, 0);
			checkErrorGL();
			
			if (position != -1)
				glDisableVertexAttribArray(position);
			if (normal != -1)
				glDisableVertexAttribArray(normal);
			if (color != -1)
				glDisableVertexAttribArray(color);
			if (texcoord != -1)
				glDisableVertexAttribArray(texcoord);
			if (boneIndices != -1)
				glDisableVertexAttribArray(boneIndices);
			if (boneWeights != -1)
				glDisableVertexAttribArray(boneWeights);
			checkErrorGL();
			
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			checkErrorGL();
			
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
			checkErrorGL();
		}
	}
	
	if (drawFlags & DrawNormals)
	{
		clearShader();
		
		for (int i = 0; i < m_model->meshSet->m_numMeshes; ++i)
		{
			const Mesh * mesh = m_model->meshSet->m_meshes[i];
			
			std::vector<Vec3> positions;
			std::vector<Vec3> normals;
			
			positions.resize(mesh->m_numVertices);
			normals.resize(mesh->m_numVertices);
			
			for (int j = 0; j < mesh->m_numVertices; ++j)
			{
				const Vertex & vertex = mesh->m_vertices[j];
				
				// -- software vertex blend (soft skinned) --
				Vec3 p(0.f, 0.f, 0.f);
				Vec3 n(0.f, 0.f, 0.f);
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
				
				positions[j] = p;
				normals[j] = n;
			}
			
			glBegin(GL_LINES);
			{
				for (int j = 0; j < mesh->m_numVertices; ++j)
				{
					const float scale = 3.f;
					
					const Vec3 & n  = normals[j];
					const Vec3 & p1 = positions[j];
					const Vec3   p2 = positions[j] + n * scale;
					
					glColor3ub(127, 127, 127);
					glNormal3f(n[0], n[1], n[2]);
					glVertex3f(p1[0], p1[1], p1[2]);
					glVertex3f(p2[0], p2[1], p2[2]);
				}
			}
			glEnd();
		}
	}
	
	if (drawFlags & DrawBones)
	{
		clearShader();
		
		// bone to object matrix translation
		glDisable(GL_DEPTH_TEST);
		glColor3ub(127, 127, 127);
		glBegin(GL_LINES);
		{
			for (int boneIndex = 0; boneIndex < m_model->boneSet->m_numBones; ++boneIndex)
			{
				const int parentBoneIndex = m_model->boneSet->m_bones[boneIndex].parent;
				if (parentBoneIndex != -1)
				{
					const Mat4x4 & m1 = worldMatrices[boneIndex];
					const Mat4x4 & m2 = worldMatrices[parentBoneIndex];
					glVertex3f(m1(3, 0), m1(3, 1), m1(3, 2));
					glVertex3f(m2(3, 0), m2(3, 1), m2(3, 2));
				}
			}
		}
		glEnd();
		glColor3ub(0, 255, 0);
		glPointSize(5.f);
		glBegin(GL_POINTS);
		{
			for (int boneIndex = 0; boneIndex < m_model->boneSet->m_numBones; ++boneIndex)
			{
				const Mat4x4 & m = worldMatrices[boneIndex];
				glVertex3f(m(3, 0), m(3, 1), m(3, 2));
			}
		}
		glEnd();
		glEnable(GL_DEPTH_TEST);
	}
	
	if (drawFlags & DrawPoseMatrices)
	{
		clearShader();
		
		// object to bone matrix translation
		glDisable(GL_DEPTH_TEST);
		glColor3ub(127, 127, 127);
		glBegin(GL_LINES);
		{
			for (int boneIndex = 0; boneIndex < m_model->boneSet->m_numBones; ++boneIndex)
			{
				const int parentBoneIndex = m_model->boneSet->m_bones[boneIndex].parent;
				if (parentBoneIndex != -1)
				{
					const Mat4x4 m1 = matrix * m_model->meshToObject * m_model->boneSet->m_bones[boneIndex].poseMatrix.CalcInv();
					const Mat4x4 m2 = matrix * m_model->meshToObject * m_model->boneSet->m_bones[parentBoneIndex].poseMatrix.CalcInv();
					glVertex3f(m1(3, 0), m1(3, 1), m1(3, 2));
					glVertex3f(m2(3, 0), m2(3, 1), m2(3, 2));
				}
			}
		}
		glEnd();
		glColor3ub(255, 0, 0);
		glPointSize(7.f);
		glBegin(GL_POINTS);
		{
			for (int boneIndex = 0; boneIndex < m_model->boneSet->m_numBones; ++boneIndex)
			{
				const Mat4x4 m = matrix * m_model->meshToObject * m_model->boneSet->m_bones[boneIndex].poseMatrix.CalcInv();
				glVertex3f(m(3, 0), m(3, 1), m(3, 2));
			}
		}
		glEnd();
		glEnable(GL_DEPTH_TEST);
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
			log("unable to find animation: %s", m_animSegmentName.c_str());
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
	// todo: evaluate bone transforms, and update root motion
	
	animRootMotion.SetZero();
	
	const float oldTime = animTime;
	animTime += animSpeed * timeStep;
	const float newTime = animTime;
	
	Anim * anim = static_cast<Anim*>(m_animSegment);
	
	if (anim)
	{
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
			
			//log("added anim trigger. time=%g, actions=%s", time, actions.c_str());
			
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
	meshToObject(std::abs(right  ) - 1, 0) = right   < 0 ? -scale : +scale;
	meshToObject(std::abs(up     ) - 1, 1) = up      < 0 ? -scale : +scale;
	meshToObject(std::abs(forward) - 1, 2) = forward < 0 ? -scale : +scale;
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
