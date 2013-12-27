#include <ft2build.h>
#include FT_FREETYPE_H
#include <map>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <string.h>

class GlyphCache
{
public:
	class Elem
	{
	public:
		FT_GlyphSlotRec g;
		GLuint texture;
	};
	
private:
	class Key
	{
	public:
		FT_Face face;
		int size;
		char c;
		
		inline bool operator<(const Key & other) const
		{
			if (face != other.face)
				return face < other.face;
			if (size != other.size)
				return size < other.size;
			return c < other.c;
		}
	};
	
	typedef std::map<Key, Elem> Map;
	
	Map m_map;
	
public:
	~GlyphCache()
	{
		Clear();
	}
	
	void Clear()
	{
		for (Map::iterator i = m_map.begin(); i != m_map.end(); ++i)
		{
			if (i->second.texture != 0)
			{
				glDeleteTextures(1, &i->second.texture);
			}
		}
		
		m_map.clear();
	}
	
	const Elem & FindOrCreate(FT_Face face, int size, char c)
	{
		Key key;
		key.face = face;
		key.size = size;
		key.c = c;
		
		Map::iterator i = m_map.find(key);
		
		if (i != m_map.end())
		{
			return i->second;
		}
		else
		{
			// lookup failed. render the glyph and add the new element to the cache
			
			Elem elem;
			
			FT_Set_Pixel_Sizes(face, 0, size);
			
			if (FT_Load_Char(face, c, FT_LOAD_RENDER) == 0)
			{
				// capture current OpenGL states before we change them
				
				GLuint restoreTexture;
				glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
				GLint restoreUnpack;
				glGetIntegerv(GL_UNPACK_ALIGNMENT, &restoreUnpack);
				
				// create texture and copy image data
				
				elem.g = *face->glyph;
				glGenTextures(1, &elem.texture);
				
				glBindTexture(GL_TEXTURE_2D, elem.texture);
				glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
				glTexImage2D(
					GL_TEXTURE_2D,
					0,
					GL_ALPHA,
					elem.g.bitmap.width,
					elem.g.bitmap.rows,
					0,
					GL_ALPHA,
					GL_UNSIGNED_BYTE,
					elem.g.bitmap.buffer);
				
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				
				// restore previous OpenGL states
				
				glBindTexture(GL_TEXTURE_2D, restoreTexture);
				glPixelStorei(GL_UNPACK_ALIGNMENT, restoreUnpack);
			}
			else
			{
				// failed to render the glyph. return the NULL texture handle
				
				elem.texture = 0;
			}
			
			i = m_map.insert(Map::value_type(key, elem)).first;
			
			printf("added glyph cache element. face=%p, size=%d, character=%c, texture=%u. count=%d\n", face, size, c, elem.texture, (int)m_map.size());
			
			return i->second;
		}
	}
};

static GlyphCache g_glyphCache;

void drawText(FT_Face face, const char * text, int size)
{
	float x = 0.f;
	float y = 0.f;
	
	// the (0,0) coordinate represents the lower left corner of a glyph
	// we want to render the glyph using its top left corner at (0,0)
	
	y += size;
	
	for (int i = 0; text[i]; ++i)
	{
		// find or create glyph. skip current character if the element is invalid
		
		const GlyphCache::Elem & elem = g_glyphCache.FindOrCreate(face, size, text[i]);
		
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

void drawLine(float x1, float y1, float x2, float y2)
{
	glBegin(GL_LINES);
	{
		glVertex2f(x1, y1);
		glVertex2f(x2, y2);
	}
	glEnd();
}

int main(int argc, char * argv[])
{
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_Surface * surface = SDL_SetVideoMode(600, 400, 32, SDL_OPENGL);
	if (!surface)
		return 0;
	
	// initialize FreeType
	
	FT_Library ft;
 	
	if (FT_Init_FreeType(&ft))
	{
		printf("failed to initialize FreeType\n");
		exit(-1);
	}
	
	// open font files
	
	FT_Face face1;
	
	if (FT_New_Face(ft, "test1.ttf", 0, &face1))
	{
		printf("unable to open font\n");
		exit(-1);
	}
	
	FT_Face face2;
	
	if (FT_New_Face(ft, "test2.ttf", 0, &face2))
	{
		printf("unable to open font\n");
		exit(-1);
	}
	
	FT_Face face3;
	
	if (FT_New_Face(ft, "test3.ttf", 0, &face3))
	{
		printf("unable to open font\n");
		exit(-1);
	}
	
	// enter loop
	
	bool stop = false;
	
	int frameCount = 0;
	
	int mouseX = 300;
	int mouseY = 200;
	
	while (!stop)
	{
		// poll SDL event queue
		
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_KEYDOWN)
			{
				if (e.key.keysym.sym == SDLK_ESCAPE)
					stop = true;
				else
					g_glyphCache.Clear();
			}
			else if (e.type == SDL_MOUSEMOTION)
			{
				mouseX = e.motion.x;
				mouseY = e.motion.y;
			}
		}
		
		// initialize viewport and OpenGL matrices
		
		glViewport(0, 0, 600, 400);
		glMatrixMode(GL_PROJECTION);
		{
			glLoadIdentity();
			glScalef(1.f, -1.f, 1.f); // flip Y axis so the vertical axis runs top to bottom
			// convert from (0,0),(1,1) to (-1,-1),(+1+1)
			glTranslatef(-1.f, -1.f, 0.f);
			glScalef(2.f, 2.f, 1.f);
			// convert from (0,0),(600,400) to (0,0),(1,1)
			glScalef(1.f/600.f, 1.f/400.f, 1.f);
		}
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		
		// clear render target
		
		glClearColor(1.f, 1.f, 1.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);
		
		// setup blend mode
		
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		
		// draw lines
		
		glColor4ub(255, 0, 0, 255);
		drawLine(0.f, 200.f, 600.f, 200.f);
		drawLine(300.f, 0.f, 300.f, 400.f);
		
		// draw text
		
		char text[32];
		sprintf(text, "Hello world ! %d", frameCount);
		
		const int numLoops = frameCount % 32;
		
		for (int i = numLoops - 1; i >= 0; --i)
		{
			glPushMatrix();
			{
				glTranslatef(300.f, 200.f, 0.f);
				glRotatef(frameCount + i * 11, 0.f, 0.f, 1.f);
				glColor4ub(i == 0 ? 191 : 255, 0, 0, 255 - i * 255 / numLoops);
				drawText(face1, text, 32);
			}
			glPopMatrix();
		}
		
		glColor4ub(0, 0, 0, 127);
		drawText(face3, "OpenGL font rendering test app using FreeType", 18);
		
		glPushMatrix();
		{
			sprintf(text, "(%d, %d)", mouseX, mouseY);
		
			glTranslatef(mouseX, mouseY - 24, 0.f);
			glColor4ub(0, 0, 255, 255);
			drawText(face2, text, 24);
		}
		glPopMatrix();
		
  		// check for errors
  		
  		GLenum error = glGetError();
  		if (error != GL_NO_ERROR)
  			printf("OpenGL error: %x\n", error);
  		
  		// flip back buffers
  		
		SDL_GL_SwapBuffers();
		
		++frameCount;
	}
	
	SDL_Quit();
	
	return 0;
}
