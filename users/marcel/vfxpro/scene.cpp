#include "drawable.h"
#include "effect.h"
#include "framework.h"
#include "scene.h" 
#include "tinyxml2.h"
#include "types.h"
#include "xml.h"

using namespace tinyxml2;

//

extern const int GFX_SX;
extern const int GFX_SY;

extern Config config;

//

Scene * g_currentScene = nullptr;
SceneLayer * g_currentSceneLayer = nullptr;
Surface * g_currentSurface = nullptr;

//

static SceneLayer::BlendMode parseBlendMode(const std::string & blend)
{
	if (blend == "add")
		return SceneLayer::kBlendMode_Add;
	else if (blend == "subtract")
		return SceneLayer::kBlendMode_Subtract;
	else if (blend == "alpha")
		return SceneLayer::kBlendMode_Alpha;
	else if (blend == "opaque")
		return SceneLayer::kBlendMode_Opaque;
	else
	{
		logWarning("unknown blend type: %s", blend.c_str());
		return SceneLayer::kBlendMode_Add;
	}
}

//

SceneEffect::SceneEffect()
	: m_effect(nullptr)
	, m_strength(1.f)
{
}

SceneEffect::~SceneEffect()
{
	m_strength = 0.f;

	delete m_effect;
	m_effect = nullptr;
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
		
		if (shader.empty())
		{
			logWarning("shader not set. skipping effect");
		}
		else
		{
			effect = new Effect_Fsfx(m_name.c_str(), shader.c_str());

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

		//effect->blendMode = parseBlendMode(stringAttrib(xmlEffect, "blend", "add"));

		const bool enabled = boolAttrib(xmlEffect, "enabled", true);

		if (!enabled)
		{
			delete effect;
			return false;
		}

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
	, m_copyPreviousLayer(false)
	, m_opacity(1.f)
	, m_surface(nullptr)
	, m_debugEnabled(true)
{
	addVar("opacity", m_opacity);

	m_surface = new Surface(GFX_SX, GFX_SY);
}

SceneLayer::~SceneLayer()
{
	for (auto i = m_effects.begin(); i != m_effects.end(); ++i)
	{
		SceneEffect * effect = *i;

		delete effect;
	}

	m_effects.clear();

	delete m_surface;
	m_surface = nullptr;
}

void SceneLayer::load(const XMLElement * xmlLayer)
{
	m_name = stringAttrib(xmlLayer, "name", "");

	std::string blend = stringAttrib(xmlLayer, "blend", "add");

	m_blendMode = parseBlendMode(blend);
	m_autoClear = boolAttrib(xmlLayer, "auto_clear", true);
	m_copyPreviousLayer = boolAttrib(xmlLayer, "copy", false);

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
			g_currentSurface->blitTo(m_surface);
		}
		else
		{
			m_surface->clear();
		}
	}
	else if (m_autoClear)
	{
		m_surface->clear();
	}

	pushSurface(m_surface);
	{
		ScopedSurfaceBlock surfaceBlock(m_surface);

		setColor(colorWhite);
		setBlend(BLEND_ADD);

		drawableList.draw();
	}
	popSurface();

	// compose

	gxSetTexture(m_surface->getTexture());
	{
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
		case kBlendMode_Opaque:
			setBlend(BLEND_OPAQUE);
			setColorf(1.f, 1.f, 1.f, 1.f, m_opacity);
			break;
		default:
			Assert(false);
			break;
		}

		drawRect(0, 0, GFX_SX, GFX_SY);

		setBlend(BLEND_ADD);
	}
	gxSetTexture(0);
}

//

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
			m_tween.m_targetType = Tween::kTargetType_Layer;
			m_tween.m_targetName = layer;
		}
		else if (!effect.empty())
		{
			m_tween.m_targetType = Tween::kTargetType_Effect;
			m_tween.m_targetName = effect;
		}

		m_tween.m_varName = stringAttrib(xmlAction, "var", "");
		m_tween.m_tweenTo = floatAttrib(xmlAction, "to", 0.f);
		m_tween.m_tweenTime = floatAttrib(xmlAction, "time", 1.f);

		const std::string easeType = stringAttrib(xmlAction, "ease", "linear");
		m_tween.m_easeType = kEaseType_Linear;
		if (easeType == "linear")
			m_tween.m_easeType = kEaseType_Linear;
		else if (easeType == "pow_in")
			m_tween.m_easeType = kEaseType_PowIn;
		else if (easeType == "pow_out")
			m_tween.m_easeType = kEaseType_PowOut;
		else if (easeType == "sine_in")
			m_tween.m_easeType = kEaseType_SineIn;
		else if (easeType == "sine_out")
			m_tween.m_easeType = kEaseType_SineOut;
		else if (easeType == "sine_inout")
			m_tween.m_easeType = kEaseType_SineInOut;
		else if (easeType == "back_in")
			m_tween.m_easeType = kEaseType_BackIn;
		else if (easeType == "back_out")
			m_tween.m_easeType = kEaseType_BackOut;
		else if (easeType == "bounce_in")
			m_tween.m_easeType = kEaseType_BounceIn;
		else if (easeType == "bounce_out")
			m_tween.m_easeType = kEaseType_BounceOut;
		else if (easeType == "bounce_inout")
			m_tween.m_easeType = kEaseType_BounceInOut;
		else
			logError("unknown ease type: %s", easeType.c_str());
		m_tween.m_easeParam = floatAttrib(xmlAction, "ease_param", 1.f);
		m_tween.m_replaceTween = boolAttrib(xmlAction, "replace", false);
		m_tween.m_addTween = boolAttrib(xmlAction, "add", false);

		m_tween.m_preDelay = floatAttrib(xmlAction, "pre_delay", 0.f);
		m_tween.m_postDelay = floatAttrib(xmlAction, "post_delay", 0.f);

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
{
}

SceneEvent::~SceneEvent()
{
	for (auto i = m_actions.begin(); i != m_actions.end(); ++i)
	{
		SceneAction * action = *i;

		delete action;
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
			case SceneAction::Tween::kTargetType_Layer:
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

			case SceneAction::Tween::kTargetType_Effect:
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
				TweenFloat * var = varCollection->getVar(action->m_tween.m_varName.c_str());

				if (var == 0)
				{
					logWarning("couldn't find tween value by name. name=%s", action->m_tween.m_varName.c_str());
				}
				else
				{
					if (action->m_tween.m_replaceTween)
					{
						var->clear();
					}

					if (action->m_tween.m_preDelay > 0.f)
					{
						var->to(var->getFinalValue(), action->m_tween.m_preDelay, kEaseType_Linear, 0.f);
					}

					float to =  action->m_tween.m_tweenTo;

					if (action->m_tween.m_addTween)
					{
						to += var->getFinalValue();
					}

					var->to(
						to,
						action->m_tween.m_tweenTime,
						action->m_tween.m_easeType,
						action->m_tween.m_easeParam);

					if (action->m_tween.m_postDelay > 0.f)
					{
						var->to(var->getFinalValue(), action->m_tween.m_postDelay, kEaseType_Linear, 0.f);
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

	for (const XMLElement * xmlAction = xmlEvent->FirstChildElement(); xmlAction; xmlAction = xmlAction->NextSiblingElement())
	{
		SceneAction * action = new SceneAction();

		if (!action->load(xmlAction))
		{
			delete action;
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
{
}

Scene::~Scene()
{
	clear();
}

void Scene::tick(const float dt)
{
	ScopedSceneBlock block(this);

	for (auto i = m_layers.begin(); i != m_layers.end(); ++i)
	{
		SceneLayer * layer = *i;

		layer->tick(dt);
	}

	m_time += dt;

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

	// process debug text

	for (auto i = m_debugTexts.begin(); i != m_debugTexts.end(); )
	{
		DebugText & t = *i;

		if (m_time >= t.endTime)
			i = m_debugTexts.erase(i);
		else
			++i;
	}
}

void Scene::draw(DrawableList & list)
{
	ScopedSceneBlock block(this);

	for (auto i = m_layers.begin(); i != m_layers.end(); ++i)
	{
		SceneLayer * layer = *i;

		layer->draw(list);
	}

	// draw debug text

	setFont("calibri.ttf");
	setColor(colorWhite);

	float x = GFX_SX / 2.f;
	float y = GFX_SY / 2.f;

	for (auto i = m_debugTexts.begin(); i != m_debugTexts.end(); ++i)
	{
		drawText(x, y, 24, 0.f, 0.f, i->text.c_str());

		y += 30.f;
	}
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

			addDebugText(name);
		}
	}
}

void Scene::addDebugText(const char * text)
{
	DebugText t;
	t.text = text;
	t.endTime = m_time + 4.f;

	m_debugTexts.push_back(t);
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

					layer->load(xmlLayer);

					m_layers.push_back(layer);
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
	}

	m_layers.clear();

	//

	for (auto i = m_events.begin(); i != m_events.end(); ++i)
	{
		SceneEvent * event = *i;

		delete event;
	}

	m_events.clear();

	//

	m_midiMaps.clear();

	//

	m_time = 0.f;

	//

	m_debugTexts.clear();
}

bool Scene::reload()
{
	std::string filename = m_filename;

	clear();

	return load(filename.c_str());
}
