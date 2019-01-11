#pragma once

#include "laserTypes.h"

void drawCalibrationImage_rectangle(LaserPoint * points, const int numPoints);
void drawCalibrationImage_line_vscroll(LaserPoint * points, const int numPoints, const float phase);
void drawCalibrationImage_line_hscroll(LaserPoint * points, const int numPoints, const float phase);
