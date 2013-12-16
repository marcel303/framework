#pragma once

#include <map>
#include "DX11.h"

#include "DX11BlendState.h"
#include "DX11DepthStencilState.h"
#include "DX11RasterizerState.h"

#include "Graphics/GraphicsTypes.h"

static inline D3D11_COMPARISON_FUNC ToDX11(TEST_FUNC func);
static inline D3D11_CULL_MODE ToDX11(CULL_MODE mode);

/* Manages all possible render states and make them current when changed.
 */
class StateManager
{
public:
	StateManager(ID3D11Device * pDevice)
		: m_pDevice(pDevice)
		, m_dirtyBits(0xffffffff)
	{
		ZeroMemory(&m_DepthStencilDesc, sizeof(m_DepthStencilDesc));
		// depth test
		m_DepthStencilDesc.DepthEnable = FALSE;
		m_DepthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
		m_DepthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
		// stencil test
		m_DepthStencilDesc.StencilEnable = FALSE;
		m_DepthStencilDesc.StencilReadMask = 0xff;
		m_DepthStencilDesc.StencilWriteMask = 0xff;
		// stencil front
		m_DepthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		m_DepthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		m_DepthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		m_DepthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		// stencil back
		m_DepthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
		m_DepthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
		m_DepthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
		m_DepthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
		
		ZeroMemory(&m_RasterizerDesc, sizeof(m_RasterizerDesc));
		m_RasterizerDesc.FillMode = D3D11_FILL_SOLID;
		m_RasterizerDesc.CullMode = D3D11_CULL_BACK;
		m_RasterizerDesc.FrontCounterClockwise = FALSE;
		m_RasterizerDesc.DepthBias = 0;
		m_RasterizerDesc.SlopeScaledDepthBias = 0.0f;
		m_RasterizerDesc.DepthBiasClamp = 0.0f;
		m_RasterizerDesc.DepthClipEnable = FALSE;
		m_RasterizerDesc.ScissorEnable = FALSE;
		m_RasterizerDesc.MultisampleEnable = FALSE;
		m_RasterizerDesc.AntialiasedLineEnable = FALSE;

		ZeroMemory(&m_BlendDesc, sizeof(m_BlendDesc));
		m_BlendDesc.AlphaToCoverageEnable = FALSE;
		m_BlendDesc.IndependentBlendEnable = FALSE;
		D3D11_RENDER_TARGET_BLEND_DESC rtDesc;
		rtDesc.BlendEnable = FALSE;
		rtDesc.SrcBlend = D3D11_BLEND_ONE;
		rtDesc.DestBlend = D3D11_BLEND_ZERO;
		rtDesc.BlendOp = D3D11_BLEND_OP_ADD;
		rtDesc.SrcBlendAlpha = D3D11_BLEND_ONE;
		rtDesc.DestBlendAlpha = D3D11_BLEND_ZERO;
		rtDesc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
		rtDesc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
		for (uint32_t i = 0; i < 8; ++i)
			memcpy(&m_BlendDesc.RenderTarget[i], &rtDesc, sizeof(rtDesc));
	}

	~StateManager()
	{
		for (std::map<uint32_t, DepthStencilState *>::iterator i = m_DepthStencilCache.begin(); i != m_DepthStencilCache.end(); ++i)
		{
			DepthStencilState * pState = i->second;
			delete pState;
			pState = 0;
		}
		m_DepthStencilCache.clear();

		for (std::map<uint32_t, RasterizerState *>::iterator i = m_RasterizerCache.begin(); i != m_RasterizerCache.end(); ++i)
		{
			RasterizerState * pState = i->second;
			delete pState;
			pState = 0;
		}
		m_RasterizerCache.clear();

		for (std::map<uint32_t, BlendState *>::iterator i = m_BlendCache.begin(); i != m_BlendCache.end(); ++i)
		{
			BlendState * pState = i->second;
			delete pState;
			pState = 0;
		}
		m_BlendCache.clear();
	}

	void SetRS(RENDER_STATE state, uint32_t value)
	{
		switch (state)
		{
		case RS_ALPHATEST:
			A(false);
			break;
		case RS_ALPHATEST_FUNC:
			A(false);
			break;
		case RS_ALPHATEST_REF:
			A(false);
			break;
		case RS_BLEND:
			{
				BOOL v = value ? TRUE : FALSE;
				for (uint32_t i = 0; i < kMaxRenderTargets; ++i)
				{
					if (m_BlendDesc.RenderTarget[i].BlendEnable != v)
					{
						m_BlendDesc.RenderTarget[i].BlendEnable = v;
						m_dirtyBits |= kBitBlendState;
					}
				}
				break;
			}
		case RS_BLEND_SRC:
			for (uint32_t i = 0; i < kMaxRenderTargets; ++i)
			{
				A(false);
				//todo
			}
			m_dirtyBits |= kBitBlendState;
			break;
		case RS_BLEND_DST:
			for (uint32_t i = 0; i < kMaxRenderTargets; ++i)
			{
				A(false);
				//todo
			}
			m_dirtyBits |= kBitBlendState;
			break;
		case RS_CULL:
			{
				D3D11_CULL_MODE mode = ToDX11(static_cast<CULL_MODE>(value));
				if (m_RasterizerDesc.CullMode != mode)
				{
					m_RasterizerDesc.CullMode = mode;
					m_dirtyBits |= kBitRasterizerState;
				}
				break;
			}
		case RS_DEPTHTEST:
			{
				BOOL v = value ? TRUE : FALSE;
				if (m_DepthStencilDesc.DepthEnable != v)
				{
					m_DepthStencilDesc.DepthEnable = v;
					m_dirtyBits |= kBitDepthStencil;
				}
				break;
			}
		case RS_DEPTHTEST_FUNC:
			{
				D3D11_COMPARISON_FUNC func = ToDX11(static_cast<TEST_FUNC>(value));
				if (m_DepthStencilDesc.DepthFunc != func)
				{
					m_DepthStencilDesc.DepthFunc = func;
					m_dirtyBits |= kBitDepthStencil;
				}
				break;
			}
		case RS_SCISSORTEST:
			{
				BOOL v = value ? TRUE : FALSE;
				if (m_RasterizerDesc.ScissorEnable != v)
				{
					m_RasterizerDesc.ScissorEnable = v;
					m_dirtyBits |= kBitRasterizerState;
				}
				break;
			}
		case RS_STENCILTEST:
			{
				BOOL v = value ? TRUE : FALSE;
				if (m_DepthStencilDesc.StencilEnable != v)
				{
					m_DepthStencilDesc.StencilEnable = v;
					m_dirtyBits |= kBitDepthStencil;
				}
				break;
			}
		case RS_STENCILTEST_FUNC:
			A(false);
			///todo
			m_dirtyBits |= kBitDepthStencil;
			break;
		case RS_STENCILTEST_REF:
			A(false);
			///todo
			m_dirtyBits |= kBitDepthStencil;
			break;
		case RS_STENCILTEST_MASK:
			A(false);
			///todo
			m_dirtyBits |= kBitDepthStencil;
			break;
		case RS_STENCILTEST_ONPASS:
			A(false);
			///todo
			m_dirtyBits |= kBitDepthStencil;
			break;
		case RS_STENCILTEST_ONFAIL:
			A(false);
			///todo
			m_dirtyBits |= kBitDepthStencil;
			break;
		case RS_STENCILTEST_ONZFAIL:
			A(false);
			///todo
			m_dirtyBits |= kBitDepthStencil;
			break;
		case RS_WRITE_COLOR:
			{
				UINT8 mask = value ? 0xff : 0x00;
				for (uint32_t i = 0; i < kMaxRenderTargets; ++i)
				{
					if (m_BlendDesc.RenderTarget[i].RenderTargetWriteMask != mask)
					{
						m_BlendDesc.RenderTarget[i].RenderTargetWriteMask = mask;
						m_dirtyBits |= kBitBlendState;
					}
				}
				break;
			}
		case RS_WRITE_DEPTH:
			{
				D3D11_DEPTH_WRITE_MASK mask = value ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
				if (m_DepthStencilDesc.DepthWriteMask != mask)
				{
					m_DepthStencilDesc.DepthWriteMask = mask;
					m_dirtyBits |= kBitDepthStencil;
				}
				break;
			}
		}
	}

	void SetDepthTest(D3D11_COMPARISON_FUNC test, bool writeEnabled)
	{
		m_DepthStencilDesc.DepthFunc = test;
		m_DepthStencilDesc.DepthWriteMask = writeEnabled ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
	}

	void MakeCurrent(ID3D11DeviceContext * pDeviceCtx)
	{
		if (m_dirtyBits & kBitDepthStencil)
		{
			uint32_t hash = DXHash(&m_DepthStencilDesc, sizeof(m_DepthStencilDesc));
			std::map<uint32_t, DepthStencilState *>::iterator i = m_DepthStencilCache.find(hash);
			DepthStencilState * pState = 0;
			if (i == m_DepthStencilCache.end())
			{
				pState = new DepthStencilState(m_pDevice, m_DepthStencilDesc);
				m_DepthStencilCache.insert(std::pair<uint32_t, DepthStencilState *>(hash, pState));
			}
			else
			{
				pState = i->second;
			}
			pDeviceCtx->OMSetDepthStencilState(pState->GetDepthStencilState(), 0x00);
		}

		if (m_dirtyBits & kBitRasterizerState)
		{
			uint32_t hash = DXHash(&m_RasterizerDesc, sizeof(m_RasterizerDesc));
			std::map<uint32_t, RasterizerState *>::iterator i = m_RasterizerCache.find(hash);
			RasterizerState * pState = 0;
			if (i == m_RasterizerCache.end())
			{
				pState = new RasterizerState(m_pDevice, m_RasterizerDesc);
				m_RasterizerCache.insert(std::pair<uint32_t, RasterizerState *>(hash, pState));
			}
			else
			{
				pState = i->second;
			}
			pDeviceCtx->RSSetState(pState->GetRasterizerState());
		}

		if (m_dirtyBits & kBitBlendState)
		{
			uint32_t hash = DXHash(&m_BlendDesc, sizeof(m_BlendDesc));
			std::map<uint32_t, BlendState *>::iterator i = m_BlendCache.find(hash);
			BlendState * pState = 0;
			if (i == m_BlendCache.end())
			{
				pState = new BlendState(m_pDevice, m_BlendDesc);
				m_BlendCache.insert(std::pair<uint32_t, BlendState *>(hash, pState));
			}
			else
			{
				pState = i->second;
			}
			pDeviceCtx->OMSetBlendState(pState->GetBlendState(), 0, 0xffffffff);
		}

		if (m_dirtyBits != 0)
		{
			m_dirtyBits = 0;
		}
	}

private:
	const static uint32_t kBitDepthStencil    = 1 << 0;
	const static uint32_t kBitRasterizerState = 1 << 1;
	const static uint32_t kBitBlendState      = 1 << 2;

	const static uint32_t kMaxRenderTargets = 8;

	ID3D11Device * m_pDevice;

	uint32_t m_dirtyBits;

	D3D11_DEPTH_STENCIL_DESC m_DepthStencilDesc;
	std::map<uint32_t, DepthStencilState *> m_DepthStencilCache;

	D3D11_RASTERIZER_DESC m_RasterizerDesc;
	std::map<uint32_t, RasterizerState *> m_RasterizerCache;

	D3D11_BLEND_DESC m_BlendDesc;
	std::map<uint32_t, BlendState *> m_BlendCache;
};

static inline D3D11_COMPARISON_FUNC ToDX11(TEST_FUNC func)
{
	switch (func)
	{
	case CMP_ALWAYS:
		return D3D11_COMPARISON_ALWAYS;
	case CMP_EQ:
		return D3D11_COMPARISON_EQUAL;
	case CMP_G:
		return D3D11_COMPARISON_GREATER;
	case CMP_GE:
		return D3D11_COMPARISON_GREATER_EQUAL;
	case CMP_L:
		return D3D11_COMPARISON_LESS;
	case CMP_LE:
		return D3D11_COMPARISON_LESS_EQUAL;
	default:
		assert(false);
		return D3D11_COMPARISON_ALWAYS;
	}
}

static inline D3D11_CULL_MODE ToDX11(CULL_MODE mode)
{
	switch (mode)
	{
	case CULL_CCW:
		return D3D11_CULL_BACK; // todo: check translation
	case CULL_CW:
		return D3D11_CULL_FRONT;
	case CULL_NONE:
		return D3D11_CULL_NONE;
	default:
		assert(false);
		return D3D11_CULL_NONE;
	}
}
