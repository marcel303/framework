#include "EntitySound.h"
#include "Renderer.h"
#include "ResMgr.h"

EntitySound::EntitySound(const std::string& filename) : Entity()
{
	SetClassName("Sound");
	AddParameter(Parameter(PARAM_STRING, "file", REP_ONCHANGE, COMPRESS_NONE, &m_filename));

	m_filename = filename;

	m_src = ShSndSrc(new ResSndSrc());
}

EntitySound::~EntitySound()
{
}

void EntitySound::OnSceneAdd(Scene* scene)
{
	Entity::OnSceneAdd(scene);

	m_snd = RESMGR.GetSnd(m_filename);

	SoundDevice* sfx = Renderer::I().GetSoundDevice();

	sfx->PlaySound(m_src.get(), m_snd.get(), true);
}

void EntitySound::OnSceneRemove(Scene* scene)
{
	Entity::OnSceneRemove(scene);
}

void EntitySound::OnTransformChange()
{
	m_src->SetPosition(GetPosition());
}
