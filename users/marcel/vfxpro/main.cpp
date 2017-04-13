#include "ip/UdpSocket.h"
#include "osc/OscOutboundPacketStream.h"
#include "osc/OscPacketListener.h"
#include "audiostream/AudioOutput.h"
#include "audiofft.h"
#include "audioin.h"
#include "Calc.h"
#include "config.h"
#include "drawable.h"
#include "effect.h"
#include "FileStream.h"
#include "framework.h"
#include "leap/Leap.h"
#include "Path.h"
#include "scene.h"
#include "StringEx.h"
#include "Timer.h"
#include "tinyxml2.h"
#include "types.h"
#include "video.h"
#include "xml.h"
#include <algorithm>
#include <list>
#include <map>
#include <sys/stat.h>

#ifdef WIN32
	#include <Windows.h>
#endif

#if ENABLE_VIDEO
	#if defined(MACOS)
		#include "mediaplayer_new/MPContext.h"
		#include "mediaplayer_new/MPUtil.h"
	#else
		#include "mediaplayer_old/MPContext.h"
		#include "mediaplayer_old/MPUtil.h"
	#endif
#endif

#ifdef MACOS
	//#include <stat.h>
#endif

#include "data/ShaderConstants.h"

#include "BezierPath.h" // fixme

#if defined(WIN32) && !defined(DEBUG) && !ENABLE_LOADTIME_PROFILING
	#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

using namespace tinyxml2;

#define NUM_SCREENS 1
#define ENABLE_3D 0

const int SCREEN_SX = (1024 / NUM_SCREENS);
const int SCREEN_SY = 768;

int GFX_SX = (SCREEN_SX * NUM_SCREENS);
int GFX_SY = (SCREEN_SY * 1);
const int GFX_SCALE = ENABLE_UPSCALING ? 2 : 1;
int GFX_SX_SCALED = GFX_SX * GFX_SCALE;
int GFX_SY_SCALED = GFX_SY * GFX_SCALE;

#define OSC_ADDRESS "127.0.0.1"
#define OSC_RECV_PORT 8000

Config config;

Scene * g_scene = nullptr;
Scene * g_prevScene = nullptr;
float g_prevSceneTime = 0.f;
float g_prevSceneTimeRcp = 0.f;

float g_pcmVolume = 0.f;
GLuint g_pcmTexture = 0;
GLuint g_fftTexture = 0;
GLuint g_fftTextureWithFade = 0;

void nextScene(const char * filename)
{
	logLoadtime(0, "----------------------------------------");

	const uint64_t t1_new = loadtimer();

	Scene * scene = new Scene();

	const uint64_t t2_load = loadtimer();

	if (!scene->load(filename))
	{
		delete scene;
		scene = nullptr;
	}
	
	const uint64_t t3_delete = loadtimer();

	if (scene)
	{
		delete g_prevScene;
		g_prevScene = nullptr;

		g_prevScene = g_scene;
		g_scene = scene;
	}

	const uint64_t t4_end = loadtimer();

	logLoadtime(t2_load   - t1_new,    "Scene::new");
	logLoadtime(t3_delete - t2_load,   "Scene::load");
	logLoadtime(t4_end    - t3_delete, "Scene::delete");

	const float kTransitionTime = 4.f;
	g_prevSceneTime = kTransitionTime;
	g_prevSceneTimeRcp = 1.f / kTransitionTime;
}

void reloadScene()
{
	const std::string filename = g_scene->m_filename;

	delete g_scene;
	g_scene = nullptr;

	g_scene = new Scene();

	if (!g_scene->load(filename.c_str()))
	{
		delete g_scene;
		g_scene = nullptr;

		g_scene = new Scene();
	}
}

float virtualToScreenX(const float x)
{
#if NUM_SCREENS == 1
	//return ((x / 100.f) + 1.5f) * SCREEN_SX / 3.f;
	return x + GFX_SX/2;
#elif NUM_SCREENS == 3
	return ((x / 100.f) + 1.5f) * SCREEN_SX;
#endif
}

float virtualToScreenY(const float y)
{
	//return (y / 100.f + .5f) * SCREEN_SY;
	return y + GFX_SY/2;
}

float screenXToVirtual(const float x)
{
#if NUM_SCREENS == 1
	return x - GFX_SX/2;
	//return (x * 3.f / SCREEN_SX - 1.5f) * 100.f;
#elif NUM_SCREENS == 3
	return (x / SCREEN_SX - 1.5f) * 100.f;
#endif
}

float screenYToVirtual(const float y)
{
	return y - GFX_SY/2;
	//return (y / SCREEN_SY - .5f) * 100.f;
}

static void applyGfxTransform()
{
	gxScalef(GFX_SCALE, GFX_SCALE, 1.f);
	gxTranslatef(+GFX_SX/2, 0.f, 0.f);
	gxScalef(config.display.mirror ? -1.f : +1.f, 1.f, 0.f);
	gxTranslatef(-GFX_SX/2, 0.f, 0.f);
}

static std::vector<SceneEffect*> buildEffectsList()
{
	std::vector<SceneEffect*> result;

	for (auto layer : g_scene->m_layers)
	{
		for (auto effect : layer->m_effects)
		{
			result.push_back(effect);
		}
	}

	std::sort(result.begin(), result.end(), [](SceneEffect * e1, SceneEffect * e2) { return e1->m_name < e2->m_name; });

	return result;
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
	+ define scene XML representation
	+ discuss with Max what would be needed for life act

:: todo :: projector output

	- add brightness control
		+ write shader which does a lookup based on the luminance of the input and transforms the input
		- add ability to change the setting
	- add border blends to hide projector seam. unless eg MadMapper already does this, it may be necessary to do it ourselvess

:: todo :: utility functions

	+ add PCM capture
	+ add FFT calculation PCM data
	+ add loudness calculation PCM data

:: todo :: post processing and graphics quality

	- smooth line drawing with high AA. use a post process pass to blur the result ?
	+ add FXAA post process

:: todo :: visuals tech 2D

	+ add a box blur shader. allow it to darken the output too

	- integrate Box2D ?

	+ add flow map shader
		+ use an input texture to warp/distort the previous frame

	- add film grain shader

	- picture effect : add blend mode parameter. mul mode will allow us to mask parts

	- mask effect : add an FSFX shader that generates a mask

	- circle effect : draw concentric circles. increase in size depending on beat and audio volume

	- laser line rendering

	- UV displacement flow map thingy

:: todo :: visuals tech 3D

	- virtual camera positioning
		- allow positioning of the virtual camera based on settings XML
		- allow tweens on the virtual camera position ?

	+ compute virtual camera matrices

	+ add lighting shader code

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

:: shaders

	- add power tween value to vignette falloff
	- rename vignette distance to falloff and inner_radius to radius


*/

//

static SDL_mutex * s_oscMessageMtx = nullptr;
static SDL_Thread * s_oscMessageThread = nullptr;

enum OscMessageType
{
	kOscMessageType_None,
	// scene :: constantly reinforced
	kOscMessageType_SceneReload,
	kOscMessageType_SceneAdvanceTo,
	kOscMessageType_SceneSyncTime,
	// events
	kOscMessageType_Event,
	kOscMessageType_ReplayEvent,
	// time effect
	kOscMessageType_TimeDilation,
	// vfxedit
	kOscMessageType_AudioBegin,
	kOscMessageType_AudioEnd
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
	virtual void ProcessBundle(const osc::ReceivedBundle & b, const IpEndpointName & remoteEndpoint) override
	{
		//logDebug("ProcessBundle: timeTag=%llu", b.TimeTag());

		osc::OscPacketListener::ProcessBundle(b, remoteEndpoint);
	}

	virtual void ProcessMessage(const osc::ReceivedMessage & m, const IpEndpointName & remoteEndpoint) override
	{
		try
		{
			//logDebug("ProcessMessage");

			osc::ReceivedMessageArgumentStream args = m.ArgumentStream();

			OscMessage message;

			if (strcmp(m.AddressPattern(), "/scene_reload") == 0)
			{
				message.type = kOscMessageType_SceneReload;
			}
			else if (strcmp(m.AddressPattern(), "/scene_sync_time") == 0)
			{
				// timeMs
				osc::int32 timeMs;
				args >> timeMs;

				message.type = kOscMessageType_SceneSyncTime;
				message.param[0] = timeMs;
			}
			else if (strcmp(m.AddressPattern(), "/scene_advance_to") == 0)
			{
				// timeMs
				osc::int32 timeMs;
				args >> timeMs;

				message.type = kOscMessageType_SceneAdvanceTo;
				message.param[0] = timeMs;
			}
			else if (strcmp(m.AddressPattern(), "/audio_begin") == 0)
			{
				message.type = kOscMessageType_AudioBegin;
			}
			else if (strcmp(m.AddressPattern(), "/audio_end") == 0)
			{
				message.type = kOscMessageType_AudioEnd;
			}
			else if (strcmp(m.AddressPattern(), "/event_replay") == 0)
			{
				// eventId
				osc::int32 eventId;
				args >> eventId;

				message.type = kOscMessageType_ReplayEvent;
				message.param[0] = eventId;
			}
			else if (strcmp(m.AddressPattern(), "/event") == 0 || true)
			{
				// eventId
				osc::int32 eventId;
				args >> eventId;

				message.type = kOscMessageType_Event;
				message.param[0] = eventId;
				message.str = std::string(m.AddressPattern()).substr(1);
			}
			else
			{
				logWarning("unknown message type: %s", m.AddressPattern());
			}

			if (message.type != kOscMessageType_None)
			{
				SDL_LockMutex(s_oscMessageMtx);
				{
					logDebug("enqueue OSC message. type=%d, id=%d", message.type, (int)message.param[0]);

					s_oscMessages.push_back(message);
				}
				SDL_UnlockMutex(s_oscMessageMtx);
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

static int ExecuteOscThread(void * data)
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

#if ENABLE_3D

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

#endif

#if ENABLE_3D

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

#endif

#if ENABLE_DEBUG_MENUS

enum DebugMode
{
	kDebugMode_None,
	kDebugMode_Help,
#if ENABLE_3D
	kDebugMode_Camera,
#endif
	kDebugMode_EventList,
	kDebugMode_EffectList,
	kDebugMode_EffectListCondensed,
	kDebugMode_LayerList
};

static DebugMode s_debugMode = kDebugMode_None;

static void setDebugMode(DebugMode mode)
{
	if (mode == s_debugMode)
		s_debugMode = kDebugMode_None;
	else
		s_debugMode = mode;
}

#endif

static void handleAction(const std::string & action, const Dictionary & args)
{
	if (action == "filedrop")
	{
		const std::string filename = args.getString("file", "");

		nextScene(filename.c_str());
	}
}

//

#if ENABLE_RESOURCE_PRECACHE

static void fillCachesCallback(float filePercentage)
{
	framework.process();

	framework.beginDraw(0, 0, 0, 0);
	{
		setColor(31, 31, 31);
		drawRect(0, 0, GFX_SX * filePercentage, GFX_SY);

		setColor(colorWhite);
		setFont("calibri.ttf");
		drawText(GFX_SX/2, GFX_SY/2, 32, 0.f, 0.f, "loading.. %d%%", (int)std::round(filePercentage * 100.f));
	}
	framework.endDraw();
}

#endif

//

#if ENABLE_RESOURCE_PRECACHE

static void preloadResourceFiles()
{
	const std::vector<std::string> files = listFiles(".", true);

	for (const auto & filename : files)
	{
		const std::string baseName = Path::GetFileName(filename);

		if (String::StartsWith(baseName, "fsfx_") && String::EndsWith(baseName, ".ps"))
		{
			Shader(filename.c_str(), "fsfx.vs", filename.c_str());
		}
	}
}

#endif

//

#if ENABLE_RESOURCE_PRECACHE

static std::map<std::string, Array<uint8_t>> s_sceneFiles;

static void preloadSceneFiles()
{
	const std::vector<std::string> files = listFiles("tracks", false);

	for (auto & filename : files)
	{
		if (String::EndsWith(filename, ".scene.xml"))
		{
			try
			{
				FileStream stream(filename.c_str(), OpenMode_Read);
				const int length = stream.Length_get();

				const std::string name = String::ToLower(Path::GetFileName(filename));
				Array<uint8_t> & bytes = s_sceneFiles[name];

				bytes.resize(length, false);
				if (stream.Read(bytes.data, length) != length)
				{
					logError("failed to read file contents for %s", filename.c_str());

					s_sceneFiles.erase(name);
				}
			}
			catch (std::exception & e)
			{
				logError("%s", e.what());
			}
		}
	}
}

bool getSceneFileContents(const std::string & filename, Array<uint8_t> *& out_bytes)
{
	const std::string name = String::ToLower(Path::GetFileName(filename));

	const auto i = s_sceneFiles.find(name);

	if (i == s_sceneFiles.end())
	{
		out_bytes = nullptr;

		return false;
	}
	else
	{
		out_bytes = &i->second;

		return true;
	}
}

#else

bool getSceneFileContents(const std::string & filename, Array<uint8_t> *& out_bytes)
{
	return false;
}

#endif

//

#if ENABLE_REALTIME_EDITING

static void handleRealTimeEdit(const std::string & filename)
{
	if (filename == "settings.xml")
		config.load(filename.c_str());
	else if (filename == "effects_meta.xml")
		g_effectInfosByName.load(filename.c_str());
	else if (filename == g_scene->m_filename)
		reloadScene();
	else
	{
		const std::string extension = Path::GetExtension(filename);

		if (extension == "ps")
		{
			const std::string baseName = Path::GetBaseName(filename);

			if (String::StartsWith(baseName, "fsfx_"))
			{
				Shader(filename.c_str(), "fsfx.vs", filename.c_str()).reload();
			}
		}
		else if (extension == "inc")
		{
			clearCaches(CACHE_SHADER);
		}
	}
}

#endif

//

#if ENABLE_LEAPMOTION

LeapState g_leapState;

class LeapListener : public Leap::Listener
{
	SDL_mutex * mutex;
	LeapState shadowState;

public:
	LeapListener()
		: mutex(nullptr)
	{
		mutex = SDL_CreateMutex();
	}

	~LeapListener()
	{
		SDL_DestroyMutex(mutex);
		mutex = nullptr;
	}

	void tick()
	{
		SDL_LockMutex(mutex);
		{
			g_leapState = shadowState;
		}
		SDL_UnlockMutex(mutex);
	}

	virtual void onFrame(const Leap::Controller & controller)
	{
		SDL_LockMutex(mutex);
		{
			auto frame = controller.frame(0);
			auto hands = frame.hands();
			if (!hands.isEmpty())
			{
				// todo : track whether it's the same hand ?

				auto & hand = *hands.begin();
				auto palmPosition = hand.palmPosition();
				shadowState.hasPalm = true;
				shadowState.palmX = palmPosition.x;
				shadowState.palmY = palmPosition.y;
				shadowState.palmZ = palmPosition.z;
			}
			else
			{
				shadowState.hasPalm = false;
			}
		}
		SDL_UnlockMutex(mutex);
	}
};

#endif

//

static void showTestImage()
{
	while (keyboard.isIdle())
	{
		framework.process();

		framework.beginDraw(0, 0, 0, 0);
		{
			gxPushMatrix();
			{
				applyGfxTransform();

				setBlend(BLEND_ALPHA);

				//setColor(colorWhite);
				//Sprite("testimage.jpg").draw();

				setColor(0, 31, 31);
				drawRect(0, GFX_SY/2-30, GFX_SX, GFX_SY/2+30);

				setColor(colorWhite);
				setFont("calibri.ttf");
				if (std::fmodf(framework.time, 1.f) < .5f)
					drawText(GFX_SX/2, GFX_SY/2-8 + 250, 32, 0.f, 0.f, "press any key to start visualizing");
			}
			gxPopMatrix();

			//

			Mat4x4 matP;
			matP.MakePerspectiveLH(Calc::RadToDeg(90.f), GFX_SY/float(GFX_SX), .01f, 10000.f);

			setTransform3d(matP);
			setTransform(TRANSFORM_3D);

			gxPushMatrix();
			{
				glEnable(GL_LINE_SMOOTH);
				glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

				Sprite sprite("testimage.jpg");

				sprite.pivotX = sprite.getWidth()/2;
				sprite.pivotY = sprite.getHeight()/2;

				gxTranslatef(0.f, 0.f, 1000.f);

				for (int i = 0; i < 200; ++i)
				{
					gxPushMatrix();
					{
						gxTranslatef(i * 10, i * 10, i * 10);

						const float rotOffset = i * 3;
						const float rotSpeed = 2.f;// * std::sinf(framework.time);
						const float scale = 1.f - i / 100.f / 2.f;

						gxRotatef(rotOffset + framework.time * rotSpeed * 1.234f, 1, 0, 0);
						gxRotatef(rotOffset + framework.time * rotSpeed * 2.345f, 0, 1, 0);
						gxRotatef(rotOffset + framework.time * rotSpeed * 3.456f, 0, 0, 1);
						gxScalef(scale, scale, scale);

						setBlend(BLEND_ALPHA);

						setColor(colorWhite);

						gxScalef(400, 400, 400);
						drawRectLine(-1, -1, +1, +1);
					}
					gxPopMatrix();
				}

				glDisable(GL_LINE_SMOOTH);
			}
			gxPopMatrix();

			setTransform(TRANSFORM_SCREEN);
		}
		framework.endDraw();
	}
}

//

#if defined(DEBUG)

static const char * getRandomSceneName(int rng)
{
	const char * filenames[] =
	{
		"tracks/ChangeItAll.scene.xml",
		"tracks/ElevateLove.scene.xml",
		"tracks/Healer.scene.xml",
		"tracks/Heroes.scene.xml",
		"tracks/intro.scene.xml",
		"tracks/LetMeBe.scene.xml",
		"tracks/OnTopOfYou.scene.xml",
		"tracks/Oxygen.scene.xml",
		"tracks/Sarayani.scene.xml",
		"tracks/WhereAreYouNow.scene.xml"
	};

	const int numScenes = (sizeof(filenames) / sizeof(filenames[0]));

	const int index = rng % numScenes;

	return filenames[index];
}

#endif

#if ENABLE_STRESS_TEST

static void doStressTest()
{
	for (int i = 0; i < 1000000 * 1 + 0 && !keyboard.wentDown(SDLK_RSHIFT); ++i)
	{
		framework.process();

		const int c1 = GetAllocState().allocationCount;
		g_scene->load(getRandomSceneName(rand()));
		//g_scene->load(getRandomSceneName(i));

		for (int i = 0; i < 1; ++i)
			g_scene->triggerEventByOscId(i);

		const double step = 1.0 / 60.0;
		double trackTime = 0.0;
		double eventTime = 0.0;
		double drawTime = 0.0;

		while (trackTime < 180.0 && !keyboard.wentDown(SDLK_RSHIFT))
		{
			if (keyboard.wentDown(SDLK_SPACE))
				break;

			trackTime += step;
			eventTime += step;
			drawTime += step;

			g_scene->tick(step);

			if (eventTime >= 1.0)
			{
				eventTime = 0.0;

				g_scene->triggerEventByOscId(rand() % 32);
			}

			if (drawTime >= 0.25)
			{
				drawTime = 0.0;

				DrawableList drawableList;

				g_scene->draw(drawableList);

				drawableList.sort();

				framework.process();

				framework.beginDraw(0, 0, 0, 0);
				{
					gxPushMatrix();
					{
						gxScalef(GFX_SCALE, GFX_SCALE, 1.f);

						setBlend(BLEND_ADD);
						drawableList.draw();
					}
					gxPopMatrix();
				}
				framework.endDraw();
			}
		}

		delete g_scene;
		g_scene = new Scene();
		const int c2 = GetAllocState().allocationCount;
	}
}

#endif

//

int main(int argc, char * argv[])
{
	if (!config.load("settings.xml"))
	{
		logError("failed to load: settings.xml");
		return -1;
	}
	
	GFX_SX = config.display.sx;
	GFX_SY = config.display.sy;
	GFX_SX_SCALED = GFX_SX * GFX_SCALE;
	GFX_SY_SCALED = GFX_SY * GFX_SCALE;

	if (!g_effectInfosByName.load("effects_meta.xml"))
	{
		logError("failed to load: effects_meta.xml");
		return -1;
	}

#if ENABLE_MIDI
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
#endif

	AudioIn audioIn;

	float audioInProvideTime = 0.f;
	int audioInHistoryMaxSize = 0;
	int audioInHistorySize = 0;
	AudioSample * audioInHistory = nullptr;

	if (config.audioIn.enabled)
	{
		if (!audioIn.init(config.audioIn.deviceIndex, config.audioIn.numChannels, config.audioIn.sampleRate, config.audioIn.bufferLength))
		{
			logError("failed to initialise audio in!");
		}
		else
		{
			audioInHistoryMaxSize = config.audioIn.bufferLength;
			audioInHistory = new AudioSample[audioInHistoryMaxSize];
		}
	}

#if ENABLE_LEAPMOTION
	// initialise LeapMotion controller

	Leap::Controller leapController;
	leapController.setPolicy(Leap::Controller::POLICY_BACKGROUND_FRAMES);

	LeapListener * leapListener = new LeapListener;
	leapController.addListener(*leapListener);
#endif

	// initialise FFT

	fftInit();

	// initialise OSC

	s_oscMessageMtx = SDL_CreateMutex();
	s_oscMessageThread = SDL_CreateThread(ExecuteOscThread, "OSC thread", nullptr);

	// initialize avcodec

#if ENABLE_VIDEO
	MP::Util::InitializeLibAvcodec();
#endif

	// initialise framework

#if ENABLE_WINDOWED_MODE || 0
	framework.fullscreen = false;
	framework.minification = 1;
	framework.windowX = 0;
	framework.windowY = 60;
#else
	framework.fullscreen = true;
	framework.exclusiveFullscreen = false;
	framework.useClosestDisplayMode = true;
#endif

#if ENABLE_3D
	framework.enableDepthBuffer = true;
#endif

	framework.enableMidi = ENABLE_MIDI;
	framework.midiDeviceIndex = config.midi.deviceIndex;

#ifdef DEBUG
	//framework.reloadCachesOnActivate = true;
#endif

#if ENABLE_REALTIME_EDITING
	framework.enableRealTimeEditing = true;
	framework.realTimeEditCallback = handleRealTimeEdit;
#endif

	framework.filedrop = true;
	framework.actionHandler = handleAction;

	if (framework.init(0, 0, GFX_SX * GFX_SCALE, GFX_SY * GFX_SCALE))
	{
	#if ENABLE_RESOURCE_PRECACHE
		framework.fillCachesCallback = fillCachesCallback;
		framework.fillCachesWithPath(".", true);
		preloadResourceFiles();
		//preloadSceneFiles();
	#endif

	#if !ENABLE_DEBUG_INFOS
		if (framework.fullscreen)
			SDL_ShowCursor(0);
	#endif

		g_sceneSurfacePool = new SceneSurfacePool(16);

		Assert(g_pcmTexture == 0);
		glGenTextures(1, &g_pcmTexture);

		Assert(g_fftTexture == 0);
		glGenTextures(1, &g_fftTexture);
		
		Assert(g_fftTextureWithFade == 0);
		glGenTextures(1, &g_fftTextureWithFade);

		std::list<TimeDilationEffect> timeDilationEffects;

		bool debugDraw = false;

		bool drawScreenIds = false;
		bool drawProjectorSetup = false;

	#if ENABLE_REALTIME_EDITING
		bool fileMonitorEnabled = true;
	#endif

		if (config.display.showTestImage)
			showTestImage();
		
		g_prevScene = new Scene();

		g_scene = new Scene();

	#if ENABLE_STRESS_TEST || 0
		doStressTest();
	#endif
	
	#if 1
		nextScene("tracks/Wobbly.scene.xml");
	#endif

		Surface prevSurface(GFX_SX, GFX_SY, true);
		Surface surface(GFX_SX, GFX_SY, true);

	#if ENABLE_3D
		Vec3 cameraPosition(0.f, .5f, -1.f);
		Vec3 cameraRotation(0.f, 0.f, 0.f);
		Mat4x4 cameraMatrix;
		cameraMatrix.MakeIdentity();

		int activeCamera = NUM_SCREENS;
	#endif

		bool stop = false;

		bool showCredits = false;
		float showCreditsAlpha = 0.f;
		
		bool showUi = true;
		
		while (!stop)
		{
			// process

			framework.process();
			
			if (keyboard.wentDown(SDLK_TAB))
			{
				showUi = !showUi;
			}
			
			if (keyboard.wentDown(SDLK_b))
			{
				showCredits = !showCredits;
				showCreditsAlpha = 0.f;
			}

		#if ENABLE_REALTIME_EDITING
			if (keyboard.wentDown(SDLK_f))
			{
				fileMonitorEnabled = !fileMonitorEnabled;
			}
		#endif

			// process audio input

			float * samplesThisFrame = nullptr;
			int numSamplesThisFrame = 0;

			float loudnessThisFrame = 0.f;

			if (config.audioIn.enabled)
			{
				audioInHistorySize = audioInHistoryMaxSize;

				while (audioIn.provide(audioInHistory, audioInHistorySize))
				{
					//logDebug("got audio data! numSamples=%04d", audioInHistorySize);

					audioInProvideTime = framework.time;
				}

				// todo : increment depending on time passed, not not 1/60 assuming 60 fps
				numSamplesThisFrame = Calc::Min(config.audioIn.sampleRate / 60, audioInHistorySize);
				Assert(audioInHistorySize >= numSamplesThisFrame);

				// todo : secure this code

				samplesThisFrame = new float[numSamplesThisFrame];

				int offset = (framework.time - audioInProvideTime) * config.audioIn.sampleRate;
				Assert(offset >= 0);
				if (offset < 0)
					offset = 0;
				if (offset + numSamplesThisFrame > audioInHistorySize)
					offset = audioInHistorySize - numSamplesThisFrame;

				//logDebug("offset = %04d/%04d (numSamplesThisFrame=%04d)", offset, audioInHistorySize, numSamplesThisFrame);

				const AudioSample * __restrict src = audioInHistory + offset;
				float             * __restrict dst = samplesThisFrame;

				fftProvide(src, numSamplesThisFrame, framework.time);

				const float sampleToFloat = 1.f / (1 << 15) / 2.f * 10.f;

				for (int i = 0; i < numSamplesThisFrame; ++i)
				{
					int value = 0;

					value += src->channel[0];
					value += src->channel[1];
					++src;

					*dst++ = value * sampleToFloat;

					Assert(src <= audioInHistory + audioInHistorySize);
					Assert(dst <= samplesThisFrame + numSamplesThisFrame);
				}
			}

			// calculate loudness

			if (numSamplesThisFrame > 0)
			{
				float total = 0;
				for (int i = 0; i < numSamplesThisFrame; ++i)
					total += std::abs(samplesThisFrame[i]);
				loudnessThisFrame = sqrtf(total / float(numSamplesThisFrame));
			}
			else
			{
				loudnessThisFrame = 0.f;
			}

			g_pcmVolume = loudnessThisFrame;

			// process FFT

			fftProcess(framework.time);

		#if ENABLE_LEAPMOTION
			// process LeapMotion input

			leapListener->tick();
		#endif

			// input

		#if defined(DEBUG) || 1
			if (keyboard.wentDown(SDLK_ESCAPE))
				stop = true;
		#else
			if (keyboard.wentDown(SDLK_ESCAPE) && keyboard.isDown(SDLK_RSHIFT))
				stop = true;
		#endif

			if (keyboard.wentDown(SDLK_r))
			{
				reloadScene();
			}

		#if defined(DEBUG)
			if (keyboard.wentDown(SDLK_t))
			{
				nextScene(getRandomSceneName(rand()));
			}
		#endif

			if (keyboard.wentDown(SDLK_d))
				debugDraw = !debugDraw;

			if (keyboard.wentDown(SDLK_m))
				config.display.mirror = !config.display.mirror;
			if (keyboard.wentDown(SDLK_n))
				config.display.showScaleOverlay = !config.display.showScaleOverlay;

		#if ENABLE_DEBUG_MENUS && ENABLE_3D
			if (keyboard.wentDown(SDLK_RSHIFT))
				setDebugMode(kDebugMode_Camera);
		#endif

		#if ENABLE_DEBUG_MENUS
			if (keyboard.wentDown(SDLK_F1))
			{
				if (s_debugMode == kDebugMode_Help)
					setDebugMode(kDebugMode_None);
				else
					setDebugMode(kDebugMode_Help);
			}

			if (keyboard.wentDown(SDLK_e))
			{
				if (s_debugMode == kDebugMode_EventList)
					setDebugMode(kDebugMode_EffectList);
				else if (s_debugMode == kDebugMode_EffectList)
					setDebugMode(kDebugMode_EffectListCondensed);
				else if (s_debugMode == kDebugMode_EffectListCondensed)
					setDebugMode(kDebugMode_None);
				else
					setDebugMode(kDebugMode_EventList);
			}

			if (keyboard.wentDown(SDLK_l))
				setDebugMode(kDebugMode_LayerList);

			if (keyboard.wentDown(SDLK_i))
				drawScreenIds = !drawScreenIds;
			if (keyboard.wentDown(SDLK_s))
				drawProjectorSetup = !drawProjectorSetup;

			if (s_debugMode == kDebugMode_None)
			{
			}
		#if ENABLE_3D
			else if (s_debugMode == kDebugMode_Camera)
			{
				if (keyboard.wentDown(SDLK_o))
				{
					cameraPosition = Vec3(0.f, .5f, -1.f);
					cameraRotation.SetZero();
				}

			}
		#endif
			else if (s_debugMode == kDebugMode_EffectList || s_debugMode == kDebugMode_EffectListCondensed)
			{
				const int base = keyboard.isDown(SDLK_LSHIFT) ? 10 : 0;

				int index = 0;

				const auto effects = buildEffectsList();

				for (auto i = effects.begin(); i != effects.end(); ++i, ++index)
				{
					if (index - base < 0 || index - base > 9)
						continue;

					if (keyboard.wentDown((SDLKey)(SDLK_0 + index - base)))
					{
						Effect * effect = (*i)->m_effect;

						effect->debugEnabled = !effect->debugEnabled;
					}
				}
			}
			else if (s_debugMode == kDebugMode_EventList)
			{
				const int base = keyboard.isDown(SDLK_LSHIFT) ? 10 : 0;

				for (size_t i = 0; i < g_scene->m_events.size(); ++i)
				{
					if (int(i) - base < 0 || int(i) - base > 9)
						continue;

					if (keyboard.wentDown((SDLKey)(SDLK_0 + i - base)))
					{
						g_scene->triggerEvent(g_scene->m_events[i]->m_name.c_str());
					}
				}
			}
			else if (s_debugMode == kDebugMode_LayerList)
			{
				const int base = keyboard.isDown(SDLK_LSHIFT) ? 10 : 0;

				for (int i = 0; i < (int)g_scene->m_layers.size(); ++i)
				{
					if (i - base < 0 || i - base > 9)
						continue;

					if (keyboard.wentDown((SDLKey)(SDLK_0 + i - base)))
					{
						SceneLayer * layer = g_scene->m_layers[i];

						layer->m_debugEnabled = !layer->m_debugEnabled;
					}
				}
			}
		#endif

		#if ENABLE_DEBUG_MENUS && ENABLE_3D
			SDL_SetRelativeMouseMode(s_debugMode == kDebugMode_Camera ? SDL_TRUE : SDL_FALSE);
		#else
			SDL_SetRelativeMouseMode(SDL_FALSE);
		#endif

			const float dtReal = framework.timeStep;

		#if ENABLE_3D
			Mat4x4 cameraPositionMatrix;
			Mat4x4 cameraRotationMatrix;
		#endif

		#if ENABLE_DEBUG_MENUS && ENABLE_3D
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
		#endif

		#if ENABLE_3D
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
		#endif

			// update network input

			SDL_LockMutex(s_oscMessageMtx);
			{
				while (!s_oscMessages.empty())
				{
					const OscMessage & message = s_oscMessages.front();

					switch (message.type)
					{
					case kOscMessageType_SceneReload:
						reloadScene();
						break;

					case kOscMessageType_SceneAdvanceTo:
						g_scene->advanceTo(message.param[0] / 1000.f);
						break;

					case kOscMessageType_SceneSyncTime:
						g_scene->syncTime(message.param[0] / 1000.f);
						break;

						//

					case kOscMessageType_Event:
						{
							const bool isFullname = message.str.find('/') != std::string::npos;

							const std::string filename =
								isFullname
								? message.str + ".scene.xml"
								: std::string("tracks/") + message.str + ".scene.xml";
							
							if (filename != g_scene->m_filename)
							{
								logDebug("scene change detected. transitioning from %s to %s", g_scene->m_filename.c_str(), filename.c_str());

								nextScene(filename.c_str());
							}
							else if (message.param[0] == 0 && g_scene->m_time >= 1.f)
							{
								logDebug("scene restart detected. transitioning from %s to %s", g_scene->m_filename.c_str(), filename.c_str());

								nextScene(filename.c_str());
							}

							g_scene->triggerEventByOscId(message.param[0]);
						}
						break;

					case kOscMessageType_ReplayEvent:
						g_isReplay = true;
						g_scene->triggerEventByOscId(message.param[0]);
						g_isReplay = false;
						break;

						/*
					case kOscMessageType_AudioBegin:
						{
							INPUT in;
							memset(&in, 0, sizeof(in));
							in.type = INPUT_KEYBOARD;
							in.ki.wVk = VK_F12;
							SendInput(1, &in, sizeof(in));

							in.ki.dwFlags = KEYEVENTF_KEYUP;
							SendInput(1, &in, sizeof(in));
						}
						break;

					case kOscMessageType_AudioEnd:
						{
							INPUT in;
							memset(&in, 0, sizeof(in));
							in.type = INPUT_KEYBOARD;
							in.ki.wVk = VK_F12;
							SendInput(1, &in, sizeof(in));

							in.ki.dwFlags = KEYEVENTF_KEYUP;
							SendInput(1, &in, sizeof(in));
						}
						break;
						*/
					case kOscMessageType_AudioBegin:
					case kOscMessageType_AudioEnd:
						break;

					default:
						fassert(false);
						break;
					}

					s_oscMessages.pop_front();
				}
			}
			SDL_UnlockMutex(s_oscMessageMtx);

		#if 1
			const float timeDilationMultiplier = 1.f;
		#else
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
		#endif

			float dtTodo = dtReal;

			while (dtTodo > 0.f)
			{
				const float dtMin = 1.f / 30.f;

				float dt;

				if (dtTodo > dtMin)
				{
					dt = dtMin;
					dtTodo -= dt;
				}
				else
				{
					dt = dtTodo;
					dtTodo = 0.f;
				}

				dt *= timeDilationMultiplier;

				// process effects

				if (g_prevScene != nullptr)
				{
					g_prevScene->tick(dt);

					g_prevSceneTime -= dt;
					if (g_prevSceneTime < 0.f)
					{
						g_prevSceneTime = 0.f;
						
						delete g_prevScene;
						g_prevScene = nullptr;
					}
				}

				g_scene->tick(dt);
			}

			// draw

			// convert PCM data to shader input texture

			glBindTexture(GL_TEXTURE_2D, g_pcmTexture);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, numSamplesThisFrame, 1, 0, GL_RED, GL_FLOAT, samplesThisFrame);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glBindTexture(GL_TEXTURE_2D, 0);
			checkErrorGL();

			// convert FFT data to shader input texture

			glBindTexture(GL_TEXTURE_2D, g_fftTexture);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			float powerValues[kFFTComplexSize];
			for (int i = 0; i < kFFTComplexSize; ++i)
				powerValues[i] = fftPowerValue(i);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, kFFTComplexSize, 1, 0, GL_RED, GL_FLOAT, powerValues);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glBindTexture(GL_TEXTURE_2D, 0);
			checkErrorGL();

			glBindTexture(GL_TEXTURE_2D, g_fftTextureWithFade);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			static float powerValuesWithFade[kFFTComplexSize] = { };
			const float fftFadeA = std::powf(g_scene->m_fftFade, dtReal);
			for (int i = 0; i < kFFTComplexSize; ++i)
				powerValuesWithFade[i] = std::max(powerValuesWithFade[i] * fftFadeA, powerValues[i]);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, kFFTComplexSize, 1, 0, GL_RED, GL_FLOAT, powerValuesWithFade);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glBindTexture(GL_TEXTURE_2D, 0);
			checkErrorGL();

			//

			DrawableList prevDrawableList;
			if (g_prevScene != nullptr)
				g_prevScene->draw(prevDrawableList);
			prevDrawableList.sort();

			DrawableList drawableList;
			g_scene->draw(drawableList);
			drawableList.sort();

			framework.beginDraw(0, 0, 0, 0);
			{
			#if ENABLE_3D
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
			#endif

				pushSurface(&prevSurface);
				{
					ScopedSurfaceBlock scopedBlock(&prevSurface);

					glClearColor(0.f, 0.f, 0.f, 0.f);
					glClear(GL_COLOR_BUFFER_BIT);

					{
						gpuTimingBlock(prevDrawableList);
						setBlend(BLEND_ADD);
						prevDrawableList.draw();
					}
				}
				popSurface();

				pushSurface(&surface);
				{
					ScopedSurfaceBlock scopedBlock(&surface);

					glClearColor(0.f, 0.f, 0.f, 1.f);
					glClear(GL_COLOR_BUFFER_BIT);

					setBlend(BLEND_ALPHA);

				#if ENABLE_3D
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

							setBlend(BLEND_ALPHA);
						}
						camera.endView();
					}
				#endif

					{
						gpuTimingBlock(drawableList);
						setBlend(BLEND_ADD);
						drawableList.draw();
					}
				}
				popSurface();

				// blit result to back buffer

				if (g_prevScene)
				{
					gpuTimingBlock(blitToBackBuffer);
					
					setBlend(BLEND_OPAQUE);
					const float alpha = g_prevSceneTime * g_prevSceneTimeRcp;

					Shader shader("gamma_prev");
					setShader(shader);
					shader.setImmediate("gamma", config.display.gamma);
					shader.setImmediate("alpha", alpha);
					shader.setTexture("prevColormap", 0, prevSurface.getTexture(), false, true);
					shader.setTexture("currColormap", 1, surface.getTexture(), false, true);

					if (config.display.mirror)
						drawRect(GFX_SX_SCALED, 0, 0, GFX_SY_SCALED);
					else
						drawRect(0, 0, GFX_SX_SCALED, GFX_SY_SCALED);

					clearShader();
					setBlend(BLEND_ALPHA);
				}
				else
				{
					gpuTimingBlock(blitToBackBuffer);

					setBlend(BLEND_OPAQUE);

					Shader shader("gamma");
					setShader(shader);
					shader.setImmediate("gamma", config.display.gamma);
					shader.setTexture("colormap", 0, surface.getTexture(), false, true);

					if (config.display.mirror)
						drawRect(GFX_SX_SCALED, 0, 0, GFX_SY_SCALED);
					else
						drawRect(0, 0, GFX_SX_SCALED, GFX_SY_SCALED);

					clearShader();
					setBlend(BLEND_ALPHA);
				}

				if (showUi && config.display.showScaleOverlay)
				{
					gxPushMatrix();
					{
						const float height = 3.f;
						const float lift = 30.f / 100.f;
					#define HEIGHT_TO_SCREEN(h) (1.f - ((h/100.f - lift)/height)) * GFX_SY

						applyGfxTransform();

						setBlend(BLEND_ALPHA);

						//setColor(colorWhite);
						//Sprite("scaleoverlay.png").draw();

						setColor(colorWhite);
						setFont("calibri.ttf");
						for (int y = 0; y <= 350; y += 50)
						{
							drawText(GFX_SX - 5, HEIGHT_TO_SCREEN(y), 16, -1, -3, "%.2fcm", y / 100.f);
							drawLine(0, HEIGHT_TO_SCREEN(y), GFX_SX, HEIGHT_TO_SCREEN(y));
						}
					}
					gxPopMatrix();
				}

			#if ENABLE_3D
				if (showUi && drawProjectorSetup)
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
			#endif

			#if ENABLE_3D
				if (showUi && drawScreenIds)
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
			#endif

			#if ENABLE_DEBUG_MENUS
				if (showUi)
				{
					setFont("VeraMono.ttf");
					setColor(colorWhite);
					//const int spacingY = 28;
					//const int fontSize = 24;
					const int spacingY = 16;
					const int fontSize = 14;
					int x = 20;
					int y = 45;

					if (s_debugMode == kDebugMode_None)
					{
					}
					else if (s_debugMode == kDebugMode_EffectList || s_debugMode == kDebugMode_EffectListCondensed)
					{
						drawText(x, y, fontSize, +1.f, +1.f, (s_debugMode == kDebugMode_EffectList) ? "effects list:" : "effects list (condensed):");
						x += 50;
						y += spacingY;

						int index = 0;

						const auto effects = buildEffectsList();

						for (auto i = effects.begin(); i != effects.end(); ++i, ++index)
						{
							float xOld = x;

							const std::string & effectName = (*i)->m_name;
							Effect * effect = (*i)->m_effect;
							
							char temp[1024];
							sprintf_s(temp, sizeof(temp), "%d %-20s", index, effectName.c_str());

							float sx, sy;
							measureText(fontSize, sx, sy, "%s", temp);

							setColor(effect->debugEnabled ? colorWhite : colorRed);
							drawText(x, y, fontSize, +1.f, +1.f, "%s", temp);

							x += sx;
							x += 4.f;

							bool anyActive = false;

							if (s_debugMode != kDebugMode_EffectListCondensed)
							{
								for (auto j : effect->m_tweenVars)
									anyActive |= j.second->isActive();
							}
							else
							{
								anyActive |= !effect->m_tweenVars.empty();
							}

							float yNew = y;

							if (anyActive)
							{
								for (auto j : effect->m_tweenVars)
								{
									float yOld = y;

									std::string varName = effectParamToName(effectName, j.first);
									TweenFloat & var = *j.second;

									setColor(colorWhite);
									drawText(x, y, fontSize, +1.f, +1.f, "%-8s", varName.c_str());
									y += spacingY;

									setColor(var.isActive() ? colorYellow : colorWhite);
									drawText(x, y, fontSize, +1.f, +1.f, "%.2f", (float)var);
									y += spacingY;

									x += 150.f;
									yNew = y;
									y = yOld;
								}
							}
							else
							{
								yNew += spacingY;
							}

							x = xOld;
							y = yNew;
						}

						y += spacingY;
						x -= 50;
					}
				#if ENABLE_3D
					else if (s_debugMode == kDebugMode_Camera)
					{
					}
				#endif
					else if (s_debugMode == kDebugMode_EventList)
					{
						drawText(x, y, fontSize, +1.f, +1.f, "events list:");
						x += 50;
						y += spacingY;

						for (size_t i = 0; i < g_scene->m_events.size(); ++i)
						{
							setColor(g_scene->m_events[i]->m_enabled ? colorWhite : colorRed);
							drawText(x, y, fontSize, +1.f, +1.f, "%02d [osc=%02d]: %-40s", i, g_scene->m_events[i]->m_oscId, g_scene->m_events[i]->m_name.c_str());
							y += spacingY;
						}

						y += spacingY;
						x -= 50;
					}
					else if (s_debugMode == kDebugMode_LayerList)
					{
						drawText(x, y, fontSize, +1.f, +1.f, "layer list:");
						x += 50;
						y += spacingY;

						for (int i = 0; i < (int)g_scene->m_layers.size(); ++i)
						{
							SceneLayer * layer = g_scene->m_layers[i];

							char temp[1024];
							sprintf_s(temp, sizeof(temp), "%d %-20s", i, layer->m_name.c_str());

							setColor(layer->m_debugEnabled ? colorWhite : colorRed);
							drawText(x, y, fontSize, +1.f, +1.f, "%s", temp);
							y += spacingY;
						}

						y += spacingY;
						x -= 50;
					}
					else if (s_debugMode == kDebugMode_Help)
					{
						drawText(x, y, fontSize, +1.f, +1.f, "Press F1 to toggle help"); y += spacingY;
						x += 50;
						drawText(x, y, fontSize, +1.f, +1.f, ""); y += spacingY;

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
							drawCircle(GFX_SX/2 + i, GFX_SY/2 + EvalEase(i / 100.f, (EaseType)easeFunction, mouse.y / float(GFX_SY) * 2.f) * 100.f, 5.f, 4);
					}
				}
			#endif

			#if ENABLE_DEBUG_PCMTEX
				if (showUi)
				{
					setBlend(BLEND_ADD);
					setColor(colorWhite);
					gxSetTexture(g_pcmTexture);
					drawRect(0, 0, GFX_SX, GFX_SY);
					gxSetTexture(0);
					setBlend(BLEND_ALPHA);
				}
			#endif

			#if ENABLE_DEBUG_FFTTEX || 1
				if (showUi && keyboard.isDown(SDLK_f))
				{
					const int sx = 800;
					const int sy = 100;
					const int x = 5;
					const int y = GFX_SY - sy - 5;
					const float s = 1.f/4.f;
					const float w = .1f;

					setBlend(BLEND_ALPHA);
					setColorf(s, s, s, .5f);
					gxSetTexture(g_fftTexture);
					gxBegin(GL_QUADS);
					{
						gxTexCoord2f(0.f, 0.f); gxVertex2f(x,      y     );
						gxTexCoord2f(w,   0.f); gxVertex2f(x + sx, y     );
						gxTexCoord2f(w,   1.f); gxVertex2f(x + sx, y + sy);
						gxTexCoord2f(0.f, 1.f); gxVertex2f(x,      y + sy);
					}
					gxEnd();
					gxSetTexture(0);

					setBlend(BLEND_ALPHA);

					setFont("calibri.ttf");
					setColor(colorWhite);
					for (int i = 0; i <= 5; ++i)
						drawText(x + sx/5.f*i, y, 24, 0.f, -1.f, "%d", int(kFFTComplexSize*w/5.f*i));
				}
			#endif

			#if ENABLE_DEBUG_INFOS && 0
				if (showUi)
				{
					setFont("VeraMono.ttf");
					setColor(colorBlack);
					drawRect(mouse.x-100, mouse.y-10, mouse.x+100, mouse.y+20);
					setColor(colorWhite);
					drawText(mouse.x, mouse.y, 24, 0, 0, "(%d, %d)",
						mouse.isDown(BUTTON_LEFT) ? (int)screenXToVirtual(mouse.x) : mouse.x,
						mouse.isDown(BUTTON_LEFT) ? (int)screenYToVirtual(mouse.y) : mouse.y);
				}
			#endif

			#if ENABLE_DEBUG_INFOS
				if (showUi)
				{
					setFont("VeraMono.ttf");
					setColor(colorWhite);

					int y = 5;

				#if ENABLE_REALTIME_EDITING
					drawText(5, y, 24, +1.f, +1.f, "FileMonitor: %s", fileMonitorEnabled ? "enabled" : "disabled");
					y += 30;
				#endif

				#if ENABLE_LEAPMOTION
					drawText(5, y, 24, +1, +1, "LeapMotion connected: %d, hasFocus: %d", (int)leapController.isConnected(), (int)leapController.hasFocus());
					y += 30;

					if (leapController.isConnected() && leapController.hasFocus())
					{
						drawText(5, y, 24, +1, +1, "LeapMotion palm position: (%03d, %03d, %03d)",
							(int)g_leapState.palmX,
							(int)g_leapState.palmY,
							(int)g_leapState.palmZ);
						y += 30;
					}
				#endif
				}
			#endif

				if (showCredits)
				{
					gxPushMatrix();
					{
						showCreditsAlpha += dtReal;
						if (showCreditsAlpha > 1.f)
							showCreditsAlpha = 1.f;

						applyGfxTransform();
						setBlend(BLEND_ALPHA);
						setColorf(1.f, 1.f, 1.f, showCreditsAlpha);
						Sprite("track-credits/Credits.png").draw();
					}
					gxPopMatrix();
				}
			}
			framework.endDraw();

			//

			delete [] samplesThisFrame;
			samplesThisFrame = nullptr;
		}

		delete g_scene;
		g_scene = nullptr;

		delete g_prevScene;
		g_prevScene = nullptr;

		delete g_sceneSurfacePool;
		g_sceneSurfacePool = nullptr;

		glDeleteTextures(1, &g_fftTextureWithFade);
		g_fftTextureWithFade = 0;

		glDeleteTextures(1, &g_fftTexture);
		g_fftTexture = 0;

		glDeleteTextures(1, &g_pcmTexture);
		g_pcmTexture = 0;

		framework.shutdown();
	}

	//

#if ENABLE_LEAPMOTION
	leapController.removeListener(*leapListener);

	delete leapListener;
	leapListener = nullptr;
#endif

	//

	fftShutdown();

	//

	s_oscReceiveSocket->AsynchronousBreak();
	SDL_WaitThread(s_oscMessageThread, nullptr);

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
