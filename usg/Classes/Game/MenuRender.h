#pragma once

namespace GameMenu
{
	void Render_Empty(void* obj, void* arg);

	void HitEffect_Particles_Centroid(const Vec2F& pos);
	void HitEffect_Particles_Rect(const RectF& rect);
}
