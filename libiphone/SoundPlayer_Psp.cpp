#include <audiooutput.h>
#include <kernel.h>
#include <libatrac3plus.h>
#include <libsas.h>
#include "Exception.h"
#include "SoundPlayer_Psp.h"
#include "SoundPlayer_Psp_AT3.h"
#include "System.h"
#include "Types.h"

SoundPlayer_Psp::SoundPlayer_Psp()
{
	m_Res = 0;
	m_Volume = 1.0f;
	m_Loop = false;
	m_IsEnabled = true;
}

SoundPlayer_Psp::~SoundPlayer_Psp()
{
}

void SoundPlayer_Psp::Initialize(bool playBackgroundMusic)
{
}

void SoundPlayer_Psp::Shutdown()
{
	Stop();
}

void SoundPlayer_Psp::Play(Res* res, bool loop)
{
	if (res == m_Res)
		return;

	Stop();

	m_Res = res;
	m_Loop = loop;

	Start();
}

void SoundPlayer_Psp::Start()
{
	if (m_Res == 0)
		return;

	Stop();

	At3Begin(g_System.GetResourcePath(m_Res->m_FileName).c_str(), m_Loop);
}

void SoundPlayer_Psp::Stop()
{
	if (m_Res == 0)
		return;

	At3End();
}

void SoundPlayer_Psp::IsEnabled_set(bool enabled)
{
	if (enabled == m_IsEnabled)
		return;

	At3SetEnabled(enabled);

	m_IsEnabled = enabled;
}

void SoundPlayer_Psp::Volume_set(float volume)
{
	At3SetVolume(volume);

	m_Volume = volume;
}

void SoundPlayer_Psp::Update()
{
	At3CheckLoop();
}
