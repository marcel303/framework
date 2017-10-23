#include "Ease.h"
#include "framework.h"
#include "slideshow.h"
#include <math.h>

extern const int GFX_SX;
extern const int GFX_SY;

static void drawPic(Sprite & sprite, const float t, const float alpha, const float zoom1, const float zoom2)
{
	setColorf(1.f, 1.f, 1.f, alpha);
	const float scaleX = GFX_SX / (float)sprite.getWidth();
	const float scaleY = GFX_SY / (float)sprite.getHeight();
	const float scale = fmaxf(scaleX, scaleY);
	const float sx = sprite.getWidth() * scale;
	const float sy = sprite.getHeight() * scale;
	const float px = GFX_SX - sx;
	const float py = GFX_SY - sy;
	const float pt = EvalEase(t, kEaseType_SineInOut, 0.f);
	const float zoom = lerp(zoom1, zoom2, pt);
	gxPushMatrix();
	{
		gxTranslatef(px * pt, py * pt, 0.f);
		gxTranslatef(+sx/2.f, +sy/2.f, 0.f);
		gxScalef(zoom, zoom, 1.f);
		gxTranslatef(-sx/2.f, -sy/2.f, 0.f);

		sprite.drawEx(0.f, 0.f, 0.f, scale, scale, true, FILTER_LINEAR);
		setColor(colorWhite);
	}
	gxPopMatrix();
}

void Slideshow::tick(const float dt)
{
	picTimer -= dt;

	if (picTimer < 0.f)
		picTimer = 0.f;

	if (picTimer == 0.f)
	{
		if (!pics.empty())
		{
			picInfo[0] = picInfo[1];
			do
			{
				picInfo[1].index = rand() % pics.size();
				picInfo[1].zoom1 = random(1.f, 1.5f);
				picInfo[1].zoom2 = random(1.f, 1.5f);
				picTimer = 16.f;
				picTimerRcp = 1.f / picTimer;
			}
			while (picInfo[1].index == picInfo[0].index && pics.size() != 1);
		}
	}
}

void Slideshow::draw() const
{
	pushBlend(BLEND_OPAQUE);
	if (picInfo[0].index != -1)
	{
		Sprite oldPic(pics[picInfo[0].index].c_str());
		drawPic(oldPic, 1.f, 1.f, picInfo[0].zoom1, picInfo[0].zoom2);
	}
	popBlend();
	if (picInfo[1].index != -1)
	{
		pushBlend(BLEND_ALPHA);
		Sprite newPic(pics[picInfo[1].index].c_str());
		drawPic(newPic, 1.f - picTimer * picTimerRcp, clamp((1.f / picTimerRcp) - picTimer, 0.f, 1.f), picInfo[1].zoom1, picInfo[1].zoom2);
		popBlend();
	}
}
