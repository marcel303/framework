#pragma once

#include "DX11.h"
#include "DX11VertexShader.h"

class InputLayout
{
public:
	InputLayout(ID3D11Device * pDevice, VertexShader & vs, D3D11_INPUT_ELEMENT_DESC * elems, uint32_t numElems)
		: m_pInputLayout(0)
	{
		V(pDevice->CreateInputLayout(
			elems,
			numElems,
			vs.GetByteCode()->GetBufferPointer(),
			vs.GetByteCode()->GetBufferSize(),
			&m_pInputLayout));
	}

	~InputLayout()
	{
		SafeRelease(m_pInputLayout);
	}

	ID3D11InputLayout * GetInputLayout()
	{
		return m_pInputLayout;
	}

private:
	ID3D11InputLayout * m_pInputLayout;
};
