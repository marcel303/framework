#include "framework.h"
#include "gx_texture.h"
#include "image.h"

#include "audioSourcePcm.h"
#include "spatialAudioSystem-binaural.h"

#include "audiooutput-hd/AudioOutputHD_Native.h"

// idea : a floor which can be interacted with using the vr pointer
//        tiles can be made to disappear, revealing the user is high up in the air

// test : perception of height as the user is responsible themselves for creating vertigo

#define kGridSx 7
#define kGridSz 7

#define kTileSize .7f
#define kTileExtents (kTileSize / 2.f)

#define kGravityY -9.8f
#define kTileDrag .99f

enum RenderPass
{
	kRenderPass_Opaque,
	kRenderPass_Translucent
};

static Shader shader;

static SpatialAudioSystemInterface * spatialAudioSystem = nullptr;

static PcmData pcmDataNoise;

static void fillCube_textured(Vec3Arg position, Vec3Arg size)
{
	const float vertices[8][3] =
	{
		{ position[0]-size[0], position[1]-size[1], position[2]-size[2] },
		{ position[0]+size[0], position[1]-size[1], position[2]-size[2] },
		{ position[0]+size[0], position[1]+size[1], position[2]-size[2] },
		{ position[0]-size[0], position[1]+size[1], position[2]-size[2] },
		{ position[0]-size[0], position[1]-size[1], position[2]+size[2] },
		{ position[0]+size[0], position[1]-size[1], position[2]+size[2] },
		{ position[0]+size[0], position[1]+size[1], position[2]+size[2] },
		{ position[0]-size[0], position[1]+size[1], position[2]+size[2] }
	};

	const float texcoords[4][2] =
	{
		{ 0.f, 0.f },
		{ 1.f, 0.f },
		{ 1.f, 1.f },
		{ 0.f, 1.f }
	};

	// note : the vertex indices for these faces have been determined one by one to
	//        cull correctly when the culling mode is set to CULL_BACK + CULL_CCW

	const int faces[6][4] =
	{
		{ 0, 1, 2, 3 },
		{ 7, 6, 5, 4 },
		{ 3, 7, 4, 0 },
		{ 1, 5, 6, 2 },
		{ 4, 5, 1, 0 },
		{ 3, 2, 6, 7 }
	};

	const float normals[6][3] =
	{
		{  0,  0, -1 },
		{  0,  0, +1 },
		{ -1,  0,  0 },
		{ +1,  0,  0 },
		{  0, -1,  0 },
		{  0, +1,  0 }
	};

	for (int face_idx = 0; face_idx < 6; ++face_idx)
	{
		const float * __restrict normal = normals[face_idx];

		gxNormal3fv(normal);

		const int * __restrict face = faces[face_idx];

		for (int vertex_idx = 0; vertex_idx < 4; ++vertex_idx)
		{
			const float * __restrict vertex = vertices[face[vertex_idx]];
			const float * __restrict texcoord = texcoords[vertex_idx];

			gxTexCoord2f(texcoord[0], texcoord[1]);
			gxVertex3fv(vertex);
		}
	}
}

struct Tile
{
	enum State
	{
		kState_Idle,
		kState_Interacting,
		kState_FallUp,
		kState_FallDown
	};

	float px = 0.f;
	float py = 0.f;
	float pz = 0.f;

	float vy = 0.f;

	State state = kState_Idle;

	float audioTimer = 0.f;
	AudioSourcePcm audioSource;
	void * spatialAudioSource = nullptr;
	
	void init()
	{
		audioSource.setPcmData(&pcmDataNoise);
		
		audioSource.play();
	}
	
	void tick(const float dt)
	{
		switch (state)
		{
		case kState_Idle:
			break;

		case kState_Interacting:
			break;

		case kState_FallUp:
			vy -= kGravityY * dt;
			break;

		case kState_FallDown:
			vy += kGravityY * dt;
			break;
		}

		vy *= powf(1.f - kTileDrag, dt);
		py += vy * dt;
		
		// tick audio
		
		if (audioTimer > 0.f)
		{
			spatialAudioSystem->setSourceTransform(spatialAudioSource, Mat4x4(true).Translate(px, py, pz));
			
			audioTimer = fmaxf(0.f, audioTimer - dt);
			
			if (audioTimer == 0.f)
			{
				endAudio();
			}
		}
	}
	
	void beginAudio()
	{
		audioTimer = 2.f;
		
		if (spatialAudioSource == nullptr)
		{
			spatialAudioSource = spatialAudioSystem->addSource(Mat4x4(true).Translate(px, py, pz), &audioSource, 1.f, 6.f);
		}
	}
	
	void endAudio()
	{
		if (spatialAudioSource != nullptr)
		{
			spatialAudioSystem->removeSource(spatialAudioSource);
			
			audioTimer = 0.f;
		}
		
		Assert(audioTimer == 0.f);
	}
};

struct World
{
	Tile tiles[kGridSx][kGridSz];

	bool hasTeleportTarget = false;
	float teleportTargetX = 0.f;
	float teleportTargetZ = 0.f;

	bool hasTile = false;
	int tileX = 0;
	int tileZ = 0;

	GxTexture tileTexture;

	void init()
	{
		for (int x = 0; x < kGridSx; ++x)
		{
			for (int z = 0; z < kGridSz; ++z)
			{
				tiles[x][z].init();
				
				tiles[x][z].px = (x - (kGridSx - 1) / 2.f) * kTileSize;
				tiles[x][z].pz = (z - (kGridSz - 1) / 2.f) * kTileSize;
			}
		}

		ImageData img(512, 512);

		for (int y = 0; y < img.sy; ++y)
		{
			auto * line = img.getLine(y);

			for (int x = 0; x < img.sx; ++x)
			{
				const int value = 256 - 64 + (rand() & 63);

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

		tileTexture.allocate(p);
		tileTexture.upload(img.imageData, 0, 0);

		tileTexture.generateMipmaps();
	}
	
	void shut()
	{
		for (int x = 0; x < kGridSx; ++x)
		{
			for (int z = 0; z < kGridSz; ++z)
			{
				tiles[x][z].endAudio();
			}
		}
	}
	
	void tick(const float dt)
	{
		auto & pointer = vrPointer[0];

		// raycast : determine the new teleportation target and the tile to interact with

		hasTeleportTarget = false;

		hasTile = false;

		Mat4x4 transform;
		bool hasTransform;
		
		if (framework.isStereoVr())
		{
			hasTransform = pointer.hasTransform;
			transform = pointer.getTransform(framework.vrOrigin);
		}
		else
		{
			hasTransform = true;
			transform = framework.getHeadTransform();
		}
		
		if (hasTransform)
		{
			const Vec3 rayOrigin = transform.GetTranslation();
			const Vec3 rayDirection = transform.GetAxis(2);

			const float t = -rayOrigin[1] / rayDirection[1];

			if (t > 0.f)
			{
				const Vec3 targetPosition = rayOrigin + rayDirection * t;

				const int x = (int)roundf(targetPosition[0] / kTileSize + (kGridSx - 1) / 2.f);
				const int z = (int)roundf(targetPosition[2] / kTileSize + (kGridSz - 1) / 2.f);

				if (x >= 0 && x < kGridSx &&
					z >= 0 && z < kGridSz)
				{
					hasTile = true;
					tileX = x;
					tileZ = z;

					if (tiles[x][z].py == 0.f)
					{
						hasTeleportTarget = true;
						teleportTargetX = targetPosition[0];
						teleportTargetZ = targetPosition[2];
					}
				}
			}
		}

		// tick teleportation

		if (hasTeleportTarget && (pointer.wentDown(VrButton_Trigger) || mouse.wentDown(BUTTON_LEFT)))
		{
			framework.vrOrigin[0] = teleportTargetX;
			framework.vrOrigin[2] = teleportTargetZ;
		}

		// tick tile interaction

		if (hasTile && tiles[tileX][tileZ].state == Tile::kState_Idle && (pointer.wentDown(VrButton_GripTrigger) || mouse.wentDown(BUTTON_RIGHT)))
		{
			if ((rand() % 4) == 0)
				tiles[tileX][tileZ].state = Tile::kState_FallUp;
			else
				tiles[tileX][tileZ].state = Tile::kState_FallDown;
			
			tiles[tileX][tileZ].beginAudio();
		}

		// tick tiles

		for (int x = 0; x < kGridSx; ++x)
		{
			for (int z = 0; z < kGridSz; ++z)
			{
				tiles[x][z].tick(dt);
			}
		}
	}
	
	void draw(RenderPass pass) const
	{
		// draw tiles

		if (pass == kRenderPass_Opaque)
		{
			setColor(colorWhite);

			shader.setTexture("u_texture", 0, tileTexture.id, true, false);
			shader.setImmediate("u_hasTexture", 1);

			beginCubeBatch();
			{
				const float e = kTileExtents * .97f;

				for (int x = 0; x < kGridSx; ++x)
				{
					for (int z = 0; z < kGridSz; ++z)
					{
						auto & t = tiles[x][z];

						fillCube_textured(Vec3(t.px, t.py - e, t.pz), Vec3(e, e, e));
					}
				}
			}
			endCubeBatch();

		#if 0
			gxBegin(GX_QUADS);
			{
				const float e = kTileExtents * .97f;

				for (int x = 0; x < kGridSx; ++x)
				{
					for (int z = 0; z < kGridSz; ++z)
					{
						auto & t = tiles[x][z];

						gxNormal3f(0, 1, 0);
						gxTexCoord2f(0, 0); gxVertex3f(t.px - e, t.py + .01f, t.pz - e);
						gxTexCoord2f(1, 0); gxVertex3f(t.px + e, t.py + .01f, t.pz - e);
						gxTexCoord2f(1, 1); gxVertex3f(t.px + e, t.py + .01f, t.pz + e);
						gxTexCoord2f(0, 1); gxVertex3f(t.px - e, t.py + .01f, t.pz + e);
					}
				}
			}
			gxEnd();
		#endif

			shader.setImmediate("u_hasTexture", 0);
		}

		// draw distant objects below

		if (pass == kRenderPass_Opaque)
		{
			setColor(colorWhite);

			shader.setTexture("u_texture", 0, tileTexture.id, true, false);
			shader.setImmediate("u_hasTexture", 1);

			beginCubeBatch();
			{
				for (int i = 0; i < 11; ++i)
				{
					const float a = i * 321;
					const float r = sinf(i * 123) * 200.f;
					
					const float x = cosf(a) * r;
					const float z = sinf(a) * r;

					const float y = -300.f + sinf(i) * 40.f;

					const float e = lerp<float>(20.f, 100.f, ((i * 321) % 11) / 10.f);
					
					fillCube_textured(Vec3(x, y, z), Vec3(e, 10.f, e));
				}
			}
			endCubeBatch();

			shader.setImmediate("u_hasTexture", 0);
		}

		// draw teleportation path-curve

		if (pass == kRenderPass_Translucent)
		{
			if (hasTeleportTarget)
			{
				const Mat4x4 headTransform = framework.getHeadTransform();
				const Vec3 side = headTransform.GetAxis(0) * .1f;
				
				const Vec3 from = framework.vrOrigin;//+ Vec3(0, 2.5f, 0);
				const Vec3 to = Vec3(teleportTargetX, 0.f, teleportTargetZ);
				
				setColor(colorWhite);
				
				shader.setTexture("u_texture", 0, getTexture("020-arrow.png"), true, false);
				shader.setImmediate("u_hasTexture", 1);
				
				pushBlend(BLEND_ADD);
				gxBegin(GX_TRIANGLE_STRIP);
				{
					for (int i = 0; i <= 100; ++i)
					{
						const float t = i / 100.f;
						
						const float h = sinf(t * float(M_PI)) * 2.f;
						const float y = lerp<float>(from[1], h, t);
						
						const float x = lerp<float>(from[0], to[0], t);
						const float z = lerp<float>(from[2], to[2], t);
						
						setAlphaf(t * t * t);
						gxTexCoord2f(0.f, - t * 3.f + framework.time); gxVertex3f(x - side[0], y - side[1], z - side[2]);
						gxTexCoord2f(1.f, - t * 3.f + framework.time); gxVertex3f(x + side[0], y + side[1], z + side[2]);
					}
				}
				gxEnd();
				popBlend();
				
				shader.setImmediate("u_hasTexture", 0);
			}
		}

		// draw teleportation target location

		if (pass == kRenderPass_Translucent)
		{
			if (hasTeleportTarget)
			{
				setColor(colorWhite);

				shader.setTexture("u_texture", 0, getTexture("020-circle.png"), true, true);
				shader.setImmediate("u_hasTexture", 1);

				gxBegin(GX_QUADS);
				{
					const float e = kTileExtents * 1.2f;
					const float y = .02f;

					gxNormal3f(0, 1, 0);
					gxTexCoord2f(0, 0); gxVertex3f(teleportTargetX - e, y, teleportTargetZ - e);
					gxTexCoord2f(1, 0); gxVertex3f(teleportTargetX + e, y, teleportTargetZ - e);
					gxTexCoord2f(1, 1); gxVertex3f(teleportTargetX + e, y, teleportTargetZ + e);
					gxTexCoord2f(0, 1); gxVertex3f(teleportTargetX - e, y, teleportTargetZ + e);
				}
				gxEnd();

				shader.setImmediate("u_hasTexture", 0);
			}
		}
	}
};

struct MyAudioStream : AudioStreamHD
{
	virtual int Provide(
		const ProvideInfo & provideInfo,
		const StreamInfo & streamInfo) override
	{
		const int numFrames = provideInfo.numFrames;
		
		float * outputSamplesL = provideInfo.outputSamples[0];
		float * outputSamplesR = provideInfo.outputSamples[1];

		// note : we update the audio in steps of 32 samples, to improve the timing for notes when the sample size is
		//        large. hopefully this fixes strange timing issues during Oculus Vr movie recordings. also this will
		//        reduce (the maximum) binauralization latency by a little and make it more stable/less jittery
		
		for (int i = 0; i < numFrames; )
		{
			const int kUpdateSize = 32;
			const int numSamplesRemaining = numFrames - i;
			const int numSamplesThisUpdate =
				numSamplesRemaining < kUpdateSize
					? numSamplesRemaining
					: kUpdateSize;

			spatialAudioSystem->generateLR(
				outputSamplesL + i,
				outputSamplesR + i,
				numSamplesThisUpdate);
			
			i += numSamplesThisUpdate;
		}

		return 2;
	}
};

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.vrMode = true;
	framework.enableVrMovement = false;

	framework.enableDepthBuffer = true;

	if (!framework.init(800, 600))
		return -1;
	
	spatialAudioSystem = new SpatialAudioSystem_Binaural("binaural");
	
	AudioOutputHD_Native audioOutput;
	audioOutput.Initialize(0, 2, 48000, 256);
	MyAudioStream audioStream;
	audioOutput.Play(&audioStream);
	
	pcmDataNoise.alloc(1000);
	for (int i = 0; i < pcmDataNoise.numSamples; ++i)
		pcmDataNoise.samples[i] = random<float>(-1.f, +1.f);
	
	World world;
	
	world.init();
	
	const Color fogColor(.2f, .1f, .05f);
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		world.tick(framework.timeStep);

		// Update the listener transform for the spatial audio system.
		
		const Mat4x4 listenerTransform = framework.getHeadTransform();
		spatialAudioSystem->setListenerTransform(listenerTransform);

		// Update the panning for the spatial audio system. This basically tells the spatial audio system we're done making a batch of modifications to the audio source and listener transforms, and tell it to update panning.
		
		spatialAudioSystem->updatePanning();
		
		const Vec3 lightDir = Vec3(
			cosf(framework.time / 13.21f),
			1.f,
			sinf(framework.time) / 14.56f).CalcNormalized();

		for (int i = 0; i < framework.getEyeCount(); ++i)
		{
			framework.beginEye(i, fogColor);
			{
				shader = Shader("020-shader");

				setShader(shader);
				shader.setImmediate("u_fogColor", fogColor.r, fogColor.g, fogColor.b);
				shader.setImmediate("u_fogDistance", 430.f);
				shader.setImmediate("u_hasTexture", 0);
				shader.setImmediate("u_lightDir", lightDir[0], lightDir[1], lightDir[2]);

				pushDepthTest(true, DEPTH_LESS);
				pushCullMode(CULL_BACK, CULL_CCW);
				pushBlend(BLEND_OPAQUE);
				{
					world.draw(kRenderPass_Opaque);

					framework.drawVrPointers();
				}
				popBlend();
				popCullMode();
				popDepthTest();

				setShader(shader);

				pushDepthTest(true, DEPTH_LESS, false);
				pushBlend(BLEND_ALPHA);
				{
					world.draw(kRenderPass_Translucent);
				}
				popBlend();
				popDepthTest();
			}
			framework.endEye();
		}
		
		framework.present();
	}
	
	world.shut();
	
	audioOutput.Stop();
	audioOutput.Shutdown();
	
	pcmDataNoise.free();
	
	delete spatialAudioSystem;
	spatialAudioSystem = nullptr;
	
	framework.shutdown();
	
	return 0;
}
