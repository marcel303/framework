#pragma once

#include <OpenGLES/ES1/gl.h>
#include <math.h>

#define VGU_USE_POW 0
//#define VGU_POW 8.0f
#define VGU_POW 2.0f

class VGU
{
public:
	VGU()
	{
		Initialize();
	}
	
	void Initialize()
	{
		GLubyte data[32][32][3];
		
		// Generate line segment texture.

		glGenTextures(1, &m_TextureLine);
		glBindTexture(GL_TEXTURE_2D, m_TextureLine);
		
		for (int x = 0; x < 32; ++x)
		{
			for (int y = 0 ; y < 32; ++y)
			{
				float ty = (y / 31.0f - 0.5f) * 2.0f;
				
				float d = fabs(ty);
				
				float t = 1.0f - d;
				
#if VGU_USE_POW
				if (t > 0.0f)
					t = pow(t, VGU_POW);
#endif
				
				int c = t * 255.0f;
				
				if (c < 0)
					c = 0;
				if (c > 255)
					c = 255;
				
				data[x][y][0] = c;
				data[x][y][1] = c;
				data[x][y][2] = c;
			}
		}
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, 1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 32, 32, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		
		// Generate corner texture.
		
		glGenTextures(1, &m_Texture);
		glBindTexture(GL_TEXTURE_2D, m_Texture);
				
		for (int x = 0; x < 32; ++x)
		{
			for (int y = 0 ; y < 32; ++y)
			{
				float tx = (x / 31.0f - 0.5f) * 2.0f;
				float ty = (y / 31.0f - 0.5f) * 2.0f;
				
				float d;
				
				if (tx < 0.0f)
					d = sqrt(tx * tx + ty * ty);
				else
					d = fabs(ty);
				
				float t = 1.0f - d;
				
#if VGU_USE_POW
				if (t > 0.0f)
					t = pow(t, VGU_POW);
#endif
				
				int c = t * 255.0f;
				
				if (c < 0)
					c = 0;
				if (c > 255)
					c = 255;
				
				data[x][y][0] = c;
				data[x][y][1] = c;
				data[x][y][2] = c;
			}
		}
		
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, 1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 32, 32, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
	}
	
	void Draw_Line(float x1, float y1, float x2, float y2, float w)
	{
		// todo: special case for w  > ds
		//       else, end-points will overlap..
		
		w *= 0.5f;
		
		float dx = x2 - x1;
		float dy = y2 - y1;
		
		const float ds = sqrt(dx * dx + dy * dy);
		const float dsInv = 1.0f / ds;
		
		dx *= dsInv;
		dy *= dsInv;
		
		const float nx = -dy;
		const float ny = +dx;
		
		float vertices[12][2];
		
		vertices[0][0] = x1 - dx * w + nx * w;
		vertices[0][1] = y1 - dy * w + ny * w;
		vertices[1][0] = x1 - dx * w - nx * w;
		vertices[1][1] = y1 - dy * w - ny * w;
		vertices[2][0] = x1 + dx * w + nx * w;
		vertices[2][1] = y1 + dy * w + ny * w;
		vertices[3][0] = x1 + dx * w - nx * w;
		vertices[3][1] = y1 + dy * w - ny * w;
		
		vertices[4][0] = x2 - dx * w + nx * w;
		vertices[4][1] = y2 - dy * w + ny * w;
		vertices[5][0] = x2 - dx * w - nx * w;
		vertices[5][1] = y2 - dy * w - ny * w;
		vertices[6][0] = x2 + dx * w + nx * w;
		vertices[6][1] = y2 + dy * w + ny * w;
		vertices[7][0] = x2 + dx * w - nx * w;
		vertices[7][1] = y2 + dy * w - ny * w;
		
		vertices[8][0] = x1 + dx * w + nx * w;
		vertices[8][1] = y1 + dy * w + ny * w;
		vertices[9][0] = x1 + dx * w - nx * w;
		vertices[9][1] = y1 + dy * w - ny * w;
		vertices[10][0] = x2 - dx * w + nx * w;
		vertices[10][1] = y2 - dy * w + ny * w;
		vertices[11][0] = x2 - dx * w - nx * w;
		vertices[11][1] = y2 - dy * w - ny * w;
		
		float texCoords[12][2];
		
		texCoords[0][0] = 0.0f;
		texCoords[0][1] = 0.0f;
		texCoords[1][0] = 1.0f;
		texCoords[1][1] = 0.0f;
		texCoords[2][0] = 0.0f;
		texCoords[2][1] = 1.0f;
		texCoords[3][0] = 1.0f;
		texCoords[3][1] = 1.0f;
		texCoords[4][0] = 0.0f;
		texCoords[4][1] = 1.0f;
		texCoords[5][0] = 1.0f;
		texCoords[5][1] = 1.0f;
		texCoords[6][0] = 0.0f;
		texCoords[6][1] = 0.0f;
		texCoords[7][0] = 1.0f;
		texCoords[7][1] = 0.0f;
		texCoords[8][0] = 0.0f;
		texCoords[8][1] = 0.0f;
		texCoords[9][0] = 1.0f;
		texCoords[9][1] = 0.0f;
		texCoords[10][0] = 0.0f;
		texCoords[10][1] = 1.0f;
		texCoords[11][0] = 1.0f;
		texCoords[11][1] = 1.0f;
		
		//
			
		glDisable(GL_DEPTH_TEST);
		glBlendFunc(GL_SRC_COLOR, GL_ONE);
//		glBlendFunc(GL_SRC_COLOR, GL_ONE_MINUS_SRC_COLOR);
		glEnable(GL_BLEND);
		
		// Setup arrays.
		
		glVertexPointer(2, GL_FLOAT, 0, (float*)vertices);
		glEnableClientState(GL_VERTEX_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, 0, texCoords);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		
		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_TEXTURE_2D);
		
		// todo: could make do with one draw call and a more clever texture.
		
		// Draw corners.
		
		glBindTexture(GL_TEXTURE_2D, m_Texture);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glDrawArrays(GL_TRIANGLE_STRIP, 4, 4);
		
		// Draw middle segment.
		
		glBindTexture(GL_TEXTURE_2D, m_TextureLine);
		glDrawArrays(GL_TRIANGLE_STRIP, 8, 4);

		//
		
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		glDisableClientState(GL_VERTEX_ARRAY);
		
		//
		
		glDisable(GL_BLEND);
		
		//
		
		glDisable(GL_TEXTURE_2D);
	}
	
	GLuint m_Texture;
	GLuint m_TextureLine;
};
