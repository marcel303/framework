/*
 *
 *                _______  _______  __________  _______  _____
 *               /____  / / _____/ /         / / ___  / / ___ \
 *               __  / / / / ____ / //   // / / /  / / / /  / /
 *             /  /_/ / / /__/ / / / /_/ / / / /__/ / / /__/ /
 *            /______/ /______/ /_/     /_/ /______/ /______/
 *
 *
 *
 *
 *  Visualisation and player interaction. */

#pragma once

#include <new>

struct JGMOD_PLAYER;

struct JGVIS
{
	struct OLD_CHN_INFO
	{
		int old_sample = -1;
		float hue = 0.f;
	};
	
	OLD_CHN_INFO old_chn_info[JGMOD_MAX_VOICES];

	int note_length = 0;
	int note_relative_pos = 0;
	
	int start_chn = 0;
	int end_chn = -1;
	
	void reset()
	{
		for (int index = 0; index < JGMOD_MAX_VOICES; ++index)
		{
			new (old_chn_info + index) OLD_CHN_INFO();
		}
	}
};

void jgvis_tick(JGVIS & vis, JGMOD_PLAYER & player, bool & inputIsCaptured);
void jgvis_draw(JGVIS & vis, const JGMOD_PLAYER & player, const bool drawCircles);
