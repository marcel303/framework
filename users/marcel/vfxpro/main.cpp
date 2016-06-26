#include "ip/UdpSocket.h"
#include "mediaplayer_old/MPContext.h"
#include "osc/OscOutboundPacketStream.h"
#include "osc/OscPacketListener.h"
#include "audiostream/AudioOutput.h"
#include "audiofft.h"
#include "audioin.h"
#include "Calc.h"
#include "config.h"
#include "drawable.h"
#include "effect.h"
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
#include <Windows.h>

#include "data/ShaderConstants.h"

#if !defined(DEBUG)
	//#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

using namespace tinyxml2;

#define DEMODATA 0

#define NUM_SCREENS 1

const int SCREEN_SX = (1920 / NUM_SCREENS);
const int SCREEN_SY = 1080;

const int GFX_SX = (SCREEN_SX * NUM_SCREENS);
const int GFX_SY = (SCREEN_SY * 1);

#define OSC_ADDRESS "127.0.0.1"
#define OSC_RECV_PORT 8000

Config config;

Scene * g_scene = nullptr;

float g_pcmVolume = 0.f;
GLuint g_pcmTexture = 0;
GLuint g_fftTexture = 0;

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

static CRITICAL_SECTION s_oscMessageMtx;
static HANDLE s_oscMessageThread = INVALID_HANDLE_VALUE;

enum OscMessageType
{
	kOscMessageType_None,
	// scene :: constantly reinforced
	kOscMessageType_SetScene,
	// events
	kOscMessageType_Event,
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

			if (strcmp(m.AddressPattern(), "/event") == 0 || true)
			{
			#if 1
				// eventId
				osc::int32 eventId;
				args >> eventId;

				message.type = kOscMessageType_Event;
				message.param[0] = eventId;
			#else
				// NULL, eventId
				const char * str;
				osc::int32 eventId;
				args >> str >> eventId;
				
				message.type = kOscMessageType_Event;
				message.param[0] = eventId;
			#endif
			}
			else if (strcmp(m.AddressPattern(), "/box") == 0)
			{
				// NULL, width, angle1, angle2
				const char * str;
				osc::int32 width;
				osc::int32 angle1;
				osc::int32 angle2;
				
				args >> str >> width >> angle1 >> angle2;
				
				message.type = kOscMessageType_Sprite;
				message.str = str;
				message.param[0] = width;
				message.param[1] = angle1;
				message.param[2] = angle2;
			}
			else if (strcmp(m.AddressPattern(), "/sprite") == 0)
			{
				// filename, x, y, scale
				const char * str;
				osc::int32 x;
				osc::int32 y;
				osc::int32 scale;
				args >> str >> x >> y >> scale;
				
				message.type = kOscMessageType_Sprite;
				message.str = str;
				message.param[0] = x;
				message.param[1] = y;
				message.param[2] = scale;
			}
			else if (strcmp(m.AddressPattern(), "/video") == 0)
			{
				// filename, x, y, scale
				const char * str;
				osc::int32 x;
				osc::int32 y;
				osc::int32 scale;
				args >> str >> x >> y >> scale;
				
				message.type = kOscMessageType_Video;
				message.str = str;
				message.param[0] = x;
				message.param[1] = y;
				message.param[2] = scale;
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

#if ENABLE_DEBUG_MENUS

enum DebugMode
{
	kDebugMode_None,
	kDebugMode_Help,
	kDebugMode_Camera,
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

		//

		delete g_scene;
		g_scene = nullptr;

		//

		g_scene = new Scene();

		if (!g_scene->load(filename.c_str()))
		{
			delete g_scene;
			g_scene = nullptr;

			g_scene = new Scene();
		}
	}
}

//

static void handleFileChange(const std::string & filename)
{
	if (filename == "settings.xml")
		config.load(filename.c_str());
	else if (filename == "effects_meta.xml")
		g_effectInfosByName.load(filename.c_str());
	else if (filename == g_scene->m_filename)
		g_scene->reload();
	else
	{
		const std::string extension = Path::GetExtension(filename);

		if (extension == "vs")
		{
			const std::string name = Path::StripExtension(filename);

			Shader(name.c_str()).reload();
		}
		else if (extension == "ps")
		{
			const std::string baseName = Path::GetBaseName(filename);

			if (String::StartsWith(baseName, "fsfx_"))
			{
				Shader(filename.c_str(), "fsfx.vs", filename.c_str()).reload();
			}
			else
			{
				const std::string name = Path::StripExtension(filename);

				Shader(name.c_str()).reload();
			}
		}
		else if (extension == "inc")
		{
			clearCaches(CACHE_SHADER);
		}
		else if (extension == "png" || extension == "jpg")
		{
			Sprite(filename.c_str()).reload();
		}
	}
}

//

#if ENABLE_REALTIME_EDITING

struct FileInfo
{
	std::string filename;
	time_t time;
};

static std::vector<FileInfo> s_fileInfos;

static void initFileMonitor()
{
	std::vector<std::string> files = listFiles(".", true);

	for (auto & file : files)
	{
		FILE * f = fopen(file.c_str(), "rb");
		if (f)
		{
			struct _stat s;
			if (_fstat(fileno(f), &s) == 0)
			{
				FileInfo fi;
				fi.filename = file;
				fi.time = s.st_mtime;

				s_fileInfos.push_back(fi);
			}

			fclose(f);
			f = 0;
		}
	}
}

static void tickFileMonitor()
{
	for (auto & fi: s_fileInfos)
	{
		FILE * f = fopen(fi.filename.c_str(), "rb");
		if (f)
		{
			struct _stat s;
			if (_fstat(fileno(f), &s) == 0)
			{
				if (fi.time < s.st_mtime)
				{
					// file has changed!

					logDebug("%s has changed!", fi.filename.c_str());

					fi.time = s.st_mtime;

					handleFileChange(fi.filename);
				}
			}

			fclose(f);
			f = 0;
		}
	}
}

#else

static void initFileMonitor()
{
}

static void tickFileMonitor()
{
}

#endif

//

struct LeapState
{
	Leap::Vector palmPosition;
};

class LeapListener : public Leap::Listener
{
	SDL_mutex * mutex;
	LeapState shadowState;

public:
	LeapState state;

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
			state = shadowState;
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
				auto & hand = *hands.begin();

				shadowState.palmPosition = hand.palmPosition();
			}
		}
		SDL_UnlockMutex(mutex);
	}
};

//

#if 0

float evalPoint(float n1, float n2, float t)
{
	float diff = n2 - n1;

	return n1 + (diff * t);
}    

static void testBezier()
{
	static float _x1;
	static float _y1;
	static float _x2;
	static float _y2;
	static float _x3;
	static float _y3;

	static float dx;
	static float dy;

	static int frame = 0;

	if ((frame % 20) == 0)
	{
		_x1 = random(-1.f, +1.f);
		_y1 = random(-1.f, +1.f);
		_x2 = random(-1.f, +1.f);
		_y2 = random(-1.f, +1.f);
		_x3 = random(-1.f, +1.f);
		_y3 = random(-1.f, +1.f);

		dx = random(-1.f, +1.f) * .3f;
		dy = random(-1.f, +1.f) * .3f;
	}

	const float t = (frame % 20) / 20.f;

	frame++;

	gxPushMatrix();
	{
		gxTranslatef(GFX_SX/2, GFX_SY/2, 0.f);
		gxScalef(600.f, 600.f, 1.f);

		gxColor4f(1.f, 1.f, 1.f, 1.f);

		for (float ja = -1.f; ja <= +1.f; ja += 2.f / 10.f)
		{
			const float j = ja * t;

			gxBegin(GL_LINE_STRIP);
			{
				for (float i = 0.f; i <= 1.f; i += 0.01f)
				{
					float x1 = _x1;
					float y1 = _y1;
					float x2 = _x2;
					float y2 = _y2;
					float x3 = _x3;
					float y3 = _y3;

					x2 += dx * j;
					y2 += dx * j;

					const float xa = evalPoint(x1, x2, i);
					const float ya = evalPoint(y1, y2, i);
					const float xb = evalPoint(x2, x3, i);
					const float yb = evalPoint(y2, y3, i);

					const float x = evalPoint(xa, xb, i);
					const float y = evalPoint(ya, yb, i);

					gxVertex2f(x, y);
				}
			}
			gxEnd();
		}
	}
	gxPopMatrix();
}

#endif

//

int main(int argc, char * argv[])
{
	//changeDirectory("data");

	initFileMonitor();

	if (!config.load("settings.xml"))
	{
		logError("failed to load settings.xml");
		return -1;
	}

	if (!g_effectInfosByName.load("effects_meta.xml"))
	{
		logError("failed to load effects_meta.xml");
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

	// initialise LeapMotion controller

	Leap::Controller leapController;
	leapController.setPolicy(Leap::Controller::POLICY_BACKGROUND_FRAMES);

	LeapListener * leapListener = new LeapListener;
	leapController.addListener(*leapListener);

	// initialise FFT

	fftInit();

	// initialise OSC

	InitializeCriticalSectionAndSpinCount(&s_oscMessageMtx, 256);

	s_oscMessageThread = CreateThread(NULL, 64 * 1024, ExecuteOscThread, NULL, CREATE_SUSPENDED, NULL);
	ResumeThread(s_oscMessageThread);

	// initialise framework

#if ENABLE_WINDOWED_MODE || 1
	framework.fullscreen = false;
	framework.minification = 1;
	framework.windowX = 0;
	framework.windowY = 60;
#else
	framework.fullscreen = true;
	framework.exclusiveFullscreen = false;
	framework.useClosestDisplayMode = true;
#endif

	framework.enableDepthBuffer = true;
	framework.enableMidi = true;
	framework.midiDeviceIndex = config.midi.deviceIndex;

#ifdef DEBUG
	//framework.reloadCachesOnActivate = true;
#endif

	framework.filedrop = true;
	framework.actionHandler = handleAction;

	//framework.highPrecisionRT = true;

	if (framework.init(0, 0, GFX_SX, GFX_SY))
	{
		//framework.fillCachesWithPath(".", true);

		std::list<TimeDilationEffect> timeDilationEffects;

		bool debugDraw = false;

	#if DEMODATA
		bool postProcess = false;

		bool drawCloth = false;
		bool drawBoxes = true;
		bool drawPCM = true;
	#endif

		bool drawScreenIds = false;
		bool drawProjectorSetup = false;

		g_scene = new Scene();
		//g_scene->load("healer/scene.xml");
		//g_scene->load("scene.xml");
		g_scene->load("tracks/cesitest.scene.xml");

	#if DEMODATA
		Effect_Cloth cloth("cloth");
		cloth.setup(CLOTH_MAX_SX, CLOTH_MAX_SY);

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
	#endif

		Surface surface(GFX_SX, GFX_SY);

	#if DEMODATA
		Shader jitterShader("jitter");
		Shader boxblurShader("boxblur");
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

			tickFileMonitor();

			// todo : process audio input

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

			// todo : process OSC input

			// todo : process direct MIDI input

			// process LeapMotion input

			leapListener->tick();

			// input

			if (keyboard.wentDown(SDLK_ESCAPE))
				stop = true;

			if (keyboard.wentDown(SDLK_r))
			{
				g_scene->reload();
			}

			if (keyboard.wentDown(SDLK_d))
				debugDraw = !debugDraw;
		#if ENABLE_DEBUG_MENUS
			if (keyboard.wentDown(SDLK_RSHIFT))
				setDebugMode(kDebugMode_Camera);
		#endif

		#if DEMODATA
			if (keyboard.wentDown(SDLK_p) || config.midiWentDown(64))
				postProcess = !postProcess;

			if (s_debugMode == kDebugMode_Help)
			{
				if (keyboard.wentDown(SDLK_1))
					drawCloth = !drawCloth;
				if (keyboard.wentDown(SDLK_2))
					drawBoxes = !drawBoxes;
				if (keyboard.wentDown(SDLK_3))
					drawPCM = !drawPCM;
			}
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
			else if (s_debugMode == kDebugMode_Camera)
			{
				if (keyboard.wentDown(SDLK_o))
				{
					cameraPosition = Vec3(0.f, .5f, -1.f);
					cameraRotation.SetZero();
				}

			}
			else if (s_debugMode == kDebugMode_EffectList || s_debugMode == kDebugMode_EffectListCondensed)
			{
				const int base = keyboard.isDown(SDLK_LSHIFT) ? 10 : 0;

				int index = 0;

				for (auto i = g_effectsByName.begin(); i != g_effectsByName.end(); ++i, ++index)
				{
					if (index - base < 0 || index - base > 9)
						continue;

					if (keyboard.wentDown((SDLKey)(SDLK_0 + index - base)))
					{
						Effect * effect = i->second;

						effect->debugEnabled = !effect->debugEnabled;
					}
				}
			}
			else if (s_debugMode == kDebugMode_EventList)
			{
				const int base = keyboard.isDown(SDLK_LSHIFT) ? 10 : 0;

				for (size_t i = 0; i < 10 && i < g_scene->m_events.size(); ++i)
				{
					if (i - base < 0 || i - base > 9)
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

				for (int i = 0; i < g_scene->m_layers.size(); ++i)
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

		#if ENABLE_DEBUG_MENUS
			SDL_SetRelativeMouseMode(s_debugMode == kDebugMode_Camera ? SDL_TRUE : SDL_FALSE);
		#else
			SDL_SetRelativeMouseMode(SDL_FALSE);
		#endif

			//const float dtReal = Calc::Min(1.f / 30.f, framework.timeStep) * config.midiGetValue(100, 1.f);
			const float dtReal = framework.timeStep * config.midiGetValue(100, 1.f);

			Mat4x4 cameraPositionMatrix;
			Mat4x4 cameraRotationMatrix;

		#if ENABLE_DEBUG_MENUS
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

					case kOscMessageType_Event:
						g_scene->triggerEventByOscId(message.param[0]);
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

			g_scene->tick(dt);

		#if DEMODATA
			for (int i = 0; i < 10; ++i)
				cloth.tick(dt / 10.f);

			boxes.tick(dt);

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

			// todo : convert loudness to shader input

			// todo : convert PCM data to shader input texture

			Assert(g_pcmTexture == 0);
			glGenTextures(1, &g_pcmTexture);
			glBindTexture(GL_TEXTURE_2D, g_pcmTexture);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, numSamplesThisFrame, 1, 0, GL_RED, GL_FLOAT, samplesThisFrame);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glBindTexture(GL_TEXTURE_2D, 0);
			checkErrorGL();

			// todo : convert FFT data to shader input texture

			glGenTextures(1, &g_fftTexture);
			glBindTexture(GL_TEXTURE_2D, g_fftTexture);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			fftPowerValue(0);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, kFFTComplexSize, 1, 0, GL_RED, GL_FLOAT, s_fftReal);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glBindTexture(GL_TEXTURE_2D, 0);
			checkErrorGL();

			DrawableList drawableList;

			g_scene->draw(drawableList);

		#if DEMODATA
			if (drawCloth)
				cloth.draw(drawableList);
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

					glClearColor(0.f, 0.f, 0.f, 1.f);
					glClear(GL_COLOR_BUFFER_BIT);

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
								glEnable(GL_LINE_SMOOTH);
								glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
								gxBegin(GL_LINES);
								{
									const float scaleX = GFX_SX / float(numSamplesThisFrame - 2);
									const float scaleY = 300.f;

									for (int i = 0; i < numSamplesThisFrame - 1; ++i)
									{
										gxVertex2f((i + 0) * scaleX, GFX_SY/2.f + samplesThisFrame[i + 0] * scaleY);
										gxVertex2f((i + 1) * scaleX, GFX_SY/2.f + samplesThisFrame[i + 1] * scaleY);
									}
								}
								gxEnd();
								glDisable(GL_LINE_SMOOTH);
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

					// todo : remove this loudness test
					//setColorf(.25f, .5f, 1.f, loudnessThisFrame);
					//drawRect(0, 0, GFX_SX, GFX_SY);

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

			#if ENABLE_DEBUG_MENUS
				setFont("VeraMono.ttf");
				setColor(colorWhite);
				const int spacingY = 28;
				const int fontSize = 24;
				int x = 20;
				int y = 20;

				if (s_debugMode == kDebugMode_None)
				{
				}
				else if (s_debugMode == kDebugMode_EffectList || s_debugMode == kDebugMode_EffectListCondensed)
				{
					drawText(x, y, fontSize, +1.f, +1.f, (s_debugMode == kDebugMode_EffectList) ? "effects list:" : "effects list (condensed):");
					x += 50;
					y += spacingY;

					int index = 0;

					for (auto i = g_effectsByName.begin(); i != g_effectsByName.end(); ++i, ++index)
					{
						float xOld = x;

						const std::string & effectName = i->first;
						Effect * effect = i->second;

						const EffectInfo & info = g_effectInfosByName[effectName];

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
				else if (s_debugMode == kDebugMode_Camera)
				{
				}
				else if (s_debugMode == kDebugMode_EventList)
				{
					drawText(x, y, fontSize, +1.f, +1.f, "events list:");
					x += 50;
					y += spacingY;

					for (size_t i = 0; i < g_scene->m_events.size(); ++i)
					{
						drawText(x, y, fontSize, +1.f, +1.f, "%02d: %-40s", i, g_scene->m_events[i]->m_name.c_str());
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

					for (int i = 0; i < g_scene->m_layers.size(); ++i)
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
						drawCircle(GFX_SX/2 + i, GFX_SY/2 + EvalEase(i / 100.f, (EaseType)easeFunction, mouse.y / float(GFX_SY) * 2.f) * 100.f, 5.f, 4);
				}
			#endif

				// fixme : remove

			#if 0
				setBlend(BLEND_ADD);
				setColor(colorWhite);
				gxSetTexture(g_pcmTexture);
				drawRect(0, 0, GFX_SX, GFX_SY);
				gxSetTexture(0);
				setBlend(BLEND_ALPHA);
			#endif

			#if 0
				setBlend(BLEND_ADD);
				setColor(colorWhite);
				gxSetTexture(g_fftTexture);
				drawRect(0, 0, GFX_SX, GFX_SY);
				gxSetTexture(0);
				setBlend(BLEND_ALPHA);
			#endif

			#if 1 && ENABLE_DEBUG_INFOS
				setFont("VeraMono.ttf");
				setColor(colorWhite);
				drawText(mouse.x, mouse.y, 24, 0, -1, "(%d, %d)", (int)screenXToVirtual(mouse.x), (int)screenYToVirtual(mouse.y));
			#endif

			#if 1 && ENABLE_DEBUG_INFOS
				setFont("VeraMono.ttf");
				setColor(colorWhite);
				drawText(5, 5, 24, +1, +1, "LeapMotion connected: %d, hasFocus: %d", (int)leapController.isConnected(), (int)leapController.hasFocus());

				if (leapController.isConnected() && leapController.hasFocus())
				{
					//leapController.frame();
					//leapController.now();
					//leapController.frame().images();

					drawText(5, 35, 24, +1, +1, "LeapMotion palm position: (%03d, %03d, %03d)",
						(int)leapListener->state.palmPosition.x,
						(int)leapListener->state.palmPosition.y,
						(int)leapListener->state.palmPosition.z);
				}
			#endif

			#if 0
				testBezier();
			#endif
			}
			framework.endDraw();

			//

			delete [] samplesThisFrame;
			samplesThisFrame = nullptr;

			//

			glDeleteTextures(1, &g_fftTexture);
			g_fftTexture = 0;

			glDeleteTextures(1, &g_pcmTexture);
			g_pcmTexture = 0;

			//

			time += dt;
			timeReal += dtReal;
		}

		delete g_scene;
		g_scene = nullptr;

		framework.shutdown();
	}

	//

	leapController.removeListener(*leapListener);

	delete leapListener;
	leapListener = nullptr;

	//

	fftShutdown();

	//

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
