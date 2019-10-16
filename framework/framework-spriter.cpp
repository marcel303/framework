#include "framework.h"
#include "internal.h"
#include "spriter.h"

SpriterState::SpriterState()
{
	memset(this, 0, sizeof(SpriterState));

	scale = 1.f;
	scaleX = FLT_MAX;
	scaleY = FLT_MAX;

	animIndex = -1;
	animSpeed = 1.f;
}

bool SpriterState::startAnim(const Spriter & spriter, int index)
{
	animIsActive = true;
	animIndex = index;
	animTime = 0.f;

	return animIndex >= 0;
}

bool SpriterState::startAnim(const Spriter & spriter, const char * name)
{
	return startAnim(spriter, spriter.getAnimIndexByName(name));
}

void SpriterState::stopAnim(const Spriter & spriter)
{
	animIsActive = false;
}

bool SpriterState::updateAnim(const Spriter & spriter, float dt)
{
	if (animIsActive)
		animTime += animSpeed * dt;

	const bool isDone =
		!animIsActive ||
		spriter.isAnimDoneAtTime(animIndex, animTime);

	if (isDone)
	{
		if (animLoop)
			startAnim(spriter, animIndex);
		else
			stopAnim(spriter);
	}

	return isDone;
}

void SpriterState::setCharacterMap(const Spriter & spriter, int index)
{
	//fassert(index >= 0 && index < spriter.m_spriter->m_scene->m_fileCaches.size());
	if (index >= 0 && index < (int)spriter.m_spriter->m_scene->m_fileCaches.size())
		characterMap = index;
	else
	{
		logError("character map does not exist: index=%d", index);
		characterMap = 0;
	}
}

void SpriterState::setCharacterMap(const Spriter & spriter, const char * name)
{
	const int index = spriter.m_spriter->m_scene->getCharacterMapIndexByName(name);
	fassert(index != -1);
	if (index == -1)
		logError("character map not found: %s", name);

	characterMap = index < 0 ? 0 : characterMap;
}

// -----

Spriter::Spriter(const char * filename)
{
	m_spriter = &g_spriterCache.findOrCreate(filename);
}

void Spriter::getDrawableListAtTime(const SpriterState & state, spriter::Drawable * drawables, int & numDrawables)
{
	if (m_spriter->m_scene->m_entities.empty())
		numDrawables = 0;
	else
	{
		m_spriter->m_scene->m_entities[0]->getDrawableListAtTime(
			state.animIndex,
			state.characterMap,
			state.animTime * 1000.f,
			drawables, numDrawables);
	}
}

void Spriter::draw(const SpriterState & state)
{
	if (m_spriter->m_scene->m_entities.empty())
	{
		return;
	}

	const int kMaxDrawables = 64;
	spriter::Drawable drawables[kMaxDrawables];
	int numDrawables = kMaxDrawables;

	m_spriter->m_scene->m_entities[0]->getDrawableListAtTime(
		state.animIndex,
		state.characterMap,
		state.animTime * 1000.f,
		drawables, numDrawables);

	draw(state, drawables, numDrawables);
}

void Spriter::draw(const SpriterState & state, const spriter::Drawable * drawables, int numDrawables)
{
	gxPushMatrix();
	gxTranslatef(state.x, state.y, 0.f);
	gxRotatef(state.angle, 0.f, 0.f, 1.f);
	const float scaleX = state.scaleX != FLT_MAX ? state.scaleX : state.scale;
	const float scaleY = state.scaleY != FLT_MAX ? state.scaleY : state.scale;
	if (scaleX != 1.f || scaleY != 1.f)
		gxScalef(scaleX, scaleY, 1.f);
	if (state.flipX || state.flipY)
		gxScalef(state.flipX ? -1.f : +1.f, state.flipY ? -1.f : +1.f, 1.f);

	const Color oldColor = globals.color;

	for (int i = 0; i < numDrawables; ++i)
	{
		const spriter::Drawable & d = drawables[i];

		setColorf(
			globals.color.r,
			globals.color.g,
			globals.color.b,
			d.a * oldColor.a);

		Sprite sprite(d.filename, d.pivotX, d.pivotY, 0, false, false);
		sprite.x = d.x;
		sprite.y = d.y;
		sprite.angle = d.angle;
		sprite.separateScale = true;
		sprite.scaleX = d.scaleX;
		sprite.scaleY = d.scaleY;
		sprite.pixelpos = false;
		sprite.filter = FILTER_MIPMAP;
		sprite.draw();

	#if 0
		if (k == 0)
		{
			printf("transform @ t=%+04.2f: (%+04.2f, %+04.2f) @ %+04.2f x (%+04.2f, %+04.2f) * %+04.2f\n",
				t,
				tf.x, tf.y,
				tf.angle,
				tf.scaleX,
				tf.scaleY,
				tf.a);
		}
	#endif
	}

	setColor(oldColor);

	gxPopMatrix();
}

int Spriter::getAnimCount() const
{
	if (m_spriter->m_scene->m_entities.empty())
		return 0;
	return (int)m_spriter->m_scene->m_entities[0]->m_animations.size();
}

const char * Spriter::getAnimName(const int animIndex) const
{
	if (m_spriter->m_scene->m_entities.empty())
		return 0;
	
	return m_spriter->m_scene->m_entities[0]->getAnimName(animIndex);
}

int Spriter::getAnimIndexByName(const char * name) const
{
	if (m_spriter->m_scene->m_entities.empty())
		return -1;
	return m_spriter->m_scene->m_entities[0]->getAnimIndexByName(name);
}

float Spriter::getAnimLength(int animIndex) const
{
	if (animIndex == -1)
		return 0.f;
	fassert(!m_spriter->m_scene->m_entities.empty());
	return m_spriter->m_scene->m_entities[0]->getAnimLength(animIndex);
}

bool Spriter::isAnimDoneAtTime(int animIndex, float time) const
{
	if (animIndex == -1)
		return true;
	fassert(!m_spriter->m_scene->m_entities.empty());
	if (m_spriter->m_scene->m_entities.empty())
		return true;
	if (m_spriter->m_scene->m_entities[0]->isAnimLooped(animIndex))
		return false;
	return time >= getAnimLength(animIndex) / 1000.f;
}

bool Spriter::getHitboxAtTime(int animIndex, const char * name, float time, Vec2 * points)
{
	if (animIndex == -1)
		return true;
	fassert(!m_spriter->m_scene->m_entities.empty());
	spriter::Hitbox hitbox;
	if (!m_spriter->m_scene->m_entities[0]->getHitboxAtTime(animIndex, name, time * 1000.f, hitbox))
		return false;

	const float x1 = 0.f;
	const float x2 = +hitbox.sx;
	const float y1 = 0.f;
	const float y2 = +hitbox.sy;

	Vec3 p1(x1, y1, 0.f);
	Vec3 p2(x2, y1, 0.f);
	Vec3 p3(x2, y2, 0.f);
	Vec3 p4(x1, y2, 0.f);

	Mat4x4 matT;
	Mat4x4 matR;
	Mat4x4 matS;
	
	matT.MakeTranslation(hitbox.x, hitbox.y, 0.f);
	matR.MakeRotationZ(-hitbox.angle / 180.f * M_PI);
	matS.MakeScaling(hitbox.scaleX, hitbox.scaleY, 1.f);

	Mat4x4 mat = matT * matR * matS;

	p1 = mat * p1;
	p2 = mat * p2;
	p3 = mat * p3;
	p4 = mat * p4;

	points[0] = Vec2(p1[0], p1[1]);
	points[1] = Vec2(p2[0], p2[1]);
	points[2] = Vec2(p3[0], p3[1]);
	points[3] = Vec2(p4[0], p4[1]);

	return true;
}

bool Spriter::hasCharacterMap(int index) const
{
	if (m_spriter->m_scene->m_entities.empty())
		return false;

	return m_spriter->m_scene->hasCharacterMap(index);
}

spriter::Scene * Spriter::getSpriterScene() const
{
	return m_spriter->m_scene;
}
