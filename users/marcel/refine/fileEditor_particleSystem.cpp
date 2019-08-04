#include "fileEditor_particleSystem.h"
#include "particle_editor.h"
#include "ui.h"

FileEditor_ParticleSystem::FileEditor_ParticleSystem(const char * path)
{
	particleEditor.load(path);
}

FileEditor_ParticleSystem::~FileEditor_ParticleSystem()
{
	
}

void FileEditor_ParticleSystem::tick(const int sx, const int sy, const float dt, const bool hasFocus, bool & inputIsCaptured)
{
	if (hasFocus == false)
		return;
	
	clearSurface(0, 0, 0, 0);
	
	particleEditor.tick(inputIsCaptured == false, sx, sy, dt);
	
	particleEditor.draw(inputIsCaptured == false, sx, sy);
}
