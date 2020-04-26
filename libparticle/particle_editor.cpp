#include "framework.h"
#include "nfd.h"
#include "particle.h"
#include "particle_editor.h"
#include "particle_framework.h"
#include "Path.h"
#include "tinyxml2.h"
#include "ui.h"
#include <math.h>

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

#ifdef _MSC_VER
	#ifndef PATH_MAX
		#define PATH_MAX _MAX_PATH
	#endif
#endif

using namespace tinyxml2;

// ui design
static const int kMenuWidth = 300;
static const int kMenuSpacing = 15;

struct ParticleEditorState
{
	// ui draw state
	bool forceUiRefreshRequested = false;

	// resources
	char basePath[PATH_MAX] = { };

	// library
	static const int kMaxParticleInfos = 6;
	ParticleEmitterInfo peiList[kMaxParticleInfos];
	ParticleInfo piList[kMaxParticleInfos];

	// current editing
	int activeEditingIndex = 0;
	ParticleEmitterInfo & activeParticleEmitterInfo() { return peiList[activeEditingIndex]; }
	ParticleInfo & getActiveParticleInfo() { return piList[activeEditingIndex]; }

	// preview
	ParticlePool pool[kMaxParticleInfos];
	ParticleEmitter particleEmitter[kMaxParticleInfos];
	ParticleCallbacks callbacks;

	static int randomInt(void * userData, int min, int max) { return min + (rand() % (max - min + 1)); }
	static float randomFloat(void * userData, float min, float max) { return min + (rand() % 4096) / 4095.f * (max - min); }
	static bool getEmitterByName(void * userData, const char * name, const ParticleEmitterInfo *& pei, const ParticleInfo *& pi, ParticlePool *& pool, ParticleEmitter *& pe)
	{
		ParticleEditorState & self = *(ParticleEditorState*)userData;
		
		for (int i = 0; i < kMaxParticleInfos; ++i)
		{
			if (!strcmp(name, self.peiList[i].name))
			{
				pei = &self.peiList[i];
				pi = &self.piList[i];
				pool = &self.pool[i];
				pe = &self.particleEmitter[i];
				return true;
			}
		}
		return false;
	}

	static bool checkCollision(void * userData, float x1, float y1, float x2, float y2, float & t, float & nx, float & ny)
	{
		//ParticleEditorState & self = *(ParticleEditorState*)userData;
		
		const float px = 0.f;
		const float py = 100.f; // todo : plane position and normal should be editable
		
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
			while (pool[i].head != nullptr)
				pool[i].freeParticle(pool[i].head);
		}
	}

	void refreshUi()
	{
		forceUiRefreshRequested = true;
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

	bool load(const char * path)
	{
		XMLDocument d;

		if (d.LoadFile(path) != XML_NO_ERROR)
		{
			return false;
		}
		else
		{
			const std::string directory = Path::GetDirectory(path);
			strcpy_s(basePath, sizeof(basePath), directory.c_str());
			
			for (int i = 0; i < kMaxParticleInfos; ++i)
			{
				peiList[i] = ParticleEmitterInfo();
				piList[i] = ParticleInfo();
				particleEmitter[i].clearParticles(pool[i]);
				fassert(pool[i].head == 0);
				fassert(pool[i].tail == 0);
				particleEmitter[i] = ParticleEmitter();
			}

			int peiIdx = 0;
			for (XMLElement * emitterElem = d.FirstChildElement("emitter"); emitterElem; emitterElem = emitterElem->NextSiblingElement("emitter"))
			{
				peiList[peiIdx++].load(emitterElem);
			}

			int piIdx = 0;
			for (XMLElement * particleElem = d.FirstChildElement("particle"); particleElem; particleElem = particleElem->NextSiblingElement("particle"))
			{
				piList[piIdx++].load(particleElem);
			}
			
			return true;
		}
	}

	void doMenu_LoadSave(Menu_LoadSave & menu, const float dt)
	{
		if (doButton("Load", 0.f, 1.f, true))
		{
			nfdchar_t * path = 0;
			nfdresult_t result = NFD_OpenDialog("pfx", "", &path);

			if (result == NFD_OKAY)
			{
				load(path);
				
				menu.activeFilename = path;
				
				free(path);
				path = nullptr;

				activeEditingIndex = 0;
				refreshUi();
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
				
				if (path != nullptr)
				{
					free(path);
					path = nullptr;
				}
			}

			if (!saveFilename.empty())
			{
				XMLPrinter p;

				for (int i = 0; i < kMaxParticleInfos; ++i)
				{
					p.OpenElement("emitter");
					{
						peiList[i].save(&p);
					}
					p.CloseElement();

					p.OpenElement("particle");
					{
						piList[i].save(&p);
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
				particleEmitter[i].restart(pool[i]);
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
				activeEditingIndex = i;
				refreshUi();
			}
		}
	}

	void doMenu_Pi(const float dt)
	{
		auto & pi = getActiveParticleInfo();
		
		if (doButton("Copy", 0.f, .5f, !g_copyPiIsValid))
		{
			g_copyPi = pi;
			g_copyPiIsValid = true;
		}
		
		if (g_copyPiIsValid && doButton("Paste", .5f, .5f, true))
		{
			pi = g_copyPi;
			refreshUi();
		}

		doTextBox(pi.rate, "Rate (Particles/Sec)", dt);

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
		doEnum(pi.shape, "Shape", shapeValues);
		
		doCheckBox(pi.randomDirection, "Random Direction", false);
		doTextBox(pi.circleRadius, "Circle Radius", dt);
		doTextBox(pi.boxSizeX, "Box Width", dt);
		doTextBox(pi.boxSizeY, "Box Height", dt);
		doCheckBox(pi.emitFromShell, "Emit From Shell", false);

		if (doCheckBox(pi.velocityOverLifetime, "Velocity Over Lifetime", true))
		{
			pushMenu("Velocity Over Lifetime");
			ScopedValueAdjust<int> xAdjust(g_drawX, +10);
			doTextBox(pi.velocityOverLifetimeValueX, "X", 0.f, .5f, false, dt);
			doTextBox(pi.velocityOverLifetimeValueY, "Y", .5f, .5f, true, dt);
			popMenu();
		}
		
		if (doCheckBox(pi.velocityOverLifetimeLimit, "Velocity Over Lifetime Limit", true))
		{
			pushMenu("Velocity Over Lifetime Limit");
			ScopedValueAdjust<int> xAdjust(g_drawX, +10);
			doParticleCurve(pi.velocityOverLifetimeLimitCurve, "Curve");
			doTextBox(pi.velocityOverLifetimeLimitDampen, "Dampen/Sec", dt);
			popMenu();
		}
		
		if (doCheckBox(pi.forceOverLifetime, "Force Over Lifetime", true))
		{
			pushMenu("Force Over Lifetime");
			ScopedValueAdjust<int> xAdjust(g_drawX, +10);
			doTextBox(pi.forceOverLifetimeValueX, "X", 0.f, .5f, false, dt);
			doTextBox(pi.forceOverLifetimeValueY, "Y", .5f, .5f, true, dt);
			popMenu();
		}
		
		if (doCheckBox(pi.colorOverLifetime, "Color Over Lifetime", true))
		{
			pushMenu("Color Over Lifetime");
			ScopedValueAdjust<int> xAdjust(g_drawX, +10);
			doParticleColorCurve(pi.colorOverLifetimeCurve, "Curve");
			popMenu();
		}
		
		if (doCheckBox(pi.colorBySpeed, "Color By Speed", true))
		{
			pushMenu("Color By Speed");
			ScopedValueAdjust<int> xAdjust(g_drawX, +10);
			doParticleColorCurve(pi.colorBySpeedCurve, "Curve");
			doTextBox(pi.colorBySpeedRangeMin, "Range", 0.f, .5f, false, dt);
			doTextBox(pi.colorBySpeedRangeMax, "", .5f, .5f, true, dt);
			popMenu();
		}
		
		if (doCheckBox(pi.sizeOverLifetime, "Size Over Lifetime", true))
		{
			pushMenu("Size Over Lifetime");
			ScopedValueAdjust<int> xAdjust(g_drawX, +10);
			doParticleCurve(pi.sizeOverLifetimeCurve, "Curve");
			popMenu();
		}
		
		if (doCheckBox(pi.sizeBySpeed, "Size By Speed", true))
		{
			pushMenu("Size By Speed");
			ScopedValueAdjust<int> xAdjust(g_drawX, +10);
			doParticleCurve(pi.sizeBySpeedCurve, "Curve");
			doTextBox(pi.sizeBySpeedRangeMin, "Range", 0.f, .5f, false, dt);
			doTextBox(pi.sizeBySpeedRangeMax, "", .5f, .5f, true, dt);
			popMenu();
		}
		
		if (doCheckBox(pi.rotationOverLifetime, "Rotation Over Lifetime", true))
		{
			pushMenu("Rotation Over Lifetime");
			ScopedValueAdjust<int> xAdjust(g_drawX, +10);
			doTextBox(pi.rotationOverLifetimeValue, "Degrees/Sec", dt);
			popMenu();
		}
		
		if (doCheckBox(pi.rotationBySpeed, "Rotation By Speed", true))
		{
			pushMenu("Rotation By Speed");
			ScopedValueAdjust<int> xAdjust(g_drawX, +10);
			doParticleCurve(pi.rotationBySpeedCurve, "Curve");
			doTextBox(pi.rotationBySpeedRangeMin, "Range", 0.f, .5f, false, dt);
			doTextBox(pi.rotationBySpeedRangeMax, "", .5f, .5f, true, dt);
			popMenu();
		}
		
		if (doCheckBox(pi.collision, "Collision", true))
		{
			pushMenu("Collision");
			ScopedValueAdjust<int> xAdjust(g_drawX, +10);
			doTextBox(pi.bounciness, "Bounciness", dt);
			doTextBox(pi.lifetimeLoss, "Lifetime Loss On Collision", dt);
			doTextBox(pi.minKillSpeed, "Kill Speed", dt);
			doTextBox(pi.collisionRadius, "Collision Radius", dt);
			popMenu();
		}
		
		if (doCheckBox(pi.enableSubEmitters, "Sub Emitters", true))
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
				
				for (int i = 0; i < ParticleInfo::kSubEmitterEvent_COUNT; ++i)
				{
					if (doCheckBox(pi.subEmitters[i].enabled, eventNames[i], true))
					{
						pushMenu(eventNames[i]);
						{
							ScopedValueAdjust<int> xAdjust(g_drawX, +10);
							doTextBox(pi.subEmitters[i].chance, "Spawn Chance", dt);
							doTextBox(pi.subEmitters[i].count, "Spawn Count", dt);

							std::string emitterName = pi.subEmitters[i].emitterName;
							doTextBox(emitterName, "Emitter Name", dt);
							strcpy_s(pi.subEmitters[i].emitterName, sizeof(pi.subEmitters[i].emitterName), emitterName.c_str());
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
		doEnum(pi.sortMode, "Sort Mode", sortModeValues);

		std::vector<EnumValue> blendModeValues;
		blendModeValues.push_back(EnumValue(ParticleInfo::kBlendMode_AlphaBlended, "Use Alpha"));
		blendModeValues.push_back(EnumValue(ParticleInfo::kBlendMode_Additive, "Additive"));
		doEnum(pi.blendMode, "Blend Mode", blendModeValues);
	}

	void doMenu_Pei(const float dt)
	{
		auto & pei = activeParticleEmitterInfo();
		
		if (doButton("Copy", 0.f, .5f, !g_copyPeiIsValid))
		{
			g_copyPei = pei;
			g_copyPeiIsValid = true;
		}
		
		if (g_copyPeiIsValid && doButton("Paste", .5f, .5f, true))
		{
			pei = g_copyPei;
			refreshUi();
		}
		
		std::string name;
		name = pei.name;
		doTextBox(name, "Name", dt);
		strcpy_s(pei.name, sizeof(pei.name), name.c_str());
		
		doTextBox(pei.duration, "Duration", dt);
		doCheckBox(pei.loop, "Loop", false);
		doCheckBox(pei.prewarm, "Prewarm", false);
		doTextBox(pei.startDelay, "Start Delay", dt);
		doTextBox(pei.startLifetime, "Start Lifetime", dt);
		doTextBox(pei.startSpeed, "Start Speed", dt);
		doTextBox(pei.startSpeedAngle, "Start Direction", dt);
		doTextBox(pei.startSize, "Start Size", dt);
		doTextBox(pei.startRotation, "Start Rotation", dt);
		doParticleColor(pei.startColor, "Start Color");
		doTextBox(pei.gravityMultiplier, "Gravity Multiplier", dt);
		doCheckBox(pei.inheritVelocity, "Inherit Velocity", false);
		doCheckBox(pei.worldSpace, "World Space", false);
		doTextBox(pei.maxParticles, "Max Particles", dt);
		
		std::string materialName = pei.materialName;
		doTextBox(materialName, "Material", dt);
		strcpy_s(pei.materialName, sizeof(pei.materialName), materialName.c_str());
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
		
		if (g_doActions && forceUiRefreshRequested)
		{
			forceUiRefreshRequested = false;
			
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
		callbacks.userData = this;
		callbacks.randomInt = randomInt;
		callbacks.randomFloat = randomFloat;
		callbacks.getEmitterByName = getEmitterByName;
		callbacks.checkCollision = checkCollision;

		piList[0].rate = 1.f;
		for (int i = 0; i < kMaxParticleInfos; ++i)
			strcpy_s(peiList[i].materialName, sizeof(peiList[i].materialName), "texture.png");
	}

	void tick(const bool menuActive, const float sx, const float sy, const float dt)
	{
		if (menuActive)
			doMenu(s_menu, true, false, sx, sy, dt);

		for (int i = 0; i < kMaxParticleInfos; ++i)
		{
			const float gravityX = 0.f;
			const float gravityY = 100.f;
			for (Particle * p = pool[i].head; p; )
				if (!tickParticle(callbacks, peiList[i], piList[i], dt, gravityX, gravityY, *p))
					p = pool[i].freeParticle(p);
				else
					p = p->next;
			tickParticleEmitter(callbacks, peiList[i], piList[i], pool[i], dt, gravityX, gravityY, particleEmitter[i]);
		}
	}

	void draw(const bool menuActive, const float sx, const float sy)
	{
		gxPushMatrix();
		gxTranslatef(sx/2.f, sy/2.f, 0.f);

	#if 1
	// todo : add option for drawing collision shapes
	
		auto & pi = getActiveParticleInfo();
		
		setColor(0, 255, 0, 31);
		switch (pi.shape)
		{
		case ParticleInfo::kShapeBox:
			drawRectLine(
				-pi.boxSizeX,
				-pi.boxSizeY,
				+pi.boxSizeX,
				+pi.boxSizeY);
			break;
		case ParticleInfo::kShapeCircle:
			drawRectLine(
				-pi.circleRadius,
				-pi.circleRadius,
				+pi.circleRadius,
				+pi.circleRadius);
			break;
		case ParticleInfo::kShapeEdge:
			drawLine(
				-pi.boxSizeX,
				0.f,
				+pi.boxSizeX,
				0.f);
			break;
		}

		drawParticles();
	#endif

		gxPopMatrix();

		if (menuActive)
			doMenu(s_menu, false, true, sx, sy, 0.f);
	}

	void drawParticles()
	{
		pushBlend(BLEND_OPAQUE);
		
		for (int i = 0; i < kMaxParticleInfos; ++i)
		{
			char materialPath[PATH_MAX];
			sprintf_s(materialPath, sizeof(materialPath), "%s/%s", basePath, peiList[i].materialName);
			
			gxSetTexture(Sprite(materialPath).getTexture());
			gxSetTextureSampler(GX_SAMPLE_LINEAR, true);

			if (piList[i].blendMode == ParticleInfo::kBlendMode_AlphaBlended)
				setBlend(BLEND_ALPHA);
			else if (piList[i].blendMode == ParticleInfo::kBlendMode_Additive)
				setBlend(BLEND_ADD);
			else
				fassert(false);

			gxBegin(GX_QUADS);
			{
				for (Particle * p = (piList[i].sortMode == ParticleInfo::kSortMode_OldestFirst) ? pool[i].head : pool[i].tail;
							 p; p = (piList[i].sortMode == ParticleInfo::kSortMode_OldestFirst) ? p->next : p->prev)
				{
					const float particleLife = 1.f - p->life;
					const float particleSpeed = p->speedScalar;

					ParticleColor color(true);
					computeParticleColor(peiList[i], piList[i], particleLife, particleSpeed, color);
					const float size_div_2 = computeParticleSize(peiList[i], piList[i], particleLife, particleSpeed) / 2.f;

					const float s = sinf(-p->rotation * float(M_PI) / 180.f);
					const float c = cosf(-p->rotation * float(M_PI) / 180.f);

					gxColor4fv(color.rgba);
					gxTexCoord2f(0.f, 1.f); gxVertex2f(p->position[0] + (- c - s) * size_div_2, p->position[1] + (+ s - c) * size_div_2);
					gxTexCoord2f(1.f, 1.f); gxVertex2f(p->position[0] + (+ c - s) * size_div_2, p->position[1] + (- s - c) * size_div_2);
					gxTexCoord2f(1.f, 0.f); gxVertex2f(p->position[0] + (+ c + s) * size_div_2, p->position[1] + (- s + c) * size_div_2);
					gxTexCoord2f(0.f, 0.f); gxVertex2f(p->position[0] + (- c + s) * size_div_2, p->position[1] + (+ s + c) * size_div_2);
				}
			}
			gxEnd();
			
			gxSetTextureSampler(GX_SAMPLE_NEAREST, false);
			gxSetTexture(0);
		}
		
		popBlend();
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

bool ParticleEditor::load(const char * filename)
{
	return state->load(filename);
}

void ParticleEditor::tick(const bool menuActive, const float sx, const float sy, const float dt)
{
	state->tick(menuActive, sx, sy, dt);
}

void ParticleEditor::draw(const bool menuActive, const float sx, const float sy)
{
	state->draw(menuActive, sx, sy);
}
