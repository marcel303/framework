#pragma once

struct LaserPoint;
struct etherdream_point;

void convertLaserImageToEtherdream(
	const LaserPoint * points,
	const int numPoints,
	etherdream_point * out_points);
