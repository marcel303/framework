#include "DX11FVF.h"
#include "DX11InputLayout.h"
#include "DX11VertexShader.h"
#include "MemAllocators2.h"
#include "ResVB.h"

static void InitInputElemDesc(
	D3D11_INPUT_ELEMENT_DESC & desc,
	const char * pSemanticName,
	uint32_t semanticIdx,
	DXGI_FORMAT format,
	uint32_t offset)
{
	desc.SemanticName = pSemanticName;
	desc.SemanticIndex = semanticIdx;
	desc.Format = format;
	desc.InputSlot = 0;
	desc.AlignedByteOffset = offset;
	desc.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
	desc.InstanceDataStepRate = 0;
}

InputLayout * CreateInputLayoutFromFVF(
	ID3D11Device* pDevice,
	IMemAllocator * pHeap,
	VertexShader& vs,
	uint32_t fvf)
{
	const uint32_t kMaxElems = 12;

	D3D11_INPUT_ELEMENT_DESC elems[kMaxElems];
	
	uint32_t elemIdx = 0;
	uint32_t offset = 0;

	if (fvf & FVF_XYZ)
	{
		InitInputElemDesc(elems[elemIdx++], "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, D3D11_APPEND_ALIGNED_ELEMENT);
		offset += 12;
	}

	if (fvf & FVF_NORMAL)
	{
		InitInputElemDesc(elems[elemIdx++], "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, D3D11_APPEND_ALIGNED_ELEMENT);
		offset += 12;
	}

	if (fvf & FVF_COLOR)
	{
		InitInputElemDesc(elems[elemIdx++], "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UINT, D3D11_APPEND_ALIGNED_ELEMENT);
		offset += 4;
	}

	const uint32 texCount = FVF_TEX_EXTRACT(fvf);

	for (uint32 i = texCount, j = 0; i != 0; --i, ++j)
	{
		InitInputElemDesc(elems[elemIdx++], "TEXTURE", j, DXGI_FORMAT_R32G32_FLOAT, D3D11_APPEND_ALIGNED_ELEMENT);
		offset += 8;
	}

	return new(pHeap) InputLayout(pDevice, vs, elems, elemIdx);
}

uint32_t GetVetexSizeUsingFVF(uint32_t fvf)
{
	uint32_t result = 0;

	if (fvf & FVF_XYZ)
		result += sizeof(float) * 3;
	if (fvf & FVF_NORMAL)
		result += sizeof(float) * 3;
	if (fvf & FVF_COLOR)
		result += sizeof(uint8) * 4;

	uint32_t texCnt = FVF_TEX_EXTRACT(fvf);

	result += sizeof(float) * 2 * texCnt;

	return result;
}

ID3D11Buffer * CreateBufferFromFVF(
	ID3D11Device * pDevice,
	uint32_t fvf,
	uint32_t vertexCount)
{
	const UINT stride = static_cast<UINT>(GetVetexSizeUsingFVF(fvf));
	const UINT size = stride * vertexCount;

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.ByteWidth = size;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;

	D3D11_SUBRESOURCE_DATA bufferInit;
	ZeroMemory(&bufferInit, sizeof(bufferInit));
	bufferInit.pSysMem = 0;

	ID3D11Buffer * pBuffer = 0;
	V(pDevice->CreateBuffer(&bufferDesc, &bufferInit, &pBuffer));

	return pBuffer;
}

ID3D11Buffer * CreateBufferFromFVFWithData(
	ID3D11Device * pDevice,
	IMemAllocator * pHeap,
	uint32_t fvf,
	ResVB * pVB)
{
	UINT stride = static_cast<UINT>(GetVetexSizeUsingFVF(fvf));

	UINT size = stride * pVB->m_vCnt;

	BYTE * pBytesAlloc = pHeap->New<BYTE>(size);
	BYTE * pBytes = pBytesAlloc;

	for (uint32 i = 0; i < pVB->m_vCnt; ++i)
	{
		if (fvf & FVF_XYZ)
		{
			memcpy(pBytes, &pVB->position[i], sizeof(float) * 3);
			pBytes += sizeof(float) * 3;
		}
		if (fvf & FVF_NORMAL)
		{
			memcpy(pBytes, &pVB->normal[i], sizeof(float) * 3);
			pBytes += sizeof(float) * 3;
		}
		if (fvf & FVF_COLOR)
		{
			// TODO: Color!.
			uint32 color =
				int(pVB->color[i][2] * 255.0f) << 0 |
				int(pVB->color[i][1] * 255.0f) << 8 |
				int(pVB->color[i][0] * 255.0f) << 16 |
				int(pVB->color[i][3] * 255.0f) << 24;

			memcpy(pBytes, &color, sizeof(uint8) * 4);
			pBytes += sizeof(uint8) * 4;
		}
		for (uint32 j = 0; j < pVB->m_texCnt; ++j)
		{
			memcpy(pBytes, &pVB->tex[j][i], sizeof(float) * 2);
			pBytes += sizeof(float) * 2;
		}
	}

	D3D11_BUFFER_DESC bufferDesc;
	ZeroMemory(&bufferDesc, sizeof(bufferDesc));
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.ByteWidth = size;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.Usage = D3D11_USAGE_IMMUTABLE;

	D3D11_SUBRESOURCE_DATA bufferInit;
	ZeroMemory(&bufferInit, sizeof(bufferInit));
	bufferInit.pSysMem = pBytesAlloc;

	ID3D11Buffer * pBuffer = 0;
	V(pDevice->CreateBuffer(&bufferDesc, &bufferInit, &pBuffer));

	pHeap->Free(pBytesAlloc);
	pBytesAlloc = 0;
	pBytes = 0;

	return pBuffer;
}
