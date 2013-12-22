#include <assert.h>
#include <cmath>
#include <iostream>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <string.h>
#include <time.h>
#include <vector>
#include <xmmintrin.h>
#include "ssemath.h"

static float Random(float min, float max)
{
	float t = (rand() & 4095) / 4095.f;
	float w1 = 1.f - t;
	float w2 = t;
	float result = min * w1 + max * w2;
	assert(result >= min && result <= max);
	return result;
}

class TimeScope
{
public:
	TimeScope(const char * name)
		: mName(name)
		, mStartTick(clock())
	{
	}
	
	~TimeScope()
	{
		clock_t endTick = clock();
		clock_t tickCount = endTick - mStartTick;
		printf("%s: %dus\n", mName, int(1000000.0 * tickCount / CLOCKS_PER_SEC));
	}
	
	const char * mName;
	clock_t mStartTick;
};

#define TIME_BLOCK() TimeScope scope(__FUNCTION__)
//#define TIME_BLOCK() do { } while (false)

class VCmd
{
public:
	/*
	 Stages:
	 	Integrate Frame Time
	 		Modify mLife

	 	Integrate Force
	 		Modify mVel

		Integrate Velocity
	 		Modify mPos, mPosPrev

	 	Emit Probes

	 	Modulate Color
	 		Modify mColor

	 	Emit Vertices
	 		Modify mVertices
	 */
	enum Op
	{
		Op_LifeIntegrate,
		Op_VelocityIntegrate,
		Op_PositionHistorySwap,
		Op_ColorLoad,
		Op_ColorModulateLife,
		Op_DrawQuadFacingOrigin,
		Op_WindIntegrate,
		Op_ProbeLine,
	};
	
	VCmd()
	{
	}
	
	VCmd(Op op)
		: mOp(op)
	{
	}
	
	Op mOp;
};

class VProg
{
public:
	typedef std::vector<VCmd> CmdList;
	
	enum SystemFlag
	{
		SystemFlag_HasPositionHistory = 1 << 0	
	};
	
	int GetSystemFlags() const
	{
		int result = 0;
		
		for (CmdList::const_iterator i = mCmdList.begin(); i != mCmdList.end(); ++i)
		{
			const VCmd & cmd = *i;
			
			switch (cmd.mOp)
			{
				case VCmd::Op_PositionHistorySwap:
					result |= SystemFlag_HasPositionHistory;
					break;
				default:
					break;
			}
		}
		
		return result;
	}
	
	void * Alloc(int size)
	{
		for (int i = 0; i < size; ++i)
			mBytes.push_back(0);
		
		return &mBytes[mBytes.size() - 4];
	}
	
	void AllocFloat(float v)
	{
		*(float*)Alloc(4) = v;
	}
	
	CmdList mCmdList;
	
	std::vector<char> mBytes;
};

class V4
{
public:
	typedef V4 Self;
	
	static const int N = 4;
	
	V4()
	{
	}
	
	V4(float _v)
	{
		v = _mm_set_ps1(_v);
	}
	
	V4(const __m128 & _v)
	{
		v = _v;
	}
	
	Self operator+(const Self & rhs) const
	{
		return _mm_add_ps(v, rhs.v);
	}
	
	Self operator-(const Self & rhs) const
	{
		return _mm_sub_ps(v, rhs.v);
	}
	
	Self operator*(const Self & rhs) const
	{
		return _mm_mul_ps(v, rhs.v);
	}
	
	Self operator*(const float rhs) const
	{
		return _mm_mul_ps(v, _mm_set_ps1(rhs));
	}
	
	float & operator[](int index)
	{
		return reinterpret_cast<float*>(&v)[index];
	}
	
	static V4 Zero()
	{
		return _mm_setzero_ps();
	}
	
	__m128 v;
};

inline V4 Max(const V4 & v1, const V4 & v2)
{
	return _mm_max_ps(v1.v, v2.v);
}

inline V4 Abs(const V4 & v)
{
	return _mm_max_ps(_mm_sub_ps(_mm_setzero_ps(), v.v), v.v);
}

static const float PI     = 3.14159265358979323846264338327950288f;
static const float PI2    = 6.28318530717958647692528676655900577f;
static const float PID2   = 1.57079632679489661923132169163975144f;
static const float PI_SQR = 9.86960440108935861883449099987615114f;

template <bool accurate>
inline V4 FastSin(const V4 & v)
{
    static const V4 B = +4.f / PI;
    static const V4 C = -4.f / PI_SQR;
	
    V4 y = B * v + C * v * Abs(v);
	
    if (accurate)
    {
		//static const float Q = 0.775;
        static const V4 P = 0.225f;
		
        y = P * (y * Abs(y) - y) + y;   // Q * y + P * y * abs(y)
    }
	
    return y;
}

inline V4 SlowSin(const V4 & v)
{
	return sin_ps(v.v);
}

inline void GetWind(const V4 & x, const V4 & y, const V4 & z, V4 & velX, V4 & velY, V4 & velZ)
{
	velX = SlowSin((x + y + z) * 1.f);
	velY = SlowSin((x - y + z) * 1.f);
	velZ = SlowSin((x - y - z) * 1.f);
}

template <typename T>
class StorageArray
{
public:
	StorageArray()
	{
		m_data = 0;
		m_own = false;
	}
	
	~StorageArray()
	{
		if (m_own)
		{
			delete [] m_data;
		}
	}
	
	void Resize(unsigned int size, bool clearToZero = false)
	{
		delete [] m_data;
		
		if (size == 0)
		{
			m_data = 0;
			m_own = false;
		}
		else
		{
			m_data = new T[size];
			m_own = true;
			
			if (clearToZero)
			{
				memset(m_data, 0, sizeof(T) * size);
			}
		}
	}
	
	void Assign(T * data, bool own)
	{
		m_data = data;
		m_own = own;
	}
	
	void Reference(StorageArray<T> & other)
	{
		m_data = other.m_data;
		m_own = false;
	}
	
	void SwapWith(StorageArray<T> & other)
	{
		std::swap(m_data, other.m_data);
		std::swap(m_own, other.m_own);
	}
	
	inline T * data()
	{
		return m_data;
	}
	
	inline T & operator[](unsigned int idx)
	{
		return m_data[idx];
	}
	
protected:
	T * m_data;
	bool m_own;
};

template <typename V>
class ParticleSystem
{
public:
	ParticleSystem(VProg * prog, int count)
	{
		mProg = prog;
		mProgFlags = prog->GetSystemFlags();
		mCount = count;
		
		//
		
		mLife.Resize(count, true);
		mLifeRcp.Resize(count, true);
		
		//
		
		mCurX.Resize(count, true);
		mCurY.Resize(count, true);
		mCurZ.Resize(count, true);
		
		//
		
		if (mProgFlags & VProg::SystemFlag_HasPositionHistory)
		{
			mNewX.Resize(count, true);
			mNewY.Resize(count, true);
			mNewZ.Resize(count, true);
		}
		else
		{
			mNewX.Reference(mCurX);
			mNewY.Reference(mCurY);
			mNewZ.Reference(mCurZ);
		}
		
		//
		
		mVelX.Resize(count, true);
		mVelY.Resize(count, true);
		mVelZ.Resize(count, true);
		
		//
		
		mColorR.Resize(count, true);
		mColorG.Resize(count, true);
		mColorB.Resize(count, true);
		
		//
		
		unsigned int vertexCount = count * V::N * 4;
		
		mVertexXYZ.Resize(vertexCount * 3, true);
		mVertexRGBA.Resize(vertexCount * 4, true);
	}
	
	VProg * mProg;
	int mProgFlags;
	int mProgDataPtr;
	
	int mCount;
	
	int mVertexPtr;
	
	StorageArray<V> mLife;
	StorageArray<V> mLifeRcp;
	
	StorageArray<V> mCurX;
	StorageArray<V> mCurY;
	StorageArray<V> mCurZ;

	StorageArray<V> mNewX;
	StorageArray<V> mNewY;
	StorageArray<V> mNewZ;
	
	StorageArray<V> mVelX;
	StorageArray<V> mVelY;
	StorageArray<V> mVelZ;
	
	StorageArray<V> mColorR;
	StorageArray<V> mColorG;
	StorageArray<V> mColorB;
	
	StorageArray<float> mVertexXYZ;
	StorageArray<unsigned char> mVertexRGBA;
	
	float ReadFloat()
	{
		return *(float*)&mProg->mBytes[mProgDataPtr += 4];
	}
	
	void Randomize();
	
	void Update(float dt)
	{
		mProgDataPtr = 0;
		
		for (VProg::CmdList::const_iterator i = mProg->mCmdList.begin(); i != mProg->mCmdList.end(); ++i)
		{
			const VCmd & cmd = *i;
			
			switch (cmd.mOp)
			{
				case VCmd::Op_LifeIntegrate:
				{
					ExecLifeIntegrate(dt);
					break;
				}
				case VCmd::Op_VelocityIntegrate:
				{
					ExecVelocityIntegrate(dt);
					break;
				}
				case VCmd::Op_PositionHistorySwap:
				{
					ExecPositionHistorySwap();
					break;
				}
				case VCmd::Op_ColorLoad:
				{
					ExecColorLoad();
					break;
				}
				case VCmd::Op_ColorModulateLife:
				{
					ExecColorModulateLife();
					break;
				}
				case VCmd::Op_DrawQuadFacingOrigin:
				{
					ExecDrawQuadFacingOrigin();
					break;
				}
				case VCmd::Op_WindIntegrate:
				{
					ExecWindIntegrate(dt);
					break;
				}
				case VCmd::Op_ProbeLine:
				{
					ExecLineProbe();
					break;
				}
			}
		}
	}
	
	void ExecSpringIntegrate(float _dt, float _x, float _y, float _z, float _c)
	{
		TIME_BLOCK();
		
		const V x(_x);
		const V y(_y);
		const V z(_z);
		const V c = _c * _dt;
		
		for (int i = 0; i < mCount; ++i)
		{
			const V dx = x - mCurX[i];
			const V dy = y - mCurY[i];
			const V dz = z - mCurZ[i];
			
			mVelX[i] = mVelX[i] + dx * c;
			mVelY[i] = mVelY[i] + dy * c;
			mVelZ[i] = mVelZ[i] + dz * c;
		}
	}
	
	void ExecLifeIntegrate(float _dt)
	{
		TIME_BLOCK();
		
		const V dt = _dt;
		
		V * __restrict life = mLife.data();
		
		for (unsigned int i = mCount; i != 0; --i, ++life)
		{
			life[0] = Max(life[0] - dt, V::Zero());
		}
	}
	
	void ExecDrawQuadFacingOrigin()
	{
		TIME_BLOCK();
		
		//V sizeX = +0.04f;
		//V sizeY = +0.04f;
		V sizeX = +0.02f;
		V sizeY = +0.02f;
		
		float * __restrict xyz = mVertexXYZ.data();
		
		for (int i = 0; i < mCount; ++i)
		{
			V x1 = mCurX[i] - sizeX;
			V x2 = mCurX[i] + sizeX;
			V y1 = mCurY[i] - sizeY;
			V y2 = mCurY[i] + sizeY;
			V z  = mCurZ[i];

			for (int j = 0; j < V::N; ++j)
			{
				xyz[0] = x1[j];
				xyz[1] = y1[j];
				xyz[2] = z[j];
				
				xyz[3] = x2[j];
				xyz[4] = y1[j];
				xyz[5] = z[j];
				
				xyz[6] = x2[j];
				xyz[7] = y2[j];
				xyz[8] = z[j];
				
				xyz[9] = x1[j];
				xyz[10] = y2[j];
				xyz[11] = z[j];
				
				xyz += 12;
			}
		}
		
		unsigned char * __restrict rgba = mVertexRGBA.data();
		
		V c255 = 255.f;
		
		for (int i = 0; i < mCount; ++i)
		{
			V r = mColorR[i] * c255;
			V g = mColorG[i] * c255;
			V b = mColorB[i] * c255;
			
			for (int j = 0; j < V::N; ++j)
			{
				unsigned char ri = static_cast<unsigned char>(r[j]);
				unsigned char gi = static_cast<unsigned char>(g[j]);
				unsigned char bi = static_cast<unsigned char>(b[j]);
				unsigned char ai = static_cast<unsigned char>(255);
				
				for (unsigned int ir = 4; ir != 0; --ir)
				{
					*rgba++ = ri;
					*rgba++ = gi;
					*rgba++ = bi;
					*rgba++ = ai;
				}
			}
		}
	}
	
	void ExecWindIntegrate(float _dt)
	{
		TIME_BLOCK();

		// fixme
		static float _s = 0.f;
		_s += _dt * 0.5f;
		const V s = (std::sin(_s) + 1.f) * 0.1f * _dt;
		
		const V c = _dt * 10.f;
		
		const V * __restrict posX = mCurX.data();
		const V * __restrict posY = mCurY.data();
		const V * __restrict posZ = mCurZ.data();
		
		V * __restrict velX = mVelX.data();
		V * __restrict velY = mVelY.data();
		V * __restrict velZ = mVelZ.data();
		
		for (unsigned int i = 0; i < mCount; ++i)
		{
			V windVelX;
			V windVelY;
			V windVelZ;
			
			GetWind(posX[i], posY[i], posZ[i], windVelX, windVelY, windVelZ);
			
			velX[i] = velX[i] + windVelX * c;
			velY[i] = velY[i] + windVelY * c;
			velZ[i] = velZ[i] + windVelZ * c;
			
#if 1
			V dx = posX[i] - V::Zero();
			V dy = posY[i] - V::Zero();
			V dz = posZ[i] - V::Zero();
			
			velX[i] = velX[i] - dx * s;
			velY[i] = velY[i] - dy * s;
			velZ[i] = velZ[i] - dz * s;
#endif
		}
	}
	
	void ExecColorLoad()
	{
		TIME_BLOCK();
		
		const V r = ReadFloat();
		const V g = ReadFloat();
		const V b = ReadFloat();
		const V a = ReadFloat();
		
		V * __restrict dstR = mColorR.data();
		V * __restrict dstG = mColorG.data();
		V * __restrict dstB = mColorB.data();
		
		for (unsigned int i = mCount; i != 0; --i)
			*dstR++ = r;
		
		for (unsigned int i = mCount; i != 0; --i)
			*dstG++ = g;
		
		for (unsigned int i = mCount; i != 0; --i)
			*dstB++ = b;
	}
	
	void ExecColorModulateLife()
	{
		TIME_BLOCK();

		const V * __restrict life = mLife.data();
		const V * __restrict lifeRcp = mLifeRcp.data();
		
		V * __restrict dstR = mColorR.data();
		V * __restrict dstG = mColorG.data();
		V * __restrict dstB = mColorB.data();
		
		for (unsigned int i = mCount; i != 0; --i)
		{
			const V value = life[0] * lifeRcp[0];
			
			dstR[0] = dstR[0] * value;
			dstG[0] = dstG[0] * value;
			dstB[0] = dstB[0] * value;
			
			life++;
			lifeRcp++;
			
			dstR++;
			dstG++;
			dstB++;
		}
	}
	
	void ExecVelocityIntegrate(float _dt)
	{
		TIME_BLOCK();
		
		V dt = _dt;
		
		V * __restrict dstPosX = mNewX.data();
		V * __restrict dstPosY = mNewY.data();
		V * __restrict dstPosZ = mNewZ.data();
		V * __restrict srcPosX = mCurX.data();
		V * __restrict srcPosY = mCurY.data();
		V * __restrict srcPosZ = mCurZ.data();
		V * __restrict srcVelX = mVelX.data();
		V * __restrict srcVelY = mVelY.data();
		V * __restrict srcVelZ = mVelZ.data();

		for (unsigned int i = 0; i < mCount; ++i)
		{
			dstPosX[i] = srcPosX[i] + srcVelX[i] * dt;
			dstPosY[i] = srcPosY[i] + srcVelY[i] * dt;
			dstPosZ[i] = srcPosZ[i] + srcVelZ[i] * dt;
		}
	}
	
	void ExecLineProbe()
	{
		TIME_BLOCK();
		
		// todo: execute a line probe from mPosPrev to mPos
	}
	
	void ExecPositionHistorySwap()
	{
		TIME_BLOCK();
		
		mCurX.SwapWith(mNewX);
		mCurY.SwapWith(mNewY);
		mCurZ.SwapWith(mNewZ);
	}
	
	void Print();
};

template <typename V>
void ParticleSystem<V>::Randomize()
{
	for (int i = 0; i < mCount; ++i)
	{
		for (int j = 0; j < V::N; ++j)
		{
			//mLife[i][j] = Random(+1.f, +4.f);
			mLife[i][j] = Random(+20.f, +40.f);
			mLifeRcp[i][j] = 1.f / mLife[i][j];
			
			mCurX[i][j] = Random(-1.f, +1.f);
			mCurY[i][j] = Random(-1.f, +1.f);
			mCurZ[i][j] = Random(-1.f, +1.f);
			
			mVelX[i][j] = Random(-1.f, +1.f);
			mVelY[i][j] = Random(-1.f, +1.f);
			mVelZ[i][j] = Random(-1.f, +1.f);
		}
		
		mCurX[i] = mCurX[i] * 0.1f;
		mCurY[i] = mCurY[i] * 0.1f;
		mCurZ[i] = mCurZ[i] * 0.1f;
		
		mVelX[i] = mVelX[i] * 0.1f;
		mVelY[i] = mVelY[i] * 0.1f;
		mVelZ[i] = mVelZ[i] * 0.1f;
	}
}

template <typename V>
void ParticleSystem<V>::Print()
{
	/*
	glBegin(GL_POINTS);
	
	for (int i = 0; i < mCount; ++i)
	{
		for (int j = 0; j < V::N; ++j)
		{
			glColor3f(mColorR[i][j], mColorG[i][j], mColorB[i][j]);
			//glVertex3f(mCurX[i][j], mCurY[i][j], mCurZ[i][j]);
			glVertex3f(mCurX[i][j], mCurY[i][j], 0.f);
		}
	}
	
	glEnd();
	*/
	
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glBlendEquation(GL_FUNC_ADD);

	glVertexPointer(3, GL_FLOAT, 0, mVertexXYZ.data());
	glColorPointer(4, GL_UNSIGNED_BYTE, 0, mVertexRGBA.data());
	
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_COLOR_ARRAY);
	
	glDrawArrays(GL_QUADS, 0, mCount * V::N * 4);
	
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);
}

int main (int argc, char * argv[])
{
	SDL_Init(SDL_INIT_VIDEO);
	
	int initFlags = SDL_INIT_JOYSTICK | SDL_INIT_NOPARACHUTE | SDL_INIT_TIMER | SDL_INIT_VIDEO;
	if (SDL_Init(initFlags) < 0)
		printf("SDL init failed\n");
	
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE,     8); SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,    8); SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, 1);

	SDL_Surface * pScreen = SDL_SetVideoMode(800, 600, 32, SDL_OPENGL | SDL_DOUBLEBUF);
	//SDL_Surface * pScreen = SDL_SetVideoMode(1280, 1024, 32, SDL_OPENGL | SDL_DOUBLEBUF | SDL_FULLSCREEN);
	if (pScreen == 0)
		printf("set video mode failed\n");
	
    SDL_ShowCursor(0);
    SDL_WM_GrabInput(SDL_GRAB_ON);
	
	VProg prog;
	
	prog.mCmdList.push_back(VCmd(VCmd::Op_LifeIntegrate));
	prog.mCmdList.push_back(VCmd(VCmd::Op_WindIntegrate));
	prog.mCmdList.push_back(VCmd(VCmd::Op_VelocityIntegrate));
	prog.mCmdList.push_back(VCmd(VCmd::Op_ProbeLine));
	prog.mCmdList.push_back(VCmd(VCmd::Op_PositionHistorySwap));
	*(float*)prog.Alloc(4) = 0.8f;
	*(float*)prog.Alloc(4) = 0.4f;
	*(float*)prog.Alloc(4) = 0.2f;
	*(float*)prog.Alloc(4) = 1.0f;
	prog.mCmdList.push_back(VCmd(VCmd::Op_ColorLoad));
	prog.mCmdList.push_back(VCmd(VCmd::Op_ColorModulateLife));
	prog.mCmdList.push_back(VCmd(VCmd::Op_DrawQuadFacingOrigin));
	
	ParticleSystem<V4> system(&prog, 50000);
	
	system.Randomize();
	
	bool stop = false;
	
	float rx = 0.f;
	float ry = 0.f;
	
	do
	{
		SDL_PumpEvents();
		
		SDL_Event e;
		
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
				case SDL_KEYDOWN:
					if (e.key.keysym.sym == SDLK_ESCAPE)
					{
						SDL_Event event;
						event.type = SDL_QUIT;
						SDL_PushEvent(&event);
					}
					break;
					
				case SDL_MOUSEMOTION:
					{
						rx += e.motion.xrel / 10.f;
						ry += e.motion.yrel / 10.f;
					}
					break;
					
				case SDL_QUIT:
					stop = true;
					break;
			}
		}
		
		system.Update(1.f /  100.f);
		
		glClearColor(0.f, 0.f, 0.f, 1.f);
		glClear(GL_COLOR_BUFFER_BIT);
		
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-1.0, +1.0, +1.0, -1.0, -100.0, +100.0);
		glScalef(0.1f, 0.1f, 1.0f);
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glRotatef(rx, 1.f, 0.f, 0.f);
		glRotatef(ry, 0.f, 1.f, 0.f);
		
		system.Print();
		
		SDL_GL_SwapBuffers();
		
	} while (stop == false);
	
	SDL_FreeSurface(pScreen);
	pScreen = 0;
	
	SDL_Quit();
	
    return 0;
}

