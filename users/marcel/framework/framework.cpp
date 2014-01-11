#define NOMINMAX

#include <assert.h>
#include <cmath>
#include <map>
#include <stdarg.h>
#include <stdlib.h>
#include <string>
#include <vector>

#ifdef WIN32
	#include <SDL/SDL_opengl.h>
	#include <Windows.h>
	#include <Xinput.h>
	static PFNGLBLENDEQUATIONPROC glBlendEquation = 0;
#else
	#include <dirent.h>
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
	windowTitle = "GGJ 2014 - Unknown Project";
	numSoundSources = 32;
	actionHandler = 0;
	
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
	
	SDL_WM_SetCaption(windowTitle.c_str(), 0);
	
	g_globals.g_displaySize[0] = sx;
	g_globals.g_displaySize[1] = sy;

#ifdef WIN32
	if (glBlendEquation == 0)
	{
		glBlendEquation = (PFNGLBLENDEQUATIONPROC)wglGetProcAddress("glBlendEquation");
	
		if (glBlendEquation == 0)
		{
			logError("unable to find required OpenGL extension(s)");
			return false;
		}
	}
#endif
	
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
	
#ifdef WIN32
	glBlendEquation = 0;
#endif

	// shut down SDL
	
	SDL_Quit();
	
	// clear globals
	
	g_globals = Globals();
	
	// reset self
	
	minification = 1;
	numSoundSources = 32;
	actionHandler = 0;
	
	return result;
}

void Framework::process()
{
	g_soundPlayer.process();
	
	bool doReload = !keyboard.isDown(SDLK_r);
	
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
	
	doReload &= keyboard.isDown(SDLK_r);
	
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
			FILE * file = fopen(f, "rb");
			if (file)
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
	
	GLenum error = glGetError();
	
	if (error != GL_NO_ERROR)
		logError("OpenGL error: %x", error);
		
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
	drawEx(x, y, angle, scale, blend, pixelpos);
}

void Sprite::drawEx(float x, float y, float angle, float scale, BLEND_MODE blendMode, bool pixelpos)
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
				0.f, 0.f,
				1.f, 0.f,
				1.f, 1.f,
				0.f, 1.f
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
		 		glTexCoord2f(0.f, 0.f); glVertex2f(0.f, 0.f);
		 		glTexCoord2f(1.f, 0.f); glVertex2f(rsx, 0.f);
		 		glTexCoord2f(1.f, 1.f); glVertex2f(rsx, rsy);
		 		glTexCoord2f(0.f, 1.f); glVertex2f(0.f, rsy);
			}
			glEnd();
		#endif
			
			//glDisable(GL_TEXTURE_2D);
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
					d.getInt("x", 0),
					d.getInt("y", 0),
					0,
					d.getInt("scale", 1));
			}
		}
	}
}

Dictionary & Ui::operator[](const char * name)
{
	return m_ui->map[name];
}

// -----

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
	default:
		fassert(false);
		break;
	}
}

void setColor(const Color & color)
{
	setColorf(color.r, color.g, color.b, color.a);
}

void setColor(int r, int g, int b, int a)
{
	setColorf(r/255.f, g/255.f, b/255.f, a/255.f);
}

void setColorf(float r, float g, float b, float a)
{
	r = clamp(r, 0.f, 1.f);
	g = clamp(g, 0.f, 1.f);
	b = clamp(b, 0.f, 1.f);
	a = clamp(a, 0.f, 1.f);
	
	glColor4f(r, g, b, a);
}

void setFont(Font & font)
{
	g_globals.g_font = font.getFont();
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
		glVertex2f(x1, y1);
		glVertex2f(x2, y2);
	}
	glEnd();
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

void drawText(float x, float y, int size, int alignX, int alignY, const char * format, ...)
{
	char text[1024];
	va_list args;
	va_start(args, format);
	vsprintf_s(text, sizeof(text), format, args);
	va_end(args);
	
	glPushMatrix();
	{
		glTranslatef(x, y, 0.f);
		
		drawTextInternal(g_globals.g_font->face, size, text);
	}
	glPopMatrix();
}

#if ENABLE_LOGGING

void logDebug(const char * format, ...)
{
	char text[1024];
	va_list args;
	va_start(args, format);
	vsprintf(text, format, args); // todo: safer version
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
