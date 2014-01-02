#include <assert.h>
#include <cmath>
#include <map>
#include <stdarg.h>
#include <stdlib.h>
#include <string>
#include <vector>

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

// -----

template <typename T>
static T clamp(T v, T vmin, T vmax)
{
	return std::min(std::max(v, vmin), vmax);
}

// -----

Framework::Framework()
{
	m_minification = 1;
	m_numSoundSources = 32;
	m_actionHandler = 0;
	
	timeStep = 1.f / 60.f;
}

Framework::~Framework()
{
	shutdown();
}

void Framework::setMinification(int scale)
{
	m_minification = scale;
}

void Framework::setNumSoundSources(int num)
{
	m_numSoundSources = num;
}

void Framework::setActionHandler(ActionHandler actionHandler)
{
	m_actionHandler = actionHandler;
}

bool Framework::init(int argc, char * argv[], int sx, int sy)
{
	// initialize SDL
	
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
	{
		logError("failed to initialize SDL");
		return false;
	}
	
	if (SDL_SetVideoMode(sx / m_minification, sy / m_minification, 32, SDL_OPENGL) == 0)
	{
		logError("failed to set video mode");
		return false;
	}
	
	g_globals.g_displaySize[0] = sx;
	g_globals.g_displaySize[1] = sy;
	
	// initialize FreeType
	
	if (FT_Init_FreeType(&g_globals.g_freeType) != 0)
	{
		logError("failed to initialize FreeType");
		return false;
	}
	
	// initialize sound player
	
	if (!g_soundPlayer.init(m_numSoundSources))
	{
		logError("failed to initialize sound player");
		return false;
	}
	
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
	
	// shut down FreeType
	
	if (FT_Done_FreeType(g_globals.g_freeType) != 0)
	{
		logError("failed to shut down FreeType");
		result = false;
	}
	g_globals.g_freeType = 0;
	
	// shut down SDL
	
	SDL_Quit();
	
	// clear globals
	
	g_globals = Globals();
	
	// reset self
	
	m_minification = 1;
	m_numSoundSources = 32;
	m_actionHandler = 0;
	
	return result;
}

void Framework::process()
{
	g_soundPlayer.process();
	
	bool doReload = !keyboard.isDown(SDLK_r);
	
	// poll SDL event queue
	
	SDL_Event e;
	
	while (SDL_PollEvent(&e))
	{
		if (e.type == SDL_KEYDOWN || e.type == SDL_KEYUP)
		{
			if (e.key.keysym.sym >= 0 && e.key.keysym.sym < SDLK_LAST)
				g_globals.g_keyDown[e.key.keysym.sym] = e.key.state == SDL_PRESSED;
		}
		else if (e.type == SDL_MOUSEBUTTONDOWN || e.type == SDL_MOUSEBUTTONUP)
		{
			const int index = e.button.type == SDL_BUTTON_LEFT ? 0 : e.button.type == SDL_BUTTON_RIGHT ? 1 : -1;
			if (index >= 0)
				g_globals.g_mouseDown[index] = e.button.state == SDL_PRESSED;
		}
		else if (e.type == SDL_MOUSEMOTION)
		{
			mouse.x = e.motion.x * m_minification;
			mouse.y = e.motion.y * m_minification;
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
        	
        	const Gamepad & g = state.Gamepad;
        	
       	#define APPLY_DEADZONE(v, t) (std::abs(v) <= t ? 0.f : clamp((std::abs(v) - t) * std::sign(v) / float(32767 - t), -1.f, +1.f))
       	
       		float ** analog = gamepad[i].m_analog;
       		
			analog[0][ANALOG_X] = APPLY_DEADZONE(g.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
			analog[0][ANALOG_Y] = APPLY_DEADZONE(g.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
			analog[1][ANALOG_X] = APPLY_DEADZONE(g.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
			analog[1][ANALOG_Y] = APPLY_DEADZONE(g.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
			
		#undef APPLY_DEADZONE
			
			const int buttons = g.wButtons;
			bool * isDown = gamepad[i].isDown;
			
			isDown[DPAD_LEFT]     = buttons & XINPUT_GAMEPAD_DPAD_LEFT;
			isDown[DPAD_RIGHT]    = buttons & XINPUT_GAMEPAD_DPAD_RIGHT;
			isDown[DPAD_UP]       = buttons & XINPUT_GAMEPAD_DPAD_UP;
			isDown[DPAD_DOWN]     = buttons & XINPUT_GAMEPAD_DPAD_DOWN;
			isDown[GAMEPAD_A]     = buttons & XINPUT_GAMEPAD_A;
			isDown[GAMEPAD_B]     = buttons & XINPUT_GAMEPAD_B;
			isDown[GAMEPAD_X]     = buttons & XINPUT_GAMEPAD_X;
			isDown[GAMEPAD_Y]     = buttons & XINPUT_GAMEPAD_Y;
			isDown[GAMEPAD_L1]    = buttons & XINPUT_GAMEPAD_LEFT_SHOULDER;
			isDown[GAMEPAD_R1]    = buttons & XINPUT_GAMEPAD_RIGHT_SHOULDER;
			isDown[GAMEPAD_L2]    = g.bLeftTrigger  > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
			isDown[GAMEPAD_R2]    = g.bRightTrigger > XINPUT_GAMEPAD_TRIGGER_THRESHOLD;
			isDown[GAMEPAD_START] = buttons & XINPUT_GAMEPAD_START;
			isDown[GAMEPAD_BACK]  = buttons & XINPUT_GAMEPAD_BACK;
        }
        else
        {
        	memset(&gamepad[i], 0, sizeof(Gamepad));
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
	
	if (m_actionHandler)
	{
		m_actionHandler(action, args);
	}
}

void Framework::reloadCaches()
{
	g_textureCache.reload();
	g_animCache.reload();
	g_soundCache.reload();
	g_fontCache.reload();
	g_glyphCache.clear();
	
	g_globals.g_resourceVersion++;
	
	for (SpriteSet::iterator i = m_sprites.begin(); i != m_sprites.end(); ++i)
	{
		(*i)->updateAnimationSegment();
	}
}

void Framework::beginDraw(int r, int g, int b, int a)
{
	// clear back buffer
	
	glClearColor(r/255.f, g/255.f, b/255.f, a/255.f);
	glClear(GL_COLOR_BUFFER_BIT);
	
	// initialize viewport and OpenGL matrices
	
	glViewport(0, 0, g_globals.g_displaySize[0] / m_minification, g_globals.g_displaySize[1] / m_minification);
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
	sprintf(text, "%d", value);
	setString(name, text);
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
	drawEx(x, y, angle, scale, blend);
}

void Sprite::drawEx(float x, float y, float angle, float scale, BLEND_MODE blendMode)
{
	if (m_texture->textures)
	{
		setBlend(blendMode);
		
		glPushMatrix();
		{
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
			
			const int rsx = m_texture->sx / m_anim->m_gridSize[0];
			const int rsy = m_texture->sy / m_anim->m_gridSize[1];
			
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
	m_animFramef = frame;
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
			m_animFramef = frame;
			
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
			
			this->pivotX = anim->pivot[0];
			this->pivotY = anim->pivot[1];
			
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
			m_animFramef = std::fmod(m_animFramef, anim->numFrames);
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
			args.setInt("x", args.getInt("x", 0) + this->x);
			args.setInt("y", args.getInt("y", 0) + this->y);
			
			framework.processAction(trigger.action, args);
		}
	}
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

bool Mouse::isDown(BUTTON button)
{
	switch (button)
	{
	case BUTTON_LEFT:
		return g_globals.g_mouseDown[0];
	case BUTTON_RIGHT:
		return g_globals.g_mouseDown[1];
	}
	
	return false;
}

// -----

bool Keyboard::isDown(SDLKey key)
{
	return g_globals.g_keyDown[key];
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
				const float sx = elem.g.bitmap.width;
				const float sy = elem.g.bitmap.rows;
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
	vsprintf(text, format, args); // todo: safer version
	va_end(args);
	
	glPushMatrix();
	{
		glTranslatef(x, y, 0.f);
		
		drawTextInternal(g_globals.g_font->face, size, text);
	}
	glPopMatrix();
}

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
	vsprintf(text, format, args); // todo: safer version
	va_end(args);
	
	fprintf(stderr, "[II] %s\n", text);
}

void logWarning(const char * format, ...)
{
	char text[1024];
	va_list args;
	va_start(args, format);
	vsprintf(text, format, args); // todo: safer version
	va_end(args);
	
	fprintf(stderr, "[WW] %s\n", text);
}

void logError(const char * format, ...)
{
	char text[1024];
	va_list args;
	va_start(args, format);
	vsprintf(text, format, args); // todo: safer version
	va_end(args);
	
	fprintf(stderr, "[EE] %s\n", text);
}
