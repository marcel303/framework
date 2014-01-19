#define NOMINMAX

#include <assert.h>
#include <cmath>
#include <limits>
#include <map>
#include <stdarg.h>
#include <stdlib.h>
#include <string>
#include <vector>

#ifdef WIN32
	#include <direct.h>
	#include <GL/glew.h>
	#include <SDL/SDL_opengl.h>
	#include <Windows.h>
	#include <Xinput.h>
#else
	#include <dirent.h>
	#include <unistd.h>
	#define _chdir chdir
#endif

#include "audio.h"
#include "framework.h"
#include "internal.h"

// -----

Color colorBlack(0, 0, 0, 255);
Color colorWhite(255, 255, 255, 255);
Color colorRed(255, 0, 0, 255);
Color colorGreen(0, 255, 0, 255);
Color colorBlue(0, 0, 255, 255);

//

Framework framework;
Mouse mouse;
Keyboard keyboard;
Gamepad gamepad[MAX_GAMEPAD];
Stage stage;
Ui ui;

// -----

Framework::Framework()
{
	fullscreen = false;
	minification = 1;
	reloadCachesOnActivate = false;
	windowTitle = "GGJ 2014 - Unknown Project";
	numSoundSources = 32;
	actionHandler = 0;
	
	time = 0.f;
	timeStep = 1.f / 60.f;
}

Framework::~Framework()
{
}

bool Framework::init(int argc, char * argv[], int sx, int sy)
{
#ifdef WIN32
	_putenv("SDL_VIDEO_WINDOW_POS");
	_putenv("SDL_VIDEO_CENTERED=1");
#endif

	// initialize SDL
	
	const int initFlags = SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER | SDL_INIT_VIDEO;
	
	if (SDL_Init(initFlags) < 0)
	{
		logError("failed to initialize SDL: %s", SDL_GetError());
		return false;
	}
	
#if 0
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#endif

#if 1
	SDL_GL_SetAttribute(SDL_GL_BUFFER_SIZE, 32);
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
#endif

#if 1
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
#endif

#if 1
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1); // make sure we have vsync enabled
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
#endif

	int flags = SDL_OPENGL;
	
	if (fullscreen && minification == 1)
	{
		flags |= SDL_DOUBLEBUF;
		flags |= SDL_FULLSCREEN;
	}
	
	if (SDL_SetVideoMode(sx / minification, sy / minification, 32, flags) == 0)
	{
		logError("failed to set video mode (%dx%d @ %dbpp): %s", sx / minification, sy / minification, 32, SDL_GetError());
		return false;
	}
	
	glewExperimental = GL_TRUE; // force GLEW to resolve all supported extension methods

	const int glewStatus = glewInit();

	if (glewStatus != GLEW_OK)
	{
		logError("failed to initialize GLEW: %s", glewGetErrorString(glewStatus));
		return false;
	}

	log("using GLEW %s", glewGetString(GLEW_VERSION));
	
	if (!GLEW_VERSION_3_0)
	{
		logWarning("OpenGL 3.0 not supported");
	}
	
	SDL_WM_SetCaption(windowTitle.c_str(), 0);
	
	g_globals.g_displaySize[0] = sx;
	g_globals.g_displaySize[1] = sy;

	// enable optional OpenGL debugging
	
#if 0
	glEnable(GL_DEBUG_OUTPUT);
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif
	
	// resolve OpenGL extensions
	
	/*
	if (glFramebufferTexture == 0)
		glFramebufferTexture = (PFNGLFRAMEBUFFERTEXTUREPROC)SDL_GL_GetProcAddress("glFramebufferTexture");
	if (glBlendEquation == 0)
		glBlendEquation = (PFNGLBLENDEQUATIONPROC)SDL_GL_GetProcAddress("glBlendEquation");
	if (glClampColor == 0)
		glClampColor = (PFNGLCLAMPCOLORPROC)SDL_GL_GetProcAddress("glClampColor");

	if (glFramebufferTexture == 0 || glBlendEquation == 0 || glClampColor == 0)
	{
		logError("unable to find required OpenGL extension(s)");
		return false;
	}
	*/
	
	glClampColor(GL_CLAMP_VERTEX_COLOR, GL_FALSE);
	
	// initialize FreeType
	
	if (FT_Init_FreeType(&g_globals.g_freeType) != 0)
	{
		logError("failed to initialize FreeType");
		return false;
	}
	
	// initialize sound player
	
	if (!g_soundPlayer.init(numSoundSources))
	{
		logError("failed to initialize sound player");
		return false;
	}
	
	// initialize UI
	
	ui.load("default_ui");
	
	return true;
}

bool Framework::shutdown()
{
	bool result = true;
	
	// shut down sound player
	
	if (!g_soundPlayer.shutdown())
	{
		logError("failed to shut down sound player");
		result = false;
	}
	
	// free resources
	
	g_textureCache.clear();
	g_shaderCache.clear();
	g_animCache.clear();
	g_soundCache.clear();
	g_fontCache.clear();
	g_glyphCache.clear();
	g_uiCache.clear();
	
	// shut down FreeType
	
	if (FT_Done_FreeType(g_globals.g_freeType) != 0)
	{
		logError("failed to shut down FreeType");
		result = false;
	}
	g_globals.g_freeType = 0;
	
	glBlendEquation = 0;
	glClampColor = 0;
	
	// shut down SDL
	
	SDL_Quit();
	
	// clear globals
	
	g_globals = Globals();
	
	// reset self
	
	fullscreen = false;
	minification = 1;
	reloadCachesOnActivate = false;
	numSoundSources = 32;
	actionHandler = 0;
	
	return result;
}

void Framework::process()
{
	time += timeStep;
	
	g_soundPlayer.process();
	
	bool doReload = false;
	
	bool reloadKey = keyboard.isDown(SDLK_r);
	
	// poll SDL event queue
	
	memset(g_globals.g_keyChange, 0, sizeof(g_globals.g_keyChange));
	memset(g_globals.g_mouseChange, 0, sizeof(g_globals.g_mouseChange));
	
	SDL_Event e;
	
	while (SDL_PollEvent(&e))
	{
		if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
		{
			if (e.key.keysym.sym >= 0 && e.key.keysym.sym < SDLK_LAST)
			{
				g_globals.g_keyDown[e.key.keysym.sym] = e.key.state == SDL_PRESSED;
				g_globals.g_keyChange[e.key.keysym.sym] = true;
			}
		}
		else if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP)
		{
			const int index = e.button.button == SDL_BUTTON_LEFT ? 0 : e.button.button == SDL_BUTTON_RIGHT ? 1 : -1;
			if (index >= 0)
			{
				g_globals.g_mouseDown[index] = e.button.state == SDL_PRESSED;
				g_globals.g_mouseChange[index] = true;
			}
		}
		else if (e.type == SDL_MOUSEMOTION)
		{
			mouse.x = e.motion.x * minification;
			mouse.y = e.motion.y * minification;
		}
		else if (e.type == SDL_ACTIVEEVENT)
		{
			if (reloadCachesOnActivate)
				doReload |= (e.active.state & SDL_APPINPUTFOCUS) && e.active.gain;
		}
	}
	
#ifdef __WIN32__
	// use XInput to poll gamepad state
	for (int i = 0; i < MAX_GAMEPAD; ++i)
	{
		XINPUT_STATE state;
		ZeroMemory(&state, sizeof(XINPUT_STATE));
		DWORD result = XInputGetState(i, &state);

		if (result == ERROR_SUCCESS)
		{
			gamepad[i].isConnected = true;
			
			const XINPUT_GAMEPAD & g = state.Gamepad;
			
		#define APPLY_DEADZONE(v, t) (std::abs(v) <= t ? 0.f : clamp((std::abs(v) - t) * (v < 0.f ? -1.f : +1.f) / float(32767 - t), -1.f, +1.f))

			gamepad[i].m_analog[0][ANALOG_X] = APPLY_DEADZONE(g.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
			gamepad[i].m_analog[0][ANALOG_Y] = APPLY_DEADZONE(g.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
			gamepad[i].m_analog[1][ANALOG_X] = APPLY_DEADZONE(g.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
			gamepad[i].m_analog[1][ANALOG_Y] = APPLY_DEADZONE(g.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
			
		#undef APPLY_DEADZONE
			
			const int buttons = g.wButtons;
			bool * isDown = gamepad[i].isDown;
			
			isDown[DPAD_LEFT]     = 0 != (buttons & XINPUT_GAMEPAD_DPAD_LEFT);
			isDown[DPAD_RIGHT]    = 0 != (buttons & XINPUT_GAMEPAD_DPAD_RIGHT);
			isDown[DPAD_UP]       = 0 != (buttons & XINPUT_GAMEPAD_DPAD_UP);
			isDown[DPAD_DOWN]     = 0 != (buttons & XINPUT_GAMEPAD_DPAD_DOWN);
			isDown[GAMEPAD_A]     = 0 != (buttons & XINPUT_GAMEPAD_A);
			isDown[GAMEPAD_B]     = 0 != (buttons & XINPUT_GAMEPAD_B);
			isDown[GAMEPAD_X]     = 0 != (buttons & XINPUT_GAMEPAD_X);
			isDown[GAMEPAD_Y]     = 0 != (buttons & XINPUT_GAMEPAD_Y);
			isDown[GAMEPAD_L1]    = 0 != (buttons & XINPUT_GAMEPAD_LEFT_SHOULDER);
			isDown[GAMEPAD_R1]    = 0 != (buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
			isDown[GAMEPAD_L2]    = g.bLeftTrigger  > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
			isDown[GAMEPAD_R2]    = g.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
			isDown[GAMEPAD_START] = 0 != (buttons & XINPUT_GAMEPAD_START);
			isDown[GAMEPAD_BACK]  = 0 != (buttons & XINPUT_GAMEPAD_BACK);
		}
		else
		{
			memset(&gamepad[i], 0, sizeof(Gamepad));
		}
	}
#endif
	
	doReload |= keyboard.isDown(SDLK_r) && !reloadKey;
	
	if (doReload)
	{
		reloadCaches();
	}
	
	for (SpriteSet::iterator i = m_sprites.begin(); i != m_sprites.end(); ++i)
	{
		(*i)->updateAnimation(timeStep);
	}
}

void Framework::processAction(const std::string & action, const Dictionary & args)
{
	if (action == "sound")
	{
		Sound(args.getString("sound", "").c_str()).play(args.getInt("volume", 100));
	}
	
	if (actionHandler)
	{
		actionHandler(action, args);
	}
}

void Framework::reloadCaches()
{
	g_textureCache.reload();
	g_shaderCache.reload();
	g_animCache.reload();
	g_soundCache.reload();
	g_fontCache.reload();
	g_glyphCache.clear();
	g_uiCache.reload();
	
	g_globals.g_resourceVersion++;
	
	for (SpriteSet::iterator i = m_sprites.begin(); i != m_sprites.end(); ++i)
	{
		(*i)->updateAnimationSegment();
	}
}

std::vector<std::string> listFiles(const char * path)
{
#ifdef WIN32
	std::vector<std::string> result;
	WIN32_FIND_DATAA ffd;
	char wildcard[MAX_PATH];
	sprintf_s(wildcard, sizeof(wildcard), "%s\\*", path);
	HANDLE find = FindFirstFileA(wildcard, &ffd);
	if (find != INVALID_HANDLE_VALUE)
	{
		do
		{
			result.push_back(ffd.cFileName);
		} while (FindNextFileA(find, &ffd) != 0);

		FindClose(find);
	}
	return result;
#else
	std::vector<std::string> result;
	DIR * dir = opendir(path);
	dirent * ent;
	if (dir)
	{
		while ((ent = readdir(dir)) != 0)
			result.push_back(ent->d_name);
		closedir(dir);
	}
	return result;
#endif
}

void Framework::fillCachesWithPath(const char * path)
{
	std::vector<std::string> files = listFiles(path);
	for (size_t i = 0; i < files.size(); ++i)
	{
		const char * f = files[i].c_str();
		if (strstr(f, ".png") || strstr(f, ".bmp"))
			Sprite(f, 0.f, 0.f);
		if (strstr(f, ".wav"))
			g_soundCache.findOrCreate(f);
		if (strstr(f, ".ogg"))
		{
			FILE * file;
			if (fopen_s(&file, f, "rb") == 0)
			{
				fseek(file, 0, SEEK_END);
				const int size = ftell(file);
				fclose(file);
				if (size <= 512*1024)
					g_soundCache.findOrCreate(f);
			}
		}
		if (strstr(f, ".ttf"))
			g_fontCache.findOrCreate(f);
		if (strstr(f, ".txt"))
		{
			FileReader r;
			std::string line;
			if (r.open(f, true) && r.read(line) && strstr(line.c_str(), "#ui"))
				g_uiCache.findOrCreate(f);
		}
	}
}

void Framework::beginDraw(int r, int g, int b, int a)
{
	// clear back buffer
	
	glClearColor(r/255.f, g/255.f, b/255.f, a/255.f);
	glClear(GL_COLOR_BUFFER_BIT);
	
	// initialize viewport and OpenGL matrices
	
	glViewport(0, 0, g_globals.g_displaySize[0] / minification, g_globals.g_displaySize[1] / minification);

	glMatrixMode(GL_PROJECTION);
	{
		glLoadIdentity();
		
		// flip Y axis so the vertical axis runs top to bottom
		glScalef(1.f, -1.f, 1.f);
		
		// convert from (0,0),(1,1) to (-1,-1),(+1+1)
		glTranslatef(-1.f, -1.f, 0.f);
		glScalef(2.f, 2.f, 1.f);
		
		// convert from (0,0),(sx,sy) to (0,0),(1,1)
		glScalef(1.f/g_globals.g_displaySize[0], 1.f/g_globals.g_displaySize[1], 1.f);
	}
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void Framework::endDraw()
{
	// check for errors
	
	checkErrorGL();
			
	// flip back buffers
	
	SDL_GL_SwapBuffers();
}

void Framework::registerSprite(Sprite * sprite)
{
	m_sprites.insert(sprite);
}

void Framework::unregisterSprite(Sprite * sprite)
{
	m_sprites.erase(m_sprites.find(sprite));
}

// -----

void Surface::construct()
{
	m_size[0] = 0;
	m_size[1] = 0;
	
	m_bufferId = 0;
	
	m_buffer[0] = 0;
	m_buffer[1] = 0;
	m_texture[0] = 0;
	m_texture[1] = 0;
}

void Surface::destruct()
{
	m_size[0] = 0;
	m_size[1] = 0;
	
	m_bufferId = 0;
	
	for (int i = 0; i < 2; ++i)
	{
		if (m_buffer[i])
		{
			glDeleteFramebuffers(1, &m_buffer[i]);
			m_buffer[i] = 0;
		}
		
		if (m_texture[i])
		{
			glDeleteTextures(1, &m_texture[i]);
			m_texture[i] = 0;
		}
	}
}

void Surface::swapBuffers()
{
	m_bufferId = (m_bufferId + 1) % 2;
}

Surface::Surface()
{
	construct();
}

Surface::Surface(int sx, int sy)
{
	construct();
	
	init(sx, sy);
}

Surface::~Surface()
{
	destruct();
}

bool Surface::init(int sx, int sy)
{
	fassert(m_buffer[0] == 0);
	
	sx /= framework.minification;
	sy /= framework.minification;
	
	GLuint oldBuffer = 0;
	GLuint oldTexture = 0;
	
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, (GLint*)&oldBuffer);
	glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&oldTexture);
	
	//
	
	bool result = true;
	
	m_size[0] = sx * framework.minification;
	m_size[1] = sy * framework.minification;
	
	for (int i = 0; i < 2; ++i)
	{
		// allocate storage
		
		glGenTextures(1, &m_texture[i]);
		result &= m_texture[i] != 0;
		checkErrorGL();
		
		glBindTexture(GL_TEXTURE_2D, m_texture[i]);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, sx, sy, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);
		checkErrorGL();
		
		// set filtering
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		checkErrorGL();
		
		// create attachment
		
		glGenFramebuffers(1, &m_buffer[i]);
		result &= m_buffer[i] != 0;
		checkErrorGL();
		
		glBindFramebuffer(GL_FRAMEBUFFER, m_buffer[i]);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture[i], 0);
		checkErrorGL();
		
		// check if all went well
		
		const int status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			logError("failed to init surface. status: %d", status);
			
			result = false;
		}
	}
	
	if (!result)
	{
		logError("failed to init surface. calling destruct()");
		
		destruct();
	}
	
	// restore the old framebuffer and texture bindings
	
	glBindFramebuffer(GL_FRAMEBUFFER, oldBuffer);
	checkErrorGL();
	glBindTexture(GL_TEXTURE_2D, oldTexture);
	checkErrorGL();
	
	return result;
}

GLuint Surface::getFramebuffer() const
{
	return m_buffer[m_bufferId];
}

GLuint Surface::getTexture() const
{
	return m_texture[m_bufferId];
}

int Surface::getWidth() const
{
	return m_size[0];
}

int Surface::getHeight() const
{
	return m_size[1];
}

void Surface::clear(int r, int g, int b, int a)
{
	clearf(r/255.f, g/255.f, b/255.f, a/255.f);
}

void Surface::clearf(float r, float g, float b, float a)
{
	pushSurface(this);
	{
		glClearColor(r, g, b, a);
		glClear(GL_COLOR_BUFFER_BIT);
	}
	popSurface();
}

void Surface::clearAlpha()
{
	setAlphaf(0.f);
}

void Surface::setAlpha(int a)
{
	setAlphaf(a/255.f);
}

void Surface::setAlphaf(float a)
{
	pushSurface(this);
	{
		setBlend(BLEND_OPAQUE);
		setColorf(1.f, 1.f, 1.f, a);
		glColorMask(0, 0, 0, 1);
		{
			drawRect(0, 0, m_size[0], m_size[1]);
		}
		glColorMask(1, 1, 1, 1);
	}
	popSurface();
}

void Surface::mulf(float r, float g, float b, float a)
{
	pushSurface(this);
	{
		setBlend(BLEND_MUL);
		setColorf(r, g, b, a);
		drawRect(0, 0, m_size[0], m_size[1]);
	}
	popSurface();
}

void Surface::postprocess(Shader & shader)
{
	swapBuffers();
	
	pushSurface(this);
	{
		setShader(shader);
		{
			const int bufferId = (m_bufferId + 1) % 2;
			glBindTexture(GL_TEXTURE_2D, m_texture[bufferId]);
			glEnable(GL_TEXTURE_2D);
			{
				drawRect(0, 0, m_size[0], m_size[1]);
			}
			glDisable(GL_TEXTURE_2D);
		}
		clearShader();
	}
	popSurface();
}

void Surface::invert()
{
	pushSurface(this);
	{
		setBlend(BLEND_INVERT);
		setColorf(1.f, 1.f, 1.f, 1.f);
		drawRect(0, 0, m_size[0], m_size[1]);
	}
	popSurface();
}

void Surface::invertColor()
{
	glColorMask(1, 1, 1, 0);
	{
		invert();
	}
	glColorMask(1, 1, 1, 1);
}

void Surface::invertAlpha()
{
	glColorMask(0, 0, 0, 1);
	{
		invert();
	}
	glColorMask(1, 1, 1, 1);
}

void Surface::blitTo(Surface * surface)
{
	pushSurface(surface);
	{
		// todo: draw fullscreen rect using our own texture
	}
	popSurface();
}

// -----

Shader::Shader()
{
	m_shader = 0;
}

Shader::Shader(const char * filename)
{
	m_shader = 0;
	
	load(filename);
}

void Shader::load(const char * filename)
{
	m_shader = &g_shaderCache.findOrCreate(filename);
}

GLuint Shader::getProgram() const
{
	return m_shader ? m_shader->program : 0;
}

// -----

Color::Color()
{
	r = g = b = a = 0.f;
}

Color::Color(int r, int g, int b, int a)
{
	this->r = r / 255.f;
	this->g = g / 255.f;
	this->b = b / 255.f;
	this->a = a / 255.f;
}

Color::Color(float r, float g, float b, float a)
{
	this->r = r;
	this->g = g;
	this->b = b;
	this->a = a;
}

Color Color::interp(const Color & other, float t) const
{
	const float t1 = 1.f - t;
	const float t2 = t;
	
	return Color(
		r * t1 + other.r * t2,
		g * t1 + other.g * t2,
		b * t1 + other.b * t2,
		a * t1 + other.a * t2);
}

// -----

Gradient::Gradient()
{
	x1 = y1 = 0.f;
	x2 = y2 = 0.f;
}

Gradient::Gradient(float x1, float y1, const Color & color1, float x2, float y2, const Color & color2)
{
	set(x1, y1, color1, x2, y2, color2);
}

void Gradient::set(float x1, float y1, const Color & color1, float x2, float y2, const Color & color2)
{
	this->x1 = x1;
	this->y1 = y1;
	this->color1 = color1;
	this->x2 = x2;
	this->y2 = y2;
	this->color2 = color2;
}

Color Gradient::eval(float x, float y) const
{
	const float dx = x2 - x1;
	const float dy = y2 - y1;
	const float ds = dx * dx + dy * dy;
	const float pd = dx * x1 + dy * y1;
	const float t = (dx * x + dy * y - pd) / ds;
	return color1.interp(color2, t);
}

// -----

bool Dictionary::parse(const std::string & line)
{
	bool result = true;
	
	m_map.clear();
	
	std::vector<std::string> parts;
	splitString(line, parts);
	
	for (size_t i = 0; i < parts.size(); ++i)
	{
		const size_t separator = parts[i].find(':');
		
		if (separator == std::string::npos)
		{
			logError("%s: incorrect key:value syntax: %s (%s)", __FUNCTION__, line.c_str(), parts[i].c_str());
			result = false;
			continue;
		}
		
		const std::string key = parts[i].substr(0, separator);
		const std::string value = parts[i].substr(separator + 1, parts[i].size() - separator - 1);
		
		if (key.size() == 0 || value.size() == 0)
		{
			logError("%s: incorrect key:value syntax: %s (%s)", __FUNCTION__, line.c_str(), parts[i].c_str());
			result = false;
			continue;
		}
		
		if (contains(key.c_str()))
		{
			logError("%s: duplicate key: %s (%s)", __FUNCTION__, line.c_str(), key.c_str());
			result = false;
			continue;
		}
		
		setString(key.c_str(), value.c_str());
	}
	
	return result;
}

bool Dictionary::contains(const char * name) const
{
	return m_map.count(name) != 0;
}

void Dictionary::setString(const char * name, const char * value)
{
	m_map[name] = value;
}

void Dictionary::setInt(const char * name, int value)
{
	char text[32];
	sprintf_s(text, sizeof(text), "%d", value);
	setString(name, text);
}

void Dictionary::setBool(const char * name, bool value)
{
	setInt(name, value ? 1 : 0);
}

std::string Dictionary::getString(const char * name, const char * _default) const
{
	Map::const_iterator i = m_map.find(name);
	if (i != m_map.end())
		return i->second;
	else
		return _default;
}

int Dictionary::getInt(const char * name, int _default) const
{
	Map::const_iterator i = m_map.find(name);
	if (i != m_map.end())
		return atoi(i->second.c_str());
	else
		return _default;
}

bool Dictionary::getBool(const char * name, bool _default) const
{
	return getInt(name, _default) != 0;
}

std::string & Dictionary::operator[](const char * name)
{
	return m_map[name];
}

// -----

Sprite::Sprite(const char * filename, float pivotX, float pivotY, const char * spritesheet)
{
	// drawing
	this->pivotX = pivotX;
	this->pivotY = pivotY;
	x = 0.f;
	y = 0.f;
	angle = 0.f;
	scale = 1.f;
	blend = BLEND_ALPHA;
	flipX = false;
	flipY = false;
	pixelpos = true;
	filter = FILTER_POINT;
	
	// animation
	std::string sheetFilename;
	if (spritesheet)
	{
		sheetFilename = spritesheet;
	}
	else
	{
		sheetFilename = filename;
		size_t dot = sheetFilename.rfind('.');
		if (dot != std::string::npos)
			sheetFilename = sheetFilename.substr(0, dot) + ".txt";
	}
	
	m_anim = &g_animCache.findOrCreate(sheetFilename.c_str());
	m_animVersion = m_anim->getVersion();
	m_animSegment = 0;
	animIsActive = false;
	animIsPaused = false;
	m_isAnimStarted = false;
	m_animFramef = 0.f;
	m_animFrame = 0;
	animSpeed = 1.f;
	
	// texture
	m_texture = &g_textureCache.findOrCreate(filename, m_anim->m_gridSize[0], m_anim->m_gridSize[1]);
	
	framework.registerSprite(this);
}

Sprite::~Sprite()
{
	framework.unregisterSprite(this);
}

void Sprite::draw()
{
	drawEx(x, y, angle, scale, blend, pixelpos, filter);
}

void Sprite::drawEx(float x, float y, float angle, float scale, BLEND_MODE blendMode, bool pixelpos, TEXTURE_FILTER filter)
{
	if (m_texture->textures)
	{
		setBlend(blendMode);
		
		glPushMatrix();
		{
			if (pixelpos)
			{
				x = std::floor(x / framework.minification) * framework.minification;
				y = std::floor(y / framework.minification) * framework.minification;
			}
			
			glTranslatef(x, y, 0.f);
			
			if (scale != 1.f)
				glScalef(scale, scale, 1.f);
			if (angle != 0.f)
				glRotatef(angle, 0.f, 0.f, 1.f);
			if (flipX || flipY)
				glScalef(flipX ? -1.f : +1.f, flipY ? -1.f : +1.f, 1.f);
			if (pivotX != 0.f || pivotY != 0.f)
				glTranslatef(-pivotX, -pivotY, 0.f);
			
			int cellIndex;
			
			if (m_animSegment)
			{
				AnimCacheElem::Anim * anim = reinterpret_cast<AnimCacheElem::Anim*>(m_animSegment);
				
				cellIndex = getAnimFrame() + anim->firstCell;
			}
			else
			{
				cellIndex = 0;
			}
			
			fassert(cellIndex >= 0 && cellIndex < (m_anim->m_gridSize[0] * m_anim->m_gridSize[1]));
			
			glBindTexture(GL_TEXTURE_2D, m_texture->textures[cellIndex]);
			glEnable(GL_TEXTURE_2D);

			if (filter == FILTER_POINT)
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			}
			if (filter == FILTER_LINEAR)
			{
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			}
			
			const float rsx = float(m_texture->sx / m_anim->m_gridSize[0]);
			const float rsy = float(m_texture->sy / m_anim->m_gridSize[1]);
			
		#if 1
			const float verts[16] =
			{
				0.f, 0.f, 0.f, 1.f,
				rsx, 0.f, 0.f, 1.f,
				rsx, rsy, 0.f, 1.f,
				0.f, rsy, 0.f, 1.f
			};
			
			static const float texs[8] =
			{
				0.f, 1.f,
				1.f, 1.f,
				1.f, 0.f,
				0.f, 0.f
			};
			
			glEnableClientState(GL_VERTEX_ARRAY);
			glEnableClientState(GL_TEXTURE_COORD_ARRAY);
			glVertexPointer(4, GL_FLOAT, 0, verts);
			glTexCoordPointer(2, GL_FLOAT, 0, texs);
			glDrawArrays(GL_QUADS, 0, 4);
			glDisableClientState(GL_VERTEX_ARRAY);
			glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		#else
			glBegin(GL_QUADS);
			{
		 		glTexCoord2f(0.f, 1.f); glVertex2f(0.f, 0.f);
		 		glTexCoord2f(1.f, 1.f); glVertex2f(rsx, 0.f);
		 		glTexCoord2f(1.f, 0.f); glVertex2f(rsx, rsy);
		 		glTexCoord2f(0.f, 0.f); glVertex2f(0.f, rsy);
			}
			glEnd();
		#endif
			
			glDisable(GL_TEXTURE_2D);
		}
		glPopMatrix();
	}
}

void Sprite::startAnim(const char * name, int frame)
{
	animIsPaused = false;
	m_animSegmentName = name;
	m_isAnimStarted = true;
	m_animFramef = (float)frame;
	m_animFrame = frame;

	m_animVersion = -1;
	updateAnimationSegment();
	
	if (animIsActive)
	{
		processAnimationTriggersForFrame(m_animFrame, AnimCacheElem::AnimTrigger::OnEnter);
	}
}

void Sprite::stopAnim()
{
	animIsActive = false;
	m_isAnimStarted = false;
}

const std::string & Sprite::getAnim() const
{
	return m_animSegmentName;
}

void Sprite::setAnimFrame(int frame)
{
	fassert(frame >= 0);
	
	if (m_animSegment)
	{
		AnimCacheElem::Anim * anim = reinterpret_cast<AnimCacheElem::Anim*>(m_animSegment);
		
		const int frame1 = m_animFrame;
		{
			m_animFramef = (float)frame;
			
			if (anim->loop)
				m_animFrame = frame % anim->numFrames;
			else
				m_animFrame = std::min(frame, anim->numFrames - 1);
		}
		const int frame2 = m_animFrame;
		
		processAnimationFrameChange(frame1, frame2);
	}
}

int Sprite::getAnimFrame() const
{
	return m_animFrame;
}

void Sprite::updateAnimationSegment()
{
	if (m_isAnimStarted && m_animVersion != m_anim->getVersion() && !m_animSegmentName.empty())
	{
		m_animVersion = m_anim->getVersion();
		
		if (m_anim->m_animMap.count(m_animSegmentName) != 0)
			m_animSegment = &m_anim->m_animMap[m_animSegmentName];
		else
			m_animSegment = 0;
		
		if (!m_animSegment)
		{
			log("unable to find animation: %s", m_animSegmentName.c_str());
			animIsActive = false;
			m_animFramef = 0.f;
			m_animFrame = 0;
		}
		else
		{
			AnimCacheElem::Anim * anim = reinterpret_cast<AnimCacheElem::Anim*>(m_animSegment);
			
			this->pivotX = (float)anim->pivot[0];
			this->pivotY = (float)anim->pivot[1];
			
			animIsActive = true;
		}
		
		// recache texture, since the animation grid size may have changed
		m_texture = &g_textureCache.findOrCreate(m_texture->name.c_str(), m_anim->m_gridSize[0], m_anim->m_gridSize[1]);
	}
}

void Sprite::updateAnimation(float timeStep)
{
	if (m_isAnimStarted && m_animSegment && !animIsPaused)
	{
		AnimCacheElem::Anim * anim = reinterpret_cast<AnimCacheElem::Anim*>(m_animSegment);
		
		const int frame1 = m_animFrame;
		{
			const float step = animSpeed * anim->frameRate * timeStep;
			
			m_animFramef += step;
			
			if (!anim->loop)
				m_animFrame = std::min((int)m_animFramef, anim->numFrames - 1);
			else
				m_animFrame = (int)m_animFramef;
		}
		const int frame2 = m_animFrame;
		
		for (int frame = frame1; frame < frame2; frame++)
		{
			const int oldFrame = (frame + 0) % anim->numFrames;
			const int newFrame = (frame + 1) % anim->numFrames;
			processAnimationFrameChange(oldFrame, newFrame);
		}
		
		if (anim->loop)
		{
			m_animFramef = std::fmod(m_animFramef, (float)anim->numFrames);
			m_animFrame = (int)m_animFramef;
		}
		else
		{
			if (m_animFramef >= anim->numFrames)
				animIsActive = false;
		}
		
		//if (m_animSegmentName == "default")
		//	log("%d (%d)", m_animFrame, anim->numFrames);
	}
}

void Sprite::processAnimationFrameChange(int frame1, int frame2)
{
	fassert(animIsActive);
	
	if (frame1 != frame2)
	{
		// process frame triggers
		processAnimationTriggersForFrame(frame1, AnimCacheElem::AnimTrigger::OnLeave);
		processAnimationTriggersForFrame(frame2, AnimCacheElem::AnimTrigger::OnEnter);
	}
}

void Sprite::processAnimationTriggersForFrame(int frame, int event)
{
	fassert(animIsActive);
	
	AnimCacheElem::Anim * anim = reinterpret_cast<AnimCacheElem::Anim*>(m_animSegment);
		
	for (size_t i = 0; i < anim->frameTriggers[frame].size(); ++i)
	{
		const AnimCacheElem::AnimTrigger & trigger = anim->frameTriggers[frame][i];
		
		if (trigger.event == event)
		{
			//log("event == this->event");
			
			Dictionary args = trigger.args;
			args.setInt("x", args.getInt("x", 0) + (int)this->x);
			args.setInt("y", args.getInt("y", 0) + (int)this->y);
			
			framework.processAction(trigger.action, args);
		}
	}
}

int Sprite::getWidth() const
{
	return m_texture->sx / m_anim->m_gridSize[0];
}

int Sprite::getHeight() const
{
	return m_texture->sy / m_anim->m_gridSize[1];
}

// -----

Sound::Sound(const char * filename)
{
	m_sound = &g_soundCache.findOrCreate(filename);
	m_playId = -1;
	m_volume = 100;
	m_speed = 100;
}

void Sound::play(int volume, int speed)
{
	if (volume == -1)
		volume = m_volume;
	if (speed == -1)
		speed = m_speed;
	
	volume = clamp(volume, 0, 100);
	speed = std::max(0, speed);
	
	stop();
	
	if (m_sound->buffer != 0)
	{
		m_playId = g_soundPlayer.playSound(m_sound->buffer, volume / 100.f, false);
	}
}

void Sound::stop()
{
	if (m_playId != -1)
	{
		g_soundPlayer.stopSound(m_playId);
		m_playId = -1;
	}
}

void Sound::setVolume(int volume)
{
	m_volume = volume;
	
	if (m_playId != -1)
	{
		g_soundPlayer.setSoundVolume(m_playId, volume / 100.f);
	}
}

void Sound::setSpeed(int speed)
{
	m_speed = speed;
	
	if (m_playId != -1)
	{
		// todo
	}
}

void Sound::stopAll()
{
	g_soundPlayer.stopAllSounds();
}

// -----

Music::Music(const char * filename)
{
	m_filename = filename;
}

void Music::play()
{
	g_soundPlayer.playMusic(m_filename.c_str());
}

void Music::stop()
{
	g_soundPlayer.stopMusic();
}

void Music::setVolume(int volume)
{
	g_soundPlayer.setMusicVolume(volume / 100.f);
}

// -----

Font::Font(const char * filename)
{
	m_font = &g_fontCache.findOrCreate(filename);
}

// -----

static int getButtonIndex(BUTTON button)
{
	switch (button)
	{
	case BUTTON_LEFT:
		return 0;
	case BUTTON_RIGHT:
		return 1;
	default:
		fassert(false);
	}
	return -1;
}

bool Mouse::isDown(BUTTON button) const
{
	const int index = getButtonIndex(button);
	if (index < 0)
		return false;
	return g_globals.g_mouseDown[index];
}

bool Mouse::wentDown(BUTTON button) const
{
	const int index = getButtonIndex(button);
	if (index < 0)
		return false;
	return isDown(button) && g_globals.g_mouseChange[index];
}

bool Mouse::wentUp(BUTTON button) const
{
	const int index = getButtonIndex(button);
	if (index < 0)
		return false;
	return !isDown(button) && g_globals.g_mouseChange[index];
}

void Mouse::showCursor(bool enabled)
{
	SDL_ShowCursor(enabled ? 1 : 0);
}

// -----

bool Keyboard::isDown(SDLKey key) const
{
	return g_globals.g_keyDown[key];
}

bool Keyboard::wentDown(SDLKey key) const
{
	return isDown(key) && g_globals.g_keyChange[key];
}

bool Keyboard::wentUp(SDLKey key) const
{
	return !isDown(key) && g_globals.g_keyChange[key];
}

// -----

Gamepad::Gamepad()
{
	memset(this, 0, sizeof(Gamepad));
}

float Gamepad::getAnalog(int stick, ANALOG analog, float scale) const
{
	fassert(stick >= 0 && stick <= 1);
	if (stick >= 0 && stick <= 1)
		return m_analog[stick][analog] * scale;
	else
		return 0.f;
}

// -----

StageObject::StageObject()
{
	isDead = false;
	sprite = 0;
}

StageObject::~StageObject()
{
	delete sprite;
	sprite = 0;
}

void StageObject::process(float timeStep)
{
}

void StageObject::draw()
{
	if (sprite)
	{
		sprite->draw();
	}
}

// -----

StageObject_SpriteAnim::StageObject_SpriteAnim(const char * name, const char * anim, const char * sheet)
{
	sprite = new Sprite(name, 0.f, 0.f, sheet);
	
	sprite->startAnim(anim);
}

void StageObject_SpriteAnim::process(float timeStep)
{
	StageObject::process(timeStep);
	
	if (!sprite->animIsActive)
	{
		isDead = true;
	}
}

// -----

Stage::Stage()
{
	m_objectId = 0;
}

void Stage::process(float timeStep)
{
	for (ObjectList::iterator i = m_objects.begin(); i != m_objects.end(); )
	{
		StageObject * obj = i->second;
		
		ObjectList::iterator next = i;
		++next;
		
		fassert(!obj->isDead);
		obj->process(timeStep);
		
		if (obj->isDead)
		{
			delete obj;
			m_objects.erase(i);
		}
		
		i = next;
	}
}

void Stage::draw()
{
	for (ObjectList::iterator i = m_objects.begin(); i != m_objects.end(); ++i)
	{
		StageObject * obj = i->second;
		
		obj->draw();
	}
}

int Stage::addObject(StageObject * object)
{
	const int objectId = m_objectId++;
	
	m_objects.insert(ObjectList::value_type(objectId, object));
	
	return objectId;
}

void Stage::removeObject(int objectId)
{
	ObjectList::iterator i = m_objects.find(objectId);
	
	if (i != m_objects.end())
	{
		log("removing object with object ID %d", objectId);
		StageObject * obj = i->second;
		delete obj;
		m_objects.erase(i);
	}
	else
	{
		log("removing object with object ID %d (but already dead)", objectId);
	}
}

// -----

Ui::Ui()
{
	m_ui = 0;
}

Ui::Ui(const char * filename)
{
	m_ui = 0;
	
	load(filename);
}

void Ui::load(const char * filename)
{
	m_ui = &g_uiCache.findOrCreate(filename);

	m_over.clear();
	m_down.clear();
}

std::string Ui::getImage(Dictionary & d)
{
	const std::string name = d.getString("name", "");
	const std::string type = d.getString("type", "");
	const bool isOver = name == m_over;
	const bool isDown = name == m_down && type == "button";
	const std::string filenameDefault = d.getString("image", "");
	const std::string filenameOver = d.getString("image_over", filenameDefault.c_str());
	const std::string filenameDown = d.getString("image_down", filenameDefault.c_str());
	const std::string & filename = (isDown && m_over == m_down) ? filenameDown : isOver ? filenameOver : filenameDefault;
	return filename;
}

bool Ui::getArea(Dictionary & d, int & x, int & y, int & sx, int & sy)
{
	const std::string type = d.getString("type", "");
	
	if (type == "image" || type == "button")
	{
		const Sprite sprite(getImage(d).c_str());
		const int scale = d.getInt("scale", 1);
		x = d.getInt("x", 0);
		y = d.getInt("y", 0);
		sx = d.getInt("w", sprite.getWidth()) * scale;
		sy = d.getInt("h", sprite.getHeight()) * scale;
		return true;
	}
	else
	{
		return false;
	}
}

std::string Ui::findMouseOver()
{
	for (UiCacheElem::Map::iterator i = m_ui->map.begin(); i != m_ui->map.end(); ++i)
	{
		Dictionary & d = i->second;
		
		int x, y, sx, sy;
		
		if (getArea(d, x, y, sx, sy))
		{
			if (mouse.x >= x && mouse.x < x + sx && mouse.y >= y && mouse.y < y + sy)
			{
				return i->first;
			}
		}
	}
	
	return "";
}

void Ui::process()
{
	// make sure the names are good..
	
	for (UiCacheElem::Map::iterator i = m_ui->map.begin(); i != m_ui->map.end(); ++i)
	{
		Dictionary & d = i->second;
		
		d.setString("name", i->first.c_str());
	}
	
	// update mouse over
	
	std::string over = findMouseOver();
	
	m_over = over;
	
	// update mouse down
	
	if (mouse.wentDown(BUTTON_LEFT))
	{
		m_down = m_over;
	}
	else if (mouse.wentUp(BUTTON_LEFT))
	{
		if (!m_down.empty())
		{
			if (m_down == m_over)
			{
				// trigger action, if set
				Dictionary & d = m_ui->map[m_down];
				const std::string action = d.getString("action", "");
				if (!action.empty())
					framework.processAction(action, d);
			}
			m_down.clear();
		}
	}
}

void Ui::draw()
{
	for (UiCacheElem::Map::iterator i = m_ui->map.begin(); i != m_ui->map.end(); ++i)
	{
		Dictionary & d = i->second;
		
		const bool isVisible = d.getBool("visible", true);
		
		if (isVisible)
		{
			const std::string type = d.getString("type", "");
			
			if (type == "image" || type == "button")
			{
				Sprite(getImage(d).c_str()).drawEx(
					float(d.getInt("x", 0)),
					float(d.getInt("y", 0)),
					0.f,
					float(d.getInt("scale", 1)));
			}
		}
	}
}

Dictionary & Ui::operator[](const char * name)
{
	return m_ui->map[name];
}

// -----

static const int maxStackSize = 32;
static Surface * stack[maxStackSize];
static int stackSize = 0;

static void setSurface(Surface * surface)
{
	const GLuint buffer = surface ? surface->getFramebuffer() : 0;
	const int sx = surface ? surface->getWidth() / framework.minification : g_globals.g_displaySize[0] / framework.minification;
	const int sy = surface ? surface->getHeight() / framework.minification : g_globals.g_displaySize[1] / framework.minification;
	
	glBindFramebuffer(GL_FRAMEBUFFER, buffer);
	glViewport(0, 0, sx, sy);
	
	//GLenum drawBuffers[] = { GL_COLOR_ATTACHMENT0 };
	//glDrawBuffers(1, drawBuffers);
}

void pushSurface(Surface * surface)
{
	fassert(stackSize < maxStackSize);
	stack[stackSize++] = surface;
	setSurface(surface);
	checkErrorGL();
}

void popSurface()
{
	fassert(stackSize > 0);
	stack[--stackSize] = 0;
	Surface * surface = stackSize ? stack[stackSize - 1] : 0;
	setSurface(surface);
	checkErrorGL();
}

void setDrawRect(int x, int y, int sx, int sy)
{
	y = g_globals.g_displaySize[1] - y - sy;
	
	x /= framework.minification;
	y /= framework.minification;
	sx /= framework.minification;
	sy /= framework.minification;
	
	glScissor(x, y, sx, sy);
	glEnable(GL_SCISSOR_TEST);
}

void clearDrawRect()
{
	glDisable(GL_SCISSOR_TEST);
}

void setBlend(BLEND_MODE blendMode)
{
	switch (blendMode)
	{
	case BLEND_OPAQUE:
		glDisable(GL_BLEND);
		break;
	case BLEND_ALPHA:
		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;
	case BLEND_ADD:
		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_ONE, GL_ONE);
		break;
	case BLEND_SUBTRACT:
		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_SUBTRACT);
		glBlendFunc(GL_ONE, GL_ONE);
		break;
	case BLEND_INVERT:
		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_ONE_MINUS_DST_COLOR, GL_ZERO);
		break;
	case BLEND_MUL:
		glEnable(GL_BLEND);
		glBlendEquation(GL_FUNC_ADD);
		glBlendFunc(GL_ZERO, GL_SRC_COLOR);
		break;
	default:
		fassert(false);
		break;
	}
}

void setColorMode(COLOR_MODE colorMode)
{
	g_globals.g_colorMode = colorMode;
	
	switch (colorMode)
	{
	case COLOR_MUL:
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		break;
	case COLOR_ADD:
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD);
		break;
	case COLOR_SUB:
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_SUBTRACT);
		break;
	case COLOR_IGNORE:
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
		break;
	default:
		fassert(false);
		break;
	}
}

void setColor(const Color & color)
{
	setColorf(color.r, color.g, color.b, color.a);
}

void setColor(int r, int g, int b, int a, int rgbMul)
{
	setColorf(r/255.f, g/255.f, b/255.f, a/255.f, rgbMul/255.f);
}

void setColorf(float r, float g, float b, float a, float rgbMul)
{
	r *= rgbMul;
	g *= rgbMul;
	b *= rgbMul;
	
	r = clamp(r, 0.f, 1.f);
	g = clamp(g, 0.f, 1.f);
	b = clamp(b, 0.f, 1.f);
	a = clamp(a, 0.f, 1.f);
	
	glColor4f(r, g, b, a);
}

void setGradientf(float x1, float y1, const Color & color1, float x2, float y2, const Color & color2)
{
	g_globals.g_gradient.set(x1, y1, color1, x2, y2, color2);
}

void setGradientf(float x1, float y1, float r1, float g1, float b1, float a1, float x2, float y2, float r2, float g2, float b2, float a2)
{
	setGradientf(x1, y1, Color(r1, g1, b1, a1), x2, y2, Color(r2, g2, b2, a2));
}

void setFont(Font & font)
{
	g_globals.g_font = font.getFont();
}

void setShader(Shader & shader)
{
	glUseProgram(shader.getProgram());
}

void clearShader()
{
	glUseProgram(0);
}

void drawLine(float x1, float y1, float x2, float y2)
{
	glBegin(GL_LINES);
	{
		glVertex2f(x1, y1);
		glVertex2f(x2, y2);
	}
	glEnd();
}

void drawRect(float x1, float y1, float x2, float y2)
{
	glBegin(GL_QUADS);
	{
		glTexCoord2f(0.f, 1.f); glVertex2f(x1, y1);
		glTexCoord2f(1.f, 1.f); glVertex2f(x2, y1);
		glTexCoord2f(1.f, 0.f); glVertex2f(x2, y2);
		glTexCoord2f(0.f, 0.f); glVertex2f(x1, y2);
	}
	glEnd();
}

void drawRectGradient(float x1, float y1, float x2, float y2)
{
	float oldColor[4];
	glGetFloatv(GL_CURRENT_COLOR, oldColor);

	const Color color[4] =
	{
		g_globals.g_gradient.eval(x1, y1),
		g_globals.g_gradient.eval(x2, y1),
		g_globals.g_gradient.eval(x2, y2),
		g_globals.g_gradient.eval(x1, y2)
	};
	
	glBegin(GL_QUADS);
	{
		glColor4f(color[0].r, color[0].g, color[0].b, color[0].a);
		glVertex2f(x1, y1);
		glColor4f(color[1].r, color[1].g, color[1].b, color[1].a);
		glVertex2f(x2, y1);
		glColor4f(color[2].r, color[2].g, color[2].b, color[2].a);
		glVertex2f(x2, y2);
		glColor4f(color[3].r, color[3].g, color[3].b, color[3].a);
		glVertex2f(x1, y2);
	}
	glEnd();
	
	glColor4fv(oldColor);
}

static void measureText(FT_Face face, int size, const char * text, float & sx, float & sy)
{
	float minX = std::numeric_limits<float>::max();
	float minY = std::numeric_limits<float>::max();
	float maxX = std::numeric_limits<float>::min();
	float maxY = std::numeric_limits<float>::min();
	
	float x = 0.f;
	float y = 0.f;
	
	for (int i = 0; text[i]; ++i)
	{
		// find or create glyph. skip current character if the element is invalid
		
		const GlyphCacheElem & elem = g_glyphCache.findOrCreate(face, size, text[i]);
		
		if (elem.texture != 0)
		{
			const float bsx = float(elem.g.bitmap.width);
			const float bsy = float(elem.g.bitmap.rows);
			const float x1 = x + elem.g.bitmap_left;
			const float y1 = y - elem.g.bitmap_top;
			const float x2 = x1 + bsx;
			const float y2 = y1 + bsy;
			
			minX = std::min(minX, std::min(x1, x2));
			minY = std::min(minY, std::min(y1, y2));
			maxX = std::max(maxX, std::max(x1, x2));
			maxY = std::max(maxY, std::max(y1, y2));
	 		
			x += (elem.g.advance.x / float(1 << 6));
			y += (elem.g.advance.y / float(1 << 6));
		}
	}
	
	sx = maxX - minX;
	sy = maxY - minY;
}

static void drawTextInternal(FT_Face face, int size, const char * text)
{
	float x = 0.f;
	float y = 0.f;
	
	// the (0,0) coordinate represents the lower left corner of a glyph
	// we want to render the glyph using its top left corner at (0,0)
	
	y += size;
	
	for (int i = 0; text[i]; ++i)
	{
		// find or create glyph. skip current character if the element is invalid
		
		const GlyphCacheElem & elem = g_glyphCache.findOrCreate(face, size, text[i]);
		
		if (elem.texture != 0)
		{
			glBindTexture(GL_TEXTURE_2D, elem.texture);
			glEnable(GL_TEXTURE_2D);
			
			glBegin(GL_QUADS);
			{
				const float sx = float(elem.g.bitmap.width);
				const float sy = float(elem.g.bitmap.rows);
				const float x1 = x + elem.g.bitmap_left;
				const float y1 = y - elem.g.bitmap_top;
				const float x2 = x1 + sx;
				const float y2 = y1 + sy;
				
		 		glTexCoord2f(0.f, 0.f); glVertex2f(x1, y1);
		 		glTexCoord2f(1.f, 0.f); glVertex2f(x2, y1);
		 		glTexCoord2f(1.f, 1.f); glVertex2f(x2, y2);
		 		glTexCoord2f(0.f, 1.f); glVertex2f(x1, y2);
			}
			glEnd();
			
			glDisable(GL_TEXTURE_2D);
	 		
			x += (elem.g.advance.x / float(1 << 6));
			y += (elem.g.advance.y / float(1 << 6));
		}
	}
}

void drawText(float x, float y, int size, float alignX, float alignY, const char * format, ...)
{
	char text[1024];
	va_list args;
	va_start(args, format);
	vsprintf_s(text, sizeof(text), format, args);
	va_end(args);
	
	glPushMatrix();
	{
		float sx, sy;
		measureText(g_globals.g_font->face, size, text, sx, sy);
		
		x += sx * (alignX - 1.f) / 2.f;
		y += sy * (alignY - 1.f) / 2.f;
		
		glTranslatef(x, y, 0.f);
		
		drawTextInternal(g_globals.g_font->face, size, text);
	}
	glPopMatrix();
}

void changeDirectory(const char * path)
{
	_chdir(path);
}

#if ENABLE_LOGGING

void logDebug(const char * format, ...)
{
	char text[1024];
	va_list args;
	va_start(args, format);
	vsprintf_s(text, sizeof(text), format, args);
	va_end(args);
	
	fprintf(stderr, "[DD] %s\n", text);
}

void log(const char * format, ...)
{
	char text[1024];
	va_list args;
	va_start(args, format);
	vsprintf_s(text, sizeof(text), format, args);
	va_end(args);
	
	fprintf(stderr, "[II] %s\n", text);
}

void logWarning(const char * format, ...)
{
	char text[1024];
	va_list args;
	va_start(args, format);
	vsprintf_s(text, sizeof(text), format, args);
	va_end(args);
	
	fprintf(stderr, "[WW] %s\n", text);
}

void logError(const char * format, ...)
{
	char text[1024];
	va_list args;
	va_start(args, format);
	vsprintf_s(text, sizeof(text), format, args);
	va_end(args);
	
	fprintf(stderr, "[EE] %s\n", text);
}

#endif
