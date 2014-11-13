#include <d3dx11.h>
#include "Debug.h"
#include "DisplaySDL.h"
#include "DisplayWindows.h"
#include "GraphicsDeviceD3D11.h"

static D3D11_PRIMITIVE_TOPOLOGY ConvPrimitiveType(PRIMITIVE_TYPE pt);
static int CalcPrimitiveCount(PRIMITIVE_TYPE pt, int vertexCnt, int indexCnt);
static D3D11_COMPARISON_FUNC ConvFunc(int func);
static D3D11_CULL_MODE ConvCull(int cull);
static D3D11_BLEND ConvBlendOp(int op);
static D3D11_STENCIL_OP ConvStencilOp(int op);
static uint32_t ConvertFVF(int fvf, int texCnt);

#ifdef DEBUG
static inline void D3DVERIFY(HRESULT result)
{
	D3DResult r(result);
	Assert(!r.GetError());
}
#else
#define D3DVERIFY(x) do { x; } while (false)
#endif

class Material
{
private:
	int m_rs[RS_ENUM_SIZE];

public:
	Material()
	{
		ZeroMemory(m_rs, sizeof(m_rs));
	}

	void RS(int state, int value)
	{
		m_rs[state] = value;
	}

	ID3D11BlendState* GetBlendState(ID3D11Device* device)
	{
		// create a new blend state
		D3D11_BLEND_DESC blendDesc;
		ZeroMemory(&blendDesc, sizeof(blendDesc));

		blendDesc.AlphaToCoverageEnable = false;
		blendDesc.IndependentBlendEnable = false;
		blendDesc.RenderTarget[0].BlendEnable = m_rs[RS_BLEND] != 0;
		if (blendDesc.RenderTarget[0].BlendEnable)
		{
			blendDesc.RenderTarget[0].BlendOp = blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
			blendDesc.RenderTarget[0].SrcBlend = blendDesc.RenderTarget[0].SrcBlendAlpha = ConvBlendOp(m_rs[RS_BLEND_SRC]);
			blendDesc.RenderTarget[0].DestBlend = blendDesc.RenderTarget[0].DestBlendAlpha = ConvBlendOp(m_rs[RS_BLEND_DST]);
		}
		blendDesc.RenderTarget[0].RenderTargetWriteMask = m_rs[RS_WRITE_COLOR] ? 0xFF : 0x00;

		ID3D11BlendState* result = 0;
		D3DVERIFY(device->CreateBlendState(&blendDesc, &result));

		return result;
	}

	/*
	RS_CULL,
	RS_ALPHATEST,
	RS_ALPHATEST_FUNC,
	RS_ALPHATEST_REF,
	RS_SCISSORTEST, // FIXME, TODO, implement!
*/

	ID3D11DepthStencilState* GetDepthStencilState(ID3D11Device* device)
	{
		D3D11_DEPTH_STENCIL_DESC desc;
		ZeroMemory(&desc, sizeof(desc));

		desc.DepthEnable = m_rs[RS_DEPTHTEST] != 0;
		desc.DepthWriteMask = m_rs[RS_WRITE_DEPTH] ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
		desc.DepthFunc = ConvFunc(m_rs[RS_DEPTHTEST_FUNC]);
		desc.StencilEnable = m_rs[RS_STENCILTEST] != 0;
		desc.StencilReadMask = m_rs[RS_STENCILTEST_REF];
		desc.StencilWriteMask = m_rs[RS_STENCILTEST_MASK];
		desc.FrontFace.StencilDepthFailOp = ConvStencilOp(m_rs[RS_STENCILTEST_ONZFAIL]);
		desc.FrontFace.StencilFailOp = ConvStencilOp(m_rs[RS_STENCILTEST_ONFAIL]);
		desc.FrontFace.StencilFunc = ConvFunc(m_rs[RS_STENCILTEST_FUNC]);
		desc.FrontFace.StencilPassOp = ConvStencilOp(m_rs[RS_STENCILTEST_ONPASS]);
		desc.BackFace = desc.FrontFace;

		ID3D11DepthStencilState* result = 0;
		D3DVERIFY(device->CreateDepthStencilState(&desc, &result));

		return result;
	}

	ID3D11RasterizerState* GetRasterizerState(ID3D11Device* device)
	{
		D3D11_RASTERIZER_DESC desc;
		ZeroMemory(&desc, sizeof(desc));

		desc.FillMode = D3D11_FILL_SOLID;
		desc.CullMode = ConvCull(m_rs[RS_CULL]);
		desc.FrontCounterClockwise = false;
		desc.DepthBias = 0;
		desc.DepthBiasClamp = 0.0f;
		desc.SlopeScaledDepthBias = 0.0f;
		desc.DepthClipEnable = true;
		desc.ScissorEnable = m_rs[RS_SCISSORTEST] != 0;
		desc.MultisampleEnable = false;
		desc.AntialiasedLineEnable = false;

		ID3D11RasterizerState* result = 0;
		D3DVERIFY(device->CreateRasterizerState(&desc, &result));

		return result;
	}
};

static Material s_material;

GraphicsDeviceD3D11::GraphicsDeviceD3D11()
	: GraphicsDevice()
{
	INITINIT;

	m_display = 0;
	m_device = 0;
	m_deviceCtx = 0;
	m_swapChain = 0;
	m_renderTargetView = 0;
	m_depthStencilView = 0;
	memset(m_rts, 0, sizeof(m_rts));
	m_rtd = 0;
	m_numRenderTargets = 0;
	m_renderTargetSx = 0;
	m_renderTargetSy = 0;

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

GraphicsDeviceD3D11::~GraphicsDeviceD3D11()
{
	INITCHECK(false);
}

void GraphicsDeviceD3D11::Initialize(const GraphicsOptions& options)
{
	INITCHECK(false);

	m_display = new DisplaySDL(0, 0, options.Sx, options.Sy, options.Fullscreen, false);
	//m_display = new DisplaySDL(0, 0, 640, 480, false, false);
	//m_display = new DisplayWindows(0, 0, width, height, fullscreen);

	HWND hWnd = (HWND)m_display->Get("hWnd");

	if (!hWnd)
		Assert(0);

	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 1;
	sd.BufferDesc.Width = options.Sx;
	sd.BufferDesc.Height = options.Sy;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	sd.Flags = 0;//DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.Windowed = !options.Fullscreen;

	D3D_FEATURE_LEVEL featureLevelRequested = D3D_FEATURE_LEVEL_11_0;
	D3D_FEATURE_LEVEL featureLevelSupported;

	UINT flags = 0;
	flags |= D3D11_CREATE_DEVICE_SINGLETHREADED;
#if DEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	m_result = D3D11CreateDeviceAndSwapChain(
		0,
		D3D_DRIVER_TYPE_HARDWARE,
		0,
		flags,
		&featureLevelRequested,
		1,
		D3D11_SDK_VERSION,
		&sd,
		&m_swapChain,
		&m_device,
		&featureLevelSupported,
		&m_deviceCtx);
	CheckError();

	if (!m_swapChain || !m_device || !m_deviceCtx)
		Assert(0);
	if (featureLevelSupported != featureLevelRequested)
		Assert(0);

	if (options.CreateRenderTarget)
	{
		ID3D11Texture2D* backBuffer = 0;

		m_result = m_swapChain->GetBuffer(
			0,
			__uuidof(ID3D11Texture2D),
			(void**)&backBuffer);
		CheckError();

		m_result = m_device->CreateRenderTargetView(
			backBuffer,
			0,
			&m_renderTargetView);
		CheckError();

		m_deviceCtx->OMSetRenderTargets(1, &m_renderTargetView, 0);

		// Setup the viewport
		D3D11_VIEWPORT vp;
		vp.Width = static_cast<float>(options.Sx);
		vp.Height = static_cast<float>(options.Sy);
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		m_deviceCtx->RSSetViewports(1, &vp);
	}

	DefaultRTAcquire();

	// Default render states.
	ResetStates();

	INITSET(true);
}

void GraphicsDeviceD3D11::Shutdown()
{
	INITCHECK(true);

	Assert(m_rt == 0);
	Assert(m_ib == 0);
	Assert(m_vb == 0);
	for (int i = 0; i < MAX_TEX; ++i)
		Assert(m_tex[i] == 0);
	Assert(m_vs == 0);
	Assert(m_ps == 0);

	// TODO: Reset stuff.
	/*
	for (int i = 0; i < MAX_TEX; ++i)
		SetTex(i, 0);
	SetIB(0);
	SetVB(0);
	SetVS(0);
	SetPS(0);*/

	Assert(m_cache.size() == 0);

	m_result = m_deviceCtx->Release();
	CheckError();
	m_deviceCtx = 0;

	m_result = m_device->Release();
	CheckError();
	m_device = 0;

	m_result = m_swapChain->Release();
	CheckError();
	m_swapChain = 0;

	SAFE_FREE(m_display);

	INITSET(false);
}

Display* GraphicsDeviceD3D11::GetDisplay()
{
	return m_display;
}

void GraphicsDeviceD3D11::SceneBegin()
{
	m_display->Update();
}

void GraphicsDeviceD3D11::SceneEnd()
{
}

void GraphicsDeviceD3D11::Clear(int buffers, float r, float g, float b, float a, float z)
{
	if (buffers & BUFFER_COLOR)
	{
		float color[4] = { r, g, b, a };
		m_deviceCtx->ClearRenderTargetView(m_renderTargetView, color);
	}

	if (buffers & (BUFFER_DEPTH | BUFFER_STENCIL))
	{
		UINT flags = 0;
		if (buffers & BUFFER_DEPTH)
			flags |= D3D11_CLEAR_DEPTH;
		if (buffers & BUFFER_STENCIL)
			flags |= D3D11_CLEAR_STENCIL;

		m_deviceCtx->ClearDepthStencilView(m_depthStencilView, flags, z, 0x00);
	}
}

void GraphicsDeviceD3D11::Draw(PRIMITIVE_TYPE type)
{
	//todo: apply material

	m_deviceCtx->IASetPrimitiveTopology(ConvPrimitiveType(type));

	if (m_ib)
		m_deviceCtx->DrawIndexed(m_ib->GetIndexCnt(), 0, 0);
	else
		m_deviceCtx->Draw(m_vb->GetVertexCnt(), 0);
}

void GraphicsDeviceD3D11::Present()
{
	m_result = m_swapChain->Present(0, 0);
	CheckError();
}

void GraphicsDeviceD3D11::Resolve(ResTexR* rt)
{
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
}

void GraphicsDeviceD3D11::Copy(ResBaseTex* out_tex)
{
}

void GraphicsDeviceD3D11::SetScissorRect(int x1, int y1, int x2, int y2)
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

	D3D11_RECT rect;
	rect.left = x1;
	rect.top = y1;
	rect.right = x2 + 1;
	rect.bottom = y2 + 1;

	m_deviceCtx->RSSetScissorRects(1, &rect);
}

void GraphicsDeviceD3D11::RS(int state, int value)
{
	s_material.RS(state, value);
}

void GraphicsDeviceD3D11::SS(int sampler, int state, int value)
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
		Assert(0);
		break;
	}

	CheckError();
}

Mat4x4 GraphicsDeviceD3D11::GetMatrix(MATRIX_NAME matID)
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

void GraphicsDeviceD3D11::SetMatrix(MATRIX_NAME matID, const Mat4x4& mat)
{
	switch (matID)
	{
	case MAT_WRLD:
		m_matW = mat;
		//m_result = m_device->SetTransform(D3DTS_WORLD, (D3DMATRIX*)&m_matW);
		break;
	case MAT_VIEW:
		m_matV = mat;
		//m_result = m_device->SetTransform(D3DTS_VIEW, (D3DMATRIX*)&m_matV);
		break;
	case MAT_PROJ:
		m_matP = mat;
		//m_result = m_device->SetTransform(D3DTS_PROJECTION, (D3DMATRIX*)&m_matP);
		break;
	}

	//CheckError();
}

int GraphicsDeviceD3D11::GetRTW()
{
	return m_renderTargetSx;
}

int GraphicsDeviceD3D11::GetRTH()
{
	return m_renderTargetSy;
}

void GraphicsDeviceD3D11::SetRT(ResTexR* rt)
{
	Assert(false);
}

void GraphicsDeviceD3D11::SetRTM(ResTexR* rt1, ResTexR* rt2, ResTexR* rt3, ResTexR* rt4, int numRenderTargets, ResTexD* rtd)
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
					Assert(data->m_sx == sx);
					Assert(data->m_sy == sy);
				}

				m_result = surface->Release();
				CheckError();
			}
			else
			{
				Assert(false);
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
				Assert(data->m_sx == sx);
				Assert(data->m_sy == sy);
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
		Assert(sx >= 0 && sy >= 0);

		D3DVIEWPORT9 viewport;
		viewport.X      = 0;
		viewport.Y      = 0;
		viewport.Width  = sx;
		viewport.Height = sy;
		viewport.MinZ   = 0.0f;
		viewport.MaxZ   = 1.0f;
		m_result = m_device->SetViewport(&viewport);
		CheckError();

		m_renderTargetSx = sx;
		m_renderTargetSy = sy;
	}
}

void GraphicsDeviceD3D11::SetIB(ResIB* ib)
{
	if (ib == 0)
	{
		m_deviceCtx->IASetIndexBuffer(0, DXGI_FORMAT_R16_UINT);
	}
	else
	{
		Validate(ib);

		DataIB* data = (DataIB*)m_cache[ib];

		m_deviceCtx->IASetIndexBuffer(data->m_ib, DXGI_FORMAT_R16_UINT, 0);
	}

	m_ib = ib;
}

void GraphicsDeviceD3D11::SetVB(ResVB* vb)
{
	if (vb == 0)
	{
		m_deviceCtx->IASetVertexBuffers(0, 0, 0, 0, 0);
	}
	else
	{
		Validate(vb);

		DataVB* data = (DataVB*)m_cache[vb];

		m_deviceCtx->IASetVertexBuffers(0, 1, &data->m_vb, data->m_stride, 0);
		//todo: set layout
		//m_result = m_device->SetFVF(data->m_fvf);
	}

	m_vb = vb;
}

void GraphicsDeviceD3D11::SetTex(int sampler, ResBaseTex* tex)
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
				Assert(0);
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
			Assert(0);
			break;
		}
	}

	m_tex[sampler] = tex;
}

void GraphicsDeviceD3D11::SetVS(ResVS* vs)
{
	Assert(cgD3D9GetDevice() == m_device);

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
				case SHPARAM_TEX:    Assert(0);                                                                       break; // Not supporter ATM..
				}
			}

			CheckError();
		}
	}

	m_vs = vs;
}

void GraphicsDeviceD3D11::SetPS(ResPS* ps)
{
	Assert(cgD3D9GetDevice() == m_device);

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
			{
				//printf("Warning: Cg parameter not found: %s.\n", name.c_str());
				continue;
			}

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

void GraphicsDeviceD3D11::OnResInvalidate(Res* res)
{
	Assert(res);

	UnLoad(res);
}

void GraphicsDeviceD3D11::OnResDestroy(Res* res)
{
	Assert(res);

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

int GraphicsDeviceD3D11::Validate(Res* res)
{
	Assert(res);

	if (m_cache.count(res) == 0)
	{
		UpLoad(res);
		return 1;
	}

	return 0;
}

void GraphicsDeviceD3D11::UpLoad(Res* res)
{
	INITCHECK(true);

	Assert(res);
	Assert(m_cache.count(res) == 0);

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
		Assert(0);
		break;
	}

	m_cache[res] = data;

	res->AddUser(this);

	//printf("UpLoad: Cache size: %d.\n", (int)m_cache.size());
}

void GraphicsDeviceD3D11::UnLoad(Res* res)
{
	INITCHECK(true);

	Assert(res);
	Assert(m_cache.count(res) != 0);

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
		Assert(0);
		break;
	}

	res->RemoveUser(this);

	m_cache.erase(res);

	//printf("UnLoad: Cache size: %d.\n", (int)m_cache.size());
}

void* GraphicsDeviceD3D11::UpLoadIB(ResIB* ib)
{
	DataIB* data = new DataIB();

	int size = sizeof(uint16) * ib->GetIndexCnt();

    D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.ByteWidth = size;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA init;
	ZeroMemory(&init, sizeof(init));
	init.pSysMem = ib->index.GetData();

	m_result = m_device->CreateBuffer(&desc, &init, &data->m_ib);
	CheckError();

	return data;
}

void* GraphicsDeviceD3D11::UpLoadVB(ResVB* vb)
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

	UINT size = stride * vb->m_vCnt;

	char* buffer = new char[size];

	for (int i = 0; i < vb->m_vCnt; ++i)
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
		for (int j = 0; j < vb->m_texCnt; ++j)
		{
			memcpy(buffer, &vb->tex[j][i], sizeof(float) * 4);
			buffer += sizeof(float) * 4;
		}
	}

    D3D11_BUFFER_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.ByteWidth = size;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    desc.CPUAccessFlags = 0;
    desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA init;
	ZeroMemory(&init, sizeof(init));
	init.pSysMem = buffer;

	m_result = m_device->CreateBuffer(&desc, &init, &data->m_vb);
	CheckError();

	delete[] buffer;
	buffer = 0;

	data->m_fvf = d3dFVF;
	data->m_stride = stride;

	return data;
}

void* GraphicsDeviceD3D11::UpLoadTex(ResTex* tex)
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

void* GraphicsDeviceD3D11::UpLoadTexR(ResTexR* tex, bool texColor, bool texDepth, bool rect)
{
	Assert(!(texColor && texDepth));
	Assert(texColor || texDepth);

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

void* GraphicsDeviceD3D11::UpLoadTexCR(ResTexCR* tex)
{
	DataTexCR* data = new DataTexCR();

	// TODO: Upload renderable cube texture..

	Assert(false);

	return data;
}

void* GraphicsDeviceD3D11::UpLoadTexCF(ResTexCF* tex)
{
	DataTexCF* data = new DataTexCF();

	Assert(false);

	return data;
}

void* GraphicsDeviceD3D11::UpLoadVS(ResVS* vs)
{
	DB_TRACE("uploading %s", vs->m_filename.c_str());

	DataVS* data = new DataVS();

	ID3DBlob* buffer = 0;
	ID3DBlob* errorBuffer = 0;

	m_result = D3DX11CompileFromMemory(
		vs->m_text.c_str(),
		vs->m_text.length(),
		vs->m_filename.c_str(),
		0,
		0,
		"main",
		"vs_5_0",
		D3D11_SHADER_ENABLE_STRICTNESS | D3D10_SHADER_DEBUG,
		0,
		0,
		&buffer,
		&errorBuffer,
		0);
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

void* GraphicsDeviceD3D11::UpLoadPS(ResPS* ps)
{
	DB_TRACE("uploading %s", ps->m_filename.c_str());

	DataPS* data = new DataPS();

	ID3DXBuffer* buffer = 0;
	ID3DXBuffer* errorBuffer = 0;

	m_result = D3DXCompileShader(
		ps->m_text.c_str(),
		ps->m_text.length(),
		0,
		0,
		"ps_5_0",
		D3DXGetPixelShaderProfile(m_device),
		0,
		&buffer,
		&errorBuffer,
		&data->m_constantTable);
	if (errorBuffer)
		printf((char*)errorBuffer->GetBufferPointer());
	CheckError();

	m_result = D3DReflect(buffer->GetBufferPointer(), buffer->GetBufferSize(), IID_ID3D11ShaderReflection, (void**)&data->reflection);
	CheckError();

	m_result = m_device->CreatePixelShader(buffer->GetBufferPointer(), buffer->GetBufferSize(), &data->m_shader);
	CheckError();

	if (buffer)
		buffer->Release();
	if (errorBuffer)
		errorBuffer->Release();

	return data;
}

void GraphicsDeviceD3D11::UnLoadIB(ResIB* ib)
{
	DataIB* data = (DataIB*)m_cache[ib];

	m_cache.erase(ib);

	data->m_ib->Release();

	delete data;
}

void GraphicsDeviceD3D11::UnLoadVB(ResVB* vb)
{
	DataVB* data = (DataVB*)m_cache[vb];

	m_cache.erase(vb);

	data->m_vb->Release();

	delete data;
}

void GraphicsDeviceD3D11::UnLoadTex(ResTex* tex)
{
	DataTex* data = (DataTex*)m_cache[tex];

	m_cache.erase(tex);

	data->m_tex->Release();

	delete data;
}

void GraphicsDeviceD3D11::UnLoadTexR(ResTexR* tex)
{
	DataTexR* data = (DataTexR*)m_cache[tex];

	m_cache.erase(tex);

	if (data->m_colorRT)
		data->m_colorRT->Release();
	if (data->m_depthRT)
		data->m_depthRT->Release();

	delete data;
}

void GraphicsDeviceD3D11::UnLoadTexCR(ResTexCR* tex)
{
	DataTexCR* data = (DataTexCR*)m_cache[tex];

	m_cache.erase(tex);

	data->m_cube->Release();

	delete data;
}

void GraphicsDeviceD3D11::UnLoadTexCF(ResTexCF* tex)
{
	DataTexCF* data = (DataTexCF*)m_cache[tex];

	m_cache.erase(tex);

	// NOP.

	Assert(false);

	delete data;
}

void GraphicsDeviceD3D11::UnLoadVS(ResVS* vs)
{
	DataVS* data = (DataVS*)m_cache[vs];

	m_cache.erase(vs);

	data->m_constantTable->Release();
	data->m_shader->Release();

	delete data;
}

void GraphicsDeviceD3D11::UnLoadPS(ResPS* ps)
{
	DataPS* data = (DataPS*)m_cache[ps];

	m_cache.erase(ps);

	data->m_constantTable->Release();
	data->m_shader->Release();

	delete data;
}

#ifdef DEBUG
void GraphicsDeviceD3D11::CheckError()
{
	// TODO.
	if (m_result.GetError())
		Assert(0);
}
#endif

static D3D11_PRIMITIVE_TOPOLOGY ConvPrimitiveType(PRIMITIVE_TYPE pt)
{
	switch (pt)
	{
	case PT_TRIANGLE_LIST: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	//case PT_TRIANGLE_FAN: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	case PT_TRIANGLE_STRIP: return D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	case PT_LINE_LIST: return D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
	case PT_LINE_STRIP: return D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP;
	default: Assert(0); return (D3D11_PRIMITIVE_TOPOLOGY)0;
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
		Assert(0);
		break;
	}

	return primitiveCount;
}

static D3D11_COMPARISON_FUNC ConvFunc(int func)
{
	switch (func)
	{
	case CMP_ALWAYS: return D3D11_COMPARISON_ALWAYS;
	case CMP_EQ:     return D3D11_COMPARISON_EQUAL;
	case CMP_L:      return D3D11_COMPARISON_LESS;
	case CMP_LE:     return D3D11_COMPARISON_LESS_EQUAL;
	case CMP_G:      return D3D11_COMPARISON_GREATER;
	case CMP_GE:     return D3D11_COMPARISON_GREATER;
	default: Assert(0); return D3D11_COMPARISON_ALWAYS;
	}
}

static D3D11_CULL_MODE ConvCull(int cull)
{
	switch (cull)
	{
	case CULL_NONE: return D3D11_CULL_NONE;
	case CULL_CW:   return D3D11_CULL_FRONT;
	case CULL_CCW:  return D3D11_CULL_BACK;
	default: Assert(0); return D3D11_CULL_NONE;
	}
}

static D3D11_BLEND ConvBlendOp(int op)
{
	switch (op)
	{
	case BLEND_ONE:           return D3D11_BLEND_ONE;
	case BLEND_ZERO:          return D3D11_BLEND_ZERO;
	case BLEND_SRC:           return D3D11_BLEND_SRC_ALPHA;
	case BLEND_DST:           return D3D11_BLEND_DEST_ALPHA;
	case BLEND_SRC_COLOR:     return D3D11_BLEND_SRC_COLOR;
	case BLEND_DST_COLOR:     return D3D11_BLEND_DEST_COLOR;
	case BLEND_INV_SRC:       return D3D11_BLEND_INV_SRC_ALPHA;
	case BLEND_INV_DST:       return D3D11_BLEND_INV_DEST_ALPHA;
	case BLEND_INV_SRC_COLOR: return D3D11_BLEND_INV_SRC_COLOR;
	case BLEND_INV_DST_COLOR: return D3D11_BLEND_INV_DEST_COLOR;
	default: Assert(0); return D3D11_BLEND_ONE;
	}
}

static D3D11_STENCIL_OP ConvStencilOp(int op)
{
	switch (op)
	{
	case INC:  return D3D11_STENCIL_OP_INCR;
	case DEC:  return D3D11_STENCIL_OP_DECR;
	case ZERO: return D3D11_STENCIL_OP_ZERO;
	default: Assert(0); return D3D11_STENCIL_OP_ZERO;
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


D3DXHANDLE GraphicsDeviceD3D11::DataVS::GetParameter(const std::string& name)
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

D3DXHANDLE GraphicsDeviceD3D11::DataPS::GetParameter(const std::string& name)
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
