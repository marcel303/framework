#pragma once

#include "fileEditor.h"
#include "particle_editor.h"

struct FileEditor_ParticleSystem : FileEditor
{
	ParticleEditor particleEditor;
	
	FileEditor_ParticleSystem(const char * path)
	{
		particleEditor.load(path);
	}
	
	virtual ~FileEditor_ParticleSystem() override
	{
		
	}
	
	virtual void tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured) override
	{
		if (hasFocus == false)
			return;
		
		clearSurface(0, 0, 0, 0);
		
		particleEditor.tick(inputIsCaptured == false, sx, sy, dt);
		
		particleEditor.draw(inputIsCaptured == false, sx, sy);
	}
};
