#pragma once

#include "DX11.h"

class Texture2D
{
public:
	Texture2D(ID3D11Device * pDevice, uint32_t sx, uint32_t sy, DXGI_FORMAT format, void * pBytes, uint32_t pitch)
		: m_pT(0)
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
		desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
		desc.CPUAccessFlags = 0;
		desc.MiscFlags = 0;

		D3D11_SUBRESOURCE_DATA data;
		ZeroMemory(&data, sizeof(data));
		data.pSysMem = pBytes;
		data.SysMemPitch = pitch;

		V(pDevice->CreateTexture2D(&desc, &data, &m_pT));

		D3D11_SHADER_RESOURCE_VIEW_DESC resourceDesc;
		ZeroMemory(&resourceDesc, sizeof(resourceDesc));
		resourceDesc.Format = format;
		resourceDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
		resourceDesc.Texture2D.MipLevels = 1;
		resourceDesc.Texture2D.MostDetailedMip = 0;

		V(pDevice->CreateShaderResourceView(m_pT, &resourceDesc, &m_pSRV));

		//printf("texture: 0x%08x\n", m_pT);
	}

	~Texture2D()
	{
		SafeRelease(m_pSRV);
		SafeRelease(m_pT);
	}

	static Texture2D * LoadFromFile(ID3D11Device * pDevice, const char * pFileName)
	{
		FREE_IMAGE_FORMAT format = FreeImage_GetFileType(pFileName, 0);

		FIBITMAP * pBitMap = FreeImage_Load(format, pFileName, 0);
		assert(pBitMap);

		FIBITMAP * pBitMap32 = FreeImage_ConvertTo32Bits(pBitMap);
		assert(pBitMap32);

		FreeImage_Unload(pBitMap);
		pBitMap = 0;
		
		uint32_t sx = FreeImage_GetWidth(pBitMap32);
		uint32_t sy = FreeImage_GetHeight(pBitMap32);
		uint32_t pitch = sx * 4;

		BYTE * pBytes = new BYTE[sx * sy * 4];

		for (uint32_t y = 0; y < sy; ++y)
		{
			const BYTE * pLine = FreeImage_GetScanLine(pBitMap32, sy - 1 - y);

			memcpy(pBytes + pitch * y, pLine, pitch);
		}

		FreeImage_Unload(pBitMap32);

		Texture2D * pTexture = new Texture2D(pDevice, sx, sy, DXGI_FORMAT_R8G8B8A8_UNORM, pBytes, pitch);

		delete[] pBytes;
		pBytes = 0;

		return pTexture;
	}

	ID3D11Texture2D * GetTexture() { return m_pT; }
	ID3D11ShaderResourceView * GetShaderResourceView() { return m_pSRV; }
	ID3D11ShaderResourceView * * GetShaderResourceViewPP() { return &m_pSRV; }

private:
	ID3D11Texture2D * m_pT;
	ID3D11ShaderResourceView * m_pSRV;
};