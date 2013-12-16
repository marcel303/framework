#pragma once

enum RenderMask
{
	RenderMask_WorldBackground = 1, // background
	RenderMask_WorldPrimaryNonPlayer = 2, // enemies, bullets (shadows + solid)
	RenderMask_WorldPrimaryPlayer = 4, // player ship (shadows + solid)
	RenderMask_WorldPrimary = RenderMask_WorldPrimaryNonPlayer | RenderMask_WorldPrimaryPlayer, // enemies, player ship, bullets (shadows + solid)
	RenderMask_Particles = 8, // additive particles, using world view
	RenderMask_HudInfo = 16, // score, # lives
	RenderMask_HudPlayer = 32, // player related hud items (controllers)
	RenderMask_Intermezzo = 64, // intermezzo overlays
	RenderMask_Interface = 128, // menus and additive menu particles
	RenderMask_Indicators = 256, // enemy indicators,
	RenderMask_SprayAngles = 512, // spray cone
	RenderMask_HackWarning = 1024, // application has been hacked warning
};

class IView
{
public:
	virtual ~IView();
	
	virtual void Initialize();
	
	virtual void HandleFocus();
	virtual void HandleFocusLost();
	
	virtual void Update(float dt);
	virtual void UpdateAnimation(float dt);
	virtual void Render();
	virtual void Render_Additive();
	
	virtual int RenderMask_get();
	virtual float FadeTime_get();
	
	bool IsFadeActive(float transitionTime, float time);
};
