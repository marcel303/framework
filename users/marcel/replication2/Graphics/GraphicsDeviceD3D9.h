#ifndef GRAPHICSDEVICED3D9_H
#define GRAPHICSDEVICED3D9_H
#pragma once

#include <d3d9.h>
#include <d3dx9.h>
#include <map>
#include "D3DResult.h"
#include "Debug.h"
#include "Display.h"
#include "GraphicsDevice.h"

class GraphicsDeviceD3D9 : public GraphicsDevice
{
public:
	GraphicsDeviceD3D9();
	virtual ~GraphicsDeviceD3D9();

	virtual void Initialize(const GraphicsOptions& options);
	virtual void Shutdown();

	virtual Display* GetDisplay();

	virtual void SceneBegin();
	void SceneEnd();

	virtual void Clear(int buffers, float r, float g, float b, float a, float z);
	virtual void Draw(PRIMITIVE_TYPE type);
	virtual void Present();
	virtual void Resolve(ResTexR* rt);
	virtual void Copy(ResBaseTex* out_tex);
	virtual void SetScissorRect(int x1, int y1, int x2, int y2);

	virtual void RS(int state, int value);
	virtual void SS(int sampler, int state, int value);
	virtual Mat4x4 GetMatrix(MATRIX_NAME matID);
	virtual void SetMatrix(MATRIX_NAME matID, const Mat4x4& mat);

	virtual int GetRTW();
	virtual int GetRTH();

	class DataIB
	{
	public:
		inline DataIB()
		{
			m_ib = 0;
		}

		IDirect3DIndexBuffer9* m_ib;
	};

	class DataVB
	{
	public:
		inline DataVB()
		{
			m_vb = 0;
			m_fvf = 0;
			m_stride = 0;
		}

		IDirect3DVertexBuffer9* m_vb;
		int m_fvf;
		int m_stride;
	};

	class DataTex
	{
	public:
		inline DataTex()
		{
			m_tex = 0;
			m_sx = 0;
			m_sy = 0;
		}

		IDirect3DTexture9* m_tex;
		int m_sx;
		int m_sy;
	};

	class DataTexR : public DataTex
	{
	public:
		inline DataTexR() : DataTex()
		{
			m_colorRT = 0;
			m_depthRT = 0;
		}

		IDirect3DTexture9* m_colorRT;
		IDirect3DSurface9* m_depthRT;
	};

	class DataTexCR : public DataTex
	{
	public:
		inline DataTexCR() : DataTex()
		{
			m_cube = 0;
		}

		IDirect3DCubeTexture9* m_cube;
	};

	class DataTexCF
	{
	public:
	};

	class DataVS
	{
	public:
		D3DXHANDLE GetParameter(const std::string& name);

		IDirect3DVertexShader9* m_shader;
		ID3DXConstantTable* m_constantTable;
		std::map<std::string, D3DXHANDLE> m_parameters;
	};

	class DataPS
	{
	public:
		D3DXHANDLE GetParameter(const std::string& name);

		IDirect3DPixelShader9* m_shader;
		ID3DXConstantTable* m_constantTable;
		std::map<std::string, D3DXHANDLE> m_parameters;
	};

	virtual void SetRT(ResTexR* rt);
	virtual void SetRTM(ResTexR* rt1, ResTexR* rt2, ResTexR* rt3, ResTexR* rt4, int numRenderTargets, ResTexD* rtd);
	virtual void SetIB(ResIB* ib);
	virtual void SetVB(ResVB* vb);
	virtual void SetTex(int sampler, ResBaseTex* tex);

	virtual void SetVS(ResVS* vs);
	virtual void SetPS(ResPS* ps);

	virtual void OnResInvalidate(Res* res);
	virtual void OnResDestroy(Res* res);

	int Validate(Res* res);

	void UpLoad(Res* res);
	void UnLoad(Res* res);

	void* UpLoadIB(ResIB* ib);
	void* UpLoadVB(ResVB* vb);
	void* UpLoadTex(ResTex* tex);
	void* UpLoadTexR(ResTexR* tex, bool texColor, bool texDepth, bool rect);
	void* UpLoadTexCR(ResTexCR* tex);
	void* UpLoadTexCF(ResTexCF* tex);
	void* UpLoadVS(ResVS* vs);
	void* UpLoadPS(ResPS* ps);

	void UnLoadIB(ResIB* ib);
	void UnLoadVB(ResVB* vb);
	void UnLoadTex(ResTex* tex);
	void UnLoadTexR(ResTexR* tex);
	void UnLoadTexCR(ResTexCR* tex);
	void UnLoadTexCF(ResTexCF* tex);
	void UnLoadVS(ResVS* vs);
	void UnLoadPS(ResPS* ps);

	void DefaultRTAcquire();
	void DefaultRTRelease();

#ifdef FDEBUG
	void CheckError();
#else
	inline void CheckError() { }
#endif

	const static int MAX_TEX = 8;

	INITSTATE;

	Display* m_display;
	IDirect3D9* m_d3d;
	IDirect3DDevice9* m_device;
	IDirect3DSurface9* m_defaultColorRT;
	IDirect3DSurface9* m_defaultDepthRT;
	ResTexR* m_rts[4];
	ResTexR* m_rtd;
	int m_numRenderTargets;
	D3DResult m_result;

	Mat4x4 m_matW;
	Mat4x4 m_matV;
	Mat4x4 m_matP;

	ResTexR* m_rt;
	ResIB* m_ib;
	ResVB* m_vb;
	ResBaseTex* m_tex[MAX_TEX];
	ResVS* m_vs;
	ResPS* m_ps;

	std::map<Res*, void*> m_cache;

	// States.
	int m_stBlendSrc;
	int m_stBlendDst;
};

#endif
