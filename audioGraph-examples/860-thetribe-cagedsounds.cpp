namespace Cagedsounds
{
struct MyMutex : binaural::Mutex
{
	SDL_mutex * mutex;
	
	MyMutex(SDL_mutex * _mutex)
		: mutex(_mutex)
	{
	}
	
	virtual void lock() override
	{
		SDL_LockMutex(mutex);
	}
	
	virtual void unlock() override
	{
		SDL_UnlockMutex(mutex);
	}
};

struct MyPortAudioHandler : PortAudioHandler
{
	SDL_mutex * mutex;
	
	std::vector<MultiChannelAudioSource_SoundVolume*> volumeSources;
	std::vector<AudioSource*> pointSources;
	
	MyPortAudioHandler()
		: PortAudioHandler()
		, mutex(nullptr)
		, volumeSources()
		, pointSources()
	{
	}
	
	~MyPortAudioHandler()
	{
		Assert(mutex == nullptr);
	}
	
	void init(SDL_mutex * _mutex)
	{
		mutex = _mutex;
	}
	
	void shut()
	{
		mutex = nullptr;
	}
	
	void addVolumeSource(MultiChannelAudioSource_SoundVolume * source)
	{
		SDL_LockMutex(mutex);
		{
			volumeSources.push_back(source);
		}
		SDL_UnlockMutex(mutex);
	}
	
	void addPointSource(AudioSource * source)
	{
		SDL_LockMutex(mutex);
		{
			pointSources.push_back(source);
		}
		SDL_UnlockMutex(mutex);
	}
	
	virtual void portAudioCallback(
		const void * inputBuffer,
		const int numInputChannels,
		void * outputBuffer,
		const int framesPerBuffer) override
	{
		ALIGN16 float channelL[AUDIO_UPDATE_SIZE];
		ALIGN16 float channelR[AUDIO_UPDATE_SIZE];
		
		memset(channelL, 0, sizeof(channelL));
		memset(channelR, 0, sizeof(channelR));
		
		SDL_LockMutex(mutex);
		{
			for (auto volumeSource : volumeSources)
			{
                const float gain = 1.f;
				
				volumeSource->generate(0, channelL, AUDIO_UPDATE_SIZE, gain);
				volumeSource->generate(1, channelR, AUDIO_UPDATE_SIZE, gain);
			}
			
			for (auto & pointSource : pointSources)
			{
                const float gain = 1.f;
				
				ALIGN16 float channel[AUDIO_UPDATE_SIZE];
				
				pointSource->generate(channel, AUDIO_UPDATE_SIZE);
				
				for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
				{
					channelL[i] += channel[i] * gain;
					channelR[i] += channel[i] * gain;
				}
			}
		}
		SDL_UnlockMutex(mutex);
		
		float * __restrict destinationBuffer = (float*)outputBuffer;

		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			destinationBuffer[i * 2 + 0] = channelL[i];
			destinationBuffer[i * 2 + 1] = channelR[i];
		}
	}
};

static MyPortAudioHandler * s_paHandler = nullptr;

static void drawSoundVolume(const SoundVolume & volume)
{
	gxPushMatrix();
	{
		gxMultMatrixf(volume.transform.m_v);
		
		gxPushMatrix(); { gxTranslatef(-1, 0, 0); drawGrid3dLine(10, 10, 1, 2); } gxPopMatrix();
		gxPushMatrix(); { gxTranslatef(+1, 0, 0); drawGrid3dLine(10, 10, 1, 2); } gxPopMatrix();
		gxPushMatrix(); { gxTranslatef(0, -1, 0); drawGrid3dLine(10, 10, 2, 0); } gxPopMatrix();
		gxPushMatrix(); { gxTranslatef(0, +1, 0); drawGrid3dLine(10, 10, 2, 0); } gxPopMatrix();
		gxPushMatrix(); { gxTranslatef(0, 0, -1); drawGrid3dLine(10, 10, 0, 1); } gxPopMatrix();
		gxPushMatrix(); { gxTranslatef(0, 0, +1); drawGrid3dLine(10, 10, 0, 1); } gxPopMatrix();
	}
	gxPopMatrix();
}

static bool intersectSoundVolume(const SoundVolume & soundVolume, Vec3Arg pos, Vec3Arg dir, Vec3 & p, float & t)
{
	auto & soundToWorld = soundVolume.transform;
	const Mat4x4 worldToSound = soundToWorld.CalcInv();

	const Vec3 pos_sound = worldToSound.Mul4(pos);
	const Vec3 dir_sound = worldToSound.Mul3(dir);

	const float d = pos_sound[2];
	const float dd = dir_sound[2];

	t = - d / dd;
	
	p = pos_sound + dir_sound * t;
	
	return
		p[0] >= -1.f && p[0] <= +1.f &&
		p[1] >= -1.f && p[1] <= +1.f;
}

struct World
{
	Camera3d camera;
	
	World()
		: camera()
	{
	}

	void init(binaural::HRIRSampleSet * sampleSet, binaural::Mutex * mutex)
	{
		camera.gamepadIndex = 0;
		
		const float kMoveSpeed = .2f;
		//const float kMoveSpeed = 1.f;
		camera.maxForwardSpeed *= kMoveSpeed;
		camera.maxUpSpeed *= kMoveSpeed;
		camera.maxStrafeSpeed *= kMoveSpeed;
		
		camera.position[0] = 0;
		camera.position[1] = +.3f;
		camera.position[2] = -1.f;
		camera.pitch = 10.f;
	}
	
	void tick(const float dt)
	{
		// update the camera
		
		const bool doCamera = !(keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT));
		
		camera.tick(dt, doCamera);
	}
	
	void draw3d()
	{
		camera.pushViewMatrix();
		
		glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
		glEnable(GL_LINE_SMOOTH);

		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		{
			pushBlend(BLEND_OPAQUE);
			{
                // todo : draw cage sounds, solid
			}
			popBlend();
		}
		glDisable(GL_DEPTH_TEST);
		
		glEnable(GL_DEPTH_TEST);
		glDepthMask(GL_FALSE);
		{
			gxPushMatrix();
			{
				gxScalef(10, 10, 10);
				setColor(200, 200, 200, 60);
				drawGrid3dLine(100, 100, 0, 2, true);
			}
			gxPopMatrix();
			
			// todo : draw cage sounds, translucent
		}
		glDepthMask(GL_TRUE);
		glDisable(GL_DEPTH_TEST);
		
		camera.popViewMatrix();
	}
	
	void draw2d()
	{
	}
};

static World * s_world = nullptr;

struct Cage
{
	virtual float evaluate(const float x, const float y, const float z) const
	{
		//const float elevation = std::atan2(y, x);
		//const float azimuth = std::atan2(x, z);
		
		const float zxHypot = std::hypot(z, x);
		
		const float azimuth = std::atan2(z, x);
		
		float elevation;
		
		if (zxHypot == 0.f)
		{
			if (y < 0.f)
				elevation = -90.f;
			else
				elevation = +90.f;
		}
		else
		{
			elevation = std::atan(y / zxHypot);
		}
		
		return evaluatePolar(elevation, azimuth);
	}
	
	virtual float evaluatePolar(const float elevation, const float azimuth) const
	{
		return 0.f;
	}
};

struct CagedSound
{

};

static void drawCage2d(const Cage & cage)
{
	gxPushMatrix();
	gxScalef(1.f / 360.f * GFX_SX, 1.f / 180.f * GFX_SY, 0);
	gxTranslatef(180, 90, 0);
	
	setAlphaf(1.f);
	hqBegin(HQ_FILLED_CIRCLES, true);
	
	for (float azimuth = -180.f; azimuth <= +180.f; azimuth += 4)
	{
		for (float elevation = -90.f; elevation <= +90.f; elevation += 4)
		{
			float x, y, z;
			binaural::elevationAndAzimuthToCartesian(elevation, azimuth, x, y, z);
			
			const float value = cage.evaluate(x, y, z);
			
			setLumif(value);
			hqFillCircle(azimuth, elevation, 3.f);
		}
	}
	
	hqEnd();
	
	gxPopMatrix();
}

struct Cage1 : Cage
{
	virtual float evaluatePolar(const float elevation, const float azimuth) const override
	{
		float v = 1.f;
		
		//v *= (1.f - std::cos(elevation * 2.f)) / 2.f;
		v *= (std::cos(azimuth * 5.f) + 1.f) / 2.f;
		
		return v;
	}
};

void main()
{
	const int kFontSize = 16;
	
	bool showUi = true;
	
	float fov = 90.f;
	float near = .01f;
	float far = 100.f;
	
	SDL_mutex * audioMutex = SDL_CreateMutex();
	
	MyMutex binauralMutex(audioMutex);
	
	binaural::HRIRSampleSet sampleSet;
	binaural::loadHRIRSampleSet_Cipic("subject147", sampleSet);
	sampleSet.finalize();
	
	MyPortAudioHandler * paHandler = new MyPortAudioHandler();
	paHandler->init(audioMutex);
	s_paHandler = paHandler;
	
	//
	
	World world;
	
	world.init(&sampleSet, &binauralMutex);
    s_world = &world;
	
	PortAudioObject pa;
	pa.init(SAMPLE_RATE, 2, 0, AUDIO_UPDATE_SIZE, paHandler);
	
	Cage1 cage;
	
	do
	{
		framework.process();

		const float dt = framework.timeStep;
		
		// process input
		
		if (keyboard.wentDown(SDLK_TAB))
			showUi = !showUi;
	
		// update video clips
		
		world.tick(dt);
	
		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			pushFontMode(FONT_SDF);
			
			projectPerspective3d(fov, near, far);
			{
				world.draw3d();
			}
			projectScreen2d();
            
			world.draw2d();
			
			drawCage2d(cage);
			
			if (showUi)
			{
				setColor(255, 255, 255, 127);
				drawText(GFX_SX - 10, 40, 32, -1, +1, "CAGED SOUNDS");
			}

			popFontMode();
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_ESCAPE));
	
	pa.shut();
	
	s_paHandler = nullptr;
	paHandler->shut();
	delete paHandler;
	paHandler = nullptr;
	
	SDL_DestroyMutex(audioMutex);
	audioMutex = nullptr;
	
	return 0;
}

}
