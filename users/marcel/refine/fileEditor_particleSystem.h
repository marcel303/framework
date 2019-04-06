#pragma once

#include "fileEditor.h"
#include "particle_editor.h"
#include "ui.h"

struct FileEditor_ParticleSystem : FileEditor
{
	ParticleEditor particleEditor;
	
	FileEditor_ParticleSystem(const char * path);
	virtual ~FileEditor_ParticleSystem() override;
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override;
};
