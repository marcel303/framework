#pragma once

#include "DX11.h"

class BlendState
{
public:
	BlendState(ID3D11Device * pDevice, const D3D11_BLEND_DESC & desc)
		: m_desc(desc)
	{
		Create(pDevice);
	}

	~BlendState()
	{
		SafeRelease(m_pState);
	}

	ID3D11BlendState * GetBlendState()
	{
		return m_pState;
	}

private:
	void Create(ID3D11Device * pDevice)
	{
		V(pDevice->CreateBlendState(&m_desc, &m_pState));
	}

	D3D11_BLEND_DESC m_desc;
	ID3D11BlendState * m_pState;
};
