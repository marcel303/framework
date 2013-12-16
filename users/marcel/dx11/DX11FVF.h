#pragma once

#include "DX11.h"
#include "MemAllocators2.h"
#include "ResVB.h"

InputLayout * CreateInputLayoutFromFVF(
	ID3D11Device * pDevice,
	IMemAllocator * pHeap,
	VertexShader & vs,
	uint32_t fvf);

uint32_t GetVetexSizeUsingFVF(uint32_t fvf);

ID3D11Buffer * CreateBufferFromFVF(
	ID3D11Device * pDevice,
	uint32_t fvf,
	uint32_t vertexCount);

ID3D11Buffer * CreateBufferFromFVFWithData(
	ID3D11Device * pDevice,
	IMemAllocator * pHeap,
	uint32_t fvf,
	ResVB * pVB);
