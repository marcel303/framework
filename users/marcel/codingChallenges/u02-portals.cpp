#include "framework.h"

static Mat4x4 worldToView;

static int surfaceSx = 0;
static int surfaceSy = 0;

static void draw2d_wall(const float x1, const float z1, const float x2, const float z2)
{
	const Vec3 p1 = worldToView.Mul4(Vec3(x1, 0.f, z1));
	const Vec3 p2 = worldToView.Mul4(Vec3(x2, 0.f, z2));
	
	drawLine(p1[0], p1[2], p2[0], p2[2]);
}

static void draw2d_circle(const float x, const float z, const float radius)
{
	const Vec3 p = worldToView.Mul4(Vec3(x, 0.f, z));
	
	drawCircle(p[0], p[2], radius, 20);
}

static bool clip_view_plane(Vec3 & p1, Vec3 & p2)
{
	static const float eps = 1e-6f;
	
	if (p1[2] < eps && p2[2] < eps)
		return false;
	else if (p1[2] >= eps && p2[2] >= eps)
		return true;
	
	const float dx = p2[0] - p1[0];
	const float dz = p2[2] - p1[2];
	
	if (p1[2] < eps)
	{
		const float t = (eps - p1[2]) / dz;
		
		p1[0] += dx * t;
		p1[2] += dz * t;
	}
	
	if (p2[2] < eps)
	{
		const float t = (eps - p2[2]) / dz;
		
		p2[0] += dx * t;
		p2[2] += dz * t;
	}
	
	return true;
}

static void draw2d_wall_clip(const float x1, const float z1, const float x2, const float z2)
{
	Vec3 p1 = worldToView.Mul4(Vec3(x1, 0.f, z1));
	Vec3 p2 = worldToView.Mul4(Vec3(x2, 0.f, z2));
	
	if (clip_view_plane(p1, p2))
	{
		drawLine(p1[0], p1[2], p2[0], p2[2]);
	}
}

static void draw3d_wall(const float x1, const float z1, const float x2, const float z2)
{
	Vec3 p1 = worldToView.Mul4(Vec3(x1, 0.f, z1));
	Vec3 p2 = worldToView.Mul4(Vec3(x2, 0.f, z2));
	
	if (clip_view_plane(p1, p2))
	{
		const float aspect = 1.f / 2.f;
		
		const float x1 = (p1[0] * aspect + .5f) * surfaceSx;
		const float x2 = (p2[0] * aspect + .5f) * surfaceSx;
		
		const float wall_y1 = +1.f;
		const float wall_y2 = -1.f;
		
		const float y1a = (- wall_y1 / p1[2] / 2.f + .5f) * surfaceSy;
		const float y1b = (- wall_y2 / p1[2] / 2.f + .5f) * surfaceSy;
		const float y2a = (- wall_y1 / p2[2] / 2.f + .5f) * surfaceSy;
		const float y2b = (- wall_y2 / p2[2] / 2.f + .5f) * surfaceSy;
		
		gxBegin(GX_QUADS);
		gxVertex2f(x1, y1a);
		gxVertex2f(x2, y2a);
		gxVertex2f(x2, y2b);
		gxVertex2f(x1, y1b);
		gxEnd();
	}
}

struct Edge
{
	Vec2 p1;
	Vec2 p2;
};

struct Map
{
	std::vector<Edge> edges;
};

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.allowHighDpi = false;
	
	if (!framework.init(640, 480))
		return -1;

	// create a map

	Map map;

	map.edges =
	{
		{
			{ -1.f, +1.f }, { +1.f, +1.f }
		}
	};
	
	float angle = 0.f;
	float x = 0.f;
	float z = 0.f;
	
	Surface draw2d_world(200, 200, false);
	Surface draw2d_view(200, 200, false);
	Surface draw2d_clip(200, 200, false);
	Surface draw3d_persp(200, 200, false);
	
	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;

		const float dt = framework.timeStep;
		
		float moveSpeed = 0.f;
		float angleSpeed = 0.f;
		
		if (keyboard.isDown(SDLK_UP))
			moveSpeed += 1.f;
		if (keyboard.isDown(SDLK_DOWN))
			moveSpeed -= 1.f;
		if (keyboard.isDown(SDLK_LEFT))
			angleSpeed -= 1.f;
		if (keyboard.isDown(SDLK_RIGHT))
			angleSpeed += 1.f;
		
		angle += angleSpeed * dt;
		
		const float forwardX = -sinf(angle);
		const float forwardZ = +cosf(angle);
		
		moveSpeed *= .1f;
		
		x += forwardX * moveSpeed * dt;
		z += forwardZ * moveSpeed * dt;
		
		framework.beginDraw(0, 0, 0, 0);
		{
			// world-space view
			
			pushSurface(&draw2d_world);
			{
				worldToView = Mat4x4(true);
				
				draw2d_world.clear(255, 0, 0, 0);
				
				gxTranslatef(draw2d_world.getWidth()/2.f, draw2d_world.getHeight()/2.f, 0);
				gxScalef(50, 50, 50);
				
				setColor(colorWhite);
				
				setLumi(100);
				draw2d_wall(-1, -1, +1, -1);
				draw2d_wall(+1, -1, +1, +1);
				draw2d_wall(+1, +1, -1, +1);
				draw2d_wall(-1, -1, -1, +1);
				
				setLumi(200);
				draw2d_circle(x, z, .02f);
				setLumi(100);
				draw2d_wall(x, z, x + forwardX * .1f, z + forwardZ * .1f);
				
				for (auto & edge : map.edges)
				{
					setLumi(255);
					draw2d_wall(
						edge.p1[0], edge.p1[1],
						edge.p2[0], edge.p2[1]);
				}
			}
			popSurface();
			
			// view-space view
			
			pushSurface(&draw2d_view);
			{
				worldToView = Mat4x4(true).RotateY(-angle).Translate(-x, 0.f, -z);
				
				draw2d_view.clear(255, 0, 0, 0);
				
				gxTranslatef(draw2d_view.getWidth()/2.f, draw2d_view.getHeight()/2.f, 0);
				gxScalef(50, 50, 50);
				
				setColor(colorWhite);
				
				setLumi(100);
				draw2d_wall(-1, -1, +1, -1);
				draw2d_wall(+1, -1, +1, +1);
				draw2d_wall(+1, +1, -1, +1);
				draw2d_wall(-1, -1, -1, +1);
				
				setLumi(200);
				draw2d_circle(x, z, .02f);
				setLumi(100);
				draw2d_wall(x, z, x + forwardX * .1f, z + forwardZ * .1f);
				
				for (auto & edge : map.edges)
				{
					setLumi(255);
					draw2d_wall(
						edge.p1[0], edge.p1[1],
						edge.p2[0], edge.p2[1]);
				}
			}
			popSurface();
			
			// clip-space view
			
			pushSurface(&draw2d_clip);
			{
				worldToView = Mat4x4(true).RotateY(-angle).Translate(-x, 0.f, -z);
				
				draw2d_clip.clear(255, 0, 0, 0);
				
				gxTranslatef(draw2d_clip.getWidth()/2.f, draw2d_clip.getHeight()/2.f, 0);
				gxScalef(50, 50, 50);
				
				setColor(colorWhite);
				
				for (auto & edge : map.edges)
				{
					setLumi(255);
					draw2d_wall_clip(
						edge.p1[0], edge.p1[1],
						edge.p2[0], edge.p2[1]);
				}
			}
			popSurface();
			
			// perspective view
			
			pushSurface(&draw3d_persp);
			{
				worldToView = Mat4x4(true).RotateY(-angle).Translate(-x, 0.f, -z);
				
				surfaceSx = draw3d_persp.getWidth();
				surfaceSy = draw3d_persp.getHeight();
				
				draw3d_persp.clear(255, 0, 0, 0);
				
				setColor(colorWhite);
				
				for (auto & edge : map.edges)
				{
					setLumi(255);
					draw3d_wall(
						edge.p1[0], edge.p1[1],
						edge.p2[0], edge.p2[1]);
				}
			}
			popSurface();
			
			pushBlend(BLEND_OPAQUE);
			{
				setColor(colorWhite);
				
				int x = 0;
				int y = 0;
				
				y += 10;
				
				x += 10;
				gxSetTexture(draw2d_world.getTexture());
				drawRect(x, y, x + draw2d_world.getWidth(), y + draw2d_world.getHeight());
				gxSetTexture(0);
				x += draw2d_world.getWidth();
				
				x += 10;
				gxSetTexture(draw2d_view.getTexture());
				drawRect(x, y, x + draw2d_view.getWidth(), y + draw2d_view.getHeight());
				gxSetTexture(0);
				x += draw2d_view.getWidth();
				
				x = 0;
				y += draw2d_view.getHeight();
				y += 10;
				
				x += 10;
				gxSetTexture(draw2d_clip.getTexture());
				drawRect(x, y, x + draw2d_clip.getWidth(), y + draw2d_clip.getHeight());
				gxSetTexture(0);
				x += draw2d_clip.getWidth();
				
				x += 10;
				gxSetTexture(draw3d_persp.getTexture());
				drawRect(x, y, x + draw3d_persp.getWidth(), y + draw3d_persp.getHeight());
				gxSetTexture(0);
				x += draw3d_persp.getWidth();
			}
			popBlend();
		}
		framework.endDraw();
	}

	framework.shutdown();

	return 0;
}
