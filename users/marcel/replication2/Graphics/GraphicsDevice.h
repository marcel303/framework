#ifndef GRAPHICSDEVICE_H
#define GRAPHICSDEVICE_H
#pragma once

#include "Debug.h"
#include "Display.h"
#include "GraphicsTypes.h"
#include "Mat4x4.h"
#include "ResBaseTex.h"
#include "ResIB.h"
#include "ResPS.h"
#include "ResTex.h"
#include "ResTexCR.h"
#include "ResTexD.h"
#include "ResTexR.h"
#include "ResUser.h"
#include "ResVB.h"
#include "ResVS.h"

class GraphicsDevice : public ResUser
{
public:
	GraphicsDevice();
	virtual ~GraphicsDevice();

	virtual void Initialize(const GraphicsOptions& options) = 0;
	void InitializeV1(int width, int height, bool fullscreen)
	{
		GraphicsOptions options(width, height, fullscreen, true);
		Initialize(options);
	}
	virtual void Shutdown() = 0;

	virtual Display* GetDisplay() = 0;

	virtual void SceneBegin() = 0;
	virtual void SceneEnd() = 0;

	virtual void Clear(int buffers, float r, float g, float b, float a, float z) = 0;
	virtual void Draw(PRIMITIVE_TYPE type) = 0;
	virtual void Present() = 0;
	virtual void Resolve(ResTexR* rt) = 0;
	virtual void Copy(ResBaseTex* out_tex) = 0;
	virtual void SetScissorRect(int x1, int y1, int x2, int y2) = 0;

	virtual void RS(int state, int value) = 0;
	virtual void SS(int sampler, int state, int value) = 0;
	virtual Mat4x4 GetMatrix(MATRIX_NAME matID) = 0;
	virtual void SetMatrix(MATRIX_NAME matID, const Mat4x4& mat) = 0;

	virtual int GetRTW() = 0;
	virtual int GetRTH() = 0;

	virtual void SetRT(ResTexR* rt) = 0;
	virtual void SetRTM(ResTexR* rt1, ResTexR* rt2, ResTexR* rt3, ResTexR* rt4, int numRenderTargets, ResTexD* rtd) = 0;
	virtual void SetIB(ResIB* ib) = 0;
	virtual void SetVB(ResVB* vb) = 0;
	virtual void SetTex(int sampler, ResBaseTex* tex) = 0;

	virtual void SetVS(ResVS* vs) = 0;
	virtual void SetPS(ResPS* ps) = 0;

	virtual void OnResInvalidate(Res* res) = 0;
	virtual void OnResDestroy(Res* res) = 0;

protected:
	void ResetStates();
};

#endif
