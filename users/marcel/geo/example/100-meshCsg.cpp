// libgeo includes
#include "Geo.h"

// framework includes
#include "framework.h"

// libgg includes
#include "Calc.h"

// todo : add CSG support to geom builder. 'begin(mesh, mode)' where mode is '+', '-', '&', 'x'

// todo : add CSG implementation with keeps the BSP tree of the mesh being built alive

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

int main(int argc, char * argv[])
{

	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableDepthBuffer = true;
	framework.init(800, 600);
	
#if 0
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
	
	framework.shutdown();
	
	return 0;
	
}
