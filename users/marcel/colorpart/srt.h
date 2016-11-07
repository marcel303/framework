#pragma once

#include <string>
#include <vector>

struct SrtFrame
{
	double time;
	double duration;
	std::vector<std::string> lines;
};

struct Srt
{
	std::vector<SrtFrame> frames;

	const SrtFrame * findFrameByTime(const double time) const;
};

bool loadSrt(const char * filename, Srt & srt);
