#pragma once

struct AudioInputOutputObject
{
	int numInputChannels = 0;
	int numOutputChannels = 0;

	float * inputBuffer = nullptr;
};
