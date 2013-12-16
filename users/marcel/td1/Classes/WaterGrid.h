#pragma once

#include <assert.h>
#include <cmath>
//#include <math.h>
#include "Debugging.h"
#include "OpenGLCompat.h"
#include "Log.h"
#include "Types.h"
#include "Vec2.h"
#include "Vec3.h"

static void CheckErrors()
{
	GLenum error = glGetError();
	
	if (error)
	{
		LOG_ERR("OpenGL error", 0);
	}
}

// todo: make packed.
class Col3
{
public:
	UInt8 r;
	UInt8 g;
	UInt8 b;
};

class Col4
{
public:
	union
	{
		struct
		{
			UInt8 r;
			UInt8 g;
			UInt8 b;
			UInt8 a;
		};
		UInt8 rgba[4];
	};
};

class WaterGrid
{
public:
	WaterGrid()
	{
		Initialize();
	}
	
	WaterGrid(int sx, int sy)
	{
		Initialize();
		
		SetSize(sx, sy);
	}
	
	~WaterGrid()
	{
		SetSize(0, 0);
	}
	
	inline int CalcVertexIndex(int x, int y) const
	{
		const int result = x + y * m_Sx;
		
//		Assert(result >= 0);
//		Assert(result < m_VertexCount);
		
		return result;
	}

	inline void CalcVertexIndexNB(int x, int y, int* indices) const
	{
		indices[0] = CalcVertexIndex(x - 1, y + 0);
		indices[1] = CalcVertexIndex(x + 1, y + 0);
		indices[2] = CalcVertexIndex(x + 0, y - 1);
		indices[3] = CalcVertexIndex(x + 0, y + 1);
	}
	
	void Initialize()
	{
		m_Sx = 0;
		m_Sy = 0;
		m_VertexCount = 0;
		m_DATA = 0;
		m_Vertices = 0;
		m_TexCoords = 0;
		m_Colors = 0;
		m_Normals = 0;
		m_TriCount = 0;
		m_Indices = 0;
	}
	
	void SetSize(int sx, int sy)
	{
		Assert(sx >= 0);
		Assert(sy >= 0);
		
		delete[] m_DATA; m_DATA = 0;
		delete[] m_Vertices; m_Vertices = 0;
		delete[] m_TexCoords; m_TexCoords = 0;
		delete[] m_Colors; m_Colors = 0;
		delete[] m_Normals; m_Normals = 0;
		m_VertexCount = 0;
		
		delete[] m_Indices; m_Indices = 0;
		m_TriCount = 0;
		
		m_Sx = sx;
		m_Sy = sy;
		
		if (m_Sx * m_Sy == 0)
		{
			return;
		}
		
		// Create vertex array.
		
		m_VertexCount = sx * sy;

		LOG_DBG("Creating vertex array", 0);
		
		m_DATA = new Vec3[m_VertexCount];
		m_Vertices = new Vec3[m_VertexCount];
		m_TexCoords = new Vec2[m_VertexCount];
		m_Colors = new Col4[m_VertexCount];
		m_Normals = new Vec3[m_VertexCount];
		
		// Create geometry.
		
		int quadCount = (sx - 1) * (sy - 1);
		m_TriCount = quadCount * 2;
		int indexCount = m_TriCount * 3;
		
		LOG_DBG("Creating geometry", 0);
		
		m_Indices = new UInt16[indexCount];
		
		int i = 0;
		
		for (int x1 = 0; x1 < sx - 1; ++x1)
		{
			for (int y1 = 0; y1 < sy - 1; ++y1)
			{
				int x2 = x1 + 1;
				int y2 = y1 + 1;
				
				// Triangle 1.
				
				m_Indices[i++] = CalcVertexIndex(x1, y1);
				m_Indices[i++] = CalcVertexIndex(x2, y1);
				m_Indices[i++] = CalcVertexIndex(x2, y2);
				
				// Triangle 2.
				
				m_Indices[i++] = CalcVertexIndex(x1, y1);
				m_Indices[i++] = CalcVertexIndex(x2, y2);
				m_Indices[i++] = CalcVertexIndex(x1, y2);
			}
		}
		
		// Setup vertices.

		LOG_DBG("Setting up data", 0);

		for (int x = 0; x < sx; ++x)
		{
			for (int y = 0; y < sy; ++y)
			{
				int index = CalcVertexIndex(x, y);
				
				m_DATA[index][0] = 0.0f;
				m_DATA[index][1] = 0.0f;
				m_DATA[index][2] = 0.0f;
			}
		}
		
		LOG_DBG("Setting up vertices", 0);
		
		for (int x = 0; x < sx; ++x)
		{
			for (int y = 0; y < sy; ++y)
			{
				int index = CalcVertexIndex(x, y);
				
				m_Vertices[index][0] = x;
				m_Vertices[index][1] = y;
				m_Vertices[index][2] = 0.0f;
			}
		}
		
		// Setup texture coordinates.
		
		LOG_DBG("Setting up texture coordinates", 0);
		
		for (int x = 0; x < sx; ++x)
		{
			for (int y = 0; y < sy; ++y)
			{
				int index = CalcVertexIndex(x, y);
				
				m_TexCoords[index][0] = x / (float)m_Sx;
				m_TexCoords[index][1] = y / (float)m_Sy;
			}
		}
		
		// Setup colors.
		
		LOG_DBG("Setting up colors", 0);
		
		for (int x = 0; x < sx; ++x)
		{
			for (int y = 0; y < sy; ++y)
			{
				int index = CalcVertexIndex(x, y);
				
				m_Colors[index].r = x * 4;
				m_Colors[index].g = y * 4;
				m_Colors[index].b = 255;
			}
		}
		
		LOG_DBG("Setting up normals", 0);
		
		for (int x = 0; x < sx; ++x)
		{
			for (int y = 0; y < sy; ++y)
			{
				int index = CalcVertexIndex(x, y);
				
				m_Normals[index][0] = 0.0f;
				m_Normals[index][1] = 0.0f;
				m_Normals[index][2] = 1.0f;
			}
		}
		
		// Add noise to data.
		
#if 0
		LOG_DBG("Adding noise to data", 0);
		
		for (int x = 1; x < m_Sx - 1; ++x)
		{
			for (int y = 1; y < m_Sy - 1; ++y)
			{
				int index = CalcVertexIndex(x, y);
				
				m_DATA[index][0] = (rand() & 15) - 7.5f;
			}
		}
#endif
	}

	void Update(float dt)
	{
		const float r = powf(0.5f, dt);
		
		for (int y = 1; y < m_Sy - 1; ++y)
		{
#if 1
			const Vec3* line[3] =
			{
				&m_DATA[(y - 1) * m_Sx],
				&m_DATA[(y + 0) * m_Sx],
				&m_DATA[(y + 1) * m_Sx]
			};
#endif
			
			Vec3* data = &m_DATA[y * m_Sx];
			
			const int sx1 = m_Sx - 1;
			
			for (int x = 1; x < sx1; ++x)
			{							
				const float x0 = line[1][x][0];
				
				const float f =
					line[0][x][0] - x0 +
					line[1][x - 1][0] - x0 +
					line[1][x + 1][0] - x0 +
					line[2][x][0] - x0;
				
				const float c = 10.0f; // 'spring constant'.
				
				const float a = f * c; // assume 'mass' eq 1.0

				data[x][1] += a * dt;
				data[x][0] += line[1][x][1] * dt;
				data[x][1] *= r;
			}
		}
	}
	
	static inline int Saturate(int c)
	{
		return c < 0 ? 0 : c > 255 ? 255 : c;
	}
	
	void DrawPrepare()
	{
		for (int y = 1; y < m_Sy - 1; ++y)
		{
			const Vec3* vline[3] =
			{
				&m_Vertices[(y - 1) * m_Sx],
				&m_Vertices[(y + 0) * m_Sx],
				&m_Vertices[(y + 1) * m_Sx]
			};
			
			Vec3* vline0 = &m_Vertices[y * m_Sx];
			Vec3* nline0 = &m_Normals[y * m_Sx];
			
			const Vec3* dline = &m_DATA[y * m_Sx];
			
			Col4* cline = &m_Colors[y * m_Sx];
			
			const int sx1 = m_Sx - 1;
			
			for (int x = 1; x < sx1; ++x)
			{
				vline0[x][2] = dline[x][0];
				
#if 1
				Vec3 n =
					(vline[1][x + 1] - vline[1][x - 1]) %
					(vline[2][x] - vline[0][x]);
				
#if 1
				n.Normalize();
#else
				const float invSqrt = FastInvSqrt(n.LengthSq_get());
				n *= invSqrt;
#endif
				
				nline0[x] = n;
				
//				nline0[x] = Vec3(0.0f, 0.0f, 1.0f);
				
#if 0
				float l = sqrtf(nline0[x].x * nline0[x].x + nline0[x].y * nline0[x].y + nline0[x].z * nline0[x].z);
				assert(l >= 0.99f && l <= 1.01f);
#endif

#if 1
#if 0
				cline[x].rgba[0] = Saturate((n[0] + 1.0f) * 0.5f * 255.0f);
				cline[x].rgba[1] = Saturate((n[1] + 1.0f) * 0.5f * 255.0f);
				cline[x].rgba[2] = Saturate((n[2] + 1.0f) * 0.5f * 255.0f);
#else
				cline[x].rgba[0] = int((n[0] + 1.0f) * 127.5f);
				cline[x].rgba[1] = int((n[1] + 1.0f) * 127.5f);
				cline[x].rgba[2] = int((n[2] + 1.0f) * 127.5f);
#endif				
#endif
				
#if 0
				cline[x].rgba[2] = 127 + (int)(dline[x].x * 5.0f);
#endif
#endif
			}
		}
	}
	
	void Draw()
	{
		// NOTE: glColorPointer does not accept 3 elements/color.
		//       glVertexPointer accepts 2, 3 or 4.
		//       glTexCoordPointer accepts 2, 3 or 4.
		
		glVertexPointer(3, GL_FLOAT, 0, m_Vertices);
		glEnableClientState(GL_VERTEX_ARRAY);
		CheckErrors();
//		glTexCoordPointer(2, GL_FLOAT, 0, m_TexCoords);
//		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
//		CheckErrors();		
		glColorPointer(4, GL_UNSIGNED_BYTE, 0, m_Colors);
		glEnableClientState(GL_COLOR_ARRAY);
		glNormalPointer(GL_FLOAT, 0, m_Normals);
		glEnableClientState(GL_NORMAL_ARRAY);
		CheckErrors();
		
		glDrawElements(GL_TRIANGLES, m_TriCount * 3, GL_UNSIGNED_SHORT, m_Indices);
		CheckErrors();

		glDisableClientState(GL_NORMAL_ARRAY);
		CheckErrors();
		glDisableClientState(GL_COLOR_ARRAY);
		CheckErrors();
//		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
//		CheckErrors();
		glDisableClientState(GL_VERTEX_ARRAY);
		CheckErrors();
	}
	
	void Impulse(float x, float y, float value)
	{
		const int x1 = std::floor(x);
		const int y1 = std::floor(y);
		const int x2 = x1 + 1;
		const int y2 = y1 + 1;
		const float wx2 = x - x1;
		const float wy2 = y - y1;
		const float wx1 = 1.0f - wx1;
		const float wy1 = 1.0f - wy1;
		const float w11 = wx1 * wy1;
		const float w21 = wx2 * wy1;
		const float w12 = wx1 * wy2;
		const float w22 = wx2 * wy2;
#define DO(x, y, i) if (x >= 0 && x < m_Sx && y >= 0 && y < m_Sy) m_DATA[CalcVertexIndex(x, y)][0] += i
		DO(x1, y1, w11 * value);
		DO(x2, y1, w21 * value);
		DO(x1, y2, w12 * value);
		DO(x2, y2, w22 * value);
#undef DO
	}
	
	int m_Sx;
	int m_Sy;
	int m_VertexCount;
	Vec3* m_DATA;
	Vec3* m_Vertices;
	Vec2* m_TexCoords;
	Col4* m_Colors;
	Vec3* m_Normals;
	UInt16* m_Indices;
	int m_TriCount;
};
