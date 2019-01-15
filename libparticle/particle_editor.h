#pragma once

struct ParticleEditorState;

struct ParticleEditor
{
	ParticleEditorState * state;
	
	ParticleEditor();
	~ParticleEditor();
	
	bool load(const char * filename);
	
	void tick(const bool menuActive, const float sx, const float sy, const float dt);
	void draw(const bool menuActive, const float sx, const float sy);
};
