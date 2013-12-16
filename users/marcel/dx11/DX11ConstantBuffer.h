#pragma once

#include "DX11.h"

class ConstantBuffer
{
public:
	ConstantBuffer(ID3D11Device * pDevice, uint32_t size)
		: m_pConstantBuffer(0)
	{
		D3D11_BUFFER_DESC cbDesc;
		ZeroMemory(&cbDesc, sizeof(cbDesc));
		cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
		cbDesc.ByteWidth = size;
		cbDesc.CPUAccessFlags = 0;
		cbDesc.Usage = D3D11_USAGE_DEFAULT;

		V(pDevice->CreateBuffer(&cbDesc, NULL, &m_pConstantBuffer));
	}

	~ConstantBuffer()
	{
		SafeRelease(m_pConstantBuffer);
	}

	void Update(ID3D11DeviceContext * pDeviceCtx, void * pBytes)
	{
		pDeviceCtx->UpdateSubresource(m_pConstantBuffer, 0, 0, pBytes, 0, 0);
	}

	ID3D11Buffer * GetConstantBuffer() { return m_pConstantBuffer; }
	ID3D11Buffer * * GetConstantBufferPP() { return &m_pConstantBuffer; }

private:
	ID3D11Buffer * m_pConstantBuffer;
};
