#include "ip/UdpSocket.h"
#include "mediaplayer_old/MPContext.h"
#include "osc/OscOutboundPacketStream.h"
#include "osc/OscPacketListener.h"
#include "audiostream/AudioOutput.h"
#include "audioin.h"
#include "Calc.h"
#include "config.h"
#include "drawable.h"
#include "effect.h"
#include "framework.h"
#include "Path.h"
#include "scene.h"
#include "Timer.h"
#include "tinyxml2.h"
#include "types.h"
#include "video.h"
#include "xml.h"
#include <algorithm>
#include <list>
#include <map>
#include <Windows.h>

#include "data/ShaderConstants.h"

using namespace tinyxml2;

#define DEMODATA 0

#define NUM_SCREENS 1

const int SCREEN_SX = (1920 / NUM_SCREENS);
const int SCREEN_SY = 1080;

const int GFX_SX = (SCREEN_SX * NUM_SCREENS);
const int GFX_SY = (SCREEN_SY * 1);

#define OSC_ADDRESS "127.0.0.1"
#define OSC_RECV_PORT 1121

Config config;

float virtualToScreenX(const float x)
{
#if NUM_SCREENS == 1
	return ((x / 100.f) + 1.5f) * SCREEN_SX / 3.f;
#elif NUM_SCREENS == 3
	return ((x / 100.f) + 1.5f) * SCREEN_SX;
#endif
}

float virtualToScreenY(const float y)
{
	return (y / 100.f + .5f) * SCREEN_SY;
}

/*

:: todo :: OSC

	-

:: todo :: configuration

	# prompt for MIDI controller at startup. or select by name in XML config file?
		+ defined in settings.xml instead
	# prompt for audio input device at startup. or select by name in XML config file?
		+ defined in settings xml instead
	+ add XML settings file
	- define scene XML representation
	- discuss with Max what would be needed for life act

:: todo :: projector output

	- add brightness control
		+ write shader which does a lookup based on the luminance of the input and transforms the input
		- add ability to change the setting
	- add border blends to hide projector seam. unless eg MadMapper already does this, it may be necessary to do it ourselvess

:: todo :: utility functions

	+ add PCM capture
	- add FFT calculation PCM data
	+ add loudness calculation PCM data

:: todo :: post processing and graphics quality

	- smooth line drawing with high AA. use a post process pass to blur the result ?

:: todo :: visuals tech 2D

	+ add a box blur shader. allow it to darken the output too

	- integrate Box2D ?

	+ add flow map shader
		+ use an input texture to warp/distort the previous frame

	- add film grain shader

:: todo :: visuals tech 3D

	- virtual camera positioning
		- allow positioning of the virtual camera based on settings XML
		- allow tweens on the virtual camera position ?

	+ compute virtual camera matrices

	- add lighting shader code

:: todo :: effects

	- particle effect : sea

	- particle effect : rain

	- particle effect : bouncy (sort of like rain, but with bounce effect + ability to control with sensor input)

	- cloth simulation

	- particle effect :: star cluster

:: todo :: particle system

	- ability to toggle particle trails

:: todo :: color controls

	- 

:: notes

	- seamless transitions between scenes

*/

//

static CRITICAL_SECTION s_oscMessageMtx;
static HANDLE s_oscMessageThread = INVALID_HANDLE_VALUE;

enum OscMessageType
{
	kOscMessageType_None,
	// scene :: constantly reinforced
	kOscMessageType_SetScene,
	// visual effects
	kOscMessageType_Box3D,
	kOscMessageType_Sprite,
	kOscMessageType_Video,
	// time effect
	kOscMessageType_TimeDilation,
	// sensors
	kOscMessageType_Swipe
};

struct OscMessage
{
	OscMessage()
		: type(kOscMessageType_None)
	{
		memset(param, 0, sizeof(param));
	}

	OscMessageType type;
	float param[4];
	std::string str;
};

static std::list<OscMessage> s_oscMessages;

class MyOscPacketListener : public osc::OscPacketListener
{
protected:
	virtual void ProcessMessage(const osc::ReceivedMessage & m, const IpEndpointName & remoteEndpoint)
	{
		try
		{
			osc::ReceivedMessageArgumentStream args = m.ArgumentStream();

			OscMessage message;

			if (strcmp(m.AddressPattern(), "/box") == 0)
			{
				// width, angle1, angle2
				message.type = kOscMessageType_Sprite;
				const char * str;
				args >> str >> message.param[0] >> message.param[1] >> message.param[2];
				message.str = str;
			}
			else if (strcmp(m.AddressPattern(), "/sprite") == 0)
			{
				// filename, x, y, scale
				message.type = kOscMessageType_Sprite;
				const char * str;
				osc::int32 x;
				osc::int32 y;
				osc::int32 scale;
				//args >> str >> message.param[0] >> message.param[1] >> message.param[2];
				args >> str >> x >> y >> scale;
				message.str = str;

				message.param[0] = x;
				message.param[1] = y;
				message.param[2] = scale;
			}
			else if (strcmp(m.AddressPattern(), "/video") == 0)
			{
				// filename, x, y, scale
				message.type = kOscMessageType_Video;
				const char * str;
				args >> str >> message.param[0] >> message.param[1] >> message.param[2];
				message.str = str;
			}
			else if (strcmp(m.AddressPattern(), "/timedilation") == 0)
			{
				// filename, x, y, scale
				message.type = kOscMessageType_TimeDilation;
				args >> message.param[0] >> message.param[1];
			}
			else
			{
				logWarning("unknown message type: %s", m.AddressPattern());
			}

			if (message.type != kOscMessageType_None)
			{
				EnterCriticalSection(&s_oscMessageMtx);
				{
					s_oscMessages.push_back(message);
				}
				LeaveCriticalSection(&s_oscMessageMtx);
			}
		}
		catch (osc::Exception & e)
		{
			logError("error while parsing message: %s: %s", m.AddressPattern(), e.what());
		}
	}
};

static MyOscPacketListener s_oscListener;
UdpListeningReceiveSocket * s_oscReceiveSocket = nullptr;

static DWORD WINAPI ExecuteOscThread(LPVOID pParam)
{
	s_oscReceiveSocket = new UdpListeningReceiveSocket(IpEndpointName(IpEndpointName::ANY_ADDRESS, OSC_RECV_PORT), &s_oscListener);
	s_oscReceiveSocket->Run();
	return 0;
}

struct TimeDilationEffect
{
	TimeDilationEffect()
	{
		memset(this, 0, sizeof(*this));
	}

	float duration;
	float durationRcp;
	float multiplier;
};

struct Camera
{
	Mat4x4 worldToCamera;
	Mat4x4 cameraToWorld;
	Mat4x4 cameraToView;
	float fovX;

	void setup(Vec3Arg position, Vec3 * screenCorners, int numScreenCorners, int cameraIndex)
	{
		Mat4x4 lookatMatrix;

		{
			const Vec3 d1a = screenCorners[0] - position;
			const Vec3 d2a = screenCorners[1] - position;
			const Vec3 d1 = Vec3(d1a[0], 0.f, d1a[2]).CalcNormalized();
			const Vec3 d2 = Vec3(d2a[0], 0.f, d2a[2]).CalcNormalized();
			const Vec3 d = ((d1 + d2) / 2.f).CalcNormalized();

			const Vec3 upVector(0.f, 1.f, 0.f);

			lookatMatrix.MakeLookat(position, position + d, upVector);
		}

		Mat4x4 invLookatMatrix = lookatMatrix.CalcInv();

		Vec3 * screenCornersInCameraSpace = (Vec3*)alloca(numScreenCorners * sizeof(Vec3));

		for (int i = 0; i < numScreenCorners; ++i)
			screenCornersInCameraSpace[i] = lookatMatrix * screenCorners[i];

		float fovX;

		{
			const Vec3 d1a = screenCornersInCameraSpace[0];
			const Vec3 d2a = screenCornersInCameraSpace[1];
			const Vec3 d1 = Vec3(d1a[0], 0.f, d1a[2]).CalcNormalized();
			const Vec3 d2 = Vec3(d2a[0], 0.f, d2a[2]).CalcNormalized();
			const float dot = d1 * d2;
			fovX = acosf(dot);
		}

		float fovY;

		{
			const Vec3 d1a = screenCornersInCameraSpace[cameraIndex == 0 ? 1 : 0];
			const Vec3 d2a = screenCornersInCameraSpace[cameraIndex == 0 ? 2 : 3];
			const Vec3 d1 = Vec3(0.f, d1a[1], d1a[2]).CalcNormalized();
			const Vec3 d2 = Vec3(0.f, d2a[1], d2a[2]).CalcNormalized();
			const float dot = d1 * d2;
			fovY = acosf(dot);
		}

		// calculate horizontal and vertical fov and setup projection matrix

		const float aspect = cosf(fovX) / cosf(fovY);

		Mat4x4 projection;
		projection.MakePerspectiveLH(fovY, aspect, .001f, 10.f);

		//

		cameraToWorld = invLookatMatrix;
		worldToCamera = lookatMatrix;
		cameraToView = projection;
	}

	void beginView(int c, int & sx, int & sy) const
	{
		gxMatrixMode(GL_PROJECTION);
		gxPushMatrix();
		gxLoadMatrixf(cameraToView.m_v);
		gxMultMatrixf(worldToCamera.m_v);

		gxMatrixMode(GL_MODELVIEW);
		gxPushMatrix();
	#if NUM_SCREENS == 1
		const int x1 = virtualToScreenX(-150);
		const int y1 = virtualToScreenY(-50);
		const int x2 = virtualToScreenX(+150);
		const int y2 = virtualToScreenY(+50);
	#elif NUM_SCREENS == 3
		const int x1 = virtualToScreenX(-150 + (c + 0) * 100);
		const int y1 = virtualToScreenY(-50);
		const int x2 = virtualToScreenX(-150 + (c + 1) * 100);
		const int y2 = virtualToScreenY(+50);
	#endif

		sx = x2 - x1;
		sy = y2 - y1;

		glViewport(
			x1 / framework.minification,
			y1 / framework.minification,
			sx / framework.minification,
			sy / framework.minification);
	}

	void endView() const
	{
		gxMatrixMode(GL_PROJECTION);
		gxPopMatrix();
		gxMatrixMode(GL_MODELVIEW);
		gxPopMatrix();

		// dirty hack to restore viewport

		pushSurface(nullptr);
		popSurface();
	}
};

static void drawTestObjects()
{
	for (int k = 0; k < 3; ++k)
	{
		gxPushMatrix();
		{
			const float dx = sinf(framework.time / (k + 4.f));
			const float dy = sinf(framework.time / (k + 8.f)) / 2.f;
			const float dz = cosf(framework.time / (k + 4.f) * 2.f) * 2.f;

			gxTranslatef(
				dx + (k - 1) / 1.1f,
				dy + .5f + (k - 1) / 4.f, dz + 2.f);
			gxScalef(.5f, .5f, .5f);
			gxRotatef(framework.time / (k + 1.123f) * Calc::rad2deg, 1.f, 0.f, 0.f);
			gxRotatef(framework.time / (k + 1.234f) * Calc::rad2deg, 0.f, 1.f, 0.f);
			gxRotatef(framework.time / (k + 1.345f) * Calc::rad2deg, 0.f, 0.f, 1.f);

			gxBegin(GL_TRIANGLES);
			{
				gxColor4f(1.f, 0.f, 0.f, 1.f); gxVertex3f(-1.f,  0.f,  0.f); gxVertex3f(+1.f,  0.f,  0.f); gxVertex3f(+1.f, 0.3f, 0.2f);
				gxColor4f(0.f, 1.f, 0.f, 1.f); gxVertex3f( 0.f, -1.f,  0.f); gxVertex3f( 0.f, +1.f,  0.f); gxVertex3f(0.4f, +1.f, 0.4f);
				gxColor4f(0.f, 0.f, 1.f, 1.f); gxVertex3f( 0.f,  0.f, -1.f); gxVertex3f( 0.f,  0.f, +1.f); gxVertex3f(0.1f, 0.2f, +1.f);
			}
			gxEnd();
		}
		gxPopMatrix();
	}
}

static void drawGroundPlane(const float y)
{
	gxColor4f(.2f, .2f, .2f, 1.f);
	gxBegin(GL_QUADS);
	{
		gxVertex3f(-100.f, y, -100.f);
		gxVertex3f(+100.f, y, -100.f);
		gxVertex3f(+100.f, y, +100.f);
		gxVertex3f(-100.f, y, +100.f);
	}
	gxEnd();
}

static void drawCamera(const Camera & camera, const float alpha)
{
	// draw local axis

	gxMatrixMode(GL_MODELVIEW);
	gxPushMatrix();
	{
		gxMultMatrixf(camera.cameraToWorld.m_v);
		//glMatrixMultfEXT(GL_MODELVIEW, camera.cameraToWorld.m_v);

		gxPushMatrix();
		{
			gxScalef(.2f, .2f, .2f);
			gxBegin(GL_LINES);
			{
				gxColor4f(1.f, 0.f, 0.f, alpha); gxVertex3f(0.f, 0.f, 0.f); gxVertex3f(1.f, 0.f, 0.f);
				gxColor4f(0.f, 1.f, 0.f, alpha); gxVertex3f(0.f, 0.f, 0.f); gxVertex3f(0.f, 1.f, 0.f);
				gxColor4f(0.f, 0.f, 1.f, alpha); gxVertex3f(0.f, 0.f, 0.f); gxVertex3f(0.f, 0.f, 1.f);
			}
			gxEnd();
		}
		gxPopMatrix();

		const Mat4x4 invView = camera.cameraToView.CalcInv();
		
		const Vec3 p[5] =
		{
			invView * Vec3(-1.f, -1.f, 0.f),
			invView * Vec3(+1.f, -1.f, 0.f),
			invView * Vec3(+1.f, +1.f, 0.f),
			invView * Vec3(-1.f, +1.f, 0.f),
			invView * Vec3( 0.f,  0.f, 0.f)
		};

		gxPushMatrix();
		{
			gxScalef(1.f, 1.f, 1.f);
			gxBegin(GL_LINES);
			{
				for (int i = 0; i < 4; ++i)
				{
					gxColor4f(1.f, 1.f, 1.f, alpha);
					gxVertex3f(0.f, 0.f, 0.f);
					gxVertex3f(p[i][0], p[i][1], p[i][2]);
				}
			}
			gxEnd();
		}
		gxPopMatrix();
	}
	gxMatrixMode(GL_MODELVIEW);
	gxPopMatrix();
}

static void drawScreen(const Vec3 * screenPoints, GLuint surfaceTexture, int screenId)
{
	const bool translucent = true;

	if (translucent)
	{
		//gxColor4f(1.f, 1.f, 1.f, 1.f);
		//glDisable(GL_DEPTH_TEST);

		setBlend(BLEND_ADD_OPAQUE);
	}
	else
	{
		setBlend(BLEND_OPAQUE);
	}

	setColor(colorWhite);
	gxSetTexture(surfaceTexture);
	{
		gxBegin(GL_QUADS);
		{
			gxTexCoord2f(1.f / NUM_SCREENS * (screenId + 0), 0.f); gxVertex3f(screenPoints[0][0], screenPoints[0][1], screenPoints[0][2]);
			gxTexCoord2f(1.f / NUM_SCREENS * (screenId + 1), 0.f); gxVertex3f(screenPoints[1][0], screenPoints[1][1], screenPoints[1][2]);
			gxTexCoord2f(1.f / NUM_SCREENS * (screenId + 1), 1.f); gxVertex3f(screenPoints[2][0], screenPoints[2][1], screenPoints[2][2]);
			gxTexCoord2f(1.f / NUM_SCREENS * (screenId + 0), 1.f); gxVertex3f(screenPoints[3][0], screenPoints[3][1], screenPoints[3][2]);
		}
		gxEnd();
	}
	gxSetTexture(0);

	//glEnable(GL_DEPTH_TEST);
	setBlend(BLEND_ADD);

	setColor(colorWhite);
	gxBegin(GL_LINE_LOOP);
	{
		for (int i = 0; i < 4; ++i)
			gxVertex3fv(&screenPoints[i][0]);
	}
	gxEnd();
}

enum DebugMode
{
	kDebugMode_None,
	kDebugMode_Camera,
	kDebugMode_Events
};

static DebugMode s_debugMode = kDebugMode_None;

static void setDebugMode(DebugMode mode)
{
	if (mode == s_debugMode)
		s_debugMode = kDebugMode_None;
	else
		s_debugMode = mode;
}

int main(int argc, char * argv[])
{
	//changeDirectory("data");

	if (!config.load("settings.xml"))
	{
		logError("failed to load settings.xml");
		return -1;
	}

	const int numMidiDevices = midiInGetNumDevs();

	if (numMidiDevices > 0)
	{
		printf("MIDI devices:\n");

		for (int i = 0; i < numMidiDevices; ++i)
		{
			MIDIINCAPS caps;
			memset(&caps, 0, sizeof(caps));

			if (midiInGetDevCaps(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR)
			{
				printf("device_index=%d, driver_version=%03d.%03d, name=%s\n",
					i,
					(caps.vDriverVersion & 0xff00) >> 8,
					(caps.vDriverVersion & 0x00ff) >> 0,
					caps.szPname);
			}
		}
	}

	AudioIn audioIn;

	float audioInProvideTime = 0.f;
	int audioInHistoryMaxSize = 0;
	int audioInHistorySize = 0;
	short * audioInHistory = nullptr;

	if (config.audioIn.enabled)
	{
		if (!audioIn.init(config.audioIn.deviceIndex, config.audioIn.numChannels, config.audioIn.sampleRate, config.audioIn.bufferLength))
		{
			logError("failed to initialise audio in!");
		}
		else
		{
			audioInHistoryMaxSize = config.audioIn.numChannels * config.audioIn.bufferLength;
			audioInHistory = new short[audioInHistoryMaxSize];
		}
	}

	// initialise OSC

	InitializeCriticalSectionAndSpinCount(&s_oscMessageMtx, 256);

	s_oscMessageThread = CreateThread(NULL, 64 * 1024, ExecuteOscThread, NULL, CREATE_SUSPENDED, NULL);
	ResumeThread(s_oscMessageThread);

	// initialise framework

	framework.fullscreen = false;
	//framework.windowBorder = false;
	framework.enableDepthBuffer = true;
	framework.minification = 1;
	framework.enableMidi = true;
	framework.midiDeviceIndex = config.midi.deviceIndex;

#ifdef DEBUG
	framework.reloadCachesOnActivate = true;
#endif

	if (framework.init(0, 0, GFX_SX, GFX_SY))
	{
		framework.fillCachesWithPath(".", true);

		std::list<TimeDilationEffect> timeDilationEffects;

		bool clearScreen = true;
		bool debugDraw = false;

	#if DEMODATA
		bool postProcess = false;

		bool drawRain = true;
		bool drawStarCluster = true;
		bool drawCloth = false;
		bool drawSprites = true;
		bool drawBoxes = true;
		bool drawVideo = true;
		bool drawPCM = true;
	#endif

		bool drawHelp = false;
		bool drawScreenIds = false;
		bool drawProjectorSetup = false;
		bool drawActiveEffects = false;

		Scene * scene = new Scene();
		scene->load("scene.xml");

	#if DEMODATA
		Effect_Rain rain("rain", 10000);

		Effect_StarCluster starCluster("stars", 100);
		starCluster.screenX = virtualToScreenX(0);
		starCluster.screenY = virtualToScreenY(0);

		Effect_Cloth cloth("cloth");
		cloth.setup(CLOTH_MAX_SX, CLOTH_MAX_SY);

		Effect_SpriteSystem spriteSystem("sprites");

		Effect_Boxes boxes("boxes");
		for (int b = 0; b < 2; ++b)
		{
			const float boxScale = 2.5f;
			Effect_Boxes::Box * box = boxes.addBox(0.f, 0.f, 0.f, boxScale, boxScale / 8.f, boxScale, 0);
			const float boxTimeStep = 1.f;
			for (int i = 0; i < 1000; ++i)
			{
				if ((rand() % 3) <= 1)
				{
					box->m_rx.to(random(-2.f, +2.f), boxTimeStep, (EaseType)(rand() % kEaseType_Count), random(0.f, 2.f));
					box->m_ry.to(random(-2.f, +2.f), boxTimeStep, (EaseType)(rand() % kEaseType_Count), random(0.f, 2.f));
					//box->m_rz.to(random(-2.f, +2.f), boxTimeStep, (EaseType)(rand() % kEaseType_Count), random(0.f, 2.f));
				}
				else
				{
					box->m_rx.to(box->m_rx.getFinalValue(), boxTimeStep, (EaseType)(rand() % kEaseType_Count), random(0.f, 2.f));
					box->m_ry.to(box->m_ry.getFinalValue(), boxTimeStep, (EaseType)(rand() % kEaseType_Count), random(0.f, 2.f));
					//box->m_rz.to(box->m_rz.getFinalValue(), boxTimeStep, (EaseType)(rand() % kEaseType_Count), random(0.f, 2.f));
				}

				if ((rand() % 4) <= 0)
					box->m_sy.to(random(0.f, boxScale / 4.f), boxTimeStep, (EaseType)(rand() % kEaseType_Count), random(0.f, 2.f));
				else
					box->m_sy.to(box->m_sy.getFinalValue(), boxTimeStep, (EaseType)(rand() % kEaseType_Count), random(0.f, 2.f));
			}
		}

		Effect_Video video("video");
		video.setup("doa.avi", 0.f, 0.f, 1.f, true);

		video.tweenTo("scale", 3.f, 10.f, kEaseType_PowIn, 2.f);
	#endif

		Surface surface(GFX_SX, GFX_SY);

	#if DEMODATA
		Shader jitterShader("jitter");
		Shader boxblurShader("boxblur");
		Shader luminanceShader("luminance");
		Shader flowmapShader("flowmap");
		Shader distortionBarsShader("distortion_bars");
	#endif

		Shader fxaaShader("fxaa");

		Vec3 cameraPosition(0.f, .5f, -1.f);
		Vec3 cameraRotation(0.f, 0.f, 0.f);
		Mat4x4 cameraMatrix;
		cameraMatrix.MakeIdentity();

		int activeCamera = NUM_SCREENS;

		float time = 0.f;
		float timeReal = 0.f;

		bool stop = false;

		while (!stop)
		{
			// process

			framework.process();

			// todo : process audio input

			int * samplesThisFrame = nullptr;
			int numSamplesThisFrame = 0;

			float loudnessThisFrame = 0.f;

			if (config.audioIn.enabled)
			{
				audioInHistorySize = audioInHistorySize;

				while (audioIn.provide(audioInHistory, audioInHistorySize))
				{
					//logDebug("got audio data! numSamples=%04d", audioInHistorySize);

					audioInHistorySize /= config.audioIn.numChannels;
					audioInProvideTime = framework.time;
				}

				numSamplesThisFrame = Calc::Min(config.audioIn.sampleRate / 60, audioInHistorySize);
				Assert(audioInHistorySize >= numSamplesThisFrame);

				// todo : secure this code

				samplesThisFrame = new int[numSamplesThisFrame];

				int offset = (framework.time - audioInProvideTime) * config.audioIn.sampleRate;
				Assert(offset >= 0);

				if (offset + numSamplesThisFrame > audioInHistorySize)
					offset = audioInHistorySize - numSamplesThisFrame;

				//logDebug("offset = %04d/%04d (numSamplesThisFrame=%04d)", offset, audioInHistorySize, numSamplesThisFrame);

				offset *= config.audioIn.numChannels;

				const short * __restrict src = audioInHistory + offset;
				      int   * __restrict dst = samplesThisFrame;

				for (int i = 0; i < numSamplesThisFrame; ++i)
				{
					int value = 0;

					for (int c = 0; c < config.audioIn.numChannels; ++c)
					{
						value += *src++;
					}

					*dst++ = value;

					Assert(src <= audioInHistory + audioInHistorySize * config.audioIn.numChannels);
					Assert(dst <= samplesThisFrame + numSamplesThisFrame);
				}

				// calculate loudness

				int total = 0;
				for (int i = 0; i < numSamplesThisFrame; ++i)
					total += samplesThisFrame[i];
				loudnessThisFrame = sqrtf(total / float(numSamplesThisFrame) / float(1 << 15));
			}

			// todo : process OSC input

			// todo : process direct MIDI input

			// input

			if (keyboard.wentDown(SDLK_ESCAPE))
				stop = true;

			if (keyboard.wentDown(SDLK_r))
			{
				scene->reload();
			}

		#if DEMODATA
			if (keyboard.wentDown(SDLK_a))
			{
				spriteSystem.addSprite("Diamond.scml", 0, rand() % GFX_SX, rand() % GFX_SY, 0.f, 1.f);
			}
		#endif

			if (keyboard.wentDown(SDLK_c))
				clearScreen = !clearScreen;
			if (keyboard.wentDown(SDLK_d))
				debugDraw = !debugDraw;
			if (keyboard.wentDown(SDLK_RSHIFT))
				setDebugMode(kDebugMode_Camera);

		#if DEMODATA
			if (keyboard.wentDown(SDLK_p) || config.midiWentDown(64))
				postProcess = !postProcess;

			if (keyboard.wentDown(SDLK_1))
				drawRain = !drawRain;
			if (keyboard.wentDown(SDLK_2))
				drawStarCluster = !drawStarCluster;
			if (keyboard.wentDown(SDLK_3))
				drawCloth = !drawCloth;
			if (keyboard.wentDown(SDLK_4))
				drawSprites = !drawSprites;
			if (keyboard.wentDown(SDLK_5))
				drawBoxes = !drawBoxes;
			if (keyboard.wentDown(SDLK_6))
				drawVideo = !drawVideo;
			if (keyboard.wentDown(SDLK_7))
				drawPCM = !drawPCM;
		#endif

			if (keyboard.wentDown(SDLK_F1))
				drawHelp = !drawHelp;
			if (keyboard.wentDown(SDLK_e))
				setDebugMode(kDebugMode_Events);
			if (keyboard.wentDown(SDLK_i))
				drawScreenIds = !drawScreenIds;
			if (keyboard.wentDown(SDLK_s))
				drawProjectorSetup = !drawProjectorSetup;
			if (keyboard.wentDown(SDLK_e))
				drawActiveEffects = !drawActiveEffects;

			if (s_debugMode == kDebugMode_None)
			{
			}
			else if (s_debugMode == kDebugMode_Camera)
			{
				if (keyboard.wentDown(SDLK_o))
				{
					cameraPosition = Vec3(0.f, .5f, -1.f);
					cameraRotation.SetZero();
				}

			}
			else if (s_debugMode == kDebugMode_Events)
			{
				for (size_t i = 0; i < 10 && i < scene->m_events.size(); ++i)
				{
					if (keyboard.wentDown((SDLKey)(SDLK_0 + i)))
					{
						scene->triggerEvent(scene->m_events[i]->m_name.c_str());
					}
				}
			}

			if (keyboard.wentDown(SDLK_g))
				scene->triggerEvent("fade_rain");

			SDL_SetRelativeMouseMode(s_debugMode == kDebugMode_Camera ? SDL_TRUE : SDL_FALSE);

			const float dtReal = Calc::Min(1.f / 30.f, framework.timeStep) * config.midiGetValue(100, 1.f);

			Mat4x4 cameraPositionMatrix;
			Mat4x4 cameraRotationMatrix;

			if (s_debugMode == kDebugMode_Camera && drawProjectorSetup)
			{
				cameraRotation[0] -= mouse.dy / 100.f;
				cameraRotation[1] -= mouse.dx / 100.f;

				Vec3 speed;

				if (keyboard.isDown(SDLK_RIGHT))
					speed[0] += 1.f;
				if (keyboard.isDown(SDLK_LEFT))
					speed[0] -= 1.f;
				if (keyboard.isDown(SDLK_UP))
					speed[2] += 1.f;
				if (keyboard.isDown(SDLK_DOWN))
					speed[2] -= 1.f;

				cameraPosition += cameraMatrix.CalcInv().Mul3(speed) * dtReal;

				if (keyboard.wentDown(SDLK_END))
					activeCamera = (activeCamera + 1) % (NUM_SCREENS + 1);
			}

			{
				Mat4x4 rotX;
				Mat4x4 rotY;
				rotX.MakeRotationX(cameraRotation[0]);
				rotY.MakeRotationY(cameraRotation[1]);
				cameraRotationMatrix = rotY * rotX;

				cameraPositionMatrix.MakeTranslation(cameraPosition);

				Mat4x4 invCameraMatrix = cameraPositionMatrix * cameraRotationMatrix;
				cameraMatrix = invCameraMatrix.CalcInv();
			}

			// update network input

			EnterCriticalSection(&s_oscMessageMtx);
			{
				while (!s_oscMessages.empty())
				{
					const OscMessage & message = s_oscMessages.front();

					switch (message.type)
					{
					case kOscMessageType_SetScene:
						break;

						//

				#if 0 // todo : look up sprite system and video in current scene
					case kOscMessageType_Sprite:
						{
							spriteSystem.addSprite(
								message.str.c_str(),
								0,
								virtualToScreenX(message.param[0]),
								virtualToScreenY(message.param[1]),
								0.f, message.param[2] / 100.f);
							//spriteSystem.addSprite(message.str.c_str(), 0, message.param[0], message.param[1], 0.f, message.param[2]);
						}
						break;

					case kOscMessageType_Video:
						{
							video.setup(message.str.c_str(), message.param[0], message.param[1], 1.f, true);
						}
						break;
				#endif

						//

					case kOscMessageType_TimeDilation:
						{
							TimeDilationEffect effect;
							effect.duration = message.param[0];
							effect.durationRcp = 1.f / effect.duration;
							effect.multiplier = message.param[1];
							timeDilationEffects.push_back(effect);
						}
						break;

						//

					case kOscMessageType_Swipe:
						break;

					default:
						fassert(false);
						break;
					}

					s_oscMessages.pop_front();
				}
			}
			LeaveCriticalSection(&s_oscMessageMtx);

			// figure out time dilation

			float timeDilationMultiplier = 1.f;

			for (auto i = timeDilationEffects.begin(); i != timeDilationEffects.end(); )
			{
				TimeDilationEffect & e = *i;

				const float multiplier = lerp(1.f, e.multiplier, e.duration * e.duration);

				if (multiplier < timeDilationMultiplier)
					timeDilationMultiplier = multiplier;

				e.duration = Calc::Max(0.f, e.duration - dtReal);

				if (e.duration == 0.f)
					i = timeDilationEffects.erase(i);
				else
					++i;
			}

			const float dt = dtReal * timeDilationMultiplier;

			// process effects

			scene->tick(dt);

		#if DEMODATA
			rain.tick(dt);

			starCluster.tick(dt);

			for (int i = 0; i < 10; ++i)
				cloth.tick(dt / 10.f);

			spriteSystem.tick(dt);

			boxes.tick(dt);

			video.tick(dt);

			if (mouse.isDown(BUTTON_LEFT))
			{
				const float mouseX = mouse.x / 40.f;
				const float mouseY = mouse.y / 40.f;

				for (int x = 0; x < cloth.sx; ++x)
				{
					const int y = cloth.sy - 1;

					Effect_Cloth::Vertex & v = cloth.vertices[x][y];

					const float dx = mouseX - v.x;
					const float dy = mouseY - v.y;

					const float a = x / float(cloth.sx - 1) * 30.f;

					v.vx += a * dx * dt;
					v.vy += a * dy * dt;
				}
			}
		#endif

			// draw

			DrawableList drawableList;

			scene->draw(drawableList);

		#if DEMODATA
			if (drawRain)
				rain.draw(drawableList);

			if (drawStarCluster)
				starCluster.draw(drawableList);

			if (drawCloth)
				cloth.draw(drawableList);

			if (drawSprites)
				spriteSystem.draw(drawableList);

			if (drawVideo)
				video.draw(drawableList);
		#endif

			drawableList.sort();

			framework.beginDraw(0, 0, 0, 0);
			{
				// camera setup

				Camera cameras[NUM_SCREENS];

				const float dx = +sqrtf(2.f) / 2.f; // todo : rotate the screen instead of hacking their positions
				const float dz = -sqrtf(2.f) / 2.f; // todo : rotate the screen instead of hacking their positions

			#if NUM_SCREENS == 1
				Vec3 _screenCorners[4] =
				{
					Vec3(-.66f, 0.f, 0.f),
					Vec3(+.66f, 0.f, 0.f),
					Vec3(-.66f, 1.f, 0.f),
					Vec3(+.66f, 1.f, 0.f)
				};
			#elif NUM_SCREENS == 3
				Vec3 _screenCorners[8] =
				{
					Vec3(-0.5f - dx, 0.f,  dz),
					Vec3(-0.5f,      0.f, 0.f),
					Vec3(+0.5f,      0.f, 0.f),
					Vec3(+0.5f + dx, 0.f,  dz),

					Vec3(-0.5f - dx, 1.f,  dz),
					Vec3(-0.5f,      1.f, 0.f),
					Vec3(+0.5f,      1.f, 0.f),
					Vec3(+0.5f + dx, 1.f,  dz),
				};
			#endif
				const int screenCornerStride = sizeof(_screenCorners) / sizeof(_screenCorners[0]) / 2;

				Vec3 screenCorners[NUM_SCREENS][4];

				const Vec3 cameraPosition(0.f, .5f, -1.f);

				for (int c = 0; c < NUM_SCREENS; ++c)
				{
					Camera & camera = cameras[c];

					screenCorners[c][0] = _screenCorners[c + 0 + 0],
					screenCorners[c][1] = _screenCorners[c + 1 + 0],
					screenCorners[c][2] = _screenCorners[c + 1 + screenCornerStride],
					screenCorners[c][3] = _screenCorners[c + 0 + screenCornerStride],

					camera.setup(cameraPosition, screenCorners[c], 4, c);
				}

				pushSurface(&surface);
				{
					ScopedSurfaceBlock scopedBlock(&surface);

					if (clearScreen)
					{
						glClearColor(0.f, 0.f, 0.f, 1.f);
						glClear(GL_COLOR_BUFFER_BIT);
					}
					else
					{
						// basically BLEND_SUBTRACT, but keep the alpha channel in-tact
						glEnable(GL_BLEND);
						fassert(glBlendEquation);
						if (glBlendEquation)
							glBlendEquation(GL_FUNC_REVERSE_SUBTRACT);
						glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE, GL_ZERO, GL_ONE);

						//setColorf(config.midiGetValue(102, 1.f), config.midiGetValue(102, 1.f)/2.f, config.midiGetValue(102, 1.f)/4.f, 1.f);
						//setColor(2, 2, 2, 255);
						setColor(4, 5, 31, 255);
						drawRect(0, 0, GFX_SX, GFX_SY);
					}

					setBlend(BLEND_ALPHA);

				#if 1
					for (int c = 0; c < NUM_SCREENS; ++c)
					{
						const Camera & camera = cameras[c];

						int sx;
						int sy;

						camera.beginView(c, sx, sy);
						{
							setBlend(BLEND_ADD);

							if (debugDraw)
							{
								drawTestObjects();
							}

							DrawableList drawableList;

						#if DEMODATA
							if (drawBoxes)
								boxes.draw(drawableList);
						#endif

							drawableList.sort();

							drawableList.draw();

						#if DEMODATA
							// todo : make PCM effect

							if (drawPCM && ((c == 0) || (c == 2)) && audioInHistorySize > 0)
							{
								applyTransformWithViewportSize(sx, sy);

								setColor(colorWhite);

								glLineWidth(1.5f);
								gxBegin(GL_LINES);
								{
									const float scaleX = GFX_SX / float(numSamplesThisFrame - 2);
									const float scaleY = 300.f / float(1 << 15);

									for (int i = 0; i < numSamplesThisFrame - 1; ++i)
									{
										gxVertex2f((i + 0) * scaleX, GFX_SY/2.f + samplesThisFrame[i + 0] * scaleY);
										gxVertex2f((i + 1) * scaleX, GFX_SY/2.f + samplesThisFrame[i + 1] * scaleY);
									}
								}
								gxEnd();
								glLineWidth(1.f);
							}
						#endif

							setBlend(BLEND_ALPHA);
						}
						camera.endView();
					}
				#endif

					setBlend(BLEND_ADD);
					drawableList.draw();

					setColorf(.25f, .5f, 1.f, loudnessThisFrame / 8.f);
					drawRect(0, 0, GFX_SX, GFX_SY);

				#if DEMODATA
					static volatile bool doBoxblur = false;
					static volatile bool doLuminance = false;
					static volatile bool doFlowmap = false;
					static volatile bool doDistortionBars = false;
				#endif
					static volatile bool doFxaa = false;

				#if DEMODATA
					if (postProcess)
					{
						setBlend(BLEND_OPAQUE);
						Shader & shader = jitterShader;
						shader.setTexture("colormap", 0, surface.getTexture(), true, false);
						shader.setTexture("jittermap", 1, 0, true, true);
						shader.setImmediate("jitterStrength", 1.f);
						shader.setImmediate("time", time);
						surface.postprocess(shader);
					}

					if (doBoxblur)
					{
						setBlend(BLEND_OPAQUE);
						Shader & shader = boxblurShader;
						setShader(shader);
						shader.setTexture("colormap", 0, surface.getTexture(), true, false);
						ShaderBuffer buffer;
						BoxblurData data;
						const float radius = 2.f;
						data.radiusX = radius * (1.f / GFX_SX);
						data.radiusY = radius * (1.f / GFX_SY);
						buffer.setData(&data, sizeof(data));
						shader.setBuffer("BoxblurBlock", buffer);
						surface.postprocess(shader);
					}

					if (doLuminance)
					{
						setBlend(BLEND_OPAQUE);
						Shader & shader = luminanceShader;
						setShader(shader);
						shader.setTexture("colormap", 0, surface.getTexture(), true, false);
						ShaderBuffer buffer;
						LuminanceData data;
						data.power = cosf(framework.time) + 1.f + config.midiGetValue(104, 1.f / 8.f) * 8.f;
						data.scale = 1.f * config.midiGetValue(103, 1.f);
						buffer.setData(&data, sizeof(data));
						shader.setBuffer("LuminanceBlock", buffer);
						surface.postprocess(shader);
					}

					if (doFlowmap)
					{
						setBlend(BLEND_OPAQUE);
						Shader & shader = flowmapShader;
						setShader(shader);
						shader.setTexture("colormap", 0, surface.getTexture(), true, false);
						shader.setTexture("flowmap", 0, surface.getTexture(), true, false); // todo
						shader.setImmediate("time", framework.time);
						ShaderBuffer buffer;
						FlowmapData data;
						data.strength = cosf(framework.time) * 200.f;
						buffer.setData(&data, sizeof(data));
						shader.setBuffer("FlowmapBlock", buffer);
						surface.postprocess(shader);
					}

					if (doDistortionBars)
					{
						setBlend(BLEND_OPAQUE);
						Shader & shader = distortionBarsShader;
						setShader(shader);
						shader.setTexture("colormap", 0, surface.getTexture(), true, false);
						ShaderBuffer buffer;
						DistortionBarsData data;
						Mat4x4 matR;
						Mat4x4 matT;
						matR.MakeRotationZ(framework.time * .1f);
						//matT.MakeTranslation(GFX_SX/2.f, GFX_SY/2.f, 0.f);
						matT.MakeTranslation(mouse.x, GFX_SY - mouse.y, 0.f);
						Mat4x4 mat = matT * matR;
						data.px = mat(0, 0);
						data.py = mat(0, 1);
						data.pd = mat(3, 0) * data.px + mat(3, 1) * data.py;
						data.pScale = .1f;
						data.qx = mat(1, 0);
						data.qy = mat(1, 1);
						data.qd = mat(3, 0) * data.qx + mat(3, 1) * data.qy;
						data.qScale = 1.f / 200.f;
						buffer.setData(&data, sizeof(data));
						shader.setBuffer("DistotionBarsBlock", buffer); // fixme : block name
						surface.postprocess(shader);
					}
				#endif

					if (doFxaa)
					{
						setBlend(BLEND_OPAQUE);
						Shader & shader = fxaaShader;
						setShader(shader);
						shader.setTexture("colormap", 0, surface.getTexture(), true, true);
						shader.setImmediate("inverseVP", 1.f / (surface.getWidth() / framework.minification), 1.f / (surface.getHeight() / framework.minification));
						surface.postprocess(shader);
					}
				}
				popSurface();

				// blit result to back buffer

				const GLuint surfaceTexture = surface.getTexture();

				gxSetTexture(surfaceTexture);
				{
					setBlend(BLEND_OPAQUE);
					setColor(colorWhite);
					drawRect(0, 0, GFX_SX, GFX_SY);
					setBlend(BLEND_ALPHA);
				}
				gxSetTexture(0);

				if (drawProjectorSetup)
				{
					glClearColor(0.05f, 0.05f, 0.05f, 0.f);
					glClear(GL_COLOR_BUFFER_BIT);

					setBlend(BLEND_ALPHA);

				#if 0 // todo : move to camera viewport rendering ?
					// draw projector bounds

					setColorf(1.f, 1.f, 1.f, .25f);
					for (int i = 0; i < NUM_SCREENS; ++i)
						drawRectLine(virtualToScreenX(-150 + i * 100), virtualToScreenY(0.f), virtualToScreenX(-150 + (i + 1) * 100), virtualToScreenY(100));
				#endif

					// draw 3D projector setup

					//glEnable(GL_DEPTH_TEST);
					//glDepthFunc(GL_LESS);

					{
						Mat4x4 projection;
						projection.MakePerspectiveLH(90.f * Calc::deg2rad, float(GFX_SY) / float(GFX_SX), 0.01f, 100.f);
						gxMatrixMode(GL_PROJECTION);
						gxPushMatrix();
						gxLoadMatrixf(projection.m_v);
						{
							gxMatrixMode(GL_MODELVIEW);
							gxPushMatrix();
							gxLoadMatrixf(cameraMatrix.m_v);
							{
								setBlend(BLEND_ADD);

								// draw ground

								drawGroundPlane(0.f);

								// draw the projector screens

								for (int c = 0; c < NUM_SCREENS; ++c)
								{
									drawScreen(screenCorners[c], surfaceTexture, c);
								}

								// draw test objects

								// todo : add drawScene function. this code is getting duplicated with viewport render

								if (debugDraw && keyboard.isDown(SDLK_LSHIFT))
								{
									drawTestObjects();

									DrawableList drawableList;

								#if DEMODATA
									if (drawBoxes)
										boxes.draw(drawableList);
								#endif

									drawableList.sort();

									drawableList.draw();
								}

								// draw the cameras

								for (int c = 0; c < NUM_SCREENS; ++c)
								{
									if (c < NUM_SCREENS)
									{
										const Camera & camera = cameras[c];

										drawCamera(camera, c == activeCamera ? 1.f : .1f);
									}
								}

								setBlend(BLEND_ALPHA);
							}
							gxMatrixMode(GL_MODELVIEW);
							gxPopMatrix();
						}
						gxMatrixMode(GL_PROJECTION);
						gxPopMatrix();

						gxMatrixMode(GL_MODELVIEW);
					}
					glDisable(GL_DEPTH_TEST);
				}

				if (drawScreenIds)
				{
					for (int c = 0; c < NUM_SCREENS; ++c)
					{
						const Camera & camera = cameras[c];

						int sx;
						int sy;

						camera.beginView(c, sx, sy);
						{
							setBlend(BLEND_ADD);

							applyTransformWithViewportSize(sx, sy);

							setFont("calibri.ttf");
							setColor(colorWhite);
							drawText(sx/2, sy/2, 250, 0.f, 0.f, "%d", c + 1);
						}
						camera.endView();
					}
				}

				setFont("VeraMono.ttf");
				setColor(colorWhite);
				const int spacingY = 28;
				const int fontSize = 24;
				int x = 20;
				int y = 20;

				if (drawActiveEffects)
				{
					drawText(x, y, fontSize, +1.f, +1.f, "effects list:");
					x += 50;
					y += spacingY;

					for (auto i = g_effectsByName.begin(); i != g_effectsByName.end(); ++i)
					{
						float xOld = x;

						const std::string & effectName = i->first;
						Effect * effect = i->second;

						char temp[1024];
						sprintf_s(temp, sizeof(temp), "%-20s", effectName.c_str(), effect);

						float sx, sy;
						measureText(fontSize, sx, sy, "%s", temp);

						drawText(x, y, fontSize, +1.f, +1.f, "%s", temp);

						x += sx;
						x += 4.f;

						float yNew = y;

						for (auto j : effect->m_tweenVars)
						{
							float yOld = y;

							const std::string & varName = j.first;
							TweenFloat & var = *j.second;

							drawText(x, y, fontSize, +1.f, +1.f, "%-8s", varName.c_str());
							y += spacingY;
							
							setColor(var.isActive() ? colorYellow : colorWhite);
							drawText(x, y, fontSize, +1.f, +1.f, "%.2f", (float)var);
							y += spacingY;

							x += 150.f;
							yNew = y;
							y = yOld;
						}

						x = xOld;
						y = yNew;
					}

					y += spacingY;
					x -= 50;
				}

				if (s_debugMode == kDebugMode_None)
				{
				}
				else if (s_debugMode == kDebugMode_Camera)
				{
				}
				else if (s_debugMode == kDebugMode_Events)
				{
					drawText(x, y, fontSize, +1.f, +1.f, "events list:");
					x += 50;
					y += spacingY;

					for (size_t i = 0; i < scene->m_events.size(); ++i)
					{
						drawText(x, y, fontSize, +1.f, +1.f, "%02d: %-40s", i, scene->m_events[i]->m_name.c_str());
						y += spacingY;
					}

					y += spacingY;
					x -= 50;
				}

				if (drawHelp)
				{
					drawText(x, y, fontSize, +1.f, +1.f, "Press F1 to toggle help"); y += spacingY;
					x += 50;
					drawText(x, y, fontSize, +1.f, +1.f, ""); y += spacingY;

				#if DEMODATA
					drawText(x, y, fontSize, +1.f, +1.f, "1: toggle rain effect"); y += spacingY;
					drawText(x, y, fontSize, +1.f, +1.f, "2: toggle star cluster effect"); y += spacingY;
					drawText(x, y, fontSize, +1.f, +1.f, "3: toggle cloth effect"); y += spacingY;
					drawText(x, y, fontSize, +1.f, +1.f, "4: toggle sprite effects"); y += spacingY;
					drawText(x, y, fontSize, +1.f, +1.f, "5: toggle video effects"); y += spacingY;
					drawText(x, y, fontSize, +1.f, +1.f, "");
				#endif

					drawText(x, y, fontSize, +1.f, +1.f, "E: toggle events list"); y += spacingY;
					drawText(x, y, fontSize, +1.f, +1.f, "I: identify screens"); y += spacingY;
					drawText(x, y, fontSize, +1.f, +1.f, "S: toggle project setup view"); y += spacingY;
					x += 50;
					drawText(x, y, fontSize, +1.f, +1.f, "RIGHT SHIFT: enable camera controls in project setup view"); y += spacingY;
					drawText(x, y, fontSize, +1.f, +1.f, "ARROW KEYS: move the camera around in project setup view"); y += spacingY;
					drawText(x, y, fontSize, +1.f, +1.f, "END: change the active virtual camera in project setup view"); y += spacingY;
					drawText(x, y, fontSize, +1.f, +1.f, "LEFT SHIFT: when pressed, draw test objects in 3D space"); y += spacingY;
					drawText(x, y, fontSize, +1.f, +1.f, ""); y += spacingY;
					x -= 50;
					drawText(x, y, fontSize, +1.f, +1.f, "A: spawn a Spriter effect"); y += spacingY;
					drawText(x, y, fontSize, +1.f, +1.f, "G: when pressed, enables gravity on cloth"); y += spacingY;
					drawText(x, y, fontSize, +1.f, +1.f, ""); y += spacingY;
					drawText(x, y, fontSize, +1.f, +1.f, "R: reload data caches"); y += spacingY;
					drawText(x, y, fontSize, +1.f, +1.f, "C: disable screen clear and enable a fade effect instead"); y += spacingY;
					drawText(x, y, fontSize, +1.f, +1.f, "P: toggle fullscreen shader effect"); y += spacingY;
					drawText(x, y, fontSize, +1.f, +1.f, "D: toggle debug draw"); y += spacingY;
					drawText(x, y, fontSize, +1.f, +1.f, ""); y += spacingY;
					drawText(x, y, fontSize, +1.f, +1.f, "ESCAPE: quit"); y += spacingY;

					//

					drawText(GFX_SX/2, GFX_SY/2, 32, 0.f, 0.f, "Listening for OSC messages on port %d", OSC_RECV_PORT);

					static int easeFunction = 0;
					if (keyboard.wentDown(SDLK_SPACE))
						easeFunction = (easeFunction + 1) % kEaseType_Count;
					setColor(colorWhite);
					for (int i = 0; i <= 100; ++i)
						drawCircle(GFX_SX/2 + i, GFX_SY/2 + evalEase(i / 100.f, (EaseType)easeFunction, mouse.y / float(GFX_SY) * 2.f) * 100.f, 5.f, 4);
				}
			}

			delete [] samplesThisFrame;
			samplesThisFrame = nullptr;

			framework.endDraw();

			time += dt;
			timeReal += dtReal;
		}

		delete scene;
		scene = nullptr;

		framework.shutdown();
	}

	s_oscReceiveSocket->AsynchronousBreak();
	WaitForSingleObject(s_oscMessageThread, INFINITE);
	CloseHandle(s_oscMessageThread);

	delete s_oscReceiveSocket;
	s_oscReceiveSocket = nullptr;

	//

	delete [] audioInHistory;
	audioInHistory = nullptr;
	audioInHistorySize = 0;
	audioInHistoryMaxSize = 0;

	audioIn.shutdown();

	//

	return 0;
}
