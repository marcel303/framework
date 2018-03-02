#include "audioGraph.h"
#include "audioGraphManager.h"
#include "audioNodeBase.h"
#include "audioUpdateHandler.h"
#include "audioVoiceManager4D.h"
#include "framework.h"
#include "Noise.h"
#include "objects/paobject.h"

const int GFX_SX = 1100;
const int GFX_SY = 740;

static SDL_mutex * s_audioMutex = nullptr;
static AudioVoiceManager * s_voiceMgr = nullptr;
static AudioGraphManager * s_audioGraphMgr = nullptr;

static void drawThickCircle(const float radius1, const float radius2, const int numSegments)
{
	const float angleStep = 2.f * M_PI / numSegments;
	
	gxBegin(GL_QUADS);
	{
		for (int i = 0; i < numSegments; ++i)
		{
			const float angle1 = (i + 0) * angleStep;
			const float angle2 = (i + 1) * angleStep;
			
			const float dx1 = std::cos(angle1);
			const float dy1 = std::sin(angle1);
			const float dx2 = std::cos(angle2);
			const float dy2 = std::sin(angle2);
			
			const float x1 = dx1 * radius1;
			const float y1 = dy1 * radius1;
			const float x2 = dx2 * radius1;
			const float y2 = dy2 * radius1;
			const float x3 = dx2 * radius2;
			const float y3 = dy2 * radius2;
			const float x4 = dx1 * radius2;
			const float y4 = dy1 * radius2;
			
			gxVertex2f(x1, y1);
			gxVertex2f(x2, y2);
			gxVertex2f(x3, y3);
			gxVertex2f(x4, y4);
		}
	}
	gxEnd();
}

static void drawTubeCircle_failed1(const float radius1, const float radius2, const int numSegments1, const int numSegments2)
{
	const float angleStep1 = 2.f * M_PI / numSegments1;
	const float angleStep2 = 2.f * M_PI / numSegments2;
	
	gxBegin(GL_QUADS);
	{
		for (int i = 0; i < numSegments1; ++i)
		{
			const float angle1_1 = (i + 0) * angleStep1;
			const float angle1_2 = (i + 1) * angleStep1;
			
			const float dx1 = std::cos(angle1_1);
			const float dy1 = std::sin(angle1_1);
			const float dx2 = std::cos(angle1_2);
			const float dy2 = std::sin(angle1_2);
			
			for (int j = 0; j < 10; ++j)
			{
				const float angle2_1 = (j + 0) * angleStep2;
				const float angle2_2 = (j + 1) * angleStep2;
				
				const float da1 = std::cos(angle2_1);
				const float dz1 = std::sin(angle2_1);
				const float da2 = std::cos(angle2_2);
				const float dz2 = std::sin(angle2_2);
			
				const float x1 = dx1 * radius1 + da1 * dx1 * radius2;
				const float y1 = dy1 * radius1 + da1 * dy1 * radius2;
				const float z1 = dz1 * radius2;
				
				const float x2 = dx1 * radius1 + da2 * dx1 * radius2;
				const float y2 = dy1 * radius1 + da2 * dy1 * radius2;
				const float z2 = dz2 * radius2;
				
				const float x3 = dx2 * radius1 + da2 * dx1 * radius2;
				const float y3 = dy2 * radius1 + da2 * dy1 * radius2;
				const float z3 = dz2 * radius2;
				
				const float x4 = dx2 * radius1 + da1 * dx1 * radius2;
				const float y4 = dy2 * radius1 + da1 * dy1 * radius2;
				const float z4 = dz1 * radius2;
				
				setLumif((x1 + y1 + z1 + 3.f) / 6.f);
				gxVertex3f(x1, y1, z1);
				gxVertex3f(x2, y2, z2);
				gxVertex3f(x3, y3, z3);
				gxVertex3f(x4, y4, z4);
			}
		}
	}
	gxEnd();
}

static void drawTubeCircle(const double radius1, const double radius2, const int numSegments1, const int numSegments2)
{
	const double angleStep1 = 2.0 * M_PI / numSegments1;
	const double angleStep2 = 2.0 * M_PI / numSegments2;
	
	gxBegin(GL_QUADS);
	{
		for (int i = 0; i < numSegments1; ++i)
		{
			const double outerAngle1 = (i + 0) * angleStep1;
			const double outerAngle2 = (i + 1) * angleStep1;
			
			const double dx1 = std::cos(outerAngle1);
			const double dy1 = std::sin(outerAngle1);
			const double dx2 = std::cos(outerAngle2);
			const double dy2 = std::sin(outerAngle2);
			
			for (int j = 0; j < 10; ++j)
			{
				const double innerAngle1 = (j + 0) * angleStep2;
				const double innerAngle2 = (j + 1) * angleStep2;
				
				const double da1 = std::cos(innerAngle1);
				const double dz1 = std::sin(innerAngle1);
				const double da2 = std::cos(innerAngle2);
				const double dz2 = std::sin(innerAngle2);
			
				const double x1 = dx1 * radius1 + da1 * dx1 * radius2;
				const double y1 = dy1 * radius1 + da1 * dy1 * radius2;
				const double z1 = dz1 * radius2;
				
				const double x2 = dx2 * radius1 + da1 * dx2 * radius2;
				const double y2 = dy2 * radius1 + da1 * dy2 * radius2;
				const double z2 = dz1 * radius2;
				
				const double x3 = dx2 * radius1 + da2 * dx2 * radius2;
				const double y3 = dy2 * radius1 + da2 * dy2 * radius2;
				const double z3 = dz2 * radius2;
				
				const double x4 = dx1 * radius1 + da2 * dx1 * radius2;
				const double y4 = dy1 * radius1 + da2 * dy1 * radius2;
				const double z4 = dz2 * radius2;
				
				//setLumif((x1 + y1 + z1 + 3.f) / 6.f);
				gxVertex3f(x1, y1, z1);
				gxVertex3f(x2, y2, z2);
				gxVertex3f(x3, y3, z3);
				gxVertex3f(x4, y4, z4);
			}
		}
	}
	gxEnd();
}

struct Mechanism
{
	const float increment = -.06f;
	
	const float kRadius1 = 1.00f;
	const float kRadius2 = kRadius1 + increment;
	const float kRadius3 = kRadius2 + increment;
	const float kRadius4 = kRadius3 + increment;

	const float kThickness = .02f;
	
	//
	
	double xAngle = 0.0;
	double yAngle = 0.0;
	double zAngle = 0.0;
	
	double xAngleSpeed = 0.0;
	double yAngleSpeed = 0.0;
	double zAngleSpeed = 0.0;
	
	void tick(const float dt)
	{
		xAngle += xAngleSpeed * dt;
		yAngle += yAngleSpeed * dt;
		zAngle += zAngleSpeed * dt;
		
		xAngle = std::fmod(xAngle, 360.0);
		yAngle = std::fmod(yAngle, 360.0);
		zAngle = std::fmod(zAngle, 360.0);
	}
	
	void evaluateMatrix(const int ringIndex, Mat4x4 & matrix, float & radius) const
	{
		matrix.MakeIdentity();
		radius = kRadius1;
		
		if (ringIndex >= 1)
		{
			matrix = matrix.RotateX(-xAngle * M_PI / 180.0);
			radius = kRadius2;
		}
		
		if (ringIndex >= 2)
		{
			matrix = matrix.RotateY(-yAngle * M_PI / 180.0);
			radius = kRadius3;
		}
		
		if (ringIndex >= 3)
		{
			matrix = matrix.RotateX(-zAngle * M_PI / 180.0);
			radius = kRadius4;
		}
	}
	
	Vec3 evaluatePoint(const int ringIndex, const float angle) const
	{
		Mat4x4 matrix;
		float radius;
		
		evaluateMatrix(ringIndex, matrix, radius);
		
		const Vec3 p(std::cos(angle), std::sin(angle), 0.f);
		
		return matrix * (p * radius);
	}
	
	void drawGizmo(const float radius, const float thickness)
	{
		drawCircle(0, 0, radius, 100);
		//drawThickCircle(radius - thickness, radius + thickness, 100);
		//drawTubeCircle_failed1(radius - thickness, radius + thickness, 100, 10);
		//drawTubeCircle_failed1(radius, thickness, 10, 4);
		drawTubeCircle(radius, thickness, 100, 10);
	}
	
	void draw_solid()
	{
		gxPushMatrix();
		{
			setLumi(200);
			drawGizmo(kRadius1, kThickness);
			drawLine(-kRadius1, 0, + (kRadius1 - kRadius2) - kRadius1, 0);
			drawLine(+kRadius1, 0, - (kRadius1 - kRadius2) + kRadius1, 0);
			gxRotatef(xAngle, 1, 0, 0);
			
			setLumi(180);
			drawGizmo(kRadius2, kThickness);
			drawLine(0, -kRadius2, 0, + (kRadius2 - kRadius3) - kRadius2);
			drawLine(0, +kRadius2, 0, - (kRadius2 - kRadius3) + kRadius2);
			gxRotatef(yAngle, 0, 1, 0);
			
			setLumi(160);
			drawGizmo(kRadius3, kThickness);
			drawLine(-kRadius3, 0, + (kRadius3 - kRadius4) - kRadius3, 0);
			drawLine(+kRadius3, 0, - (kRadius3 - kRadius4) + kRadius3, 0);
			gxRotatef(zAngle, 1, 0, 0);
			
			setLumi(140);
			drawGizmo(kRadius4, kThickness);
		}
		gxPopMatrix();
	}
};

static Mechanism * s_mechanism = nullptr;

struct AudioNodeMechanism : AudioNodeBase
{
	Mechanism mechanism;
	
	enum Input
	{
		kInput_Ring,
		kInput_Speed,
		kInput_Angle,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_X,
		kOutput_Y,
		kOutput_Z,
		kOutput_COUNT
	};
	
	AudioFloat xOutput;
	AudioFloat yOutput;
	AudioFloat zOutput;
	
	float time;
	
	AudioNodeMechanism()
		: AudioNodeBase()
		, xOutput(0.f)
		, yOutput(0.f)
		, zOutput(0.f)
		, time(0.f)
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Ring, kAudioPlugType_Int);
		addInput(kInput_Speed, kAudioPlugType_FloatVec);
		addInput(kInput_Angle, kAudioPlugType_FloatVec);
		addOutput(kOutput_X, kAudioPlugType_FloatVec, &xOutput);
		addOutput(kOutput_Y, kAudioPlugType_FloatVec, &yOutput);
		addOutput(kOutput_Z, kAudioPlugType_FloatVec, &zOutput);
		
		s_mechanism = &mechanism;
	};
	
	virtual void tick(const float dt)
	{
		const int ring = getInputInt(kInput_Ring, 0);
		const AudioFloat * speed = getInputAudioFloat(kInput_Speed, &AudioFloat::One);
		const AudioFloat * angle = getInputAudioFloat(kInput_Angle, &AudioFloat::Zero);
		
		angle->expand();
		
		xOutput.setVector();
		yOutput.setVector();
		zOutput.setVector();
		
		const double dtSample = double(dt) * speed->getMean() / AUDIO_UPDATE_SIZE;
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			const Vec3 p = mechanism.evaluatePoint(ring, angle->samples[i]);
			
			xOutput.samples[i] = p[0];
			yOutput.samples[i] = p[1];
			zOutput.samples[i] = p[2];
			
			mechanism.tick(dtSample);
		}
		
		time += dt;
		
		mechanism.xAngleSpeed = scaled_octave_noise_2d(8, .5f, .01f, -90.f, +90.f, 0.f, time);
		mechanism.yAngleSpeed = scaled_octave_noise_2d(8, .5f, .01f, -90.f, +90.f, 1.f, time);
		mechanism.zAngleSpeed = scaled_octave_noise_2d(8, .5f, .01f, -90.f, +90.f, 2.f, time);
	}
};

AUDIO_NODE_TYPE(mechanism, AudioNodeMechanism)
{
	typeName = "mechanism";
	
	in("ring", "int");
	in("speed", "audioValue");
	in("angle", "audioValue");
	out("x", "audioValue");
	out("y", "audioValue");
	out("z", "audioValue");
}

struct Thermalizer
{
	const static int kSize = 64;
	
	float heat[kSize];
	
	Thermalizer()
	{
		memset(this, 0, sizeof(*this));
	}
	
	void tick(const float dt)
	{
		for (int i = 0; i < kSize; ++i)
		{
			const float noiseX = i;
			const float noiseY = framework.time * .1f;
			
			const float noise = scaled_octave_noise_2d(8, .5f, 1.f, 0.f, 1.f, noiseX, noiseY);
			
			heat[i] = noise;
		}
	}
	
	void draw2d() const
	{
		gxPushMatrix();
		{
			//gxScalef(.5f, .5f, 1);
			gxScalef(8, 8, 1);
			gxTranslatef(-kSize/2, 0, 0);
			
			hqBegin(HQ_FILLED_CIRCLES);
			{
				for (int i = 0; i < kSize; ++i)
				{
					setLumif(std::abs(heat[i]));
					hqFillCircle(i, 0, .5f);
				}
			}
			hqEnd();
		}
		gxPopMatrix();
	}
};

int main(int argc, char * argv[])
{
	framework.enableDepthBuffer = true;
	
	if (!framework.init(0, nullptr, GFX_SX, GFX_SY))
		return -1;
	
	SDL_mutex * audioMutex = SDL_CreateMutex();
	s_audioMutex = audioMutex;
	
	AudioVoiceManager4D voiceMgr;
	voiceMgr.init(audioMutex, 16, 16);
	voiceMgr.outputStereo = true;
	s_voiceMgr = &voiceMgr;
	
	AudioGraphManager_RTE audioGraphMgr(GFX_SX, GFX_SY);
	audioGraphMgr.init(audioMutex, &voiceMgr);
	s_audioGraphMgr = &audioGraphMgr;
	
	AudioUpdateHandler audioUpdateHandler;
	audioUpdateHandler.init(audioMutex, nullptr, 0);
	audioUpdateHandler.voiceMgr = &voiceMgr;
	audioUpdateHandler.audioGraphMgr = &audioGraphMgr;
	
	PortAudioObject paObject;
	paObject.init(SAMPLE_RATE, 2, 0, AUDIO_UPDATE_SIZE, &audioUpdateHandler);
	
	Mechanism mechanism;
	s_mechanism = &mechanism;
	
	Thermalizer thermalizer;
	
	Camera3d camera;
	camera.gamepadIndex = 0;
	camera.position[2] = -2.f;
	
	bool drawMechanism = true;
	bool drawThermalizer = true;
	bool showEditor = true;
	
	AudioGraphInstance * instance = audioGraphMgr.createInstance("sound3.xml");
	audioGraphMgr.selectInstance(instance);
	
	for (;;)
	{
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;
		
		if (framework.quitRequested)
			break;
		
		const float dt = framework.timeStep;
		
		mechanism.xAngleSpeed = scaled_octave_noise_2d(8, .5f, .01f, -90.f, +90.f, 0.f, framework.time);
		mechanism.yAngleSpeed = scaled_octave_noise_2d(8, .5f, .01f, -90.f, +90.f, 1.f, framework.time);
		mechanism.zAngleSpeed = scaled_octave_noise_2d(8, .5f, .01f, -90.f, +90.f, 2.f, framework.time);
		
		mechanism.tick(dt * 10.f);
		
		thermalizer.tick(dt);
		
		bool inputIsCaptured = false;
		
		inputIsCaptured |= audioGraphMgr.tickEditor(dt, inputIsCaptured);
		
		if (audioGraphMgr.selectedFile != nullptr)
		{
			if (audioGraphMgr.selectedFile->graphEdit->state != GraphEdit::kState_Hidden)
				inputIsCaptured |= true;
		}
		
		camera.tick(dt, !inputIsCaptured);
		
		if (inputIsCaptured == false)
		{
			if (keyboard.wentDown(SDLK_m))
				drawMechanism = !drawMechanism;
			
			if (keyboard.wentDown(SDLK_t))
				drawThermalizer = !drawThermalizer;
		}
		
		audioGraphMgr.tickMain();
		
		framework.beginDraw(40, 40, 40, 0);
		{
			pushFontMode(FONT_SDF);
			setFont("calibri.ttf");
			
			const float fov = 100.f;
			const float near = .01f;
			const float far = 10.f;
			
			projectPerspective3d(fov, near, far);
			
			camera.pushViewMatrix();
			{
				glEnable(GL_DEPTH_TEST);
				glDepthFunc(GL_LESS);
				{
					if (drawMechanism)
					{
						setColor(colorWhite);
						mechanism.draw_solid();
						
						for (int ring = 0; ring <= 3; ++ring)
						{
							for (int i = 0; i < 10; ++i)
							{
								gxPushMatrix();
								{
									const float angle = framework.time / (i / 10.f + 2.f);
									
								#if 1
									Mat4x4 matrix;
									float radius;
									
									mechanism.evaluateMatrix(ring, matrix, radius);
									
									gxMultMatrixf(matrix.m_v);
									gxScalef(radius, radius, radius);
									gxRotatef(angle / M_PI * 180.f, 0, 0, 1);
									gxTranslatef(1.f, 0.f, 0.f);
									gxRotatef(90, 1, 0, 0);
								#else
									const Vec3 p = mechanism.evaluatePoint(ring, angle);
									gxTranslatef(p[0], p[1], p[2]);
								#endif
									
									setColor(colorGreen);
									const float s1 = .05f;
									const float s2 = .06f;
									//drawRect(-s, -s, +s, +s);
									//drawThickCircle(s1, s2, 100);
									drawTubeCircle(s1, .01f, 100, 10);
								}
								gxPopMatrix();
							}
						}
					}
				}
				glDisable(GL_DEPTH_TEST);
			}
			camera.popViewMatrix();
			
			projectScreen2d();
			
			if (drawThermalizer)
			{
				gxPushMatrix();
				{
					gxTranslatef(GFX_SX/2, GFX_SY/2, 0);
					
					setColor(colorWhite);
					thermalizer.draw2d();
				}
				gxPopMatrix();
			}
			
			if (showEditor)
			{
				audioGraphMgr.drawEditor();
			}
			
			popFontMode();
		}
		framework.endDraw();
	}
	
	audioGraphMgr.free(instance, false);
	
	paObject.shut();
	
	audioUpdateHandler.shut();
	
	audioGraphMgr.shut();
	s_audioGraphMgr = nullptr;
	
	voiceMgr.shut();
	s_voiceMgr = nullptr;
	
	SDL_DestroyMutex(audioMutex);
	audioMutex = nullptr;
	s_audioMutex = nullptr;
	
	Font("calibri.ttf").saveCache();
	
	framework.shutdown();
	
	return 0;
}
