#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
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

// -----

Framework::Framework()
{
	m_minification = 1;
}

Framework::~Framework()
{
}

void Framework::setMinification(int scale)
{
	m_minification = scale;
}

void Framework::init(int argc, char * argv[], int sx, int sy)
{
	// initialize SDL
	
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_SetVideoMode(sx / m_minification, sy / m_minification, 32, SDL_OPENGL);
	
	g_globals.g_displaySize[0] = sx;
	g_globals.g_displaySize[1] = sy;
	
	// initialize FreeType
	
	if (FT_Init_FreeType(&g_globals.g_freeType) != 0)
		printf("failed to initialize FreeType\n");
}

void Framework::shutdown()
{
	// free resources
	
	g_textureCache.clear();
	g_soundCache.clear();
	g_fontCache.clear();
	
	// shut down FreeType
	
	if (FT_Done_FreeType(g_globals.g_freeType) != 0)
		printf("failed to shut down FreeType\n");
	g_globals.g_freeType = 0;
	
	// shut down SDL
	
	SDL_Quit();
	
	// clear globals
	
	g_globals = Globals();
	
	// reset self
	
	m_minification = 1;
}

void Framework::process()
{
	bool doReload = !keyboard.isDown(SDLK_F12);
	
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
			g_globals.g_mouseX = e.motion.x * m_minification;
			g_globals.g_mouseY = e.motion.y * m_minification;
		}
	}
	
	doReload &= keyboard.isDown(SDLK_F12);
	
	if (doReload)
	{
		reloadCaches();
	}
}

void Framework::reloadCaches()
{
	g_textureCache.reload();
	g_soundCache.reload();
	g_fontCache.reload();
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
		printf("OpenGL error: %x\n", error);
		
	// flip back buffers
	
	SDL_GL_SwapBuffers();
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

Sprite::Sprite(const char * filename, float pivotX, float pivotY)
{
	m_texture = &g_textureCache.findOrCreate(filename);
	
	m_pivotX = pivotX;
	m_pivotY = pivotY;
	m_positionX = 0.f;
	m_positionY = 0.f;
	m_angle = 0.f;
	m_scaleX = 1.f;
	m_scaleY = 1.f;
	m_blendMode = BLEND_ALPHA;
	m_flipX = false;
	m_flipY = false;
}

void Sprite::draw()
{
	drawEx(m_positionX, m_positionY, m_angle, m_scaleX, m_blendMode);
}

void Sprite::drawEx(float x, float y, float angle, float scale, BLEND_MODE blendMode)
{
	// todo: set blend mode
	
	// todo: setup matrices
	
	// todo: draw
}

void Sprite::setPosition(float x, float y)
{
	m_positionX = x;
	m_positionY = y;
}

void Sprite::setAngle(float angle)
{
	m_angle = angle;
}

void Sprite::setScale(float scale)
{
	m_scaleX = scale;
	m_scaleY = scale;
}

void Sprite::setBlend(BLEND_MODE blendMode)
{
	m_blendMode = blendMode;
}

void Sprite::setFlip(bool flipX, bool flipY)
{
	m_flipX = flipX;
	m_flipY = flipY;
}

void Sprite::setAnim(const char * anim)
{
	// todo
}

void Sprite::setFrame(int frame)
{
	// todo
}

// -----

Sound::Sound(const char * filename)
{
	m_sound = &g_soundCache.findOrCreate(filename);
}

void Sound::play()
{
}

void Sound::stop()
{
}

void Sound::setVolume(float volume)
{
}

void Sound::setSpeed(float speed)
{
}

void Sound::stopAll()
{
}

// -----

Music::Music(const char * filename)
{
	// todo
}

void Music::play()
{
	// todo
}

void Music::stop()
{
	// todo
}

void Music::setVolume(float volume)
{
	// todo
}

// -----

Font::Font(const char * filename)
{
	m_font = &g_fontCache.findOrCreate(filename);
}

// -----

float Mouse::getX()
{
	return g_globals.g_mouseX;
}

float Mouse::getY()
{
	return g_globals.g_mouseY;
}

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

bool Gamepad::isConnected()
{
	return false;
}

bool Gamepad::isDown(GAMEPAD button)
{
	return false;
}

float Gamepad::getAnalog(int stick, ANALOG analog)
{
	return 0.f;
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
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		//assert(false); // todo: blend op
		break;
	case BLEND_ADD:
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		assert(false);
		break;
	case BLEND_SUBTRACT:
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		assert(false);
		break;
	default:
		assert(false);
		break;
	}
}

void setColor(const Color & color)
{
	setColor(color.r, color.g, color.b, color.a);
}

void setColor(int r, int g, int b, int a)
{
	setColor(r/255.f, g/255.f, b/255.f, a/255.f);
}

void setColorf(float r, float g, float b, float a)
{
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
