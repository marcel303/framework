#include <d3dx9.h>
#include "Debug.h"
#include "DisplaySDL.h"
#include "DisplayWindows.h"
#include "GraphicsDeviceD3D9.h"

static D3DPRIMITIVETYPE ConvPrimitiveType(PRIMITIVE_TYPE pt);
static int CalcPrimitiveCount(PRIMITIVE_TYPE pt, int vertexCnt, int indexCnt);
static int ConvFunc(int func);
static int ConvCull(int cull);
static int ConvBlendOp(int op);
static uint32_t ConvertFVF(int fvf, int texCnt);

GraphicsDeviceD3D9::GraphicsDeviceD3D9()
	: GraphicsDevice()
{
	INITINIT;

	m_display = 0;
	m_d3d = 0;
	m_device = 0;
	m_defaultColorRT = 0;
	m_defaultDepthRT = 0;
	memset(m_rts, 0, sizeof(m_rts));
	m_rtd = 0;
	m_numRenderTargets = 0;

	m_matW.MakeIdentity();
	m_matV.MakeIdentity();
	m_matP.MakeIdentity();

	m_rt = 0;
	m_ib = 0;
	m_vb = 0;
	m_vs = 0;
	m_ps = 0;

	for (int i = 0; i < MAX_TEX; ++i)
		m_tex[i] = 0;

	//m_stDepthFunc = D3DCMP_LESSEQUAL;
	//m_stAlphaFunc = CMP_GE;
	//m_stAlphaRef = 0.5f;
	m_stBlendSrc = BLEND_SRC;
	m_stBlendDst = BLEND_DST;

	// FIXME: Remove or move to window class..
#if 0
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_SetVideoMode(128, 128, 32, SDL_DOUBLEBUF);
	SDL_ShowCursor(SDL_DISABLE);
	SDL_WM_GrabInput(SDL_GRAB_ON);
#endif
}

GraphicsDeviceD3D9::~GraphicsDeviceD3D9()
{
	INITCHECK(false);
}

void GraphicsDeviceD3D9::Initialize(const GraphicsOptions& options)
{
	INITCHECK(false);

	m_display = new DisplaySDL(0, 0, options.Sx, options.Sy, options.Fullscreen, false);
	//m_display = new DisplaySDL(0, 0, 640, 480, false, false);
	//m_display = new DisplayWindows(0, 0, width, height, fullscreen);

	m_d3d = Direct3DCreate9(D3D_SDK_VERSION);

	if (!m_d3d)
		FASSERT(0);

	HWND hWnd = (HWND)m_display->Get("hWnd");

	if (!hWnd)
		FASSERT(0);

	D3DPRESENT_PARAMETERS pp;
	ZeroMemory(&pp, sizeof(D3DPRESENT_PARAMETERS));
	pp.Flags = 0;
	pp.FullScreen_RefreshRateInHz = 0;
	pp.hDeviceWindow = hWnd;
	pp.MultiSampleQuality = 0;
	pp.MultiSampleType = D3DMULTISAMPLE_NONE;
	pp.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
	pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	pp.Windowed = options.Fullscreen ? FALSE : TRUE;

	D3DFORMAT fullscreenFormat = D3DFMT_X8R8G8B8;
	
	if (options.Fullscreen)
	{
		bool success = false;
		int count = m_d3d->GetAdapterModeCount(D3DADAPTER_DEFAULT, fullscreenFormat);
		for (int i = 0; i < count; ++i)
		{
			D3DDISPLAYMODE mode;
			HRESULT result = m_d3d->EnumAdapterModes(D3DADAPTER_DEFAULT, fullscreenFormat, 0, &mode);
			if (result == S_OK)
			{
				pp.FullScreen_RefreshRateInHz = mode.RefreshRate;
				success = true;
				break;
			}
		}
		FASSERT(success);
	}

	if (options.CreateRenderTarget)
	{
		pp.BackBufferCount = 1;
		pp.BackBufferWidth = options.Sx;
		pp.BackBufferHeight = options.Sy;
		pp.EnableAutoDepthStencil = TRUE;
		pp.AutoDepthStencilFormat = D3DFMT_D24S8;

		if (!pp.Windowed)
		{
			pp.BackBufferFormat = fullscreenFormat;
		}
		else
		{
			D3DDISPLAYMODE displayMode;
			m_result = m_d3d->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &displayMode);
			CheckError();
			pp.BackBufferFormat = displayMode.Format;
		}
	}

	m_result = m_d3d->CreateDevice(
		D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		hWnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&pp,
		&m_device);
	CheckError();

	DefaultRTAcquire();

	// Default render states.
	m_result = m_device->SetRenderState(D3DRS_COLORVERTEX, 0);
	m_result = m_device->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
	m_result = m_device->SetRenderState(D3DRS_NORMALIZENORMALS, 1);
	m_result = m_device->SetRenderState(D3DRS_AMBIENT, D3DXCOLOR(0.0f, 0.0f, 0.0f, 0.0f));
	m_result = m_device->SetRenderState(D3DRS_LIGHTING, 0);
	m_result = m_device->SetRenderState(D3DRS_FOGENABLE, 0);
	CheckError();

	ResetStates();

	INITSET(true);
}

void GraphicsDeviceD3D9::Shutdown()
{
	INITCHECK(true);

	FASSERT(m_rt == 0);
	FASSERT(m_ib == 0);
	FASSERT(m_vb == 0);
	for (int i = 0; i < MAX_TEX; ++i)
		FASSERT(m_tex[i] == 0);
	FASSERT(m_vs == 0);
	FASSERT(m_ps == 0);

	// TODO: Reset stuff.
	/*
	for (int i = 0; i < MAX_TEX; ++i)
		SetTex(i, 0);
	SetIB(0);
	SetVB(0);
	SetVS(0);
	SetPS(0);*/

	FASSERT(m_cache.size() == 0);

	m_result = m_device->Release();
	CheckError();
	m_device = 0;
	m_result = m_d3d->Release();
	CheckError();
	m_d3d = 0;

	SAFE_FREE(m_display);

	INITSET(false);
}

Display* GraphicsDeviceD3D9::GetDisplay()
{
	return m_display;
}

void GraphicsDeviceD3D9::SceneBegin()
{
	// TODO: Move to engine..
	//m_display->Update();

	m_result = m_device->BeginScene();
	CheckError();
}

void GraphicsDeviceD3D9::SceneEnd()
{
	m_result = m_device->EndScene();
	CheckError();
}

void GraphicsDeviceD3D9::Clear(int buffers, float r, float g, float b, float a, float z)
{
	int d3dBuffers = 0;

	if (buffers & BUFFER_COLOR)
		d3dBuffers |= D3DCLEAR_TARGET;
	if (buffers & BUFFER_DEPTH)
		d3dBuffers |= D3DCLEAR_ZBUFFER;
	if (buffers & BUFFER_STENCIL)
		d3dBuffers |= D3DCLEAR_STENCIL;

	D3DCOLOR color = D3DCOLOR_COLORVALUE(r, g, b, a);

	m_result = m_device->Clear(0, 0, d3dBuffers, color, z, 0x00);
	CheckError();
}

void GraphicsDeviceD3D9::Draw(PRIMITIVE_TYPE type)
{
	D3DPRIMITIVETYPE pt = ConvPrimitiveType(type);
	int primitiveCount = CalcPrimitiveCount(type, m_vb->GetVertexCnt(), m_ib ? m_ib->GetIndexCnt() : 0);

	if (m_ib)
		m_result = m_device->DrawIndexedPrimitive(pt, 0, 0, m_vb->GetVertexCnt(), 0, primitiveCount);
	else
		m_result = m_device->DrawPrimitive(pt, 0, primitiveCount);

	CheckError();
}

void GraphicsDeviceD3D9::Present()
{
	//DefaultRTRelease();

	m_result = m_device->Present(0, 0, 0, 0);
	CheckError();

	//DefaultRTAcquire();
}

void GraphicsDeviceD3D9::Resolve(ResTexR* rt)
{
	m_result = m_device->BeginScene();
	CheckError();

	Validate(rt);
	DataTexR* data = (DataTexR*)m_cache[rt];

	IDirect3DSurface9* src = 0;
	IDirect3DSurface9* dst = m_defaultColorRT;

	m_result = data->m_colorRT->GetSurfaceLevel(0, &src);
	CheckError();

	m_result = m_device->StretchRect(src, 0, dst, 0, D3DTEXF_NONE);
	CheckError();

	m_result = src->Release();
	CheckError();

	m_result = m_device->EndScene();
	CheckError();
}

void GraphicsDeviceD3D9::Copy(ResBaseTex* out_tex)
{
}

void GraphicsDeviceD3D9::SetScissorRect(int x1, int y1, int x2, int y2)
{
	if (x1 < 0)
		x1 = 0;
	if (y1 < 0)
		y1 = 0;
	
	int width = GetRTW();
	int height = GetRTH();

	if (x2 >= width)
		x2 = width - 1;
	if (y2 >= height)
		y2 = height - 1;

	RECT rect;
	rect.left = x1;
	rect.top = y1;
	rect.right = x2 + 1;
	rect.bottom = y2 + 1;

	m_result = m_device->SetScissorRect(&rect);
	CheckError();
}

void GraphicsDeviceD3D9::RS(int state, int value)
{
	switch (state)
	{
	case RS_DEPTHTEST:
		m_result = m_device->SetRenderState(D3DRS_ZENABLE, value);
		break;
	case RS_DEPTHTEST_FUNC:
		m_result = m_device->SetRenderState(D3DRS_ZFUNC, ConvFunc(value));
		break;
	case RS_CULL:
		m_result = m_device->SetRenderState(D3DRS_CULLMODE, ConvCull(value));
		break;
	case RS_ALPHATEST:
		m_result = m_device->SetRenderState(D3DRS_ALPHATESTENABLE, value);
		break;
	case RS_ALPHATEST_FUNC:
		m_result = m_device->SetRenderState(D3DRS_ALPHAFUNC, ConvFunc(value));
		break;
	case RS_ALPHATEST_REF:
		m_result = m_device->SetRenderState(D3DRS_ALPHAREF, value);
		break;
	case RS_STENCILTEST:
		m_result = m_device->SetRenderState(D3DRS_STENCILENABLE, value);
		break;
		// TODO: Implement stencil test.
	case RS_STENCILTEST_FUNC:
		break;
	case RS_STENCILTEST_MASK:
		break;
	case RS_STENCILTEST_REF:
		break;
	case RS_STENCILTEST_ONPASS:
		break;
	case RS_STENCILTEST_ONFAIL:
		break;
	case RS_STENCILTEST_ONZFAIL:
		break;
	case RS_BLEND:
		m_result = m_device->SetRenderState(D3DRS_ALPHABLENDENABLE, value);
		break;
	case RS_BLEND_SRC:
		m_result = m_device->SetRenderState(D3DRS_SRCBLEND, ConvBlendOp(value));
		break;
	case RS_BLEND_DST:
		m_result = m_device->SetRenderState(D3DRS_DESTBLEND, ConvBlendOp(value));
		break;
	case RS_SCISSORTEST:
		m_result = m_device->SetRenderState(D3DRS_SCISSORTESTENABLE, value);
		break;
	case RS_WRITE_DEPTH:
		m_result = m_device->SetRenderState(D3DRS_ZWRITEENABLE, value);
		break;
	case RS_WRITE_COLOR:
		m_result = m_device->SetRenderState(D3DRS_COLORWRITEENABLE, value);
		break;
	default:
		FASSERT(0);
		break;
	}

	CheckError();
}

void GraphicsDeviceD3D9::SS(int sampler, int state, int value)
{
	switch (state)
	{
	case SS_FILTER:
#if 0
	if (state == SS_FILTER)
	{
		// Handle the special SS_FILTER state.
		switch (value)
		{
		case SSV_POINT:
			if (
				m_d3dDevice->SetSamplerState(static_cast<DWORD>(sampler), D3DSAMP_MAGFILTER, D3DTEXF_POINT) != D3D_OK ||
				m_d3dDevice->SetSamplerState(static_cast<DWORD>(sampler), D3DSAMP_MINFILTER, D3DTEXF_POINT) != D3D_OK ||
				m_d3dDevice->SetSamplerState(static_cast<DWORD>(sampler), D3DSAMP_MIPFILTER, D3DTEXF_POINT) != D3D_OK)
				m_result = D3DERR_INVALIDCALL;
			else
				m_result = D3D_OK;
			break;
		case SSV_INTERPOLATE:
			if (
				m_d3dDevice->SetSamplerState(static_cast<DWORD>(sampler), D3DSAMP_MAGFILTER, D3DTEXF_LINEAR) != D3D_OK ||
				m_d3dDevice->SetSamplerState(static_cast<DWORD>(sampler), D3DSAMP_MINFILTER, D3DTEXF_LINEAR) != D3D_OK ||
				m_d3dDevice->SetSamplerState(static_cast<DWORD>(sampler), D3DSAMP_MIPFILTER, D3DTEXF_LINEAR) != D3D_OK)
				m_result = D3DERR_INVALIDCALL;
			else
				m_result = D3D_OK;
			break;
		default:
			m_result = D3DERR_INVALIDCALL;
		}
#endif
		break;
	default:
		FASSERT(0);
		break;
	}

	CheckError();
}

Mat4x4 GraphicsDeviceD3D9::GetMatrix(MATRIX_NAME matID)
{
	Mat4x4 result;

	switch (matID)
	{
	case MAT_WRLD:
		result = m_matW;
		break;
	case MAT_VIEW:
		result = m_matV;
		break;
	case MAT_PROJ:
		result = m_matP;
		break;
	}

	return result;
}

void GraphicsDeviceD3D9::SetMatrix(MATRIX_NAME matID, const Mat4x4& mat)
{
	switch (matID)
	{
	case MAT_WRLD:
		m_matW = mat;
		m_result = m_device->SetTransform(D3DTS_WORLD, (D3DMATRIX*)&m_matW);
		break;
	case MAT_VIEW:
		m_matV = mat;
		m_result = m_device->SetTransform(D3DTS_VIEW, (D3DMATRIX*)&m_matV);
		break;
	case MAT_PROJ:
		m_matP = mat;
		m_result = m_device->SetTransform(D3DTS_PROJECTION, (D3DMATRIX*)&m_matP);
		break;
	}

	CheckError();
}

int GraphicsDeviceD3D9::GetRTW()
{
	IDirect3DSurface9* surface = 0;

	m_result = m_device->GetRenderTarget(0, &surface);
	CheckError();

	D3DSURFACE_DESC desc;

	m_result = surface->GetDesc(&desc);
	CheckError();

	m_result = surface->Release();
	CheckError();

	return desc.Width;
}

int GraphicsDeviceD3D9::GetRTH()
{
	IDirect3DSurface9* surface = 0;

	m_result = m_device->GetRenderTarget(0, &surface);
	CheckError();

	D3DSURFACE_DESC desc;

	m_result = surface->GetDesc(&desc);
	CheckError();

	m_result = surface->Release();
	CheckError();

	return desc.Height;
}

void GraphicsDeviceD3D9::SetRT(ResTexR* rt)
{
	FASSERT(false);

	if (rt == 0)
	{
		m_result = m_device->SetRenderTarget(0, m_defaultColorRT);
		CheckError();
		m_result = m_device->SetDepthStencilSurface(m_defaultDepthRT);
		CheckError();

		D3DVIEWPORT9 viewport;
		viewport.X      = 0;
		viewport.Y      = 0;
		viewport.Width  = GetRTW();
		viewport.Height = GetRTH();
		viewport.MinZ   = 0.0f;
		viewport.MaxZ   = 1.0f;
		m_result = m_device->SetViewport(&viewport);
		CheckError();

		return;
	}

	// Don't support the way of olde for now..
	return;

	if (rt == 0)
	{
		m_result = m_device->SetRenderTarget(0, m_defaultColorRT);
		CheckError();
		m_result = m_device->SetDepthStencilSurface(m_defaultDepthRT);
		CheckError();

		D3DVIEWPORT9 viewport;
		viewport.X      = 0;
		viewport.Y      = 0;
		viewport.Width  = GetRTW();
		viewport.Height = GetRTH();
		viewport.MinZ   = 0.0f;
		viewport.MaxZ   = 1.0f;
		m_result = m_device->SetViewport(&viewport);
		CheckError();
	}
	else
	{
		Validate(rt);

		if (rt->m_type == RES_TEXR || rt->m_type == RES_TEXD || rt->m_type == RES_TEXRECTR)
		{
			DataTexR* data = (DataTexR*)m_cache[rt];

			IDirect3DSurface9* colorSurface;
			IDirect3DSurface9* depthSurface;

			m_result = data->m_colorRT->GetSurfaceLevel(0, &colorSurface);
			CheckError();
			//data->m_depthRT->GetSurfaceLevel(0, &m_depthSurface);
			depthSurface = data->m_depthRT;
			CheckError();

			m_result = m_device->SetRenderTarget(0, colorSurface);
			CheckError();
			m_result = m_device->SetDepthStencilSurface(depthSurface);
			CheckError();

			colorSurface->Release();
			//depthSurface->Release();

			D3DVIEWPORT9 viewport;
			viewport.X      = 0;
			viewport.Y      = 0;
			viewport.Width  = GetRTW();
			viewport.Height = GetRTH();
			viewport.MinZ   = 0.0f;
			viewport.MaxZ   = 1.0f;
			m_result = m_device->SetViewport(&viewport);
			CheckError();
		}
		if (rt->m_type == RES_TEXCF) // FIXME: RES_TEXCRF
		{
			ResTexCF* texCF = (ResTexCF*)rt;
			ResTexCR* texCR = texCF->m_cube;

			Validate(texCR);

			//DataTexCF* dataCF = (DataTexCF*)m_cache[rt];
			DataTexCR* dataCR = (DataTexCR*)m_cache[texCR];

			// FIXME/TODO, Get cube face, assign.
			//m_device->SetRenderTarget(data->m_colorRT->GetSurfaceLevel(0), data->m_depthRT->GetSurfaceLevel(0));

			D3DVIEWPORT9 viewport;
			viewport.X      = 0;
			viewport.Y      = 0;
			viewport.Width  = GetRTW();
			viewport.Height = GetRTH();
			viewport.MinZ   = 0.0f;
			viewport.MaxZ   = 1.0f;
			m_result = m_device->SetViewport(&viewport);
			CheckError();
		}
	}

	m_rt = rt;
}

void GraphicsDeviceD3D9::SetRTM(ResTexR* rt1, ResTexR* rt2, ResTexR* rt3, ResTexR* rt4, int numRenderTargets, ResTexD* rtd)
{
	const D3DRENDERSTATETYPE state[4] = { D3DRS_COLORWRITEENABLE, D3DRS_COLORWRITEENABLE1, D3DRS_COLORWRITEENABLE2, D3DRS_COLORWRITEENABLE3 };

	for (int i = numRenderTargets; i < m_numRenderTargets; ++i)
	{
		if (i != 0)
		{
			m_result = m_device->SetRenderTarget(i, 0);
			CheckError();
		}

		m_result = m_device->SetRenderState(state[i], 0x00);
		CheckError();

		m_rts[i] = 0;
	}

	ResTexR* rts[4] = { rt1, rt2, rt3, rt4 };

	int sx = -1;
	int sy = -1;

	for (int i = 0; i < numRenderTargets; ++i)
	{
		ResTexR* rt = rts[i];

		if (rt == m_rts[i])
			continue;

		m_rts[i] = rt;

		if (rt)
		{
			Validate(rt);

			if (rt->m_type == RES_TEXR || rt->m_type == RES_TEXRECTR)
			{
				DataTexR* data = (DataTexR*)m_cache[rt];

				IDirect3DSurface9* surface = 0;
				m_result = data->m_colorRT->GetSurfaceLevel(0, &surface);
				CheckError();

				m_result = m_device->SetRenderTarget(i, surface);
				CheckError();

				m_result = m_device->SetRenderState(state[i], 0xFF);
				CheckError();

				if (sx < 0)
				{
					sx = data->m_sx;
					sy = data->m_sy;
				}
				else
				{
					FASSERT(data->m_sx == sx);
					FASSERT(data->m_sy == sy);
				}

				m_result = surface->Release();
				CheckError();
			}
			else
			{
				FASSERT(false);
			}
		}
		else
		{
			if (i != 0)
			{
				m_result = m_device->SetRenderTarget(i, 0);
				CheckError();
			}

			m_result = m_device->SetRenderState(state[i], 0x00);
			CheckError();
		}
	}

	m_numRenderTargets = numRenderTargets;

	if (rtd != m_rtd)
	{
		if (rtd)
		{
			Validate(rtd);

			DataTexR* data = (DataTexR*)m_cache[rtd];

			m_device->SetDepthStencilSurface(data->m_depthRT);
			CheckError();

			if (sx < 0)
			{
				sx = data->m_sx;
				sy = data->m_sy;
			}
			else
			{
				FASSERT(data->m_sx == sx);
				FASSERT(data->m_sy == sy);
			}
		}
		else
		{
			m_result = m_device->SetDepthStencilSurface(0);
			CheckError();
		}
	}

	if (numRenderTargets > 0 || rtd)
	{
		FASSERT(sx >= 0 && sy >= 0);

		D3DVIEWPORT9 viewport;
		viewport.X      = 0;
		viewport.Y      = 0;
		viewport.Width  = sx;
		viewport.Height = sy;
		viewport.MinZ   = 0.0f;
		viewport.MaxZ   = 1.0f;
		m_result = m_device->SetViewport(&viewport);
		CheckError();
	}
}

void GraphicsDeviceD3D9::SetIB(ResIB* ib)
{
	if (ib == 0)
	{
		m_result = m_device->SetIndices(0);
		CheckError();
	}
	else
	{
		Validate(ib);

		DataIB* data = (DataIB*)m_cache[ib];

		m_result = m_device->SetIndices(data->m_ib);
		CheckError();
	}

	m_ib = ib;
}

void GraphicsDeviceD3D9::SetVB(ResVB* vb)
{
	if (vb == 0)
	{
		m_result = m_device->SetStreamSource(0, 0, 0, 0);
		CheckError();
	}
	else
	{
		Validate(vb);

		DataVB* data = (DataVB*)m_cache[vb];

		m_result = m_device->SetStreamSource(0, data->m_vb, 0, data->m_stride);
		CheckError();
		m_result = m_device->SetFVF(data->m_fvf);
		CheckError();
	}

	m_vb = vb;
}

void GraphicsDeviceD3D9::SetTex(int sampler, ResBaseTex* tex)
{
	if (tex == 0)
	{
		if (m_tex[sampler])
		{
			switch (m_tex[sampler]->m_type)
			{
			case RES_TEX:
				m_result = m_device->SetTexture(sampler, 0);
				CheckError();

				m_result = m_device->SetSamplerState(sampler, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
				m_result = m_device->SetSamplerState(sampler, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
				CheckError();
				m_result = m_device->SetSamplerState(sampler, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
				CheckError();
				m_result = m_device->SetSamplerState(sampler, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
				m_result = m_device->SetSamplerState(sampler, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
				CheckError();
				break;
			case RES_TEXD:
				m_result = m_device->SetTexture(sampler, 0);
				CheckError();

				m_result = m_device->SetSamplerState(sampler, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
				m_result = m_device->SetSamplerState(sampler, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
				CheckError();
				m_result = m_device->SetSamplerState(sampler, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
				CheckError();
				m_result = m_device->SetSamplerState(sampler, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
				m_result = m_device->SetSamplerState(sampler, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
				CheckError();
				break;
			case RES_TEXR:
				m_result = m_device->SetTexture(sampler, 0);
				CheckError();

				m_result = m_device->SetSamplerState(sampler, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
				m_result = m_device->SetSamplerState(sampler, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
				CheckError();
				m_result = m_device->SetSamplerState(sampler, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
				CheckError();
				m_result = m_device->SetSamplerState(sampler, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
				m_result = m_device->SetSamplerState(sampler, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
				CheckError();
				break;
			case RES_TEXRECTR:
				// FIXME: Rectangle.. ?
				m_result = m_device->SetTexture(sampler, 0);
				CheckError();

				m_result = m_device->SetSamplerState(sampler, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
				m_result = m_device->SetSamplerState(sampler, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
				CheckError();
				m_result = m_device->SetSamplerState(sampler, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
				CheckError();
				m_result = m_device->SetSamplerState(sampler, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
				m_result = m_device->SetSamplerState(sampler, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
				CheckError();
				break;
			case RES_TEXCR:
				m_result = m_device->SetTexture(sampler, 0);
				CheckError();

				m_result = m_device->SetSamplerState(sampler, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
				m_result = m_device->SetSamplerState(sampler, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
				CheckError();
				m_result = m_device->SetSamplerState(sampler, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
				CheckError();
				m_result = m_device->SetSamplerState(sampler, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
				m_result = m_device->SetSamplerState(sampler, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
				CheckError();
				break;
			default:
				FASSERT(0);
				break;
			}
		}
	}
	else
	{
		Validate(tex);

		DataTex* data = (DataTex*)m_cache[tex];

		switch (tex->m_type)
		{
		case RES_TEX:
			m_result = m_device->SetTexture(sampler, data->m_tex);
			CheckError();

			m_result = m_device->SetSamplerState(sampler, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
			m_result = m_device->SetSamplerState(sampler, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
			CheckError();
			m_result = m_device->SetSamplerState(sampler, D3DSAMP_MIPFILTER, D3DTEXF_LINEAR);
			CheckError();
			m_result = m_device->SetSamplerState(sampler, D3DSAMP_ADDRESSU, D3DTADDRESS_WRAP);
			m_result = m_device->SetSamplerState(sampler, D3DSAMP_ADDRESSV, D3DTADDRESS_WRAP);
			CheckError();
			break;
		case RES_TEXD:
			m_result = m_device->SetTexture(sampler, data->m_tex);
			CheckError();
			break;
		case RES_TEXR:
			{
				DataTexR* dataR = (DataTexR*)data;
				m_result = m_device->SetTexture(sampler, dataR->m_colorRT);
				CheckError();

				m_result = m_device->SetSamplerState(sampler, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
				m_result = m_device->SetSamplerState(sampler, D3DSAMP_MAGFILTER, D3DTEXF_POINT);
				CheckError();
				m_result = m_device->SetSamplerState(sampler, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
				CheckError();
				m_result = m_device->SetSamplerState(sampler, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
				m_result = m_device->SetSamplerState(sampler, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);
				CheckError();
			}
			break;
		case RES_TEXRECTR:
			// FIXME: Rectangle..?
			m_result = m_device->SetTexture(sampler, data->m_tex);
			CheckError();
			break;
		case RES_TEXCR:
			m_result = m_device->SetTexture(sampler, data->m_tex);
			CheckError();
			break;
		default:
			FASSERT(0);
			break;
		}
	}

	m_tex[sampler] = tex;
}

void GraphicsDeviceD3D9::SetVS(ResVS* vs)
{
	if (vs == 0)
	{
		if (m_vs)
		{
			DataVS* data = (DataVS*)m_cache[m_vs];

			m_result = m_device->SetVertexShader(0);
			CheckError();
		}
	}
	else
	{
		int changed = Validate(vs);

		DataVS* data = (DataVS*)m_cache[vs];

		if (changed || vs != m_vs)
		{
			m_result = m_device->SetVertexShader(data->m_shader);
			CheckError();
		}

		// Apply parameters.
		for (ShaderParamList::ParamCollItr i = vs->p.m_parameters.begin(); i != vs->p.m_parameters.end(); ++i)
		{
			const std::string& name = i->first;
			const ShaderParam& param = i->second;

			const D3DXHANDLE p = data->GetParameter(name);
			CheckError();

			if (p)
			{
				switch (param.m_type)
				{
				case SHPARAM_FLOAT:  m_result = data->m_constantTable->SetFloat(m_device, p, param.m_v[0]);            break;
				case SHPARAM_VEC3:   m_result = data->m_constantTable->SetFloatArray(m_device, p, param.m_v, 3);       break;
				case SHPARAM_VEC4:   m_result = data->m_constantTable->SetFloatArray(m_device, p, param.m_v, 4);       break;
				case SHPARAM_MAT4X4: m_result = data->m_constantTable->SetMatrix(m_device, p, (D3DXMATRIX*)param.m_v); break;
				case SHPARAM_TEX:    FASSERT(0);                                                                       break; // Not supporter ATM..
				}
			}

			CheckError();
		}
	}

	m_vs = vs;
}

void GraphicsDeviceD3D9::SetPS(ResPS* ps)
{
	if (ps == 0)
	{
		if (m_ps)
		{
			DataPS* data = (DataPS*)m_cache[m_ps];

			for (int i = 0; i < 8; ++i)
				SetTex(i, 0);

			m_result = m_device->SetPixelShader(0);
			CheckError();
		}
	}
	else
	{
		int changed = Validate(ps);

		DataPS* data = (DataPS*)m_cache[ps];

		if (changed || ps != m_ps)
		{
			m_result = m_device->SetPixelShader(data->m_shader);
			CheckError();
		}

		// Apply parameters.
		for (ShaderParamList::ParamCollItr i = ps->p.m_parameters.begin(); i != ps->p.m_parameters.end(); ++i)
		{
			const std::string& name = i->first;
			const ShaderParam& param = i->second;

			const D3DXHANDLE p = data->GetParameter(name);
			CheckError();

			if (p == 0)
				continue;

			switch (param.m_type)
			{
			case SHPARAM_FLOAT:  m_result = data->m_constantTable->SetFloat(m_device, p, param.m_v[0]);            break;
			case SHPARAM_VEC3:   m_result = data->m_constantTable->SetFloatArray(m_device, p, param.m_v, 3);       break;
			case SHPARAM_VEC4:   m_result = data->m_constantTable->SetFloatArray(m_device, p, param.m_v, 4);       break;
			case SHPARAM_MAT4X4: m_result = data->m_constantTable->SetMatrix(m_device, p, (D3DXMATRIX*)param.m_v); break;
			case SHPARAM_TEX:    SetTex(data->m_constantTable->GetSamplerIndex(p), (ResTex*)param.m_p);            break;
			}

			CheckError();
		}
	}

	m_ps = ps;
}

void GraphicsDeviceD3D9::OnResInvalidate(Res* res)
{
	FASSERT(res);

	UnLoad(res);
}

void GraphicsDeviceD3D9::OnResDestroy(Res* res)
{
	FASSERT(res);

	if (res == m_rt)
		SetRT(0);
	if (res == m_ib)
		SetIB(0);
	if (res == m_vb)
		SetVB(0);
	for (int i = 0; i < MAX_TEX; ++i)
		if (res == m_tex[i])
			SetTex(i, 0);
	if (res == m_vs)
		SetVS(0);
	if (res == m_ps)
		SetPS(0);

	UnLoad(res);
}

int GraphicsDeviceD3D9::Validate(Res* res)
{
	FASSERT(res);

	if (m_cache.count(res) == 0)
	{
		UpLoad(res);
		return 1;
	}

	return 0;
}

void GraphicsDeviceD3D9::UpLoad(Res* res)
{
	INITCHECK(true);

	FASSERT(res);
	FASSERT(m_cache.count(res) == 0);

	void* data = 0;

	switch (res->m_type)
	{
	case RES_IB:
		data = UpLoadIB((ResIB*)res);
		break;
	case RES_VB:
		data = UpLoadVB((ResVB*)res);
		break;
	case RES_TEX:
		data = UpLoadTex((ResTex*)res);
		break;
	case RES_TEXD:
		data = UpLoadTexR((ResTexD*)res, false, true, false);
		break;
	case RES_TEXR:
		if (((ResTexR*)res)->m_target == TEXR_COLOR)
			data = UpLoadTexR((ResTexR*)res, true, false, false);
		if (((ResTexR*)res)->m_target == TEXR_COLOR32F)
			data = UpLoadTexR((ResTexR*)res, true, false, false);
		if (((ResTexR*)res)->m_target == TEXR_DEPTH)
			data = UpLoadTexR((ResTexR*)res, false, true, false);
		break;
	case RES_TEXRECTR:
		if (((ResTexR*)res)->m_target == TEXR_COLOR)
			data = UpLoadTexR((ResTexR*)res, true, false, true);
		if (((ResTexR*)res)->m_target == TEXR_COLOR32F)
			data = UpLoadTexR((ResTexR*)res, true, false, true);
		if (((ResTexR*)res)->m_target == TEXR_DEPTH)
			data = UpLoadTexR((ResTexR*)res, false, true, true);
		break;
	case RES_TEXCR:
		data = UpLoadTexCR((ResTexCR*)res);
		break;
	case RES_TEXCF:
		data = UpLoadTexCF((ResTexCF*)res); // FIXME, TEXCRF.
		break;
	case RES_VS:
		data = UpLoadVS((ResVS*)res);
		break;
	case RES_PS:
		data = UpLoadPS((ResPS*)res);
		break;
	default:
		FASSERT(0);
		break;
	}

	m_cache[res] = data;

	res->AddUser(this);

	//printf("UpLoad: Cache size: %d.\n", (int)m_cache.size());
}

void GraphicsDeviceD3D9::UnLoad(Res* res)
{
	INITCHECK(true);

	FASSERT(res);
	FASSERT(m_cache.count(res) != 0);

	switch (res->m_type)
	{
	case RES_IB:
		UnLoadIB((ResIB*)res);
		break;
	case RES_VB:
		UnLoadVB((ResVB*)res);
		break;
	case RES_TEX:
		UnLoadTex((ResTex*)res);
		break;
	case RES_TEXD:
		UnLoadTexR((ResTexR*)res);
		break;
	case RES_TEXR:
		UnLoadTexR((ResTexR*)res);
		break;
	case RES_TEXRECTR:
		UnLoadTexR((ResTexR*)res);
		break;
	case RES_TEXCR:
		UnLoadTexCR((ResTexCR*)res);
		break;
	case RES_TEXCF:
		UnLoadTexCR((ResTexCR*)res);
		break;
	case RES_VS:
		UnLoadVS((ResVS*)res);
		break;
	case RES_PS:
		UnLoadPS((ResPS*)res);
		break;
	default:
		FASSERT(0);
		break;
	}

	res->RemoveUser(this);

	m_cache.erase(res);

	//printf("UnLoad: Cache size: %d.\n", (int)m_cache.size());
}

void* GraphicsDeviceD3D9::UpLoadIB(ResIB* ib)
{
	DataIB* data = new DataIB();

	int size = sizeof(uint16) * ib->GetIndexCnt();

	m_result = m_device->CreateIndexBuffer(size, 0, D3DFMT_INDEX16, D3DPOOL_MANAGED, &data->m_ib, 0);
	CheckError();

	void* buffer = 0;

	//m_result = data->m_ib->Lock(0, size, &buffer, D3DLOCK_DISCARD);
	m_result = data->m_ib->Lock(0, size, &buffer, 0);
	CheckError();

	memcpy(buffer, ib->index.GetData(), size);

	m_result = data->m_ib->Unlock();
	CheckError();
	// TODO: Lock, copy.

	return data;
}

void* GraphicsDeviceD3D9::UpLoadVB(ResVB* vb)
{
	DataVB* data = new DataVB();

	// Convert VB to stream.

	int fvf = vb->GetFVF();

	uint32_t d3dFVF = ConvertFVF(fvf, vb->m_texCnt);

	int stride = 0;

	if (fvf & FVF_XYZ)
		stride += sizeof(float) * 3;
	if (fvf & FVF_NORMAL)
		stride += sizeof(float) * 3;
	if (fvf & FVF_COLOR)
		stride += sizeof(uint8) * 4;

	stride += sizeof(float) * 4 * vb->m_texCnt;

	int size = stride * vb->m_vCnt;

	m_result = m_device->CreateVertexBuffer(size, D3DUSAGE_WRITEONLY, d3dFVF, D3DPOOL_MANAGED, &data->m_vb, 0);
	CheckError();

	char* buffer = 0;

	//m_result = data->m_vb->Lock(0, size, (void**)&buffer, D3DLOCK_DISCARD);
	m_result = data->m_vb->Lock(0, size, (void**)&buffer, 0);
	CheckError();

	for (uint32_t i = 0; i < vb->m_vCnt; ++i)
	{
		if (fvf & FVF_XYZ)
		{
			memcpy(buffer, &vb->position[i], sizeof(float) * 3);
			buffer += sizeof(float) * 3;
		}
		if (fvf & FVF_NORMAL)
		{
			memcpy(buffer, &vb->normal[i], sizeof(float) * 3);
			buffer += sizeof(float) * 3;
		}
		if (fvf & FVF_COLOR)
		{
			// TODO: Color!.
			uint32_t color =
				int(vb->color[i][2] * 255.0f) << 0 |
				int(vb->color[i][1] * 255.0f) << 8 |
				int(vb->color[i][0] * 255.0f) << 16 |
				int(vb->color[i][3] * 255.0f) << 24;

			memcpy(buffer, &color, sizeof(uint8) * 4);
			buffer += sizeof(uint8) * 4;
		}
		for (uint32_t j = 0; j < vb->m_texCnt; ++j)
		{
			memcpy(buffer, &vb->tex[j][i], sizeof(float) * 4);
			buffer += sizeof(float) * 4;
		}
	}

	m_result = data->m_vb->Unlock();
	CheckError();

	data->m_fvf = d3dFVF;
	data->m_stride = stride;

	return data;
}

void* GraphicsDeviceD3D9::UpLoadTex(ResTex* tex)
{
	DataTex* data = new DataTex();

	D3DFORMAT format = D3DFMT_A8R8G8B8;

	int w = tex->GetW();
	int h = tex->GetH();

	bool stub = false;

	if (w == 0 || h == 0)
	{
		w = 1;
		h = 1;
		stub = true;
	}

	m_result = m_device->CreateTexture(
		w,
		h,
		0,
		D3DUSAGE_AUTOGENMIPMAP,
		format,
		D3DPOOL_MANAGED,
		&data->m_tex,
		0);
	CheckError();

	D3DLOCKED_RECT rect;
	m_result = data->m_tex->LockRect(0, &rect, 0, 0);
	CheckError();

	uint8* buffer = (uint8*)rect.pBits;

	if (stub)
	{
		for (int y = 0; y < h; ++y)
		{
			for (int x = 0; x < w; ++x)
			{
				buffer[x * 4 + 0] = 255;
				buffer[x * 4 + 1] = 0;
				buffer[x * 4 + 2] = 255;
				buffer[x * 4 + 3] = 255;
			}

			buffer += rect.Pitch;
		}
	}
	else
	{
		for (int y = 0; y < h; ++y)
		{
			const Color* p1 = tex->GetData() + y * w;
			uint32* p2 = (uint32*)buffer;
			for (int x = 0; x < w; ++x)
			{
				*p2++ = p1->m_rgba[2] | (p1->m_rgba[1] << 8) | (p1->m_rgba[0] << 16) | (p1->m_rgba[3] << 24);
				++p1;
			}
			buffer += rect.Pitch;
		}
	}

	m_result = data->m_tex->UnlockRect(0);
	CheckError();

	return data;
}

void* GraphicsDeviceD3D9::UpLoadTexR(ResTexR* tex, bool texColor, bool texDepth, bool rect)
{
	FASSERT(!(texColor && texDepth));
	FASSERT(texColor || texDepth);

	DataTexR* data = new DataTexR();

	if (texColor)
	{
		// TODO: Rectangle.. ?

		D3DFORMAT format = D3DFMT_A8R8G8B8;

		if (tex->m_target == TEXR_COLOR) format = D3DFMT_A8R8G8B8;
		if (tex->m_target == TEXR_COLOR32F) format = D3DFMT_A32B32G32R32F;

		const int sx = tex->GetW();
		const int sy = tex->GetH();

		data->m_sx = sx;
		data->m_sy = sy;

		m_result = m_device->CreateTexture(
			sx,
			sy,
			1,
			D3DUSAGE_RENDERTARGET,
			format,
			D3DPOOL_DEFAULT,
			&data->m_colorRT,
			0);
		CheckError();
	}

	if (texDepth)
	{
		// TODO: Rectangle.. ?

		const int sx = tex->GetW();
		const int sy = tex->GetH();

		data->m_sx = sx;
		data->m_sy = sy;

		m_result = m_device->CreateDepthStencilSurface(
			sx,
			sy,
			D3DFMT_D24S8,
			D3DMULTISAMPLE_NONE,
			0,
			1,
			&data->m_depthRT,
			0);
		CheckError();
	}

	return data;
}

void* GraphicsDeviceD3D9::UpLoadTexCR(ResTexCR* tex)
{
	DataTexCR* data = new DataTexCR();

	// TODO: Upload renderable cube texture..

	FASSERT(false);

	return data;
}

void* GraphicsDeviceD3D9::UpLoadTexCF(ResTexCF* tex)
{
	DataTexCF* data = new DataTexCF();

	FASSERT(false);

	return data;
}

void* GraphicsDeviceD3D9::UpLoadVS(ResVS* vs)
{
	DB_LOG("uploading %s\n", vs->m_filename.c_str());

	DataVS* data = new DataVS();

	ID3DXBuffer* buffer = 0;
	ID3DXBuffer* errorBuffer = 0;

	m_result = D3DXCompileShader(
		vs->m_text.c_str(),
		vs->m_text.length(),
		0,
		0,
		"main",
		D3DXGetVertexShaderProfile(m_device),
		0,
		&buffer,
		&errorBuffer,
		&data->m_constantTable);
	if (errorBuffer)
		printf((char*)errorBuffer->GetBufferPointer());
	CheckError();

	m_result = m_device->CreateVertexShader((DWORD*)buffer->GetBufferPointer(), &data->m_shader);
	CheckError();

	if (buffer)
		buffer->Release();
	if (errorBuffer)
		errorBuffer->Release();

	return data;
}

void* GraphicsDeviceD3D9::UpLoadPS(ResPS* ps)
{
	DB_LOG("uploading %s\n", ps->m_filename.c_str());

	DataPS* data = new DataPS();

	ID3DXBuffer* buffer = 0;
	ID3DXBuffer* errorBuffer = 0;

	m_result = D3DXCompileShader(
		ps->m_text.c_str(),
		ps->m_text.length(),
		0,
		0,
		"main",
		D3DXGetPixelShaderProfile(m_device),
		0,
		&buffer,
		&errorBuffer,
		&data->m_constantTable);
	if (errorBuffer)
		printf((char*)errorBuffer->GetBufferPointer());
	CheckError();

	m_result = m_device->CreatePixelShader((DWORD*)buffer->GetBufferPointer(), &data->m_shader);
	CheckError();

	if (buffer)
		buffer->Release();
	if (errorBuffer)
		errorBuffer->Release();

	return data;
}

void GraphicsDeviceD3D9::UnLoadIB(ResIB* ib)
{
	DataIB* data = (DataIB*)m_cache[ib];

	m_cache.erase(ib);

	data->m_ib->Release();

	delete data;
}

void GraphicsDeviceD3D9::UnLoadVB(ResVB* vb)
{
	DataVB* data = (DataVB*)m_cache[vb];

	m_cache.erase(vb);

	data->m_vb->Release();

	delete data;
}

void GraphicsDeviceD3D9::UnLoadTex(ResTex* tex)
{
	DataTex* data = (DataTex*)m_cache[tex];

	m_cache.erase(tex);

	data->m_tex->Release();

	delete data;
}

void GraphicsDeviceD3D9::UnLoadTexR(ResTexR* tex)
{
	DataTexR* data = (DataTexR*)m_cache[tex];

	m_cache.erase(tex);

	if (data->m_colorRT)
		data->m_colorRT->Release();
	if (data->m_depthRT)
		data->m_depthRT->Release();

	delete data;
}

void GraphicsDeviceD3D9::UnLoadTexCR(ResTexCR* tex)
{
	DataTexCR* data = (DataTexCR*)m_cache[tex];

	m_cache.erase(tex);

	data->m_cube->Release();

	delete data;
}

void GraphicsDeviceD3D9::UnLoadTexCF(ResTexCF* tex)
{
	DataTexCF* data = (DataTexCF*)m_cache[tex];

	m_cache.erase(tex);

	// NOP.

	FASSERT(false);

	delete data;
}

void GraphicsDeviceD3D9::UnLoadVS(ResVS* vs)
{
	DataVS* data = (DataVS*)m_cache[vs];

	m_cache.erase(vs);

	data->m_constantTable->Release();
	data->m_shader->Release();

	delete data;
}

void GraphicsDeviceD3D9::UnLoadPS(ResPS* ps)
{
	DataPS* data = (DataPS*)m_cache[ps];

	m_cache.erase(ps);

	data->m_constantTable->Release();
	data->m_shader->Release();

	delete data;
}

void GraphicsDeviceD3D9::DefaultRTAcquire()
{
	m_result = m_device->GetRenderTarget(0, &m_defaultColorRT);
	CheckError();
	m_result = m_device->GetDepthStencilSurface(&m_defaultDepthRT);
	CheckError();
}

void GraphicsDeviceD3D9::DefaultRTRelease()
{
	if (m_defaultColorRT)
	{
		m_result = m_defaultColorRT->Release();
		CheckError();
		m_defaultColorRT = 0;
	}

	if (m_defaultDepthRT)
	{
		m_result = m_defaultDepthRT->Release();
		CheckError();
		m_defaultDepthRT = 0;
	}
}

#ifdef FDEBUG
void GraphicsDeviceD3D9::CheckError()
{
	if (m_result.GetError())
		FASSERT(0);
}
#endif

static D3DPRIMITIVETYPE ConvPrimitiveType(PRIMITIVE_TYPE pt)
{
	switch (pt)
	{
	case PT_TRIANGLE_LIST: return D3DPT_TRIANGLELIST;
	case PT_TRIANGLE_FAN: return D3DPT_TRIANGLEFAN;
	case PT_TRIANGLE_STRIP: return D3DPT_TRIANGLESTRIP;
	case PT_LINE_LIST: return D3DPT_LINELIST;
	case PT_LINE_STRIP: return D3DPT_LINESTRIP;
	default: FASSERT(0); return (D3DPRIMITIVETYPE)0;
	}
}

// TODO: Move to mesh? Static method somewhere?
static int CalcPrimitiveCount(PRIMITIVE_TYPE pt, int vertexCnt, int indexCnt)
{
	int primitiveCount;
	int vertices;

    if (indexCnt > 0)
        vertices = indexCnt;
    else
        vertices = vertexCnt;

	switch (pt)
	{
	case PT_TRIANGLE_LIST:
        primitiveCount = vertices / 3;
		break;
	case PT_TRIANGLE_STRIP:
        primitiveCount = vertices - 2;
		break;
	case PT_TRIANGLE_FAN:
        primitiveCount = vertices - 2;
		break;
		/*
	case PT_POINT_LIST:
        primitiveCount = vertices;
		break;*/
	case PT_LINE_LIST:
        primitiveCount = vertices / 2;
		break;
	case PT_LINE_STRIP:
        primitiveCount = vertices - 1;
		break;
	default:
		primitiveCount = 0;
		FASSERT(0);
		break;
	}

	return primitiveCount;
}

static int ConvFunc(int func)
{
	switch (func)
	{
	case CMP_ALWAYS: return D3DCMP_ALWAYS;
	case CMP_EQ:     return D3DCMP_EQUAL;
	case CMP_L:      return D3DCMP_LESS;
	case CMP_LE:     return D3DCMP_LESSEQUAL;
	case CMP_G:      return D3DCMP_GREATER;
	case CMP_GE:     return D3DCMP_GREATEREQUAL;
	default: FASSERT(0); return 0;
	}
}

static int ConvCull(int cull)
{
	switch (cull)
	{
	case CULL_NONE: return D3DCULL_NONE;
		// FIXME, OpenGL culling borked.. Must Replace All :(.
	//case CULL_CW:   return D3DCULL_CW;
	//case CULL_CCW:  return D3DCULL_CCW;
	case CULL_CW:   return D3DCULL_CCW;
	case CULL_CCW:  return D3DCULL_CW;
	default: FASSERT(0); return 0;
	}
}

static int ConvBlendOp(int op)
{
	switch (op)
	{
	case BLEND_ONE:           return D3DBLEND_ONE;
	case BLEND_ZERO:          return D3DBLEND_ZERO;
	case BLEND_SRC:           return D3DBLEND_SRCALPHA;
	case BLEND_DST:           return D3DBLEND_DESTALPHA;
	case BLEND_SRC_COLOR:     return D3DBLEND_SRCCOLOR;
	case BLEND_DST_COLOR:     return D3DBLEND_DESTCOLOR;
	case BLEND_INV_SRC:       return D3DBLEND_INVSRCALPHA;
	case BLEND_INV_DST:       return D3DBLEND_INVDESTALPHA;
	case BLEND_INV_SRC_COLOR: return D3DBLEND_INVSRCCOLOR;
	case BLEND_INV_DST_COLOR: return D3DBLEND_INVDESTCOLOR;
	default: FASSERT(0); return 0;
	}
}

static uint32_t ConvertFVF(int fvf, int texCnt)
{
	uint32_t r = 0;

	if (fvf & FVF_XYZ)
		r |= D3DFVF_XYZ;
	if (fvf & FVF_NORMAL)
		r |= D3DFVF_NORMAL;

	if (fvf & FVF_COLOR)
		r |= D3DFVF_DIFFUSE;

	if (texCnt == 1) r |= D3DFVF_TEX1;
	if (texCnt == 2) r |= D3DFVF_TEX2;
	if (texCnt == 3) r |= D3DFVF_TEX3;
	if (texCnt == 4) r |= D3DFVF_TEX4;
	if (texCnt == 5) r |= D3DFVF_TEX5;
	if (texCnt == 6) r |= D3DFVF_TEX6;
	if (texCnt == 7) r |= D3DFVF_TEX7;
	if (texCnt == 8) r |= D3DFVF_TEX8;

	for (int i = 0; i < texCnt; ++i)
		r |= D3DFVF_TEXCOORDSIZE4(i);

	return r;
}


D3DXHANDLE GraphicsDeviceD3D9::DataVS::GetParameter(const std::string& name)
{
	std::map<std::string, D3DXHANDLE>::iterator i = m_parameters.find(name);

	if (i != m_parameters.end())
		return i->second;
	else
	{
		D3DXHANDLE p = m_constantTable->GetConstantByName(0, name.c_str());
		m_parameters[name] = p;
		return p;
	}
}

D3DXHANDLE GraphicsDeviceD3D9::DataPS::GetParameter(const std::string& name)
{
	std::map<std::string, D3DXHANDLE>::iterator i = m_parameters.find(name);

	if (i != m_parameters.end())
		return i->second;
	else
	{
		D3DXHANDLE p = m_constantTable->GetConstantByName(0, name.c_str());
		m_parameters[name] = p;
		return p;
	}
}
