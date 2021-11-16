// libgeo includes
#include "Geo.h"

// framework includes
#include "framework.h"
#include "gx_mesh.h"

// libgg includes
#include "PolledTimer.h"
#include "Quat.h"
#include "Timer.h"

int main(int argc, char * argv[])
{

	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableDepthBuffer = true;
	framework.init(800, 600);
	
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
		
		matProj.MakePerspectiveLH(float(M_PI) / 2.0f, viewSy / float(viewSx), 0.01f, 100.0f);
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
	
	framework.shutdown();
	
	return 0;

}
