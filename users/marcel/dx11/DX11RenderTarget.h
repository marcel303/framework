#pragma once

#include "DX11.h"

class RenderTarget
{
public:
	RenderTarget(ID3D11Device * pDevice, uint32_t sx, uint32_t sy, DXGI_FORMAT format)
		: m_pT(0)
		, m_pRTV(0)
		, m_pSRV(0)
	{
		D3D11_TEXTURE2D_DESC desc;
		ZeroMemory(&desc, sizeof(desc));

		desc.Width = sx;
		desc.Height = sy;
		desc.MipLevels = 1;
		desc.ArraySize = 1;
		desc.Format = format;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Usage = D3D11_USAGE_DEFAULT;
		desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		V(pDevice->CreateTexture2D(&desc, 0, &m_pT));
		V(pDevice->CreateRenderTargetView(m_pT, 0, &m_pRTV));

		D3D11_SHADER_RESOURCE_VIEW_DESC resourceDesc;
		ZeroMemory(&resourceDesc, sizeof(resourceDesc));
		resourceDesc.Format = format;
		resourceDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		resourceDesc.Texture2D.MipLevels = 1;
		resourceDesc.Texture2D.MostDetailedMip = 0;

		V(pDevice->CreateShaderResourceView(m_pT, &resourceDesc, &m_pSRV));
	}

	~RenderTarget()
	{
		SafeRelease(m_pSRV);
		SafeRelease(m_pRTV);
		SafeRelease(m_pT);
	}

	ID3D11Texture2D * GetTexture() { return m_pT; }
	ID3D11RenderTargetView * GetRenderTarget() { return m_pRTV; }
	ID3D11ShaderResourceView * GetShaderResourceView() { return m_pSRV; }
	ID3D11ShaderResourceView * * GetShaderResourceViewPP() { return &m_pSRV; }

private:
	ID3D11Texture2D * m_pT;
	ID3D11RenderTargetView * m_pRTV;
	ID3D11ShaderResourceView * m_pSRV;
};
