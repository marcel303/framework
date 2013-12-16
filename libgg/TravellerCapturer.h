#pragma once

#include <vector>
#include "Traveller.h"

class TravellerCapturer
{
public:
	std::vector<Vec2F> Capture(float step, const std::vector<Vec2F>& points);

private:
	static void HandleTravel(void* obj, const TravelEvent& e);

	Traveller mTraveller;
};
