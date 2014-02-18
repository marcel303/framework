#include <assert.h>
#include <GL/glew.h>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include "Mat4x4.h"
#include "model.h"

#define DEBUG_TRS 0

namespace Model
{
	Mesh::Mesh()
	{
		m_vertices = 0;
		m_numVertices = 0;
		
		m_indices = 0;
		m_numIndices = 0;
	}
	
	Mesh::~Mesh()
	{
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
		#if 1 // todo: normalize FBX animation data and actually use addition again
		result.translation += transform.translation;
		result.rotation *= transform.rotation;
		result.scale += transform.scale;
		#else
		result.translation = transform.translation;
		result.rotation = transform.rotation;
		result.scale = transform.scale;
		#endif
	}
	
	//
	
	BoneSet::BoneSet()
	{
		m_bones = 0;
		m_numBones = 0;
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
	}
	
	Anim::~Anim()
	{
		allocate(0, 0, m_rotationType);
	}
	
	void Anim::allocate(int numBones, int numKeys, RotationType rotationType)
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
				
				BoneTransform::add(transforms[i], transform);
				
				if (key != lastKey)
				{
					isDone = false;
				}
				
				firstKey += numKeys;
			}
		}
		
		return isDone;
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
	
	//
	
	MeshSet * Cache::findOrCreateMeshSet(const char * filename)
	{
		return 0;
	}
	
	BoneSet * Cache::findOrCreateBoneSet(const char * filename)
	{
		return 0;
	}
	
	AnimSet * Cache::findOrCreateAnimSet(const std::vector<std::string> & filenames)
	{
		return 0;
	}
}

//

using namespace Model;

AnimModel::AnimModel(const char * filename)
{
	// todo: fetch stuff from caches
}

AnimModel::AnimModel(MeshSet * meshes, BoneSet * bones, AnimSet * animations)
{
	m_meshes = meshes;
	m_bones = bones;
	m_animations = animations;
	
	currentAnim = 0;
	
	x = y = z = 0.f;
	axis = Vec3(1.f, 0.f, 0.f);
	angle = 0.f;
	scale = 1.f;
	
	animIsDone = true;
	animTime = 0.f;
	animLoop = 0;
	animSpeed = 1.f;
}

AnimModel::~AnimModel()
{
	// todo: move resource ownership to cache mgr
	
	delete m_meshes;
	m_meshes = 0;
	
	delete m_bones;
	m_bones = 0;
	
	delete m_animations;
	m_animations = 0;
}

void AnimModel::startAnim(const char * name, int loop)
{
	assert(loop != 0);
	
	std::map<std::string, Anim*>::iterator i = m_animations->m_animations.find(name);
	
	if (i != m_animations->m_animations.end())
	{
		currentAnim = i->second;
	}
	else
	{
		printf("animation not found: %s\n", name); // todo: logWarning
		
		currentAnim = 0;
	}
	
	animIsDone = false;
	animTime = 0.f;
	animLoop = loop;
	
	if (animLoop > 0)
		animLoop--;
}

void AnimModel::process(float timeStep)
{
	animTime += animSpeed * timeStep;
}

void AnimModel::draw(int drawFlags)
{
	drawEx(Vec3(x, y, z), axis, angle, scale, drawFlags);
}

void AnimModel::drawEx(Vec3 position, Vec3 axis, float angle, float scale, int drawFlags)
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

void AnimModel::drawEx(const Mat4x4 & matrix, int drawFlags)
{
	// calculate transforms in local bone space
	
	BoneTransform * transforms = (BoneTransform*)alloca(sizeof(BoneTransform) * m_bones->m_numBones);
	
	for (int i = 0; i < m_bones->m_numBones; ++i)
	{
		transforms[i] = m_bones->m_bones[i].transform;
	}
	
	// apply animations
	
	if (currentAnim)
	{
		animIsDone = currentAnim->evaluate(animTime, transforms);
		
		if (animIsDone && (animLoop > 0 || animLoop < 0))
		{
			animIsDone = false;
			animTime = 0.f;
			if (animLoop > 0)
				animLoop--;
		}
	}
	
	// convert translation / rotation pairs into matrices
	
	Mat4x4 * localMatrices = (Mat4x4*)alloca(sizeof(Mat4x4) * m_bones->m_numBones);
	
	for (int i = 0; i < m_bones->m_numBones; ++i)
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
	
	Mat4x4 * worldMatrices = (Mat4x4*)alloca(sizeof(Mat4x4) * m_bones->m_numBones);
	
	for (int i = 0; i < m_bones->m_numBones; ++i)
	{
		// todo: calculate matrices for a given bone only once
		//       will need to sort bones by their distance from parent
		
		int boneIndex = i;
		
		Mat4x4 finalMatrix = localMatrices[boneIndex];
		
		boneIndex = m_bones->m_bones[boneIndex].parent;
		
		while (boneIndex != -1)
		{
			finalMatrix = localMatrices[boneIndex] * finalMatrix;
			
			boneIndex = m_bones->m_bones[boneIndex].parent;
		}
		
		worldMatrices[i] = matrix * finalMatrix;
	}
	
	// draw
	
	if (drawFlags & DrawMesh)
	{
		for (int i = 0; i < m_meshes->m_numMeshes; ++i)
		{
			const Mesh * mesh = m_meshes->m_meshes[i];
			
			std::vector<Vec3> normals;
			
			glBegin(GL_TRIANGLES);
			{
				for (int j = 0; j < mesh->m_numIndices; ++j)
				{
					const int vertexIndex = mesh->m_indices[j];
					
					const Vertex & vertex = mesh->m_vertices[vertexIndex];
					
					// todo: apply vertex transformation using a shader
					
					// -- software vertex blend (soft skinned) --
					Vec3 p(0.f, 0.f, 0.f);
					Vec3 n(0.f, 0.f, 0.f);
					for (int b = 0; b < 4; ++b)
					{
						if (vertex.boneWeights[b] == 0)
							continue;
						const int boneIndex = vertex.boneIndices[b];
						const float boneWeight = vertex.boneWeights[b] / 255.f;						
						const Mat4x4 & boneToWorld = worldMatrices[boneIndex];
						const Mat4x4 & worldToBone = m_bones->m_bones[boneIndex].poseMatrix;
						p += boneToWorld.Mul4(worldToBone.Mul4(Vec3(vertex.px, vertex.py, vertex.pz))) * boneWeight;
						n += boneToWorld.Mul3(worldToBone.Mul3(Vec3(vertex.nx, vertex.ny, vertex.nz))) * boneWeight;
					}
					// -- software vertex blend (soft skinned) --
					
				#if 1
					float r = 1.f;
					float g = 1.f;
					float b = 1.f;
					
					if (1)
					{
						r *= (n[0] + 1.f) / 2.f;
						g *= (n[1] + 1.f) / 2.f;
						b *= (n[2] + 1.f) / 2.f;
					}
					
					glColor3f(r, g, b);
				#endif
					
					glTexCoord2f(vertex.tx, 1.f - vertex.ty);
					glNormal3f(n[0], n[1], n[2]);
					glVertex3f(p[0], p[1], p[2]);
					
					if (drawFlags & DrawNormals)
					{
						normals.push_back(Vec3(p[0], p[1], p[2]));
						normals.push_back(Vec3(n[0], n[1], n[2]));
					}
				}
			}
			glEnd();
			
			if (drawFlags & DrawNormals)
			{
				glBegin(GL_LINES);
				{
					for (size_t j = 0; j < normals.size() / 2; ++j)
					{
						Vec3 p = normals[j * 2 + 0];
						Vec3 n = normals[j * 2 + 1] * 5.f;
						
						glColor3ub(127, 127, 127);
						glNormal3f(n[0], n[1], n[2]);
						glVertex3f(p[0],        p[1],        p[2]       );
						glVertex3f(p[0] + n[0], p[1] + n[1], p[2] + n[2]);
					}
				}
				glEnd();
			}
		}
	}
	
	if (drawFlags & DrawBones)
	{
		// bone to object matrix translation
		glDisable(GL_DEPTH_TEST);
		glColor3ub(127, 127, 127);
		glBegin(GL_LINES);
		{
			for (int boneIndex = 0; boneIndex < m_bones->m_numBones; ++boneIndex)
			{
				const int parentBoneIndex = m_bones->m_bones[boneIndex].parent;
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
			for (int boneIndex = 0; boneIndex < m_bones->m_numBones; ++boneIndex)
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
		// object to bone matrix translation
		glDisable(GL_DEPTH_TEST);
		glColor3ub(127, 127, 127);
		glBegin(GL_LINES);
		{
			for (int boneIndex = 0; boneIndex < m_bones->m_numBones; ++boneIndex)
			{
				const int parentBoneIndex = m_bones->m_bones[boneIndex].parent;
				if (parentBoneIndex != -1)
				{
					const Mat4x4 m1 = m_bones->m_bones[boneIndex].poseMatrix.CalcInv();
					const Mat4x4 m2 = m_bones->m_bones[parentBoneIndex].poseMatrix.CalcInv();
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
			for (int boneIndex = 0; boneIndex < m_bones->m_numBones; ++boneIndex)
			{
				const Mat4x4 m = m_bones->m_bones[boneIndex].poseMatrix.CalcInv();
				glVertex3f(m(3, 0), m(3, 1), m(3, 2));
			}
		}
		glEnd();
		glEnable(GL_DEPTH_TEST);
	}
}
