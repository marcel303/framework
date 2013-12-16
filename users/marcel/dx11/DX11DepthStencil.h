#pragma once

#include "DX11.h"

class DepthStencil
{
public:
	DepthStencil(ID3D11Device * pDevice, uint32_t sx, uint32_t sy, DXGI_FORMAT textureFormat, DXGI_FORMAT depthFormat, DXGI_FORMAT shaderFormat)
		: m_pTexture(0)
		, m_pView(0)
		, m_pSRV(0)
	{
		D3D11_TEXTURE2D_DESC textureDepthDesc;
		ZeroMemory(&textureDepthDesc, sizeof(textureDepthDesc));
		textureDepthDesc.ArraySize = 1;
		textureDepthDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
		textureDepthDesc.CPUAccessFlags = 0;
		textureDepthDesc.Format = textureFormat;
		textureDepthDesc.Height = sy;
		textureDepthDesc.MipLevels = 1;
		textureDepthDesc.MiscFlags = 0;
		textureDepthDesc.SampleDesc.Count = 1;
		textureDepthDesc.SampleDesc.Quality = 0;
		textureDepthDesc.Usage = D3D11_USAGE_DEFAULT;
		textureDepthDesc.Width = sx;

		V(pDevice->CreateTexture2D(&textureDepthDesc, 0, &m_pTexture));

		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
		ZeroMemory(&depthStencilViewDesc, sizeof(depthStencilViewDesc));
		depthStencilViewDesc.Format = depthFormat;
		depthStencilViewDesc.Texture2D.MipSlice = 0;
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;

		V(pDevice->CreateDepthStencilView(m_pTexture, &depthStencilViewDesc, &m_pView));

		D3D11_SHADER_RESOURCE_VIEW_DESC resourceDesc;
		ZeroMemory(&resourceDesc, sizeof(resourceDesc));
		resourceDesc.Format = shaderFormat;
		resourceDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		resourceDesc.Texture2D.MipLevels = 1;
		resourceDesc.Texture2D.MostDetailedMip = 0;

		V(pDevice->CreateShaderResourceView(m_pTexture, &resourceDesc, &m_pSRV));
	}

	~DepthStencil()
	{
		SafeRelease(m_pSRV);
		SafeRelease(m_pView);
		SafeRelease(m_pTexture);
	}

	ID3D11DepthStencilView * GetDepthStencilView()
	{
		return m_pView;
	}

	ID3D11ShaderResourceView * * GetShaderResourceViewPP()
	{
		return &m_pSRV;
	}

private:
	ID3D11Texture2D * m_pTexture;
	ID3D11DepthStencilView * m_pView;
	ID3D11ShaderResourceView * m_pSRV;
};
