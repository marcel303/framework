#include "drawable.h"
#include "effect.h"
#include "framework.h"
#include "Parse.h"
#include "scene.h" 
#include "tinyxml2.h"
#include "types.h"
#include "xml.h"

using namespace tinyxml2;

// from internal.h
void splitString(const std::string & str, std::vector<std::string> & result, char c);

//

extern const int GFX_SX;
extern const int GFX_SY;

extern Config config;

//

Scene * g_currentScene = nullptr;
SceneLayer * g_currentSceneLayer = nullptr;
Surface * g_currentSurface = nullptr;

//

SceneEffect::SceneEffect()
	: m_effect(nullptr)
	, m_strength(1.f)
{
}

SceneEffect::~SceneEffect()
{
	m_name.clear();

	delete m_effect;
	m_effect = nullptr;

	m_strength = 0.f;
}

bool SceneEffect::load(const XMLElement * xmlEffect)
{
	m_name = stringAttrib(xmlEffect, "name", "");

	const std::string type = stringAttrib(xmlEffect, "type", "");

	std::string typeName = type;

	Effect * effect = nullptr;

	if (type == "fsfx")
	{
		const std::string shader = stringAttrib(xmlEffect, "shader", "");
		const std::string imageFiles = stringAttrib(xmlEffect, "images", "");
		std::vector<std::string> images;
		splitString(imageFiles, images, ',');

		if (shader.empty())
		{
			logWarning("shader not set. skipping effect");
		}
		else
		{
			effect = new Effect_Fsfx(m_name.c_str(), shader.c_str(), images);

			typeName = shader;
		}
	}
	else if (type == "rain")
	{
		const int numRaindrops = intAttrib(xmlEffect, "num_raindrops", 0);

		if (numRaindrops == 0)
		{
			logWarning("num_raindrops is 0. skipping effect");
		}
		else
		{
			effect = new Effect_Rain(m_name.c_str(), numRaindrops);
		}
	}
	else if (type == "stars")
	{
		const int numStars = intAttrib(xmlEffect, "num_stars", 0);

		if (numStars == 0)
		{
			logWarning("num_stars is 0. skipping effect");
		}
		else
		{
			effect = new Effect_StarCluster(m_name.c_str(), numStars);
		}
	}
	else if (type == "flowmap")
	{
		const std::string map = stringAttrib(xmlEffect, "map", "");

		if (map.empty())
		{
			logWarning("map not set. skipping effect");
		}
		else
		{
			effect = new Effect_Flowmap(m_name.c_str(), map.c_str());
		}
	}
	else if (type == "luminance")
	{
		effect = new Effect_Luminance(m_name.c_str());
	}
	else if (type == "colorlut2d")
	{
		const std::string lut = stringAttrib(xmlEffect, "lut", "");

		if (lut.empty())
		{
			logWarning("lut not set. skipping effect");
		}
		else
		{
			effect = new Effect_ColorLut2D(m_name.c_str(), lut.c_str());
		}
	}
	else if (type == "vignette")
	{
		effect = new Effect_Vignette(m_name.c_str());
	}
	else if (type == "video")
	{
		const std::string file = stringAttrib(xmlEffect, "file", "");
		const bool centered = boolAttrib(xmlEffect, "centered", true);

		if (file.empty())
		{
			logWarning("file not set. skipping effect");
		}
		else
		{
			effect = new Effect_Video(m_name.c_str(), file.c_str(), centered, false);
		}
	}
	else if (type == "picture")
	{
		const std::string file = stringAttrib(xmlEffect, "file", "");
		const bool centered = boolAttrib(xmlEffect, "centered", true);

		if (file.empty())
		{
			logWarning("file not set. skipping effect");
		}
		else
		{
			effect = new Effect_Picture(m_name.c_str(), file.c_str(), centered);
		}
	}
	else if (type == "draw_picture")
	{
		const std::string file = stringAttrib(xmlEffect, "file", "");

		if (file.empty())
		{
			logWarning("file not set. skipping effect");
		}
		else
		{
			effect = new Effect_DrawPicture(m_name.c_str(), file.c_str());
		}
	}
	else if (type == "blit")
	{
		const std::string layer = stringAttrib(xmlEffect, "layer", "");

		effect = new Effect_Blit(m_name.c_str(), layer.c_str());
	}
	else
	{
		logError("unknown effect type: %s", type.c_str());
	}

	if (effect != nullptr)
	{
		auto effectInfo = g_effectInfosByName.find(m_name);

		for (const XMLAttribute * xmlAttrib = xmlEffect->FirstAttribute(); xmlAttrib; xmlAttrib = xmlAttrib->Next())
		{
			std::string name = nameToEffectParam(typeName, xmlAttrib->Name());
			TweenFloat * var = effect->getVar(name.c_str());

			if (var != nullptr)
			{
				float value;

				if (xmlAttrib->QueryFloatValue(&value) == XML_SUCCESS)
				{
					*var = value;
				}
			}
		}

		effect->enabled = boolAttrib(xmlEffect, "enabled", true);

		effect->blendMode = parseBlendMode(stringAttrib(xmlEffect, "blend", "add"));

		m_effect = effect;

		return true;
	}
	else
	{
		return false;
	}
}

//

SceneLayer::SceneLayer(Scene * scene)
	: m_scene(scene)
	, m_blendMode(kBlendMode_Add)
	, m_autoClear(true)
	, m_clearColor(0.f, 0.f, 0.f, 0.f)
	, m_copyPreviousLayer(false)
	, m_copyPreviousLayerAlpha(1.f)
	, m_visible(1.f)
	, m_opacity(1.f)
	, m_surface(nullptr)
	, m_debugEnabled(true)
{
	addVar("copy_alpha", m_copyPreviousLayerAlpha);
	addVar("visible", m_visible);
	addVar("opacity", m_opacity);

	m_surface = new Surface(GFX_SX, GFX_SY);
}

SceneLayer::~SceneLayer()
{
	for (auto i = m_effects.begin(); i != m_effects.end(); ++i)
	{
		SceneEffect * effect = *i;

		delete effect;
		effect = nullptr;
	}

	m_effects.clear();

	delete m_surface;
	m_surface = nullptr;
}

bool SceneLayer::load(const XMLElement * xmlLayer)
{
	bool enabled = boolAttrib(xmlLayer, "enabled", true);

	m_name = stringAttrib(xmlLayer, "name", "");

	std::string blend = stringAttrib(xmlLayer, "blend", "add");

	m_blendMode = parseBlendMode(blend);
	m_autoClear = boolAttrib(xmlLayer, "auto_clear", true);
	m_copyPreviousLayer = boolAttrib(xmlLayer, "copy", false);
	m_copyPreviousLayerAlpha = floatAttrib(xmlLayer, "copy_alpha", 1.f);
	m_opacity = floatAttrib(xmlLayer, "opacity", 1.f);
	m_visible = floatAttrib(xmlLayer, "visible", 1.f);

	const std::string clearColor = stringAttrib(xmlLayer, "clear_color", "");

	if (!clearColor.empty())
		m_clearColor = Color::fromHex(clearColor.c_str());

	m_surface->clearf(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);

	//

	for (const XMLElement * xmlEffect = xmlLayer->FirstChildElement("effect"); xmlEffect; xmlEffect = xmlEffect->NextSiblingElement("effect"))
	{
		SceneEffect * effect = new SceneEffect();

		if (!effect->load(xmlEffect))
		{
			delete effect;
			effect = nullptr;
		}
		else
		{
			m_effects.push_back(effect);
		}
	}

	return enabled;
}

void SceneLayer::tick(const float dt)
{
	ScopedSceneLayerBlock block(this);

	TweenFloatCollection::tick(dt);

	for (auto i = m_effects.begin(); i != m_effects.end(); ++i)
	{
		SceneEffect * effect = *i;

		effect->m_effect->tick(dt);
	}
}

void SceneLayer::draw(DrawableList & list)
{
	struct SceneLayerDrawable : Drawable
	{
		SceneLayer * m_layer;

		SceneLayerDrawable(float z, SceneLayer * layer)
			: Drawable(z)
			, m_layer(layer)
		{
		}

		virtual void draw() override
		{
			m_layer->draw();
		}
	};

	if (m_debugEnabled)
	{
		new (list) SceneLayerDrawable(0.f, this);
	}
}

void SceneLayer::draw()
{
	ScopedSceneBlock sceneBlock(m_scene);
	ScopedSceneLayerBlock sceneLayerBlock(this);

	DrawableList drawableList;

	for (auto i = m_effects.begin(); i != m_effects.end(); ++i)
	{
		SceneEffect * effect = *i;

		effect->m_effect->draw(drawableList);
	}

	drawableList.sort();

	// render the effects

	if (m_copyPreviousLayer)
	{
		if (g_currentSurface)
		{
			if (m_copyPreviousLayerAlpha > 0.f)
			{
				pushSurface(m_surface);
				{
					if (m_copyPreviousLayerAlpha == 1.f)
						setBlend(BLEND_OPAQUE);
					else
						setBlend(BLEND_ALPHA);
					gxSetTexture(g_currentSurface->getTexture());
					gxColor4f(1.f, 1.f, 1.f, m_copyPreviousLayerAlpha);
					drawRect(0, 0, m_surface->getWidth(), m_surface->getHeight());
					gxSetTexture(0);
				}
				popSurface();
			}

			//g_currentSurface->blitTo(m_surface);
		}
		else
		{
			m_surface->clearf(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
		}
	}
	else if (m_autoClear)
	{
		m_surface->clearf(m_clearColor.r, m_clearColor.g, m_clearColor.b, m_clearColor.a);
	}

	pushSurface(m_surface);
	{
		ScopedSurfaceBlock surfaceBlock(m_surface);

		setColor(colorWhite);
		setBlend(BLEND_ADD);

		drawableList.draw();
	}
	popSurface();

	if (m_visible > 0.f)
	{
		// compose

		gxSetTexture(m_surface->getTexture());
		{
			Shader alphaTestShader("layer_compose_alphatest");

			switch (m_blendMode)
			{
			case kBlendMode_Add:
				setBlend(BLEND_ADD_OPAQUE);
				setColorf(1.f, 1.f, 1.f, 1.f, m_opacity);
				break;
			case kBlendMode_Subtract:
				setBlend(BLEND_SUBTRACT);
				setColorf(1.f, 1.f, 1.f, m_opacity);
				break;
			case kBlendMode_Alpha:
				setBlend(BLEND_ALPHA);
				setColorf(1.f, 1.f, 1.f, m_opacity);
				break;
			case kBlendMode_PremultipliedAlpha:
				setBlend(BLEND_PREMULTIPLIED_ALPHA);
				setColorf(m_opacity, m_opacity, m_opacity, m_opacity);
				break;
			case kBlendMode_Opaque:
				setBlend(BLEND_OPAQUE);
				setColorf(1.f, 1.f, 1.f, 1.f, m_opacity);
				break;
			case kBlendMode_AlphaTest:
				setBlend(BLEND_OPAQUE);
				alphaTestShader.setImmediate("alphaRef", 0.f);
				alphaTestShader.setTexture("srcColormap", 0, m_surface->getTexture(), false, true);
				alphaTestShader.setTexture("dstColormap", 0, g_currentSurface->getTexture(), false, true);
				setShader(alphaTestShader);
				checkErrorGL();
				break;
			default:
				Assert(false);
				break;
			}

			drawRect(0, 0, GFX_SX, GFX_SY);

			setBlend(BLEND_ADD);

			clearShader();
		}
		gxSetTexture(0);
	}
}

//

static EaseType parseEaseType(const std::string & easeType)
{
	EaseType result = kEaseType_Linear;

	if (easeType == "linear")
		result = kEaseType_Linear;
	else if (easeType == "pow_in")
		result = kEaseType_PowIn;
	else if (easeType == "pow_out")
		result = kEaseType_PowOut;
	else if (easeType == "sine_in")
		result = kEaseType_SineIn;
	else if (easeType == "sine_out")
		result = kEaseType_SineOut;
	else if (easeType == "sine_inout")
		result = kEaseType_SineInOut;
	else if (easeType == "back_in")
		result = kEaseType_BackIn;
	else if (easeType == "back_out")
		result = kEaseType_BackOut;
	else if (easeType == "bounce_in")
		result = kEaseType_BounceIn;
	else if (easeType == "bounce_out")
		result = kEaseType_BounceOut;
	else if (easeType == "bounce_inout")
		result = kEaseType_BounceInOut;
	else
		logError("unknown ease type: %s", easeType.c_str());

	return result;
}

SceneAction::SceneAction()
	: m_type(kActionType_None)
{
}

bool SceneAction::load(const XMLElement * xmlAction)
{
	const std::string type = xmlAction->Name();

	if (type == "tween")
	{
		m_type = kActionType_Tween;

		const std::string layer = stringAttrib(xmlAction, "layer", "");
		const std::string effect = stringAttrib(xmlAction, "effect", "");

		if (!layer.empty() + !effect.empty() > 1)
		{
			logError("more than one target type set on effect action!");
			return false;
		}

		if (!layer.empty())
		{
			m_tween.m_targetType = kTweenTargetType_Layer;
			m_tween.m_targetName = layer;
		}
		else if (!effect.empty())
		{
			m_tween.m_targetType = kTweenTargetType_Effect;
			m_tween.m_targetName = effect;
		}

		m_tween.m_replaceTweens = boolAttrib(xmlAction, "replace", false);
		m_tween.m_addTweens = boolAttrib(xmlAction, "add", false);

		//

		Tween::TweenSet tweenSet;

		tweenSet.m_tweenTime = 0.f;
		tweenSet.m_easeType = kEaseType_Linear;
		tweenSet.m_easeParam = 0.f;

		tweenSet.m_preDelay = floatAttrib(xmlAction, "pre_delay", 0.f);
		tweenSet.m_postDelay = floatAttrib(xmlAction, "post_delay", 0.f);

		for (const XMLAttribute * xmlAttrib = xmlAction->FirstAttribute(); xmlAttrib; xmlAttrib = xmlAttrib->Next())
		{
			if (!strcmp(xmlAttrib->Name(), "effect") || !strcmp(xmlAttrib->Name(), "layer"))
			{
			}
			else if (!strcmp(xmlAttrib->Name(), "time"))
			{
				const float time = xmlAttrib->FloatValue();

				tweenSet.m_tweenTime = time;

				m_tween.m_tweens.push_back(tweenSet);

				tweenSet = Tween::TweenSet();
			}
			else if (!strcmp(xmlAttrib->Name(), "ease"))
			{
				tweenSet.m_easeType = parseEaseType(xmlAttrib->Value());
			}
			else if (!strcmp(xmlAttrib->Name(), "ease_param"))
			{
				tweenSet.m_easeParam = xmlAttrib->FloatValue();
			}
			else
			{
				// must be a variable

				Tween::TweenSet::TweenVar var;

				var.m_varName = xmlAttrib->Name();
				var.m_tweenTo = xmlAttrib->FloatValue();
				var.m_preDelay = 0.f;

				for (const Tween::TweenSet & tweenSet : m_tween.m_tweens)
				{
					bool found = false;

					for (const Tween::TweenSet::TweenVar & tweenVar : tweenSet.m_vars)
						if (tweenVar.m_varName == var.m_varName)
							found = true;

					if (found)
						var.m_preDelay = 0.f;
					else
						var.m_preDelay += tweenSet.m_tweenTime;
				}

				tweenSet.m_vars.push_back(var);
			}
		}

		return true;
	}
	else if (type == "signal")
	{
		m_type = kActionType_Signal;

		const std::string effect = stringAttrib(xmlAction, "effect", "");
		const std::string message = stringAttrib(xmlAction, "message", "");

		if (effect.empty() || message.empty())
		{
			logError("effect or message not set!");
			return false;
		}

		m_signal.m_targetName = effect;
		m_signal.m_message = message;

		return true;
	}
	else
	{
		logError("unknown event action: %s", type.c_str());
		return false;
	}
}

//

SceneEvent::SceneEvent()
	: m_oscId(-1)
{
}

SceneEvent::~SceneEvent()
{
	m_name.clear();
	m_oscId = -1;

	for (auto i = m_actions.begin(); i != m_actions.end(); ++i)
	{
		SceneAction * action = *i;

		delete action;
		action = nullptr;
	}

	m_actions.clear();
}

void SceneEvent::execute(Scene & scene)
{
	for (auto i = m_actions.begin(); i != m_actions.end(); ++i)
	{
		SceneAction * action = *i;

		switch (action->m_type)
		{
		case SceneAction::kActionType_Tween:
		{
			TweenFloatCollection * varCollection = nullptr;

			switch (action->m_tween.m_targetType)
			{
			case SceneAction::kTweenTargetType_Layer:
			{
				for (auto l = scene.m_layers.begin(); l != scene.m_layers.end(); ++l)
				{
					SceneLayer * layer = *l;

					if (layer->m_name == action->m_tween.m_targetName)
					{
						varCollection = layer;
					}
				}
			}
			break;

			case SceneAction::kTweenTargetType_Effect:
			{
				SceneEffect * effect = scene.findEffectByName(action->m_tween.m_targetName.c_str());

				if (effect)
				{
					varCollection = effect->m_effect;
				}
			}
			break;

			default:
				Assert(false);
				break;
			}

			if (varCollection == 0)
			{
				logWarning("couldn't find tween target by name. name=%s", action->m_tween.m_targetName.c_str());
			}
			else
			{
				const SceneAction::Tween & tween = action->m_tween;

				for (const SceneAction::Tween::TweenSet & tweenSet : tween.m_tweens)
				{
					for (const SceneAction::Tween::TweenSet::TweenVar & tweenVar : tweenSet.m_vars)
					{
						TweenFloat * var = varCollection->getVar(tweenVar.m_varName.c_str());

						if (var == 0)
						{
							logWarning("couldn't find tween value by name. name=%s", tweenVar.m_varName.c_str());
						}
						else
						{
							if (tween.m_replaceTweens)
							{
								var->clear();
							}

							const float preDelay = tweenSet.m_preDelay + tweenVar.m_preDelay;

							if (preDelay > 0.f)
							{
								var->to(var->getFinalValue(), preDelay, kEaseType_Linear, 0.f);
							}

							float to =  tweenVar.m_tweenTo;

							if (tween.m_addTweens)
							{
								to += var->getFinalValue();
							}

							var->to(
								to,
								tweenSet.m_tweenTime,
								tweenSet.m_easeType,
								tweenSet.m_easeParam);

							if (tweenSet.m_postDelay > 0.f)
							{
								var->to(var->getFinalValue(), tweenSet.m_postDelay, kEaseType_Linear, 0.f);
							}
						}
					}
				}
			}
		}
		break;

		case SceneAction::kActionType_Signal:
		{
			SceneEffect * effect = scene.findEffectByName(action->m_signal.m_targetName.c_str());

			if (effect)
			{
				effect->m_effect->handleSignal(action->m_signal.m_message);
			}
		}
		break;

		default:
			Assert(false);
			break;
		}
	}
}

void SceneEvent::load(const XMLElement * xmlEvent)
{
	m_name = stringAttrib(xmlEvent, "name", "");
	m_oscId = intAttrib(xmlEvent, "osc_id", -1);

	for (const XMLElement * xmlAction = xmlEvent->FirstChildElement(); xmlAction; xmlAction = xmlAction->NextSiblingElement())
	{
		SceneAction * action = new SceneAction();

		if (!action->load(xmlAction))
		{
			delete action;
			action = nullptr;
		}
		else
		{
			m_actions.push_back(action);
		}
	}
}

//

Scene::Scene()
	: m_time(0.f)
	, m_varTime(0.f)
	, m_varPcmVolume(0.f)
{
	addVar("time", m_varTime);
	addVar("pcm_volume", m_varPcmVolume);
}

Scene::~Scene()
{
	clear();
}

void Scene::tick(const float dt)
{
	ScopedSceneBlock block(this);

	// process global variables

	m_varTime = m_time;
	m_varPcmVolume = g_pcmVolume;

	// process tween variables

	TweenFloatCollection::tick(dt);

	// process layers

	for (auto i = m_layers.begin(); i != m_layers.end(); ++i)
	{
		SceneLayer * layer = *i;

		layer->tick(dt);
	}

	// process MIDI input

	for (auto & map : m_midiMaps)
	{
		const bool g_live = false; // todo : add a global live variable

		if (g_live && !map.liveEnabled)
			continue;

		if (map.type == SceneMidiMap::kMapType_Event)
		{
			if (config.midiWentDown(map.id))
			{
				bool found = false;

				for (auto i = m_events.begin(); i != m_events.end(); ++i)
				{
					SceneEvent * event = *i;

					if (event->m_name == map.event)
					{
						event->execute(*this);

						found = true;
					}
				}

				if (!found)
				{
					logWarning("unable to find event %s", map.event.c_str());
				}
			}
		}
		else if (map.type == SceneMidiMap::kMapType_EffectVar)
		{
			if (config.midiIsDown(map.id))
			{
				SceneEffect * effect = findEffectByName(map.effect.c_str());

				if (effect != nullptr)
				{
					TweenFloat * var = effect->m_effect->getVar(map.var.c_str());

					if (var != nullptr)
					{
						if (!var->isActive())
						{
							const float value = config.midiGetValue(map.id, 0.f);

							*var = map.min + (map.max - map.min) * value;
						}
					}
					else
					{
						logWarning("unable to find effect variable %s", map.var.c_str());
					}
				}
				else
				{
					logWarning("unable to find effect %s", map.effect.c_str());
				}
			}
		}
	}

	//

	m_time += dt;

#if ENABLE_DEBUG_TEXT
	// process debug text

	for (auto i = m_debugTexts.begin(); i != m_debugTexts.end(); )
	{
		DebugText & t = *i;

		if (m_time >= t.endTime)
			i = m_debugTexts.erase(i);
		else
			++i;
	}
#endif
}

void Scene::draw(DrawableList & list)
{
	ScopedSceneBlock block(this);

	for (auto i = m_layers.begin(); i != m_layers.end(); ++i)
	{
		SceneLayer * layer = *i;

		layer->draw(list);
	}

	//

	struct SceneDebugDrawable : Drawable
	{
		Scene * m_scene;

		SceneDebugDrawable(float z, Scene * scene)
			: Drawable(z)
			, m_scene(scene)
		{
		}

		virtual void draw() override
		{
			m_scene->debugDraw();
		}
	};

	new (list) SceneDebugDrawable(0.f, this);
}

void Scene::debugDraw()
{
#if ENABLE_DEBUG_TEXT
	// draw debug text

	setFont("calibri.ttf");
	setColor(colorWhite);
	setBlend(BLEND_ALPHA);

	float x = GFX_SX / 2.f;
	float y = GFX_SY / 2.f;
	const int fontSize = 48;

	for (auto i = m_debugTexts.begin(); i != m_debugTexts.end(); ++i)
	{
		drawText(x, y, fontSize, 0.f, 0.f, i->text.c_str());

		y += fontSize + 6;
	}
#endif
}

SceneLayer * Scene::findLayerByName(const char * name)
{
	for (auto l = m_layers.begin(); l != m_layers.end(); ++l)
	{
		SceneLayer * layer = *l;

		if (layer->m_name == name)
			return layer;
	}

	return nullptr;
}

SceneEffect * Scene::findEffectByName(const char * name)
{
	for (auto l = m_layers.begin(); l != m_layers.end(); ++l)
	{
		SceneLayer * layer = *l;

		for (auto e = layer->m_effects.begin(); e != layer->m_effects.end(); ++e)
		{
			SceneEffect * effect = *e;

			if (effect->m_name == name)
			{
				return effect;
			}
		}
	}

	return nullptr;
}

void Scene::triggerEvent(const char * name)
{
	for (auto i = m_events.begin(); i != m_events.end(); ++i)
	{
		SceneEvent * event = *i;

		if (event->m_name == name)
		{
			event->execute(*this);

			addDebugText(event->m_name.c_str());
		}
	}
}

void Scene::triggerEventByOscId(int oscId)
{
	for (auto i = m_events.begin(); i != m_events.end(); ++i)
	{
		SceneEvent * event = *i;

		if (event->m_oscId == oscId)
		{
			event->execute(*this);

			addDebugText(event->m_name.c_str());
		}
	}
}

void Scene::addDebugText(const char * text)
{
#if ENABLE_DEBUG_TEXT
	DebugText t;
	t.text = text;
	t.endTime = m_time + 4.f;

	m_debugTexts.push_back(t);
#endif
}

bool Scene::load(const char * filename)
{
	bool result = true;

	m_filename = filename;

	tinyxml2::XMLDocument xmlDoc;

	if (xmlDoc.LoadFile(filename) != XML_NO_ERROR)
	{
		logError("failed to load %s", filename);

		result = false;
	}
	else
	{
		const XMLElement * xmlScene = xmlDoc.FirstChildElement("scene");

		if (xmlScene == 0)
		{
			logError("missing <scene> element");

			result = false;
		}
		else
		{
			m_name = stringAttrib(xmlScene, "name", "");

			if (m_name.empty())
			{
				logWarning("scene name not set!");
			}

			//

			const XMLElement * xmlLayers = xmlScene->FirstChildElement("layers");

			if (xmlLayers == 0)
			{
				logWarning("no layers found in scene!");
			}
			else
			{
				for (const XMLElement * xmlLayer = xmlLayers->FirstChildElement("layer"); xmlLayer; xmlLayer = xmlLayer->NextSiblingElement("layer"))
				{
					SceneLayer * layer = new SceneLayer(this);

					if (!layer->load(xmlLayer))
					{
						delete layer;
					}
					else
					{
						m_layers.push_back(layer);
					}
				}
			}

			//

			const XMLElement * xmlEvents = xmlScene->FirstChildElement("events");

			if (xmlEvents == 0)
			{
				logDebug("scene doesn't have any events");
			}
			else
			{
				for (const XMLElement * xmlEvent = xmlEvents->FirstChildElement("event"); xmlEvent; xmlEvent = xmlEvent->NextSiblingElement("event"))
				{
					SceneEvent * event = new SceneEvent();

					event->load(xmlEvent);

					m_events.push_back(event);
				}
			}

			//

			const XMLElement * xmlModifiers = xmlScene->FirstChildElement("modifiers");

			if (xmlModifiers == 0)
			{
				logDebug("scene doesn't have any modifiers");
			}
			else
			{
				for (const XMLElement * xmlModifier = xmlModifiers->FirstChildElement("modifier"); xmlModifier; xmlModifier = xmlModifier->NextSiblingElement("modifier"))
				{
					const bool enabled = boolAttrib(xmlModifier, "enabled", true);

					const std::string layerName  = stringAttrib(xmlModifier, "layer",  "");
					const std::string effectName = stringAttrib(xmlModifier, "effect", "");
					const std::string varName    = stringAttrib(xmlModifier, "var",    "");
					const std::string modName    = stringAttrib(xmlModifier, "mod",    "");
					const std::string op         = stringAttrib(xmlModifier, "op",     "");
					const std::string range      = stringAttrib(xmlModifier, "range",  "");

					TweenFloat * var = nullptr;
					
					if (!layerName.empty())
					{
						SceneLayer * layer = findLayerByName(layerName.c_str());
						if (layer)
							var = layer->getVar(varName.c_str());
						if (var == nullptr)
							logError("could not find var value %s:%s", layerName.c_str(), varName.c_str());
					}
					else if (!effectName.empty())
					{
						SceneEffect * effect = findEffectByName(effectName.c_str());
						if (effect)
							var = effect->m_effect->getVar(varName.c_str());
						if (var == nullptr)
							logError("could not find var value %s:%s", effectName.c_str(), varName.c_str());
					}

					if (var == nullptr)
					{
						//
					}
					else
					{
						TweenFloat * mod = getVar(modName.c_str());

						if (mod == nullptr)
						{
							logError("could not find global value %s", modName.c_str());
						}
						else
						{
							Modifier modifier;
							modifier.var = var;
							modifier.mod = mod;
							modifier.str = floatAttrib(xmlModifier, "strength", 1.f);
							modifier.op = Modifier::parseOp(op);

							if (!range.empty())
							{
								std::vector<std::string> values;
								splitString(range, values, ',');

								if (values.size() != 4)
								{
									logError("invalid ranges");
								}
								else
								{
									modifier.hasRange = true;
									modifier.range[0] = Parse::Float(values[0]);
									modifier.range[1] = Parse::Float(values[1]);
									modifier.range[2] = Parse::Float(values[2]);
									modifier.range[3] = Parse::Float(values[3]);
								}
							}

							if (enabled)
							{
								var->addModifier(this);

								m_modifiers.push_back(modifier);
							}
						}
					}
				}
			}

			//

			const XMLElement * xmlMidi = xmlScene->FirstChildElement("midi");

			if (xmlMidi == 0)
			{
				logDebug("scene doesn't have any MIDI mappings");
			}
			else
			{
				for (const XMLElement * xmlMidiMap = xmlMidi->FirstChildElement("map"); xmlMidiMap; xmlMidiMap = xmlMidiMap->NextSiblingElement("map"))
				{
					SceneMidiMap map;

					map.id = intAttrib(xmlMidiMap, "id", -1);

					if (map.id < 0)
						continue;

					map.liveEnabled = boolAttrib(xmlMidiMap, "live", false);

					map.event = stringAttrib(xmlMidiMap, "event", "");
					map.effect = stringAttrib(xmlMidiMap, "effect", "");
					map.var = stringAttrib(xmlMidiMap, "var", "");
					map.min = floatAttrib(xmlMidiMap, "min", 0.f);
					map.max = floatAttrib(xmlMidiMap, "max", 1.f);

					if (!map.effect.empty() && !map.var.empty())
					{
						map.type = SceneMidiMap::kMapType_EffectVar;
					}
					else if (!map.event.empty())
					{
						map.type = SceneMidiMap::kMapType_Event;
					}
					else
					{
						continue;
					}

					m_midiMaps.push_back(map);
				}
			}
		}
	}

	return result;
}

void Scene::clear()
{
	m_filename.clear();
	m_name.clear();

	for (auto i = m_layers.begin(); i != m_layers.end(); ++i)
	{
		SceneLayer * layer = *i;

		delete layer;
		layer = nullptr;
	}

	m_layers.clear();

	//

	for (auto i = m_events.begin(); i != m_events.end(); ++i)
	{
		SceneEvent * event = *i;

		delete event;
		event = nullptr;
	}

	m_events.clear();

	//

	m_midiMaps.clear();

	//

	m_time = 0.f;

	//

#if ENABLE_DEBUG_TEXT
	m_debugTexts.clear();
#endif
}

bool Scene::reload()
{
	const std::string filename = m_filename;

	clear();

	return load(filename.c_str());
}

float Scene::applyModifier(TweenFloat * tweenFloat, float value)
{
	for (Modifier & modifier : m_modifiers)
	{
		if (modifier.var == tweenFloat)
		{
			value = modifier.apply(value);
		}
	}

	return value;
}
