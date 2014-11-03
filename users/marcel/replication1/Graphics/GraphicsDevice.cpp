#include "GraphicsDevice.h"

GraphicsDevice::GraphicsDevice()
{
}

GraphicsDevice::~GraphicsDevice()
{
}

void GraphicsDevice::ResetStates()
{
	float alphaTestRef = 0.5f;

	RS(RS_DEPTHTEST, 0);
	RS(RS_DEPTHTEST_FUNC, CMP_LE);
	RS(RS_STENCILTEST, 0);
	RS(RS_STENCILTEST_FUNC, CMP_EQ);
	RS(RS_STENCILTEST_MASK, 0);
	RS(RS_STENCILTEST_REF, 0);
	RS(RS_STENCILTEST_ONPASS, ZERO);
	RS(RS_STENCILTEST_ONFAIL, ZERO);
	RS(RS_STENCILTEST_ONZFAIL, ZERO);
	RS(RS_CULL, CULL_NONE);
	RS(RS_ALPHATEST, 0);
	RS(RS_ALPHATEST_FUNC, CMP_LE);
	RS(RS_ALPHATEST_REF, *(int*)&alphaTestRef);

	// FIXME: Max textures = # ?
	for (int i = 0; i < 8; ++i)
	{
		SS(i, SS_FILTER, FILTER_INTERPOLATE);
	}
}