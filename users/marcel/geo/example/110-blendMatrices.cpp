#include "Geo.h"

#include "framework.h"

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

int main(int argc, char * argv[])
{

	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableDepthBuffer = true;
	framework.init(800, 600);
	
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
	
	framework.shutdown();
	
	return 0;
	
}
