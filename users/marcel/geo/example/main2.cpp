#include "framework.h"
#include "model.h"

#include "Geo.h"
#include "GeoBuilder.h"

#include "Calc.h"
#include "PolledTimer.h"
#include "Timer.h"

// todo : add CSG support to geom builder. 'begin(mesh, mode)' where mode is '+', '-', '&', 'x'

// todo : add CSG implementation with keeps the BSP tree of the mesh being built alive

enum Varying
{
	kVarying_Color,
	kVarying_Texcoord,
	kVarying_BlendIndices,
	kVarying_BlendWeights,
};

static void CreateBlendShape(Geo::Mesh& mesh)
{
	const int size = 12;

	Geo::Builder b;
	
	b.grid(mesh, 2, size, size, Vec3());
	b.push();
	//b.scale(.5, .5, .5);
	//b.sphere(mesh, 10, 10);
	//b.donut(mesh, 40, 40, 1.0, 0.5);
	b.pop();

	for (auto * poly : mesh.polys)
	{
	
		for (auto * vertex : poly->vertices)
		{
		
			float u = (vertex->position[0] + 1.0f) / 2.0f;
			float v = (vertex->position[1] + 1.0f) / 2.0f;

			vertex->varying[kVarying_Color].Set(
				u,
				v,
				1.0f - (u + v) / 2.0f,
				1.0f);

			float blendWeights[4] =
			{
				(1.0f - u) + (1.0f - v),
				u          + (1.0f - v),
				u          + v,
				(1.0f - u) + v
			};

			// Normalize blendWeight vector.

			float size = 0.0f;
			for (int k = 0; k < 4; ++k)
				size += blendWeights[k] * blendWeights[k];
			size = sqrtf(size);
			for (int k = 0; k < 4; ++k)
				blendWeights[k] /= size;

			vertex->varying[kVarying_BlendWeights].Set(
				blendWeights[0],
				blendWeights[1],
				blendWeights[2],
				blendWeights[3]);
			
		}
		
	}
	
}

typedef std::vector<float> InfluenceList;

class Influence
{

public:

	Vec3 position;
	
	float CalculateInfluence(Vec3Arg position) const
	{
	
		Vec3 delta = position - this->position;
		
		float distance = delta.CalcSize() * 100.0f;
		
		return 1.0f / pow((distance + 1.0f), 4.0f);
		
	}
	
};

class EditBone
{

public:

	AnimModel::Bone bone;
	
	std::vector<AnimModel::AnimKey> animation;
	
	std::vector<Influence> influences;
	
	float CalculateInfluence(Vec3Arg position) const
	{
	
		float influence = 0.0f;
		
		for (size_t i = 0; i < influences.size(); ++i)
			influence += influences[i].CalculateInfluence(position);
		
		return influence;
		
	}
	
};

class Triangle
{

public:

	Vec3 position[3];
	
};

static void Normalize(InfluenceList& influences)
{

	float norm = 0.0f;
	for (size_t i = 0; i < influences.size(); ++i)
		norm += influences[i] * influences[i];
	
	norm = sqrt(norm);
	
	for (size_t i = 0; i < influences.size(); ++i)
		influences[i] /= norm;
	
}

static Mat4x4 MakeEulerRotationMatrix(Vec3Arg rotation)
{

	Mat4x4 rotationX; rotationX.MakeRotationX(rotation[0]);
	Mat4x4 rotationY; rotationY.MakeRotationY(rotation[1]);
	Mat4x4 rotationZ; rotationZ.MakeRotationZ(rotation[2]);

	Mat4x4 matrix = rotationZ * rotationY * rotationX;
	
	return matrix;

}

static void CreateBone(std::string name, std::string parentName, Vec3 position, Vec3 rotation, AnimModel::Bone& out_bone)
{

	Mat4x4 matPos;
	Mat4x4 matRot;

	matPos.MakeTranslation(position);
	matRot = MakeEulerRotationMatrix(rotation);

	out_bone.name = name;
	out_bone.poseMatrix = (matPos * matRot).CalcInv();
	out_bone.parentName = parentName;
	
}

static void CalculateTexcoord(Vec3Arg position, Vec2 & texcoord)
{

	texcoord[0] = position[0];
	texcoord[1] = position[1];

}

static void CalculateTexcoords(Geo::Mesh& mesh)
{

	for (auto * poly : mesh.polys)
	{
	
		for (auto * vertex : poly->vertices)
		{
	
			float x = vertex->position[0];
			float y = vertex->position[1];
			float z = vertex->position[2];
			
			Vec3 position(x, y, z);
			Vec2 texcoord;
			
			CalculateTexcoord(position, texcoord);
			
			vertex->varying[kVarying_Texcoord][0] = texcoord[0];
			vertex->varying[kVarying_Texcoord][1] = texcoord[1];
			
		}
		
	}
}

static void CreateGrid(std::vector<Triangle>& out_triangles, int size, int axis1, int axis2, Vec3 center)
{

	for (int i = 0; i < size - 1; ++i)
	{
	
		for (int j = 0; j < size - 1; ++j)
		{
		
			float u1 = ((i + 0) / float(size - 1) - 0.5f) * 2.0f;
			float v1 = ((j + 0) / float(size - 1) - 0.5f) * 2.0f;
			float u2 = ((i + 1) / float(size - 1) - 0.5f) * 2.0f;
			float v2 = ((j + 1) / float(size - 1) - 0.5f) * 2.0f;
			
			Triangle triangle;
			Vec3 position;

			position[axis1] = u1;
			position[axis2] = v1;
			position[3 - axis1 - axis2] = 0.0f;
			triangle.position[0] = position + center;

			position[axis1] = u2;
			position[axis2] = v1;
			position[3 - axis1 - axis2] = 0.0f;
			triangle.position[1] = position + center;

			position[axis1] = u2;
			position[axis2] = v2;
			position[3 - axis1 - axis2] = 0.0f;
			triangle.position[2] = position + center;

			out_triangles.push_back(triangle);

			position[axis1] = u1;
			position[axis2] = v1;
			position[3 - axis1 - axis2] = 0.0f;
			triangle.position[0] = position + center;

			position[axis1] = u2;
			position[axis2] = v2;
			position[3 - axis1 - axis2] = 0.0f;
			triangle.position[1] = position + center;

			position[axis1] = u1;
			position[axis2] = v2;
			position[3 - axis1 - axis2] = 0.0f;
			triangle.position[2] = position + center;

			out_triangles.push_back(triangle);
			
		}
		
	}
	
}

static void CreateSkinMesh(int influencesPerBone, Geo::Mesh& mesh)
{

	std::vector<Triangle> triangles;

	const int size = 20;

	CreateGrid(triangles, size, 0, 1, Vec3(0.0f, 0.0f, +1.0f));
	CreateGrid(triangles, size, 0, 1, Vec3(0.0f, 0.0f, -1.0f));
	CreateGrid(triangles, size, 1, 2, Vec3(+1.0f, 0.0f, 0.0f));
	CreateGrid(triangles, size, 1, 2, Vec3(-1.0f, 0.0f, 0.0f));
	CreateGrid(triangles, size, 2, 0, Vec3(0.0f, +1.0f, 0.0f));
	CreateGrid(triangles, size, 2, 0, Vec3(0.0f, -1.0f, 0.0f));
	
	for (size_t i = 0; i < triangles.size(); ++i)
	{
	
		auto * poly = mesh.Add();
		
		for (size_t j = 0; j < 3; ++j)
		{
		
			auto * vertex = poly->Add();
			
			vertex->position = triangles[i].position[j];
			
			vertex->varying[kVarying_BlendWeights].Set(1.0f, 0.0f, 0.0f, 0.0f);
			vertex->varying[kVarying_BlendIndices].Set(0, 1, 2, 3);
			//vertex->varying[kVarying_BlendIndices].Set(3, 2, 1, 0);
			vertex->varying[kVarying_Color].Set(1, 1, 1, 1);
			
		}
		
	}

	CalculateTexcoords(mesh);
	
}

static void CreateSkin(std::vector<EditBone>& bones, Geo::Mesh& out_mesh)
{

	// Create mesh.

	CreateSkinMesh(bones.size(), out_mesh);

	// Fill influences.

	for (auto * poly : out_mesh.polys)
	{
	
		for (auto * vertex : poly->vertices)
		{
		
			InfluenceList influences;
			
			for (size_t j = 0; j < bones.size(); ++j)
			{
			
				float influence = bones[j].CalculateInfluence(vertex->position);
				
				influences.push_back(influence);
				
			}
			
			while (influences.size() < 4)
				influences.push_back(0.0f);
			
			Normalize(influences);
			
			vertex->varying[kVarying_BlendWeights].Set(
				influences[0],
				influences[1],
				influences[2],
				influences[3]);
			
			vertex->varying[kVarying_Color].Set(
				influences[1],
				influences[2],
				influences[3],
				1.0f);
			
		}
		
	}
	
}

static void CreateSkeletonAndAnimation(std::vector<EditBone>& out_bones, AnimModel::BoneSet& out_boneSet, AnimModel::Anim& out_animation)
{

	EditBone bone_root;
	EditBone bone_child1;
	EditBone bone_child2;
	EditBone bone_child3;

	#define FILL_BONE(_bone, _name, _parentName) \
	{ \
		_bone.bone.name = _name; \
		_bone.bone.parentName = _parentName; \
	}

	#define KEYFRAME(_kfAnim, _time, _pos, _rot) \
	{ \
		AnimModel::AnimKey kf; \
		kf.time = _time; \
		kf.translation = _pos; \
		kf.rotation = _rot; \
		_kfAnim.push_back(kf); \
	}

	#define ADD_INFLUENCE(_bone, _position) \
	{ \
		Influence influence; \
		influence.position = _position; \
		_bone.influences.push_back(influence); \
	}

	FILL_BONE(bone_root, "root", "");
	FILL_BONE(bone_child1, "child1", "root");
	FILL_BONE(bone_child2, "child2", "root");
	FILL_BONE(bone_child3, "child3", "root");
	
	// No rotation.
	KEYFRAME(bone_root.animation,   0.0f, Vec3( 0.0f,  0.0f, 0.0f), Vec4(0.0f,                   0.0f, 0.0f, 0.0f));
	KEYFRAME(bone_root.animation,   10.0f, Vec3( 0.0f,  0.0f, 0.0f), Vec4(0.0f,                   0.0f, 0.0f, 0.0f));
	// Rotate around Y axis.
	KEYFRAME(bone_child1.animation, 0.0f, Vec3(-1.0f,  0.0f, 0.0f), Vec4(Calc::DegToRad( 0.0f), 0.0f, 1.0f, 0.0f));
	KEYFRAME(bone_child1.animation, 4.0f, Vec3(-1.0f,  0.0f, 0.0f), Vec4(Calc::DegToRad(45.0f), 0.0f, 1.0f, 0.0f));
	// Rotate around X axis.
	KEYFRAME(bone_child2.animation, 0.0f, Vec3(+1.0f,  0.0f, 0.0f), Vec4(Calc::DegToRad( 0.0f), 1.0f, 0.0f, 0.0f));
	KEYFRAME(bone_child2.animation, 4.0f, Vec3(+2.0f,  0.0f, 0.0f), Vec4(Calc::DegToRad(22.5f), 1.0f, 0.0f, 0.0f));
	KEYFRAME(bone_child2.animation, 8.0f, Vec3(+3.0f,  0.0f, 0.0f), Vec4(Calc::DegToRad(45.0f), 1.0f, 0.0f, 0.0f));
	// No rotation.
	KEYFRAME(bone_child3.animation, 0.0f, Vec3( 0.0f, +1.0f, 0.0f), Vec4(Calc::DegToRad( 0.0f), 0.0f, 0.0f, 1.0f));
	KEYFRAME(bone_child3.animation, 4.0f, Vec3( 0.0f, +0.5f, 0.0f), Vec4(Calc::DegToRad( 0.0f), 0.0f, 0.0f, 1.0f));

	ADD_INFLUENCE(bone_root,   Vec3( 0.0f,  0.0f, 0.0f));
	ADD_INFLUENCE(bone_child1, Vec3(-1.0f,  0.0f, 0.0f));
	ADD_INFLUENCE(bone_child2, Vec3(+1.0f,  0.0f, 0.0f));
	ADD_INFLUENCE(bone_child3, Vec3( 0.0f, +1.0f, 0.0f));

	// Copy key frame 1 as initial transform of each bone.

	bone_root.bone.poseMatrix   = bone_root.animation[0].toBoneTransform(AnimModel::RotationType_Quat).toMatrix().CalcInv();
	bone_child1.bone.poseMatrix = bone_child1.animation[0].toBoneTransform(AnimModel::RotationType_Quat).toMatrix().CalcInv();
	bone_child2.bone.poseMatrix = bone_child2.animation[0].toBoneTransform(AnimModel::RotationType_Quat).toMatrix().CalcInv();
	bone_child3.bone.poseMatrix = bone_child3.animation[0].toBoneTransform(AnimModel::RotationType_Quat).toMatrix().CalcInv();

	// Fill out_bones.

	out_bones.push_back(bone_root);
	out_bones.push_back(bone_child1);
	out_bones.push_back(bone_child2);
	out_bones.push_back(bone_child3);

	// Fill out_boneSet.

	out_boneSet.allocate(out_bones.size());
	for (size_t i = 0; i < out_bones.size(); ++i)
	{
		out_boneSet.m_bones[i] = out_bones[i].bone;
		out_boneSet.m_bones[i].transform = out_bones[i].animation[0].toBoneTransform(AnimModel::RotationType_EulerXYZ);
	}
	out_boneSet.calculateParentIndices();
	out_boneSet.sortBoneIndices();
	out_boneSet.calculatePoseMatrices();

	// Fill out_animation.

	int numKeys = 0;
	for (size_t i = 0; i < out_bones.size(); ++i)
		numKeys += out_bones[i].animation.size();
	
	out_animation.allocate(out_bones.size(), numKeys, AnimModel::RotationType_Quat, false);
	
	numKeys = 0;
	for (size_t i = 0; i < out_bones.size(); ++i)
	{
		out_animation.m_numKeys[i] = out_bones[i].animation.size();
		for (auto & key : out_bones[i].animation)
			out_animation.m_keys[numKeys++] = key;
	}
	
}

static void testGraphicsDeviceBlendMatrices()
{

	Geo::Mesh mesh;
	CreateBlendShape(mesh);

	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		framework.beginDraw(0, 0, 0, 0);
		
		projectPerspective3d(90.f, 0.01f, 100.0f);

		setDepthTest(true, DEPTH_LESS);
		
		Mat4x4 view;
		view.MakeLookat(
			Vec3(0.0f, 0.0f, 0.0f),
			Vec3(0.0f, 0.0f, +1.0f),
			Vec3(0.0f, +1.0f, 0.0f));
		
		gxSetMatrixf(GX_MODELVIEW, view.m_v);

		Mat4x4 blend1;
		Mat4x4 blend2;
		Mat4x4 blend3;
		Mat4x4 blend4;

		float time = framework.time;

		float phase1 = time * 1.0f;
		float phase2 = time * 2.0f;
		float phase3 = time * 4.0f;
		float phase4 = time * 8.0f;

		blend1.MakeRotationZ(sinf(phase1) * 1.0f);
		blend2.MakeRotationZ(sinf(phase2) * 1.0f);
		blend3.MakeRotationZ(sinf(phase3) * 1.0f);
		blend4.MakeRotationZ(sinf(phase4) * 1.0f);

		Mat4x4 worldRot;
		worldRot.MakeRotationZ(time * 100.0f / 666.0f);
		Mat4x4 worldPos;
		worldPos.MakeTranslation(Vec3(0.0f, 0.0f, +3.0f));

		gxMultMatrixf(worldPos.m_v);
		gxMultMatrixf(worldRot.m_v);
		
		Mat4x4 * blend[4] = { &blend1, &blend2, &blend3, &blend4 };
		
		for (auto * poly : mesh.polys)
		{
		
			gxBegin(GX_TRIANGLE_FAN);
			{
			
				for (auto * vertex : poly->vertices)
				{
				
					Vec3 position;
					
					for (int i = 0; i < 4; ++i)
						position += blend[i]->Mul4(vertex->position) * vertex->varying[kVarying_BlendWeights][i];
					
					gxColor4fv(&vertex->varying[kVarying_Color][0]);
					gxVertex3fv(&position[0]);
				
				}
			
			}
			gxEnd();
			
		}
		
		if (mouse.isDown(BUTTON_LEFT))
		{
		
			for (int i = 0; i < 4; ++i)
			{
			
				Vec3 p1 = blend[i]->Mul4(Vec3(0.0f, 0.0f, -1.0f));
				Vec3 p2 = blend[i]->Mul4(Vec3(1.0f, 0.0f, -1.0f));
			
				setColor(colorWhite);
				gxBegin(GX_LINES);
				gxVertex3fv(&p1[0]);
				gxVertex3fv(&p2[0]);
				gxEnd();
				
			}
			
		}
	
		framework.endDraw();
		
	}
	
}

static void testGraphicsDeviceBlendSkin_Indexed()
{

	std::vector<EditBone> bones;
	AnimModel::BoneSet boneSet;
	AnimModel::Anim anim;
	CreateSkeletonAndAnimation(bones, boneSet, anim);

	Geo::Mesh mesh;
	CreateSkin(bones, mesh);

	Camera3d camera;
	
	camera.position[2] = -3.0f;
	
	float time = 0.0f;
	
	for (;;)
	{
	
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		time += framework.timeStep;
		
		if (keyboard.wentDown(SDLK_s))
			time = 0.0f;
		
		camera.tick(framework.timeStep, true);
		
		framework.beginDraw(0, 0, 0, 0);
	
		projectPerspective3d(90.f, 0.01f, 100.0f);
		
		setDepthTest(true, DEPTH_LESS);

		Mat4x4 view;
		view.MakeLookat(
			Vec3(0.0f, 0.0f, 0.0f),
			Vec3(0.0f, 0.0f, +1.0f),
			Vec3(0.0f, +1.0f, 0.0f));

		AnimModel::BoneTransform transforms[4];
		anim.evaluate(time, transforms, -1);
		
		Mat4x4 blend1 = transforms[0].toMatrix() * boneSet.m_bones[0].poseMatrix;
		Mat4x4 blend2 = transforms[1].toMatrix() * boneSet.m_bones[1].poseMatrix;
		Mat4x4 blend3 = transforms[2].toMatrix() * boneSet.m_bones[2].poseMatrix;
		Mat4x4 blend4 = transforms[3].toMatrix() * boneSet.m_bones[3].poseMatrix;

		Mat4x4 * blend[4] =
		{
			&blend1,
			&blend2,
			&blend3,
			&blend4
		};
		
		camera.pushViewMatrix();
		
		for (int i = 0; i < 4; ++i)
		{
		
			gxPushMatrix();
			{
			
				gxMultMatrixf(blend[i]->m_v);
				gxScalef(0.4f, 0.4f, 0.4f);
				gxMultMatrixf(boneSet.m_bones[i].poseMatrix.CalcInv().m_v);
				
				setColor(colorRed);
				fillCube(Vec3(), Vec3(1.0f, 0.02f, 0.02f));
				
				setColor(colorGreen);
				fillCube(Vec3(), Vec3(0.02f, 1.0f, 0.02f));
				
				setColor(colorBlue);
				fillCube(Vec3(), Vec3(0.02f, 0.02f, 1.0f));
				
				gxBegin(GX_LINES);
				{

					setColor(colorRed);
					gxVertex3f(0, 0, 0);
					gxVertex3f(1, 0, 0);

					setColor(colorGreen);
					gxVertex3f(0, 0, 0);
					gxVertex3f(0, 1, 0);

					setColor(colorBlue);
					gxVertex3f(0, 0, 0);
					gxVertex3f(0, 0, 1);

				}
				gxEnd();
				
			}
			gxPopMatrix();
			
		}

		pushWireframe(true);
		
		for (auto * poly : mesh.polys)
		{
		
			gxBegin(GX_TRIANGLE_FAN);
			{
			
				for (auto * vertex : poly->vertices)
				{
				
					Vec3 position;
					
					for (int i = 0; i < 4; ++i)
					{
					
						const int blendIndex = (int)vertex->varying[kVarying_BlendIndices][i];
						const float blendWeight = vertex->varying[kVarying_BlendWeights][i];
						
						position += blend[blendIndex]->Mul4(vertex->position) * blendWeight;
						
					}
					
					/*
					gxColor4f(
						vertex->varying[kVarying_BlendWeights][0] + vertex->varying[kVarying_BlendWeights][3],
						vertex->varying[kVarying_BlendWeights][1] + vertex->varying[kVarying_BlendWeights][3],
						vertex->varying[kVarying_BlendWeights][2] + vertex->varying[kVarying_BlendWeights][3],
						1.f);
					*/
					
					gxColor3ub(127, 127, 127);
					gxVertex3fv(&position[0]);
				
				}
			
			}
			gxEnd();
		
		}
		
		popWireframe();
		
		camera.popViewMatrix();
		
		framework.endDraw();
		
	}

}

static void drawMesh(const Geo::Mesh& mesh)
{

	for (auto * poly : mesh.polys)
	{
	
		gxColor4f(
			(poly->plane.normal[0] + 1.0f) / 2.0f,
			(poly->plane.normal[1] + 1.0f) / 2.0f,
			(poly->plane.normal[2] + 1.0f) / 2.0f,
			1.f);
		
		gxBegin(GX_TRIANGLE_FAN);
		{
		
			for (auto * vertex : poly->vertices)
			{
				
				gxVertex3fv(&vertex->position[0]);
			
			}
		
		}
		gxEnd();
	
	}

}

static void testMeshCsg()
{

#if 1
	Geo::Mesh mesh;
	
	Geo::Builder b;
	
	b.begin(mesh);
	b.scale(0.5f);
	b.cube();
	b.end();
#else
	
	Geo::Mesh mesh;
	
	Geo::Builder b;
	
	Geo::Mesh mesh1;
	Geo::Mesh mesh2;
	Geo::Mesh mesh3;
	Geo::Mesh mesh4;
	
	const int resolution = 17;
	
	b
		.begin(mesh1)
			.pushScale(0.6f)
				//.cube()
				.cylinder(resolution*2+1)
			.pop()
		.end()
		.begin(mesh2)
			.pushScale(0.4f, 0.4f, 1.0f)
				.cylinder(resolution)
			.pop()
		.end()
		.begin(mesh3)
			.rotate(Calc::DegToRad(90.0f), 1.0f, 0.0f, 0.0f)
			.pushScale(0.4f, 0.4f, 1.0f)
				.cylinder(resolution)
			.pop()
		.end()
		.begin(mesh4)
			.pushRotate(Calc::DegToRad(90.0f), 0.0f, 1.0f, 0.0f)
			.pushScale(0.4f, 0.4f, 1.0f)
				.cylinder(resolution)
			.pop()
			.pop()
		.end();
	
	Geo::Csg3D::Subtract(mesh1, mesh2, mesh);
	Geo::Csg3D::SubtractInplace(mesh, mesh3);
	Geo::Csg3D::SubtractInplace(mesh, mesh4);
#endif

	for (;;)
	{
	
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		if (mouse.wentDown(BUTTON_LEFT))
		{
		
			Geo::Mesh mesh1;
			
			b
				.begin(mesh1)
					.pushTranslate(
						random<float>(-1.0f, +1.0f),
						random<float>(-1.0f, +1.0f),
						random<float>(-1.0f, +1.0f))
					.pushScale(0.3f)
						.cube()
					.pop()
					.pop()
				.end();
			
			Geo::Csg3D::SubtractInplace(mesh, mesh1);
			
		}
		
		framework.beginDraw(0, 0, 0, 0);
		{
			
			projectPerspective3d(90.0f, 0.01f, 100.0f);
			
			setDepthTest(true, DEPTH_LESS);
			
			gxTranslatef(0, 0, +1.4f);
			gxRotatef(framework.time * 30.0f, 1.0f, 1.0f, 1.0f);
			
			pushLineSmooth(true);
			pushWireframe(keyboard.isDown(SDLK_w));
			{
			
				setColor(colorWhite);
				drawMesh(mesh);
				
			}
			popWireframe();
			popLineSmooth();
		
		}
		framework.endDraw();
		
	}
	
}

static void testCubes1()
{

	GxMesh mesh;
	GxVertexBuffer vb;
	GxIndexBuffer ib;

	gxCaptureMeshBegin(mesh, vb, ib);

	for (size_t i = 0; i < 25; ++i)
	{
		for (size_t j = 0; j < 25; ++j)
		{
		
			float u = i / float(25 - 1);
			float v = j / float(25 - 1);

			float x = (u - 0.5f) * 2.0f;
			float y = (v - 0.5f) * 2.0f;

			float scale = sin(sqrt(x * x + y * y) * M_PI * 2.0f * 2.0f);
			scale *= 1.0f / 25.0f;

			int color[3];
			if (scale >= 0.0f)
			{
				color[0] = 255;
				color[1] = 0;
				color[2] = 0;
			}
			else
			{
				color[0] = 0;
				color[1] = 255;
				color[2] = 0;
			}

			gxPushMatrix();
			{
				gxTranslatef(x, y, 0.f);
				gxScalef(scale, scale, scale);
				
				gxColor3ub(color[0], color[1], color[2]);
				fillCube(Vec3(), Vec3(1, 1, 1));
			}
			gxPopMatrix();
			
		}
	}
	
	gxCaptureMeshEnd();

	Quat rotation;

	int clearColor[3] = { };
	
	PolledTimer timer2;
	timer2.Initialize(&g_TimerRT);
	timer2.SetInterval(1.0f);
	timer2.Start();
	
	while (!framework.quitRequested)
	{
	
		framework.process();
		
		framework.beginDraw(clearColor[0], clearColor[1], clearColor[2], 255);
		
		float time = framework.time;
		
		float zDomination = (sin(time / 2.0f) + 1.0f) / 2.0f;

		if (timer2.ReadTick())
		{
		
			//int cMax = 15 / 255.0f;
			int cMax = 15;
			clearColor[0] = cMax - clearColor[0];
			clearColor[1] = cMax - clearColor[1];
			clearColor[2] = cMax - clearColor[2];
			timer2.SetIntervalMS(int((1.0f - zDomination) * 500.0f) + 1);
			
		}
	
		Vec3 axis(
			sin(time / 1.0f) * (1.0f - zDomination),
			sin(time / 1.3f) * (1.0f - zDomination),
			sin(time / 1.7f));
		Quat rotator;
		float deltaTime = framework.timeStep;
		time += deltaTime;
		rotator.fromAxisAngle(axis, M_PI * 2.0f * deltaTime * 0.5f);
		rotation *= rotator;
		rotation.normalize();

		Mat4x4 matProj;
		Mat4x4 matView;
		Mat4x4 matWorld;

		int viewSx;
		int viewSy;
		framework.getCurrentViewportSize(viewSx, viewSy);
		
		matProj.MakePerspectiveLH(M_PI / 2.0f, viewSy / float(viewSx), 0.01f, 100.0f);
		matView.MakeLookat(Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f), Vec3(0.0f, 1.0f, 0.0f));
		{
			Mat4x4 matWorldPos;
			Mat4x4 matWorldRot;
			matWorldPos.MakeTranslation(Vec3(0.0f, 0.0f, 2.0f));
			matWorldRot = Mat4x4(true).RotateY(time / 1.0f).RotateZ(time / 1.1f);
			matWorldRot = rotation.toMatrix();
			matWorld = matWorldPos * matWorldRot;
		}

		gxSetMatrixf(GX_PROJECTION, matProj.m_v);
		gxSetMatrixf(GX_MODELVIEW, (matView * matWorld).m_v);
		
		mesh.draw();

		framework.endDraw();
		
	}
	
}

int main(int argc, char * argv[])
{

	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableDepthBuffer = true;
	framework.init(800, 600);
	
// todo : separate tests into separate files

	//testMeshCsg();
	
	//testGraphicsDeviceBlendMatrices();
	
	//testGraphicsDeviceBlendSkin_Indexed();
	
	testCubes1();
	
	framework.shutdown();
	
	return 0;

}