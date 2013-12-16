#include "TravellerCapturer.h"

std::vector<Vec2F> TravellerCapturer::Capture(float step, const std::vector<Vec2F>& points)
{
	std::vector<Vec2F> samples;

	if (points.size() < 1)
		return samples;

	mTraveller.Setup(step, HandleTravel, &samples);

	mTraveller.Begin(points[0][0], points[0][1]);

	for (size_t i = 1; i < points.size(); ++i)
		mTraveller.Update(points[i][0], points[i][1]);

	mTraveller.End(points[points.size() - 1][0], points[points.size() - 1][1]);

	return samples;
}

void TravellerCapturer::HandleTravel(void* obj, const TravelEvent& e)
{
	std::vector<Vec2F>* samples = (std::vector<Vec2F>*)obj;

	Vec2F location(e.x, e.y);
	Vec2F direction(e.dx, e.dy);

	samples->push_back(location);
	samples->push_back(direction);
}
