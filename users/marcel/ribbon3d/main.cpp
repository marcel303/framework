#include "Benchmark.h"
#include "framework.h"
#include "gx_mesh.h"

struct RibbonPoint3d
{
	Vec3 position;
	Vec3 tangent;
	float size;
};

static void computeQuadVertices(const RibbonPoint3d & a, const RibbonPoint3d & b, const float size, Vec3 * vertices)
{
	const Vec3 normal = (b.position - a.position).CalcNormalized();
	const Vec3 bitangent = normal % a.tangent;
	const Vec3 tangent = normal % bitangent;
	//const Vec3 & tangent = a.tangent;

	vertices[0] = a.position + (- tangent - bitangent) * size;
	vertices[1] = a.position + (  tangent - bitangent) * size;
	vertices[2] = a.position + (  tangent + bitangent) * size;
	vertices[3] = a.position + (- tangent + bitangent) * size;
}

void drawRibbon3d(const RibbonPoint3d * points, const int numPoints)
{
	gxBegin(GX_QUADS);
	{
		for (int i = 0; i < numPoints - 2; ++i)
		{
			auto & a = points[i];
			auto & b = points[i + 1];
			auto & c = points[i + 2];
			
			Vec3 vertices_ab[4];
			Vec3 vertices_bc[4];
			computeQuadVertices(a, b, a.size, vertices_ab);
			computeQuadVertices(b, c, b.size, vertices_bc);
			
			for (int f = 0; f < 4; ++f)
			{
				const int p1 = f;
				const int p2 = f == 3 ? 0 : f + 1;
				
				const Vec3 v1 = vertices_bc[p1] - vertices_ab[p1];
				const Vec3 v2 = vertices_bc[p2] - vertices_bc[p1];
				const Vec3 n = (v1 % v2).CalcNormalized();
				
				gxColor4f(n[0], n[1], n[2], 1.f);
				
				gxVertex3fv(&vertices_ab[p1][0]);
				gxVertex3fv(&vertices_bc[p1][0]);
				gxVertex3fv(&vertices_bc[p2][0]);
				gxVertex3fv(&vertices_ab[p2][0]);
			}
		}
	}
	gxEnd();
}

static std::vector<RibbonPoint3d> points;

static void addRibbonPoint_lookat(const Vec3 & position, const Vec3 & target, const float size)
{
	const Vec3 tangent = (target - position).CalcNormalized();
	
	RibbonPoint3d point;
	point.position = position;
	point.tangent = tangent;
	point.size = size;
	
	points.push_back(point);
}

int main(int argc, char * argv[])
{
	framework.enableDepthBuffer = true;
	framework.msaaLevel = 4;
	
	if (!framework.init(800, 600))
		return -1;

	Camera3d camera;
	
	GxVertexBuffer vb;
	GxIndexBuffer ib;
	GxMesh mesh;
	bool hasCachedMesh = false;
	
	double t = 0.0;
	
	const int numDesiredPoints = 10000;
	
	points.reserve(numDesiredPoints);
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		camera.tick(framework.timeStep, true);
		
		if (mouse.isDown(BUTTON_LEFT))
		{
			hasCachedMesh = false;
			points.clear();
			points.reserve(numDesiredPoints);
		}
		
		if (points.size() < numDesiredPoints)
		{
			Benchmark bm("generate");
			
			const int maxPointsToAddPerFrame =
				keyboard.isDown(SDLK_SPACE)
				? numDesiredPoints
				: 100;
			
			for (int i = 0; i < maxPointsToAddPerFrame && points.size() < numDesiredPoints; ++i)
			{
				t += 1.0 / 100.0;
				
				const float x = cos(t / 1.23) * cos(t / 3.45);
				const float y = cos(t / 3.34) * 1.0;
				const float z = cos(t / 4.45) * 1.0;
				
				const float s = (cos(t / 1.23) + 1.0) / 2.0 * 0.2 + 0.01;
				
				addRibbonPoint_lookat(Vec3(x, y, z), Vec3(), s);
			}
			
			if (points.size() == numDesiredPoints && hasCachedMesh == false)
			{
				hasCachedMesh = true;
				
				gxCaptureMeshBegin(mesh, vb, ib);
				drawRibbon3d(points.data(), points.size());
				gxCaptureMeshEnd();
			}
		}
		
		framework.beginDraw(0, 0, 0, 0);
		{
			projectPerspective3d(60.f, .01f, 10.f);
			camera.pushViewMatrix();
			{
				pushDepthTest(true, DEPTH_LESS);
				{
					if (hasCachedMesh)
						mesh.draw();
					else
					{
						setColor(colorWhite);
						drawRibbon3d(points.data(), points.size());
					}
				}
				popDepthTest();
			}
			camera.popViewMatrix();
		}
		framework.endDraw();
	}
	
	framework.shutdown();

	return 0;
}

