#pragma once

enum MaterialSetupLock
{
	MaterialSetupLock_AlphaTest = 1 << 0,
	MaterialSetupLock_ColorMerge = 1 << 1,
	MaterialSetupLock_ColorWrite = 1 << 2,
	MaterialSetupLock_Culling = 1 << 3,
	MaterialSetupLock_DepthWrite = 1 << 4,
	MaterialSetupLock_DepthTest = 1 << 5,
	MaterialSetupLock_DepthBias = 1 << 6,
	MaterialSetupLock_Scissor = 1 << 7,
	MaterialSetupLock_StencilTest = 1 << 8
};

class StateManagerProxy
{
public:
	StateManagerProxy(StateManager * pStateManager)
		: m_pStateManager(pStateManager)
		, m_lock(0)
	{
	}

	void LockSet(uint32_t lock)
	{
		m_lock = lock;
	}

	void CheckLock(RENDER_STATE state)
	{
		switch (state)
		{
		case RS_ALPHATEST:
		case RS_ALPHATEST_FUNC:
		case RS_ALPHATEST_REF:
			assert((m_lock & MaterialSetupLock_AlphaTest) == 0);
			break;
		case RS_BLEND:
		case RS_BLEND_SRC:
		case RS_BLEND_DST:
			assert((m_lock & MaterialSetupLock_ColorMerge) == 0);
			break;
		case RS_CULL:
			assert((m_lock & MaterialSetupLock_Culling) == 0);
			break;
		case RS_DEPTHTEST:
		case RS_DEPTHTEST_FUNC:
			assert((m_lock & MaterialSetupLock_DepthTest) == 0);
			break;
		case RS_SCISSORTEST:
			assert((m_lock & MaterialSetupLock_Scissor) == 0);
			break;
		case RS_STENCILTEST:
		case RS_STENCILTEST_FUNC:
		case RS_STENCILTEST_REF:
		case RS_STENCILTEST_MASK:
		case RS_STENCILTEST_ONPASS:
		case RS_STENCILTEST_ONFAIL:
		case RS_STENCILTEST_ONZFAIL:
			assert((m_lock & MaterialSetupLock_StencilTest) == 0);
			break;
		case RS_WRITE_COLOR:
			assert((m_lock & MaterialSetupLock_ColorWrite) == 0);
			break;
		case RS_WRITE_DEPTH:
			assert((m_lock & MaterialSetupLock_DepthWrite) == 0);
			break;
		}
	}

	void SetRS(RENDER_STATE state, uint32_t value)
	{
		CheckLock(state);

		m_pStateManager->SetRS(state, value);
	}

	//void SetDepthTest(D3D11_COMPARISON_FUNC test, bool writeEnabled)

private:
	StateManager * m_pStateManager;
	uint32_t m_lock;
};
