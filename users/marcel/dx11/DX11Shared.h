#pragma once

#include <stdio.h>

//

static inline void A(bool result)
{
	if (result == false)
		printf("assertion failed\n");
}

static inline void V(HRESULT resultCode)
{
	if (FAILED(resultCode))
		printf("error: 0x%08x\n", resultCode);

	//assert(SUCCEEDED(resultCode));
}

template <typename T>
void SafeRelease(T*& obj)
{
	if (obj)
	{
		obj->Release();
		obj = 0;
	}
}

static void DX11MacroListInit(D3D10_SHADER_MACRO * pMacroList, int maxMacros, char const * const * ppText)
{
	if (ppText == 0)
	{
		pMacroList[0].Name = 0;
		pMacroList[0].Definition = 0;
	}
	else
	{
		for (int i = 0; i < maxMacros; ++i)
		{
			if (ppText[0] == 0)
			{
				pMacroList[i].Name = 0;
				pMacroList[i].Definition = 0;
				break;
			}
			else
			{
				pMacroList[i].Name = ppText[0];
				pMacroList[i].Definition = ppText[1];
				ppText += 2;
			}
		}
	}
}
