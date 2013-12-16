#pragma once

#include "DX11.h"

class SamplerState
{
public:
	SamplerState(ID3D11Device * pDevice, D3D11_FILTER filter, bool clamp)
		: m_pSS(0)
	{
		D3D11_SAMPLER_DESC desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Filter = filter;
		desc.AddressU = desc.AddressV = desc.AddressW = clamp ? D3D11_TEXTURE_ADDRESS_CLAMP : D3D11_TEXTURE_ADDRESS_WRAP;
		//desc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
		desc.ComparisonFunc = D3D11_COMPARISON_NEVER;
		desc.MinLOD = -FLT_MAX;
		desc.MaxLOD = +FLT_MAX;
		desc.MaxAnisotropy = 16;
		
		V(pDevice->CreateSamplerState(&desc, &m_pSS));
	}

	~SamplerState()
	{
		SafeRelease(m_pSS);
	}

	ID3D11SamplerState * GetSamplerState() { return m_pSS; }
	ID3D11SamplerState * * GetSamplerStatePP() { return &m_pSS; }

private:
	ID3D11SamplerState * m_pSS;
};
