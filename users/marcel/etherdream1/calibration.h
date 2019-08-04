#pragma once

#include "laserTypes.h"

void drawCalibrationPattern_rectangle(LaserPoint * points, const int numPoints);
void drawCalibrationPattern_rectanglePoints(LaserPoint * points, const int numPoints);
void drawCalibrationPattern_line_vscroll(LaserPoint * points, const int numPoints, const float phase);
void drawCalibrationPattern_line_hscroll(LaserPoint * points, const int numPoints, const float phase);
