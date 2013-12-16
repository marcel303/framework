#pragma once

#include "DX11.h"

class RenderDevice
{
public:
	RenderDevice()
		: m_pSwapChain(0)
		, m_pDevice(0)
		, m_pDeviceCtx(0)
		, m_pRasterizerState(0)
	{
	}

	~RenderDevice()
	{
		Shutdown();
	}

	void Initialize(uint32_t sx, uint32_t sy, HWND hWnd, bool fullscreen)
	{
		// create D3D device and swap chain

	#if ENABLE_TESSALATION
		D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_REFERENCE;
		D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_0;
	#else
		D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_HARDWARE;
		//D3D_DRIVER_TYPE driverType = D3D_DRIVER_TYPE_REFERENCE;
		D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_10_1;
	#endif
		D3D_FEATURE_LEVEL featureLevelGotten;

		DXGI_SWAP_CHAIN_DESC swapChainDesc;
		ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));
		swapChainDesc.BufferCount = 1;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferDesc.Height = sy;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 60;
		swapChainDesc.BufferDesc.Width = sx;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.Flags = 0;
		swapChainDesc.OutputWindow = hWnd;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.Windowed = fullscreen == false;

		V(D3D11CreateDeviceAndSwapChain(
			0, driverType, 0,
#if defined(_DEBUG)|| 1
			D3D11_CREATE_DEVICE_DEBUG,
#else
			0,
#endif
			&featureLevel, 1, D3D11_SDK_VERSION,
			&swapChainDesc, &m_pSwapChain,
			&m_pDevice, &featureLevelGotten, &m_pDeviceCtx));

		// check feature support

		D3D11_FEATURE_DATA_D3D10_X_HARDWARE_OPTIONS hardwareOptions;

		V(m_pDevice->CheckFeatureSupport(D3D11_FEATURE_D3D10_X_HARDWARE_OPTIONS, &hardwareOptions, sizeof(hardwareOptions)));

		const bool computeShaderSupported = featureLevelGotten >= D3D_FEATURE_LEVEL_11_0 || hardwareOptions.ComputeShaders_Plus_RawAndStructuredBuffers_Via_Shader_4_x;
		printf("compute shader supported: %d\n", computeShaderSupported ? 1 : 0);

		// create back buffer render target

		ID3D11Texture2D * pBackBuffer = 0;
		V(m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&pBackBuffer));

		ID3D11RenderTargetView * pRenderTargetView = 0;
		V(m_pDevice->CreateRenderTargetView(pBackBuffer, 0, &m_pBackBufferView));
		pBackBuffer->Release();

		D3D11_RASTERIZER_DESC rasterizerDesc;
		ZeroMemory(&rasterizerDesc, sizeof(rasterizerDesc));
		rasterizerDesc.FillMode = D3D11_FILL_SOLID;
		rasterizerDesc.CullMode = D3D11_CULL_NONE;
		//rasterizerDesc.CullMode = D3D11_CULL_BACK;
		rasterizerDesc.FrontCounterClockwise = FALSE;
		rasterizerDesc.DepthBias = 0;
		rasterizerDesc.DepthBiasClamp = 0.0f;
		rasterizerDesc.SlopeScaledDepthBias = 0.0f;
		rasterizerDesc.DepthClipEnable = FALSE;
		rasterizerDesc.ScissorEnable = FALSE;
		rasterizerDesc.MultisampleEnable = FALSE;
		rasterizerDesc.AntialiasedLineEnable = FALSE;
		V(m_pDevice->CreateRasterizerState(&rasterizerDesc, &m_pRasterizerState));

		m_pDeviceCtx->RSSetState(m_pRasterizerState);
	}

	void Shutdown()
	{
		SafeRelease(m_pRasterizerState);
		SafeRelease(m_pBackBufferView);

		if (m_pDeviceCtx)
		{
			m_pDeviceCtx->ClearState();
			m_pDeviceCtx->Flush();
		}

		SafeRelease(m_pDeviceCtx);

		SafeRelease(m_pDevice);

		if (m_pSwapChain)
		{
			V(m_pSwapChain->SetFullscreenState(false, 0));
		}

		SafeRelease(m_pSwapChain);
	}

	IDXGISwapChain * GetSwapChain() { return m_pSwapChain; }
	ID3D11Device * GetDevice() { return m_pDevice; }
	ID3D11DeviceContext * GetDeviceCtx() { return m_pDeviceCtx; }
	ID3D11RenderTargetView * GetBackBufferView() { return m_pBackBufferView; }

private:
	IDXGISwapChain * m_pSwapChain;
	ID3D11Device * m_pDevice;
	ID3D11DeviceContext * m_pDeviceCtx;
	ID3D11RenderTargetView * m_pBackBufferView;
	ID3D11RasterizerState * m_pRasterizerState;
};
