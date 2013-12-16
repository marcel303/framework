#include <limits>
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include "xrast.h"
#include "Mat4x4.h"

SDL_Surface* gSurface = 0;

inline void SDL_PutPixel(SDL_Surface* surface, int x, int y, uint32_t c)
{
	uint8_t* bytes = (uint8_t*)surface->pixels;
	bytes += surface->pitch * y;
	uint32_t* pixels = (uint32_t*)bytes;
	pixels += x;
	*pixels = c;
}

inline uint32_t SDL_MakeCol(float r, float g, float b, float a)
{
	uint32_t ir = static_cast<uint32_t>(r * 255.999f);
	uint32_t ig = static_cast<uint32_t>(g * 255.999f);
	uint32_t ib = static_cast<uint32_t>(b * 255.999f);
	uint32_t ia = static_cast<uint32_t>(a * 255.999f);
	
	return
		(ir << gSurface->format->Rshift) |
		(ig << gSurface->format->Gshift) |
		(ib << gSurface->format->Bshift) |
		(ia << gSurface->format->Ashift);
}

static float gDepthBuffer[480][640];

//

static float gTime = 0.0f;

static XVec MatrixMul(const Mat4x4& m, const XVec& v)
{
	Vec4 temp(v.elem[0], v.elem[1], v.elem[2], v.elem[3]);
	temp = m * temp;
	return XVec(temp[0], temp[1], temp[2], temp[3]);
}

static void HandleVertex(XShaderState& state, XVert& vOut, const XVert& vIn)
{
	vOut = vIn;
	Mat4x4 m = state.constantBuffer->GetMatrix(0);
	vOut.varying[0] = MatrixMul(m, vIn.varying[0]);
//	vOut.varying[0].elem[0] += sinf(gTime) * vIn.varying[1].elem[1] * 40.0f;
//	vOut.varying[0].elem[1] += cosf(gTime) * vIn.varying[1].elem[1] * 40.0f;
	vOut.varying[0].elem[0] += gSurface->w / 2.0f;
	vOut.varying[0].elem[1] += gSurface->h / 2.0f;
}

static void HandleSample(XShaderState& state, const XVert& vert, int x, int y)
{
	if (x < 0 || x >= gSurface->w || y < 0 || y >= gSurface->h)
		return;
	
	float d = vert.varying[0].elem[2];
	
	//printf("%g ", d);
	
	if (d >= gDepthBuffer[y][x])
		return;
	
	gDepthBuffer[y][x] = d;
	
	float v = vert.varying[1].elem[0];
	
//	printf("%g ", v);
	
	uint32_t c = SDL_MakeCol(v, v, v, 1.0f);
	
	SDL_PutPixel(gSurface, x, y, c);
}

int main(int arc, char* argv[])
{
	SDL_Init(SDL_INIT_EVERYTHING);
	
	SDL_Surface* pSurface = SDL_SetVideoMode(640, 480, 32, SDL_DOUBLEBUF);

	float tx = 0.0f;
	//float ty = 0.0f;
	
	bool stop = false;
	
	while (stop == false)
	{
		SDL_Event e;
		
		if (SDL_PollEvent(&e))
		{
			if (e.type == SDL_KEYDOWN)
				stop = true;
		}
		
		gTime += 1.0f / 60.0f;
		
		//tx += 1.0f;

		gSurface = pSurface;
		SDL_LockSurface(gSurface);
		
		for (int y = 0; y < gSurface->h; ++y)
			for (int x = 0; x < gSurface->w; ++x)
				gDepthBuffer[y][x] = std::numeric_limits<float>::max();
		
		for (int y = 0; y < gSurface->h; ++y)
			for (int x = 0; x < gSurface->w; ++x)
				SDL_PutPixel(gSurface, x, y, 0x00000000);
		
		Mat4x4 m;
		m.MakeIdentity();
		{
			Mat4x4 m1, m2, m3, m4;
			m1.MakeRotationX(gTime);
			m2.MakeRotationY(gTime);
			m3.MakeRotationZ(gTime);
			m4.MakeScaling(0.5f, 0.5f, 0.5f);
			m = m1 * m2 * m3 * m4;
		}
		
		XConstantBuffer constantBuffer;
		constantBuffer.SetMatrix(0, m);
		
		XSemanticMap semanticMap;
		semanticMap.index[XSemantic_Position] = 0;
		XShaderState shaderState;
		shaderState.handleVertex = HandleVertex;
		shaderState.handleSample = HandleSample;
		shaderState.constantBuffer = &constantBuffer;
		shaderState.semanticMap = &semanticMap;
		
		float s = 100.0f;
		
		XVert vertices1[3];
		vertices1[0].varying[0].elem[0] = -s;
		vertices1[0].varying[0].elem[1] = -s;
		vertices1[0].varying[0].elem[2] = +1.0f;
		vertices1[0].varying[1].elem[1] = 1.0f;
		vertices1[1].varying[0].elem[0] = +s;
		vertices1[1].varying[0].elem[1] = -s;
		vertices1[1].varying[0].elem[2] = +1.0f;
		vertices1[1].varying[1].elem[1] = 2.0f;
		vertices1[2].varying[0].elem[0] = +0.0f;
		vertices1[2].varying[0].elem[1] = +s;
		vertices1[2].varying[0].elem[2] = +0.8f;
		vertices1[2].varying[1].elem[1] = 3.0f;
		
		XVert vertices2[3];
		for (int i = 0; i < 3; ++i)
		{
			vertices2[i] = vertices1[i];
			vertices2[i].varying[0].elem[0] += tx;
			vertices2[i].varying[1].elem[0] = (i + 1) / 3.0f;
		}
		
		XVert vertices3[3];
		for (int i = 0; i < 3; ++i)
		{
			vertices3[i] = vertices1[i];
			vertices3[i].varying[0].elem[0] += tx;
			vertices3[i].varying[0].elem[1] *= -1.0f;
			vertices3[i].varying[1].elem[0] = (i + 1) / 3.0f;
		}
		
		int indices[3] = { 0, 1, 2 };
		
		XRastTriStream(shaderState, vertices2, indices, 3);
		XRastTriStream(shaderState, vertices3, indices, 3);
		
		SDL_UnlockSurface(gSurface);
		
		SDL_Flip(pSurface);
	}
	
	SDL_FreeSurface(pSurface);
	pSurface = 0;
	
	SDL_Quit();
	
    return 0;
}

