#pragma once

enum RenderPass
{
	kRenderPass_Opaque,
	kRenderPass_Opaque_ForwardShaded,
	kRenderPass_Lights,
	kRenderPass_Background,
	kRenderPass_Translucent,
	kRenderPass_COUNT
};

enum ShadowPass
{
	kShadowPass_Opaque,
	kShadowPass_Translucent,
	kShadowPass_COUNT
};
