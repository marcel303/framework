#include "framework.h"
#include "lattice.h"

// fibonacci sphere

static void generateFibonacciSpherePoints(const int numPoints, Vec3 * points)
{
    float rnd = 1.f;
	
    if (false)
        rnd = random<float>(0.f, numPoints);

    const float offset = 2.f / numPoints;
    const float increment = M_PI * (3.f - sqrtf(5.f));

	for (int i = 0; i < numPoints; ++i)
    {
        const float y = ((i * offset) - 1) + (offset / 2.f);
        const float r = sqrtf(1.f - y * y);

        const float phi = fmodf((i + rnd), numPoints) * increment;

        const float x = cosf(phi) * r;
        const float z = sinf(phi) * r;
		
        points[i] = Vec3(x, y, z);
	}
}

static void createFibonnacciSphere(Lattice & lattice)
{
	const int numVertices = kNumVertices;
	
	Vec3 points[numVertices];
	generateFibonacciSpherePoints(numVertices, points);
	
	for (int i = 0; i < numVertices; ++i)
	{
		lattice.vertices[i].p.set(
			points[i][0],
			points[i][1],
			points[i][2]);
		
		lattice.vertices[i].f.setZero();
		lattice.vertices[i].v.setZero();
	}
	
	for (int i = 0; i < numVertices; ++i)
	{
		const auto & p1 = lattice.vertices[i].p;
		
		struct Result
		{
			int index;
			float distance;
			
			bool operator<(const Result & other) const
			{
				return distance < other.distance;
			}
		};
		
		const int kMaxResults = 6;
		Result results[kMaxResults];
		int numResults = 0;
		
		for (int j = 0; j < numVertices; ++j)
		{
			if (j == i)
				continue;
			
			const auto & p2 = lattice.vertices[j].p;
			
			const float dx = p2.x - p1.x;
			const float dy = p2.y - p1.y;
			const float dz = p2.z - p1.z;
			
			const float dSquared = dx * dx + dy * dy + dz * dz;
			
			if (numResults < kMaxResults)
			{
				results[numResults].distance = dSquared;
				results[numResults].index = j;
				
				numResults++;
				
				if (numResults == kMaxResults)
					std::sort(results, results + kMaxResults);
			}
			else
			{
				if (dSquared < results[kMaxResults - 1].distance)
				{
					results[kMaxResults - 1].distance = dSquared;
					results[kMaxResults - 1].index = j;
					
					std::sort(results, results + kMaxResults);
				}
			}
		}
		
		for (int r = 0; r < numResults; ++r)
		{
			Lattice::Edge edge;
			edge.vertex1 = i;
			edge.vertex2 = results[r].index;
			edge.initialDistance = sqrtf(results[r].distance);
			edge.weight = 1.f;
			
			lattice.edges.push_back(edge);
		}
	}
}

// rasterization raytracing method

static void testRaster()
{
	framework.enableRealTimeEditing = true;
	
	if (!framework.init(256, 256))
		return;
	
	float init_px = 0.f;
	float init_py = 0.f;
	float a = 0.f;
	
	const int kSize = 16;
	
	Surface surface(kSize, kSize, false, false, SURFACE_R32F);
	
	for (;;)
	{
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			break;
		
		if (mouse.wentDown(BUTTON_LEFT))
		{
			init_px = random(-1.f, 1.f);
			init_py = random(-1.f, 1.f);
			a = random<float>(0.f, 2.f * M_PI);
		}
		
		framework.beginDraw(0, 0, 0, 0);
		{
			pushSurface(&surface);
			surface.clear();
			
			pushBlend(BLEND_ADD);
			
			Shader shader("test");
			setShader(shader);
			{
				const float step = mouse.x / 256.f * 2.f * M_PI * 2.f;

				glHint(GL_LINE_SMOOTH_HINT, GL_FASTEST);
				
				gxBegin(GL_LINES);
				{
					for (int x = 0; x < kSize; ++x)
					{
						for (int y = 0; y < kSize; ++y)
						{
							for (float a = 0.f; a <= 2.f * M_PI; a += 2.f * M_PI / 8.f)
							{
							float radians = 0.f;
							
							//float px = (mouse.x / 256.f - .5f) * 2.f;
							//float py = (mouse.y / 256.f - .5f) * 2.f;
							
							float px = (x / float(kSize) - .5f) * 2.f;
							float py = (y / float(kSize) - .5f) * 2.f;
				
							float dx = cosf(a);
							float dy = sinf(a);
							
							for (int i = 0; i < 100; ++i)
							{
								const float t[4] =
								{
									(-1.f - px) / dx,
									(+1.f - px) / dx,
									(-1.f - py) / dy,
									(+1.f - py) / dy
								};
								
								int idx = -1;
								
								for (int i = 0; i < 4; ++i)
									if (t[i] >= 1e-3f && (idx == -1 || t[i] < t[idx]))
										idx = i;
								
								if (idx == -1)
									break;
								
							#define emit gxVertex2f((px + 1.f) / 2.f * kSize, (py + 1.f) / 2.f * kSize)
								gxTexCoord2f(radians, 0.f); emit;
								radians += step * t[idx];
								px += dx * t[idx];
								py += dy * t[idx];
								gxTexCoord2f(radians, 0.f); emit;
								
								if (idx == 0 || idx == 1)
									dx = -dx;
								else
									dy = -dy;
								
							#if 0
								gxTexCoord2f(radians, 0.f); gxVertex2f(0, y);
								radians += step;
								gxTexCoord2f(radians, 0.f); gxVertex2f(256, y);
								
								gxTexCoord2f(radians, 0.f); gxVertex2f(256, y);
								radians += step;
								gxTexCoord2f(radians, 0.f); gxVertex2f(0, y);
							#endif
								}
							}
						}
					}
				}
				gxEnd();
			}
			clearShader();
			
			popBlend();
			popSurface();
			
			pushBlend(BLEND_OPAQUE);
			gxSetTexture(surface.getTexture());
			setColor(colorWhite);
			setLumif(1.f / 20.f);
			drawRect(0, 0, 256, 256);
			popBlend();
		}
		framework.endDraw();
	}
	
	framework.shutdown();
}
