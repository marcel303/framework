#pragma once

#include "DX11.h"

class DepthStencilState
{
public:
	DepthStencilState(
		ID3D11Device * pDevice,
		bool zEnabled,
		D3D11_COMPARISON_FUNC zTest,
		bool zWrite)
		: m_pState(0)
	{
		ZeroMemory(&m_desc, sizeof(m_desc));
		m_desc.DepthEnable = zEnabled;
		m_desc.DepthWriteMask = zWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
		m_desc.DepthFunc = zTest;

		Create(pDevice);
	}

	DepthStencilState(ID3D11Device * pDevice, const D3D11_DEPTH_STENCIL_DESC & desc)
		: m_desc(desc)
		, m_pState(0)
	{
		Create(pDevice);
	}

	~DepthStencilState()
	{
		SafeRelease(m_pState);
	}

	ID3D11DepthStencilState * GetDepthStencilState() { return m_pState; }

private:
	void Create(ID3D11Device * pDevice)
	{
		V(pDevice->CreateDepthStencilState(&m_desc, &m_pState));
	}

	D3D11_DEPTH_STENCIL_DESC m_desc;
	ID3D11DepthStencilState * m_pState;
};
