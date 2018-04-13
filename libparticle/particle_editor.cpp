#include "framework.h"
#include "nfd.h"
#include "particle.h"
#include "particle_editor.h"
#include "particle_framework.h"
#include "tinyxml2.h"
#include "ui.h"
#include <cmath>

#include "StringEx.h" // _s functions

/*

+ color curve key highlight on hover
+ color curve key color on inactive
+ make UiElem from color picker
+ implement color picker fromColor
+ add alpha selection color picker
+ fix unable to select alpha when mouse is outside bar
+ fix save and 'save as' functionality

+ fix color curve key select = move
+ move color picker and emitter info to the right side of the screen
+ fix 'loop' toggle behavior
+ add 'restart simulation' button
+ add support for multiple particle emitters
- add support for subemitters
+ fix subemitter UI reusing elems etc
+ fix elems not refreshing when not drawn at refresh frame. use version number on elems?

+ add copy & paste buttons pi, pei

*/

using namespace tinyxml2;

// ui design
static const int kMenuWidth = 300;
static const int kMenuSpacing = 15;

struct ParticleEditorState
{
// ui draw state
bool g_forceUiRefreshRequested = false;

// library
static const int kMaxParticleInfos = 6;
ParticleEmitterInfo g_peiList[kMaxParticleInfos];
ParticleInfo g_piList[kMaxParticleInfos];

// current editing
int g_activeEditingIndex = 0;
ParticleEmitterInfo & g_pei() { return g_peiList[g_activeEditingIndex]; }
ParticleInfo & g_pi() { return g_piList[g_activeEditingIndex]; }

// preview
ParticlePool g_pool[kMaxParticleInfos];
ParticleEmitter g_pe[kMaxParticleInfos];
ParticleCallbacks g_callbacks;

static int randomInt(void * userData, int min, int max) { return min + (rand() % (max - min + 1)); }
static float randomFloat(void * userData, float min, float max) { return min + (rand() % 4096) / 4095.f * (max - min); }
static bool getEmitterByName(void * userData, const char * name, const ParticleEmitterInfo *& pei, const ParticleInfo *& pi, ParticlePool *& pool, ParticleEmitter *& pe)
{
	ParticleEditorState & self = *(ParticleEditorState*)userData;
	
	for (int i = 0; i < kMaxParticleInfos; ++i)
	{
		if (!strcmp(name, self.g_peiList[i].name))
		{
			pei = &self.g_peiList[i];
			pi = &self.g_piList[i];
			pool = &self.g_pool[i];
			pe = &self.g_pe[i];
			return true;
		}
	}
	return false;
}

static bool checkCollision(void * userData, float x1, float y1, float x2, float y2, float & t, float & nx, float & ny)
{
	//ParticleEditorState & self = *(ParticleEditorState*)userData;
	
	const float px = 0.f;
	const float py = 100.f;
	const float pnx = 0.f;
	const float pny = -1.f;
	const float pd = px * pnx + py * pny;
	const float d1 = x1 * pnx + y1 * pny - pd;
	const float d2 = x2 * pnx + y2 * pny - pd;
	if (d1 >= 0.f && d2 < 0.f)
	{
		t = -d1 / (d2 - d1);
		nx = pnx;
		ny = pny;
		return true;
	}
	else
	{
		return false;
	}
}

// copy & paste
ParticleEmitterInfo g_copyPei;
bool g_copyPeiIsValid = false;
ParticleInfo g_copyPi;
bool g_copyPiIsValid = false;

//

static inline float lerp(const float v1, const float v2, const float t)
{
	return v1 * (1.f - t) + v2 * t;
}

static inline float saturate(const float v)
{
	return v < 0.f ? 0.f : v > 1.f ? 1.f : v;
}

template<typename T>
struct ScopedValueAdjust
{
	T m_oldValue;
	T & m_value;
	ScopedValueAdjust(T & value, T amount) : m_oldValue(value), m_value(value) { m_value += amount; }
	~ScopedValueAdjust() { m_value = m_oldValue; }
};

//

~ParticleEditorState()
{
	for (int i = 0; i < kMaxParticleInfos; ++i)
	{
		ParticlePool & pool = g_pool[i];
		
		while (pool.head != nullptr)
			pool.freeParticle(pool.head);
	}
}

void refreshUi()
{
	g_forceUiRefreshRequested = true;
}

//

struct Menu_LoadSave
{
	std::string activeFilename;
	
	Menu_LoadSave()
		: activeFilename()
	{
	}
};

void doMenu_LoadSave(Menu_LoadSave & menu, const float dt)
{
	if (doButton("Load", 0.f, 1.f, true))
	{
		nfdchar_t * path = 0;
		nfdresult_t result = NFD_OpenDialog("pfx", "", &path);

		if (result == NFD_OKAY)
		{
			XMLDocument d;

			if (d.LoadFile(path) == XML_NO_ERROR)
			{
				for (int i = 0; i < kMaxParticleInfos; ++i)
				{
					g_peiList[i] = ParticleEmitterInfo();
					g_piList[i] = ParticleInfo();
					g_pe[i].clearParticles(g_pool[i]);
					fassert(g_pool[i].head == 0);
					fassert(g_pool[i].tail == 0);
					g_pe[i] = ParticleEmitter();
				}

				int peiIdx = 0;
				for (XMLElement * emitterElem = d.FirstChildElement("emitter"); emitterElem; emitterElem = emitterElem->NextSiblingElement("emitter"))
				{
					g_peiList[peiIdx++].load(emitterElem);
				}

				int piIdx = 0;
				for (XMLElement * particleElem = d.FirstChildElement("particle"); particleElem; particleElem = particleElem->NextSiblingElement("particle"))
				{
					g_piList[piIdx++].load(particleElem);
				}

				menu.activeFilename = path;

				g_activeEditingIndex = 0;
				refreshUi();
			}
		}
	}

	bool save = false;
	std::string saveFilename;
	
	if (doButton("Save", 0.f, 1.f, true))
	{
		save = true;
		saveFilename = menu.activeFilename;
	}
	
	if (doButton("Save as..", 0.f, 1.f, true))
	{
		save = true;
	}

	if (save)
	{
		if (saveFilename.empty())
		{
			nfdchar_t * path = 0;
			nfdresult_t result = NFD_SaveDialog("pfx", "", &path);

			if (result == NFD_OKAY)
			{
				saveFilename = path;
			}
		}

		if (!saveFilename.empty())
		{
			XMLPrinter p;

			for (int i = 0; i < kMaxParticleInfos; ++i)
			{
				p.OpenElement("emitter");
				{
					g_peiList[i].save(&p);
				}
				p.CloseElement();

				p.OpenElement("particle");
				{
					g_piList[i].save(&p);
				}
				p.CloseElement();
			}

			XMLDocument d;
			d.Parse(p.CStr());
			d.SaveFile(saveFilename.c_str());

			menu.activeFilename = saveFilename;
		}
	}
	
	if (doButton("Restart simulation", 0.f, 1.f, true))
	{
		for (int i = 0; i < kMaxParticleInfos; ++i)
			g_pe[i].restart(g_pool[i]);
	}
}

void doMenu_EmitterSelect(const float dt)
{
	for (int i = 0; i < 6; ++i)
	{
		char name[32];
		sprintf_s(name, sizeof(name), "System %d", i + 1);
		if (doButton(name, 0.f, 1.f, true))
		{
			g_activeEditingIndex = i;
			refreshUi();
		}
	}
}

void doMenu_Pi(const float dt)
{
	if (doButton("Copy", 0.f, .5f, !g_copyPiIsValid))
	{
		g_copyPi = g_pi();
		g_copyPiIsValid = true;
	}
	
	if (g_copyPiIsValid && doButton("Paste", .5f, .5f, true))
	{
		g_pi() = g_copyPi;
		refreshUi();
	}

	doTextBox(g_pi().rate, "Rate (Particles/Sec)", dt);

	/*
	struct Burst
	{
		Burst()
			: time(0.f)
			, numParticles(10)
		{
		}

		float time;
		int numParticles;
	} bursts[kMaxBursts];
	int numBursts; // Allows extra particles to be emitted at specified times (only available when the Rate is in Time mode).
	*/

	std::vector<EnumValue> shapeValues;
	shapeValues.push_back(EnumValue('e', "Edge"));
	shapeValues.push_back(EnumValue('b', "Box"));
	shapeValues.push_back(EnumValue('c', "Circle"));
	doEnum(g_pi().shape, "Shape", shapeValues);
	
	doCheckBox(g_pi().randomDirection, "Random Direction", false);
	doTextBox(g_pi().circleRadius, "Circle Radius", dt);
	doTextBox(g_pi().boxSizeX, "Box Width", dt);
	doTextBox(g_pi().boxSizeY, "Box Height", dt);
	doCheckBox(g_pi().emitFromShell, "Emit From Shell", false);

	if (doCheckBox(g_pi().velocityOverLifetime, "Velocity Over Lifetime", true))
	{
		pushMenu("Velocity Over Lifetime");
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		doTextBox(g_pi().velocityOverLifetimeValueX, "X", 0.f, .5f, false, dt);
		doTextBox(g_pi().velocityOverLifetimeValueY, "Y", .5f, .5f, true, dt);
		popMenu();
	}
	
	if (doCheckBox(g_pi().velocityOverLifetimeLimit, "Velocity Over Lifetime Limit", true))
	{
		pushMenu("Velocity Over Lifetime Limit");
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		doParticleCurve(g_pi().velocityOverLifetimeLimitCurve, "Curve");
		doTextBox(g_pi().velocityOverLifetimeLimitDampen, "Dampen/Sec", dt);
		popMenu();
	}
	
	if (doCheckBox(g_pi().forceOverLifetime, "Force Over Lifetime", true))
	{
		pushMenu("Force Over Lifetime");
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		doTextBox(g_pi().forceOverLifetimeValueX, "X", 0.f, .5f, false, dt);
		doTextBox(g_pi().forceOverLifetimeValueY, "Y", .5f, .5f, true, dt);
		popMenu();
	}
	
	if (doCheckBox(g_pi().colorOverLifetime, "Color Over Lifetime", true))
	{
		pushMenu("Color Over Lifetime");
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		doParticleColorCurve(g_pi().colorOverLifetimeCurve, "Curve");
		popMenu();
	}
	
	if (doCheckBox(g_pi().colorBySpeed, "Color By Speed", true))
	{
		pushMenu("Color By Speed");
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		doParticleColorCurve(g_pi().colorBySpeedCurve, "Curve");
		doTextBox(g_pi().colorBySpeedRangeMin, "Range", 0.f, .5f, false, dt);
		doTextBox(g_pi().colorBySpeedRangeMax, "", .5f, .5f, true, dt);
		popMenu();
	}
	
	if (doCheckBox(g_pi().sizeOverLifetime, "Size Over Lifetime", true))
	{
		pushMenu("Size Over Lifetime");
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		doParticleCurve(g_pi().sizeOverLifetimeCurve, "Curve");
		popMenu();
	}
	
	if (doCheckBox(g_pi().sizeBySpeed, "Size By Speed", true))
	{
		pushMenu("Size By Speed");
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		doParticleCurve(g_pi().sizeBySpeedCurve, "Curve");
		doTextBox(g_pi().sizeBySpeedRangeMin, "Range", 0.f, .5f, false, dt);
		doTextBox(g_pi().sizeBySpeedRangeMax, "", .5f, .5f, true, dt);
		popMenu();
	}
	
	if (doCheckBox(g_pi().rotationOverLifetime, "Rotation Over Lifetime", true))
	{
		pushMenu("Rotation Over Lifetime");
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		doTextBox(g_pi().rotationOverLifetimeValue, "Degrees/Sec", dt);
		popMenu();
	}
	
	if (doCheckBox(g_pi().rotationBySpeed, "Rotation By Speed", true))
	{
		pushMenu("Rotation By Speed");
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		doParticleCurve(g_pi().rotationBySpeedCurve, "Curve");
		doTextBox(g_pi().rotationBySpeedRangeMin, "Range", 0.f, .5f, false, dt);
		doTextBox(g_pi().rotationBySpeedRangeMax, "", .5f, .5f, true, dt);
		popMenu();
	}
	
	if (doCheckBox(g_pi().collision, "Collision", true))
	{
		pushMenu("Collision");
		ScopedValueAdjust<int> xAdjust(g_drawX, +10);
		doTextBox(g_pi().bounciness, "Bounciness", dt);
		doTextBox(g_pi().lifetimeLoss, "Lifetime Loss On Collision", dt);
		doTextBox(g_pi().minKillSpeed, "Kill Speed", dt);
		doTextBox(g_pi().collisionRadius, "Collision Radius", dt);
		popMenu();
	}
	
	if (doCheckBox(g_pi().enableSubEmitters, "Sub Emitters", true))
	{
		pushMenu("Sub Emitters");
		{
			ScopedValueAdjust<int> xAdjust(g_drawX, +10);
			
			const char * eventNames[ParticleInfo::kSubEmitterEvent_COUNT] =
			{
				"onBirth",
				"onCollision",
				"onDeath"
			};
			
			for (int i = 0; i < ParticleInfo::kSubEmitterEvent_COUNT; ++i) // fixme : cannot use loops
			{
				if (doCheckBox(g_pi().subEmitters[i].enabled, eventNames[i], true))
				{
					pushMenu(eventNames[i]);
					{
						ScopedValueAdjust<int> xAdjust(g_drawX, +10);
						doTextBox(g_pi().subEmitters[i].chance, "Spawn Chance", dt);
						doTextBox(g_pi().subEmitters[i].count, "Spawn Count", dt);

						std::string emitterName = g_pi().subEmitters[i].emitterName;
						doTextBox(emitterName, "Emitter Name", dt);
						strcpy_s(g_pi().subEmitters[i].emitterName, sizeof(g_pi().subEmitters[i].emitterName), emitterName.c_str());
					}
					popMenu();
				}
			}
			//SubEmitter onBirth;
			//SubEmitter onCollision;
			//SubEmitter onDeath;
		}
		popMenu();
	}

	std::vector<EnumValue> sortModeValues;
	sortModeValues.push_back(EnumValue(ParticleInfo::kSortMode_OldestFirst, "Oldest First"));
	sortModeValues.push_back(EnumValue(ParticleInfo::kSortMode_YoungestFirst, "Youngest First"));
	doEnum(g_pi().sortMode, "Sort Mode", sortModeValues);

	std::vector<EnumValue> blendModeValues;
	blendModeValues.push_back(EnumValue(ParticleInfo::kBlendMode_AlphaBlended, "Use Alpha"));
	blendModeValues.push_back(EnumValue(ParticleInfo::kBlendMode_Additive, "Additive"));
	doEnum(g_pi().blendMode, "Blend Mode", blendModeValues);
}

void doMenu_Pei(const float dt)
{
	if (doButton("Copy", 0.f, .5f, !g_copyPeiIsValid))
	{
		g_copyPei = g_pei();
		g_copyPeiIsValid = true;
	}
	
	if (g_copyPeiIsValid && doButton("Paste", .5f, .5f, true))
	{
		g_pei() = g_copyPei;
		refreshUi();
	}
	
	std::string name;
	name = g_pei().name;
	doTextBox(name, "Name", dt);
	strcpy_s(g_pei().name, sizeof(g_pei().name), name.c_str());
	
	doTextBox(g_pei().duration, "Duration", dt);
	doCheckBox(g_pei().loop, "Loop", false);
	doCheckBox(g_pei().prewarm, "Prewarm", false);
	doTextBox(g_pei().startDelay, "Start Delay", dt);
	doTextBox(g_pei().startLifetime, "Start Lifetime", dt);
	doTextBox(g_pei().startSpeed, "Start Speed", dt);
	doTextBox(g_pei().startSpeedAngle, "Start Direction", dt);
	doTextBox(g_pei().startSize, "Start Size", dt);
	doTextBox(g_pei().startRotation, "Start Rotation", dt); // todo : min and max values for random start rotation?
	doParticleColor(g_pei().startColor, "Start Color");
	doTextBox(g_pei().gravityMultiplier, "Gravity Multiplier", dt);
	doCheckBox(g_pei().inheritVelocity, "Inherit Velocity", false);
	doCheckBox(g_pei().worldSpace, "World Space", false);
	doTextBox(g_pei().maxParticles, "Max Particles", dt);
	
	std::string materialName = g_pei().materialName;
	doTextBox(materialName, "Material", dt);
	strcpy_s(g_pei().materialName, sizeof(g_pei().materialName), materialName.c_str());
}

void doMenu_ColorWheel(const float dt)
{
	if (g_uiState->activeColor)
	{
		doColorWheel(*g_uiState->activeColor, "colorwheel", dt);
	}
}

struct Menu
{
	Menu_LoadSave loadSave;
	
	Menu()
		: loadSave()
	{
	}
};

Menu s_menu;
UiState s_uiState;

void doMenu(Menu & menu, const bool doActions, const bool doDraw, const int sx, const int sy, const float dt)
{
	makeActive(&s_uiState, doActions, doDraw);
	pushMenu("", kMenuWidth);
	
	if (g_doActions && g_forceUiRefreshRequested)
	{
		g_forceUiRefreshRequested = false;
		
		g_uiState->reset();
	}

	// left side menu

	g_drawX = 10;
	g_drawY = 0;
	
	pushMenu("pi");
	doMenu_Pi(dt);
	popMenu();

	// right side menu
	
	g_drawX = sx - kMenuWidth - 10;
	g_drawY = 0;
	
	pushMenu("loadSave");
	doMenu_LoadSave(menu.loadSave, dt);
	g_drawY += kMenuSpacing;
	popMenu();

	pushMenu("emitterSelect");
	doMenu_EmitterSelect(dt);
	g_drawY += kMenuSpacing;
	popMenu();
	
	pushMenu("pei");
	doMenu_Pei(dt);
	g_drawY += kMenuSpacing;
	popMenu();

	pushMenu("colorWheel");
	doMenu_ColorWheel(dt);
	g_drawY += kMenuSpacing;
	popMenu();
	
	if (g_uiState->activeElem == nullptr)
		g_uiState->activeColor = nullptr;

	popMenu();
}

ParticleEditorState()
{
	g_callbacks.userData = this;
	g_callbacks.randomInt = randomInt;
	g_callbacks.randomFloat = randomFloat;
	g_callbacks.getEmitterByName = getEmitterByName;
	g_callbacks.checkCollision = checkCollision;

	g_piList[0].rate = 1.f;
	for (int i = 0; i < kMaxParticleInfos; ++i)
		strcpy_s(g_peiList[i].materialName, sizeof(g_peiList[i].materialName), "texture.png");
}

void tick(const bool menuActive, const float sx, const float sy, const float dt)
{
	if (menuActive)
		doMenu(s_menu, true, false, sx, sy, dt);

	for (int i = 0; i < kMaxParticleInfos; ++i)
	{
		const float gravityX = 0.f;
		const float gravityY = 100.f;
		for (Particle * p = g_pool[i].head; p; )
			if (!tickParticle(g_callbacks, g_peiList[i], g_piList[i], dt, gravityX, gravityY, *p))
				p = g_pool[i].freeParticle(p);
			else
				p = p->next;
		tickParticleEmitter(g_callbacks, g_peiList[i], g_piList[i], g_pool[i], dt, gravityX, gravityY, g_pe[i]);
	}
}

void draw(const bool menuActive, const float sx, const float sy)
{
	gxPushMatrix();
	gxTranslatef(sx/2.f, sy/2.f, 0.f);

#if 1
	setColor(0, 255, 0, 31);
	switch (g_pi().shape)
	{
	case ParticleInfo::kShapeBox:
		drawRectLine(
			-g_pi().boxSizeX,
			-g_pi().boxSizeY,
			+g_pi().boxSizeX,
			+g_pi().boxSizeY);
		break;
	case ParticleInfo::kShapeCircle:
		drawRectLine(
			-g_pi().circleRadius,
			-g_pi().circleRadius,
			+g_pi().circleRadius,
			+g_pi().circleRadius);
		break;
	case ParticleInfo::kShapeEdge:
		drawLine(
			-g_pi().boxSizeX,
			0.f,
			+g_pi().boxSizeX,
			0.f);
		break;
	}

	for (int i = 0; i < kMaxParticleInfos; ++i)
	{
		gxSetTexture(Sprite(g_peiList[i].materialName).getTexture());
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (g_piList[i].blendMode == ParticleInfo::kBlendMode_AlphaBlended)
			setBlend(BLEND_ALPHA);
		else if (g_piList[i].blendMode == ParticleInfo::kBlendMode_Additive)
			setBlend(BLEND_ADD);
		else
			fassert(false);

		//if (rand() % 2)
		if (true)
		{
			gxBegin(GL_QUADS);
			{
				for (Particle * p = (g_piList[i].sortMode == ParticleInfo::kSortMode_OldestFirst) ? g_pool[i].head : g_pool[i].tail;
							 p; p = (g_piList[i].sortMode == ParticleInfo::kSortMode_OldestFirst) ? p->next : p->prev)
				{
					const float particleLife = 1.f - p->life;
					//const float particleSpeed = std::sqrtf(p->speed[0] * p->speed[0] + p->speed[1] * p->speed[1]);
					const float particleSpeed = p->speedScalar;

					ParticleColor color(true);
					computeParticleColor(g_peiList[i], g_piList[i], particleLife, particleSpeed, color);
					const float size_div_2 = computeParticleSize(g_peiList[i], g_piList[i], particleLife, particleSpeed) / 2.f;

					const float s = std::sinf(-p->rotation * float(M_PI) / 180.f);
					const float c = std::cosf(-p->rotation * float(M_PI) / 180.f);

					gxColor4fv(color.rgba);
					gxTexCoord2f(0.f, 1.f); gxVertex2f(p->position[0] + (- c - s) * size_div_2, p->position[1] + (+ s - c) * size_div_2);
					gxTexCoord2f(1.f, 1.f); gxVertex2f(p->position[0] + (+ c - s) * size_div_2, p->position[1] + (- s - c) * size_div_2);
					gxTexCoord2f(1.f, 0.f); gxVertex2f(p->position[0] + (+ c + s) * size_div_2, p->position[1] + (- s + c) * size_div_2);
					gxTexCoord2f(0.f, 0.f); gxVertex2f(p->position[0] + (- c + s) * size_div_2, p->position[1] + (+ s + c) * size_div_2);
				}
			}
			gxEnd();
		}
		else
		{
			for (Particle * p = (g_piList[i].sortMode == ParticleInfo::kSortMode_OldestFirst) ? g_pool[i].head : g_pool[i].tail;
						 p; p = (g_piList[i].sortMode == ParticleInfo::kSortMode_OldestFirst) ? p->next : p->prev)
			{
				const float particleLife = 1.f - p->life;
				const float particleSpeed = std::sqrtf(p->speed[0] * p->speed[0] + p->speed[1] * p->speed[1]);

				ParticleColor color;
				computeParticleColor(g_peiList[i], g_piList[i], particleLife, particleSpeed, color);
				const float size = computeParticleSize(g_peiList[i], g_piList[i], particleLife, particleSpeed);
				gxPushMatrix();
				gxTranslatef(
					p->position[0],
					p->position[1],
					0.f);
				gxRotatef(p->rotation, 0.f, 0.f, 1.f);
				setColorf(color.rgba[0], color.rgba[1], color.rgba[2], color.rgba[3]);
				drawRect(
					- size / 2.f,
					- size / 2.f,
					+ size / 2.f,
					+ size / 2.f);
				gxPopMatrix();
			}
		}
		gxSetTexture(0);
	}

	setBlend(BLEND_ALPHA);
#endif

	gxPopMatrix();

	if (menuActive)
		doMenu(s_menu, false, true, sx, sy, 0.f);
}
};

ParticleEditor::ParticleEditor()
	: state(nullptr)
{
	state = new ParticleEditorState();
}

ParticleEditor::~ParticleEditor()
{
	delete state;
	state = nullptr;
}

void ParticleEditor::tick(const bool menuActive, const float sx, const float sy, const float dt)
{
	state->tick(menuActive, sx, sy, dt);
}

void ParticleEditor::draw(const bool menuActive, const float sx, const float sy)
{
	state->draw(menuActive, sx, sy);
}
