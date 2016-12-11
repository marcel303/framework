//
//  Created by Alejandro Pérez on 27/04/13.
//  Copyright (c) 2013 Alejandro Pérez. All rights reserved.
//

#ifndef FEATURE_H
#define FEATURE_H

#include <string>
#include <stdlib.h>

class Feature {
  public:
    Feature(int type, int xc, int yc, int width, int height);
    float getValue(const float *ii, int iiwidth, int x, int y);
    int getType();
    int getWidth();
    int getHeight();
    void scale(float s);
    std::string toString();
  protected:
    int type; // five feature types available (corresponding to A, B, C, C^t and D types on V&J 2001 paper)
    int width, height; // width and height of the feature
    int xc, yc; // x and y coords of top-left feature corner within the detection sub-window
    float rectangleValue(const float *ii, int iiwidth, int ix, int iy, int rx, int ry, int rw, int rh);
};

#endif
