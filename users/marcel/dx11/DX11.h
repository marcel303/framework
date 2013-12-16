#pragma once

#include <d3d11.h>
#include <d3dx11.h>
#include <stdint.h>
#include "DX11Shared.h"

class BlendState;
class ConstantBuffer;
class DepthStencil;
class DepthStencilState;
class InputLayout;
class PixelShader;
class RasterizerState;
class RenderDevice;
class RenderTarget;
class SamplerState;
class StateManager;
class Texture2D;
class VertexShader;

void DXPrintError(ID3D10Blob * pErrorBytes);

uint32_t DXHash(void * pBytes, uint32_t byteCount);
