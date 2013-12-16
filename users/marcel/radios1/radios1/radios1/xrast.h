#pragma once

#include <assert.h>
#include <math.h>
#include <string.h>
#include "Mat4x4.h"

#define NUM_CONSTANTS 32
#define NUM_VARYING 4
#define NUM_SEMANTICS 8

template <typename T>
inline T Min(T v1, T v2)
{
	return v1 < v2 ? v1 : v2;
}

template <typename T>
inline T Max(T v1, T v2)
{
	return v1 > v2 ? v1 : v2;
}

//

inline void XAssert(bool expr)
{
	assert(expr);
}

enum XSemantic
{
	XSemantic_Position = 0	
};

class XVec
{
public:
	inline XVec()
	{
	}
	
	inline XVec(float v1, float v2, float v3, float v4)
	{
		elem[0] = v1;
		elem[1] = v2;
		elem[2] = v3;
		elem[3] = v4;
	}
	
	float elem[4];
};

inline void Add(XVec& v1, const XVec& v2)
{
	v1.elem[0] += v2.elem[0];
	v1.elem[1] += v2.elem[1];
	v1.elem[2] += v2.elem[2];
	v1.elem[3] += v2.elem[3];
}

inline XVec operator+(const XVec& v1, const XVec& v2)
{
	XVec r;
	r.elem[0] = v1.elem[0] + v2.elem[0];
	r.elem[1] = v1.elem[1] + v2.elem[1];
	r.elem[2] = v1.elem[2] + v2.elem[2];
	r.elem[3] = v1.elem[3] + v2.elem[3];
	return r;
}

inline XVec operator-(const XVec& v1, const XVec& v2)
{
	XVec r;
	r.elem[0] = v1.elem[0] - v2.elem[0];
	r.elem[1] = v1.elem[1] - v2.elem[1];
	r.elem[2] = v1.elem[2] - v2.elem[2];
	r.elem[3] = v1.elem[3] - v2.elem[3];
	return r;
}

inline XVec operator*(const XVec& v, float s)
{
	XVec r;
	r.elem[0] = v.elem[0] * s;
	r.elem[1] = v.elem[1] * s;
	r.elem[2] = v.elem[2] * s;
	r.elem[3] = v.elem[3] * s;
	return r;
}

class XConstantBuffer
{
public:
	void SetMatrix(int idx, const Mat4x4& m)
	{
		values[0] = XVec(m(0,0), m(1,0), m(2,0), m(3,0));
		values[1] = XVec(m(0,1), m(1,1), m(2,1), m(3,1));
		values[2] = XVec(m(0,2), m(1,2), m(2,2), m(3,2));
		values[3] = XVec(m(0,3), m(1,3), m(2,3), m(3,3));
	}
	
	Mat4x4 GetMatrix(int idx)
	{
		Mat4x4 m;
		for (int i = 0; i < 4; ++i)
		{
			m(0,i) = values[i].elem[0];
			m(1,i) = values[i].elem[1];
			m(2,i) = values[i].elem[2];
			m(3,i) = values[i].elem[3];
		}
		return m;
	}
	
	XVec values[NUM_CONSTANTS];
};

class XSemanticMap
{
public:
	int index[NUM_SEMANTICS];
};

class XVert
{
public:
	XVec varying[NUM_VARYING];	
};

inline XVert operator+(const XVert& v1, const XVert& v2);
inline XVert operator-(const XVert& v1, const XVert& v2);
inline XVert operator*(const XVert& v, float s);
inline XVert operator/(const XVert& v, float s);

inline void Add(XVert& v1, const XVert& v2)
{
	for (int i = 0; i < NUM_VARYING; ++i)
		Add(v1.varying[i], v2.varying[i]);
}

inline XVert operator+(const XVert& v1, const XVert& v2)
{
	XVert r;
	for (int i = 0; i < NUM_VARYING; ++i)
		r.varying[i] = v1.varying[i] + v2.varying[i];
	return r;
}

inline XVert operator-(const XVert& v1, const XVert& v2)
{
	XVert r;
	for (int i = 0; i < NUM_VARYING; ++i)
		r.varying[i] = v1.varying[i] - v2.varying[i];
	return r;
}

inline XVert operator*(const XVert& v, float s)
{
	XVert r;
	for (int i = 0; i < NUM_VARYING; ++i)
		r.varying[i] = v.varying[i] * s;
	return r;
}

inline XVert operator/(const XVert& v, float s)
{
	return v * (1.0f / s);
}

class XShaderVert
{
public:
	XVec position;
	XVert vert;
};

inline XShaderVert operator+(const XShaderVert& v1, const XVert& v2);
inline XShaderVert operator-(const XShaderVert& v1, const XVert& v2);
inline XShaderVert operator*(const XShaderVert& v, float s);
inline XShaderVert operator/(const XShaderVert& v, float s);

inline void Add(XShaderVert& v1, const XShaderVert& v2)
{
	Add(v1.position, v2.position);
	Add(v1.vert, v2.vert);
}

inline XShaderVert operator+(const XShaderVert& v1, const XShaderVert& v2)
{
	XShaderVert r;
	r.position = v1.position + v2.position;
	r.vert = v1.vert + v2.vert;
	return r;
}

inline XShaderVert operator-(const XShaderVert& v1, const XShaderVert& v2)
{
	XShaderVert r;
	r.position = v1.position - v2.position;
	r.vert = v1.vert - v2.vert;
	return r;
}

inline XShaderVert operator*(const XShaderVert& v, float s)
{
	XShaderVert r;
	r.position = v.position * s;
	r.vert = v.vert * s;
	return r;
}

inline XShaderVert operator/(const XShaderVert& v, float s)
{
	return v * (1.0f / s);
}

typedef void (*XVertexHandler)(class XShaderState& state, XVert& vOut, const XVert& vIn);
typedef void (*XSampleHandler)(class XShaderState& state, const XVert& v, int x, int y);

class XShaderState
{
public:
	XShaderState()
		: handleSample(0)
		, constantBuffer(0)
		, semanticMap(0)
	{
	}
	
	XVertexHandler handleVertex;
	XSampleHandler handleSample;
	XConstantBuffer* constantBuffer;
	XSemanticMap* semanticMap;
};

class XRastState
{
public:
	void Begin()
	{
		memset(state, 0, sizeof(state));
		y1 = kInvalid;
		y2 = kInvalid;
	}
	
	void Add(int y1, int y2)
	{
		XAssert(y1 >= 0);
		XAssert(y2 >= 0);
		XAssert(y1 < y2);
		
		if (this->y1 == kInvalid)
		{
			this->y1 = y1;
			this->y2 = y2;
		}
		else
		{
			this->y1 = Min(this->y1, y1);
			this->y2 = Max(this->y2, y2);
		}
	}
	
	int state[1024];
	XShaderVert v[2][1024]; // todo, size
	
	int y1;
	int y2;
	
private:
	static const int kInvalid = -1;
};

inline void XRastLine(XShaderState& state, const XShaderVert& v1, const XShaderVert& v2, int pixelY)
{
	XAssert(state.handleSample != 0);
	XAssert(state.constantBuffer != 0);
	XAssert(state.semanticMap != 0);
	
	//const int idxPosition = state.semanticMap->index[XSemantic_Position];
	
	const XShaderVert* pVert1;
	const XShaderVert* pVert2;
	
	if (v1.position.elem[0] < v2.position.elem[0])
	{
		pVert1 = &v1;
		pVert2 = &v2;
	}
	else
	{
		pVert1 = &v2;
		pVert2 = &v1;
	}
	
	float x1 = pVert1->position.elem[0];
	float x2 = pVert2->position.elem[0];
	
	//printf("x1 -> x2: %g -> %g\n", x1, x2);
	
	int pixelX1 = floorf(x1);
	int pixelX2 = floorf(x2);
	
	float offset = 1.0f - (x1 - float(pixelX1));
	
	XShaderVert d = (pVert2[0] - pVert1[0]) / (x2 - x1);
	XShaderVert v = pVert1[0] + d * offset;
	
	for (int pixelX = pixelX1; pixelX < pixelX2; ++pixelX)
	{	
		XVert final = v.vert / v.position.elem[2];
		
		state.handleSample(state, final, pixelX, pixelY);
		
		Add(v, d);
	}
}

inline void XRastBegin(XRastState& rastState)
{
	rastState.Begin();
}

inline void XRastEnd(XRastState& rastState, XShaderState& state)
{
	for (int pixelY = rastState.y1; pixelY < rastState.y2; ++pixelY)
	{
		XAssert(rastState.state[pixelY] == 2);
		
		const XShaderVert& v1 = rastState.v[0][pixelY];
		const XShaderVert& v2 = rastState.v[1][pixelY];
		
		XRastLine(state, v1, v2, pixelY);
	}
}

inline void XRastScan(XRastState& rastState, XShaderState& state, const XShaderVert& v1, const XShaderVert& v2)
{
	XAssert(state.constantBuffer != 0);
	XAssert(state.semanticMap != 0);
	
//	const int idxPosition = state.semanticMap->index[XSemantic_Position];
	
	const XShaderVert* pVert1;
	const XShaderVert* pVert2;
	
	if (v1.position.elem[1] < v2.position.elem[1])
	{
		pVert1 = &v1;
		pVert2 = &v2;
	}
	else
	{
		pVert1 = &v2;
		pVert2 = &v1;
	}
	
	float y1 = pVert1->position.elem[1];
	float y2 = pVert2->position.elem[1];
		
	int pixelY1 = floorf(y1);
	int pixelY2 = floorf(y2);
	
	if (pixelY1 != pixelY2)
	{
		rastState.Add(pixelY1, pixelY2);
		
		float offset = 1.0f - (y1 - float(pixelY1));
		
		XShaderVert d = (pVert2[0] - pVert1[0]) / (y2 - y1);
		XShaderVert v = pVert1[0] + d * offset;
		
		for (int pixelY = pixelY1; pixelY < pixelY2; ++pixelY)
		{
			XAssert(rastState.state[pixelY] < 2);
			
			rastState.v[rastState.state[pixelY]][pixelY] = v;
			
			rastState.state[pixelY]++;
			
			v = v + d;
		}
	}
}

inline void XRastTri(XShaderState& state, const XShaderVert& v1, const XShaderVert& v2, const XShaderVert& v3)
{
	XRastState rastState;
	
	XRastBegin(rastState);
	XRastScan(rastState, state, v1, v2);
	XRastScan(rastState, state, v2, v3);
	XRastScan(rastState, state, v3, v1);
	XRastEnd(rastState, state);
}

inline void XRastTriStream(XShaderState& state, const XVert* pVertices, const int* pVertIndices, int numIndices)
{
	XAssert(state.handleVertex != 0);
	
	const int idxPosition = state.semanticMap->index[XSemantic_Position];
	const int numTris = numIndices / 3;
	
	for (int i = 0; i < numTris; ++i, pVertIndices += 3)
	{
		int index1 = pVertIndices[0];
		int index2 = pVertIndices[1];
		int index3 = pVertIndices[2];
		
		const XVert& v1 = pVertices[index1];
		const XVert& v2 = pVertices[index2];
		const XVert& v3 = pVertices[index3];
		
		float z1 = v1.varying[idxPosition].elem[2];
		float z2 = v2.varying[idxPosition].elem[2];
		float z3 = v3.varying[idxPosition].elem[2];
		
		XShaderVert shaderV1;
		XShaderVert shaderV2;
		XShaderVert shaderV3;
		
		state.handleVertex(state, shaderV1.vert, v1);
		state.handleVertex(state, shaderV2.vert, v2);
		state.handleVertex(state, shaderV3.vert, v3);
		
		shaderV1.position = shaderV1.vert.varying[idxPosition];
		shaderV2.position = shaderV2.vert.varying[idxPosition];
		shaderV3.position = shaderV3.vert.varying[idxPosition];
		
		shaderV1.position.elem[2] = 1.0f / z1;
		shaderV2.position.elem[2] = 1.0f / z2;
		shaderV3.position.elem[2] = 1.0f / z3;
		
		shaderV1.vert = shaderV1.vert / z1;
		shaderV2.vert = shaderV2.vert / z2;
		shaderV3.vert = shaderV3.vert / z3;
		
		XRastTri(state, shaderV1, shaderV2, shaderV3);
	}
}

