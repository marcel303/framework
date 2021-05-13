#include "framework.h"
#include "gx_texture.h"
#include "image.h"

#include "Noise.h"

// idea : a carpet flying around. using head tilt for direction
//        add little details like particles, leafs, birds ..
//        add generative terrain

// test : immerse the user into the scene using bodily head movement

// todo : add forward shader with distance fog + simple directional lighting

static const float kHeadHeight = 1.f;

static const float kMinimumTerrainDistance = 1.f;

static float sampleTerrain(const float x, const float z)
{
	static const float kNoiseFrequencyRcp = 1.f / 100.f;
	
	return octave_noise_2d(4, .5f, kNoiseFrequencyRcp, x, z) * 10.f - 4.f;
}

struct Carpet
{
	Vec3 position;
	
	void tick(const float dt)
	{
		// determine movement speed
		
		float speed;
		if (framework.isStereoVr())
			speed = vrPointer[0].getPressure(VrButton_Trigger) * 4.f;
		else
			speed = 1.f;
		
		// move based on head orientation
		
		const Mat4x4 headTransform = framework.getHeadTransform();
		
		position += headTransform.GetAxis(2) * speed * dt;
		
		// ensure the carpet stays above the terrain
		
		const float minimumHeight = sampleTerrain(position[0], position[2]) + kMinimumTerrainDistance;
		
		if (position[1] < minimumHeight)
			position[1] = minimumHeight;
	}
	
	void draw() const
	{
		{
			// draw carpet
			
			Mat4x4 worldToView;
			gxGetMatrixf(GX_MODELVIEW, worldToView.m_v);
			
			const Mat4x4 viewToWorld = worldToView.CalcInv();
			const Vec3 viewPosition = viewToWorld.GetTranslation();
			
			const float kCarpetSize = 1.4f;
			
		#if 0
			fillCube(viewPosition + Vec3(0, -headHeight, 0), Vec3(kCarpetSize / 2.f));
		#else
			const int gridExtents = 10;
			
			const int numGridPoints = (gridExtents * 2 + 1) * (gridExtents * 2 + 1);
			
			Vec3 * gridPoints = (Vec3*)alloca(numGridPoints * sizeof(Vec3));
			
			const float gridScale = (kCarpetSize / 2.f) / gridExtents;
			const float noiseFrequencyRcp = .05f / gridScale;
			
			Vec3 * gridPoints_itr = gridPoints;
			
			for (int tx = -gridExtents; tx <= +gridExtents; ++tx)
			{
				for (int tz = -gridExtents; tz <= +gridExtents; ++tz)
				{
					const float x = tx * gridScale + position[0];
					const float z = tz * gridScale + position[2];
					
					// note : we use a noise function to give the impression of wind making waves on the carpet
					
					const float noise = octave_noise_3d(2, .5f, noiseFrequencyRcp, x, z, framework.time * .3f);
					
					const float y = position[1] + noise * .2f;
					
					gridPoints_itr->Set(x, y, z);
					
					gridPoints_itr++;
				}
			}
			
			gxBegin(GX_QUADS);
			{
				const Vec3 * gridPoints_itr = gridPoints;
				
				for (int tx = -gridExtents; tx + 1 <= +gridExtents; ++tx)
				{
					const Vec3 * gridPoints_itr1 = gridPoints_itr;
					const Vec3 * gridPoints_itr2 = gridPoints_itr + (gridExtents * 2 + 1);
					
					for (int tz = -gridExtents; tz + 1 <= +gridExtents; ++tz)
					{
						const Vec3 dx = gridPoints_itr1[1] - gridPoints_itr1[0];
						const Vec3 dz = gridPoints_itr2[0] - gridPoints_itr1[0];
						const Vec3 n = (dx % dz).CalcNormalized();
						gxColor3fv(&n[0]);
						
						gxVertex3fv(&gridPoints_itr1[0][0]);
						gxVertex3fv(&gridPoints_itr1[1][0]);
						gxVertex3fv(&gridPoints_itr2[1][0]);
						gxVertex3fv(&gridPoints_itr2[0][0]);
						
						gridPoints_itr1++;
						gridPoints_itr2++;
					}
					
					gridPoints_itr += gridExtents * 2 + 1;
				}
			}
			gxEnd();
		#endif
		}
	}
};

struct World
{
	Carpet carpet;
	
	GxTexture terrainTexture;
	
	void init()
	{
		ImageData img(512, 512);
		
		for (int y = 0; y < img.sy; ++y)
		{
			auto * line = img.getLine(y);
			
			for (int x = 0; x < img.sx; ++x)
			{
				const int value = 256 - 64 + (rand() & 63);
				//const int value = rand() & 255;
				
				line[x].r = value;
				line[x].g = value;
				line[x].b = value;
				line[x].a = 0xff;
			}
		}
		
		GxTextureProperties p;
		p.dimensions.sx = img.sx;
		p.dimensions.sy = img.sy;
		p.format = GX_RGBA8_UNORM;
		p.mipmapped = true;
		p.sampling.filter = true;
		p.sampling.clamp = false;
		
		terrainTexture.allocate(p);
		terrainTexture.upload(img.imageData, 0, 0);
		
		terrainTexture.generateMipmaps();
	}
	
	void shut()
	{
		terrainTexture.free();
	}
	
	void tick(const float dt)
	{
		carpet.tick(dt);
	}
	
	void draw() const
	{
		carpet.draw();
		
		{
			// draw terrain
			
			Mat4x4 worldToView;
			gxGetMatrixf(GX_MODELVIEW, worldToView.m_v);
			
			const Mat4x4 viewToWorld = worldToView.CalcInv();
			const Vec3 viewPosition = viewToWorld.GetTranslation();
			
			const int gridExtents = 40;
			
			const int numGridPoints = (gridExtents * 2 + 1) * (gridExtents * 2 + 1);
			
			Vec3 * gridPoints = (Vec3*)alloca(numGridPoints * sizeof(Vec3));
			
			const float gridScale = 2.f;
			
			Vec3 * gridPoints_itr = gridPoints;
			
			for (int tx = -gridExtents; tx <= +gridExtents; ++tx)
			{
				for (int tz = -gridExtents; tz <= +gridExtents; ++tz)
				{
					const float x = tx * gridScale + viewPosition[0];
					const float z = tz * gridScale + viewPosition[2];
					
					const float y = sampleTerrain(x, z);
					
					gridPoints_itr->Set(x, y, z);
					
					gridPoints_itr++;
				}
			}
			
			setColor(colorWhite);
			gxSetTexture(terrainTexture.id);
			gxSetTextureSampler(GX_SAMPLE_MIPMAP, false);
			
			gxBegin(GX_QUADS);
			{
				const Vec3 * gridPoints_itr = gridPoints;
				
				const Color color1 = colorBlue;
				const Color color2 = colorRed;
				
				for (int tx = -gridExtents; tx + 1 <= +gridExtents; ++tx)
				{
					const Vec3 * gridPoints_itr1 = gridPoints_itr;
					const Vec3 * gridPoints_itr2 = gridPoints_itr + (gridExtents * 2 + 1);
					
					for (int tz = -gridExtents; tz + 1 <= +gridExtents; ++tz)
					{
						const Vec3 dx = gridPoints_itr1[1] - gridPoints_itr1[0];
						const Vec3 dz = gridPoints_itr2[0] - gridPoints_itr1[0];
						const Vec3 n = (dx % dz).CalcNormalized();
					#if 0
						gxColor3fv(&n[0]);
					#else
						const Color color = color1.interp(color2, n[1]);
						gxColor3fv(&color.r);
					#endif
						
						gxTexCoord2f(gridPoints_itr1[0][0], gridPoints_itr1[0][2]); gxVertex3fv(&gridPoints_itr1[0][0]);
						gxTexCoord2f(gridPoints_itr1[1][0], gridPoints_itr1[1][2]); gxVertex3fv(&gridPoints_itr1[1][0]);
						gxTexCoord2f(gridPoints_itr2[1][0], gridPoints_itr2[1][2]); gxVertex3fv(&gridPoints_itr2[1][0]);
						gxTexCoord2f(gridPoints_itr2[0][0], gridPoints_itr2[0][2]); gxVertex3fv(&gridPoints_itr2[0][0]);
						
						gridPoints_itr1++;
						gridPoints_itr2++;
					}
					
					gridPoints_itr += gridExtents * 2 + 1;
				}
			}
			gxEnd();
			
			gxSetTextureSampler(GX_SAMPLE_NEAREST, true);
			gxSetTexture(0);
		}
	}
};

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.vrMode = true;

	framework.enableDepthBuffer = true;

	if (!framework.init(800, 600))
		return -1;
	
	World world;
	
	world.init();
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		world.tick(framework.timeStep);
		
		for (int i = 0; i < framework.getEyeCount(); ++i)
		{
			framework.beginEye(i, colorBlack);
			{
				Mat4x4 headTransform = framework.getHeadTransform();
				headTransform.SetTranslation(world.carpet.position + Vec3(0, kHeadHeight, 0));
				gxLoadMatrixf(headTransform.CalcInv().m_v);
				
				pushDepthTest(true, DEPTH_LESS);
				pushBlend(BLEND_OPAQUE);
				{
					world.draw();
				}
				popBlend();
				popDepthTest();
			}
			framework.endEye();
		}
		
		framework.present();
	}
	
	world.shut();
	
	framework.shutdown();
	
	return 0;
}
