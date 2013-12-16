#pragma once

#include "DX11.h"

class VertexShader
{
public:
	VertexShader(ID3D11Device * pDevice, const char * pFileName, const char * pEntryPoint, const char ** pMacroList)
		: m_pShader(0)
		, m_pBytes(0)
	{
		static const int kMaxMacros = 64;

		ID3D10Blob * pErrorBytes = 0;

#ifdef _DEBUG
		const UINT flags = D3D10_SHADER_DEBUG | D3D10_SHADER_ENABLE_STRICTNESS | D3D10_SHADER_WARNINGS_ARE_ERRORS;
#else
		const UINT flags = 0;
#endif

		D3D10_SHADER_MACRO macroList[kMaxMacros];
		DX11MacroListInit(macroList, kMaxMacros, pMacroList);

		V(D3DX11CompileFromFileA(pFileName, macroList, 0, pEntryPoint, "vs_4_0", flags, 0, 0, &m_pBytes, &pErrorBytes, 0));

		if (pErrorBytes)
		{
			DXPrintError(pErrorBytes);
		}

		V(pDevice->CreateVertexShader(m_pBytes->GetBufferPointer(), m_pBytes->GetBufferSize(), 0, &m_pShader));
	}

	~VertexShader()
	{
		SafeRelease(m_pShader);
		SafeRelease(m_pBytes);
	}

	ID3D11VertexShader * GetShader() { return m_pShader; }
	ID3D10Blob * GetByteCode() { return m_pBytes; }

private:
	ID3D11VertexShader * m_pShader;
	ID3D10Blob * m_pBytes;
};
