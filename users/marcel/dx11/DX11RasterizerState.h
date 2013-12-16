#pragma once

#include "DX11.h"

class RasterizerState
{
public:
	RasterizerState(
		ID3D11Device * pDevice,
		D3D11_FILL_MODE fillMode,
		D3D11_CULL_MODE cullMode,
		bool scissorEnable,
		bool multisampleEnable)
		: m_pState(0)
	{
		ZeroMemory(&m_desc, sizeof(m_desc));
		m_desc.FillMode = fillMode;
		m_desc.CullMode = cullMode;
		m_desc.FrontCounterClockwise = FALSE;
		m_desc.DepthBias = 0;
		m_desc.DepthBiasClamp = 0.0f;
		m_desc.SlopeScaledDepthBias = 0.0f;
		m_desc.DepthClipEnable = FALSE;
		m_desc.ScissorEnable = scissorEnable;
		m_desc.MultisampleEnable = multisampleEnable;
		m_desc.AntialiasedLineEnable = FALSE;

		Create(pDevice);
	}

	RasterizerState(ID3D11Device * pDevice, const D3D11_RASTERIZER_DESC & desc)
		: m_pState(0)
		, m_desc(desc)
	{
		Create(pDevice);
	}

	~RasterizerState()
	{
		SafeRelease(m_pState);
	}

	ID3D11RasterizerState * GetRasterizerState()
	{
		return m_pState;
	}

private:
	void Create(ID3D11Device * pDevice)
	{
		V(pDevice->CreateRasterizerState(&m_desc, &m_pState));
	}

	ID3D11RasterizerState * m_pState;
	D3D11_RASTERIZER_DESC m_desc;
};
