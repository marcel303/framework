//
//  Created by Alejandro Pérez on 27/04/13.
//  Copyright (c) 2013 Alejandro Pérez. All rights reserved.
//

#include <string>
#include <iostream>
#include "Feature.h"

Feature::Feature(int t, int x, int y, int w, int h) {
  type = t;
  xc = x;
  yc = y;
  width = w;
  height = h;
}

static std::string i2s(int i) {
  char cs[4];
  sprintf(cs, "%d", i);
  std::string s(cs);
  return s;
}

std::string Feature::toString() {
  std::string s;
  s = i2s(type)+" "+i2s(width)+" "+i2s(height)+" "+i2s(xc)+" "+i2s(yc);
  return s;
}

int Feature::getType() {
  return this->type;
}

int Feature::getWidth() {
  return this->width;
}

int Feature::getHeight() {
  return this->height;
}

float Feature::rectangleValue(const float *ii, int iiwidth, int ix, int iy, int rx, int ry, int rw, int rh) {
  float value = ii[((iy+ry+rh-1)*iiwidth)+(ix+rx+rw-1)];
  if ((ix+rx) > 0) value -= ii[((iy+ry+rh-1)*iiwidth)+(ix+rx-1)];
  if ((iy+ry) > 0) value -= ii[((iy+ry-1)*iiwidth)+(ix+rx+rw-1)];
  if ((ix+rx) > 0 && (iy+ry) > 0) value += ii[((iy+ry-1)*iiwidth)+(ix+rx-1)];
  return value;
}

float Feature::getValue(const float *ii, int iiwidth, int x, int y) {
  // 5 types of feature (A, B, C, C^t, D)
  switch(this->type) {
    case 0:
      return rectangleValue(ii, iiwidth, x, y, this->xc+(this->width/2), this->yc, this->width/2, this->height)-rectangleValue(ii, iiwidth, x, y, this->xc, this->yc, this->width/2, this->height);
      break;
    case 1:
      return rectangleValue(ii, iiwidth, x, y, this->xc, this->yc, this->width, this->height/2)-rectangleValue(ii, iiwidth, x, y, this->xc, this->yc+(this->height/2), this->width, this->height/2);
      break;
    case 2:
      return rectangleValue(ii, iiwidth, x, y, this->xc+(width/3), this->yc, this->width/3, this->height)-rectangleValue(ii, iiwidth, x, y, this->xc, this->yc, this->width/3, this->height)-rectangleValue(ii, iiwidth, x, y, this->xc+(width*2/3), this->yc, this->width/3, this->height);
      break;
    case 3:
      return rectangleValue(ii, iiwidth, x, y, this->xc, this->yc+(this->height/3), this->width, this->height/3)-rectangleValue(ii, iiwidth, x, y, this->xc, this->yc, this->width, this->height/3)-rectangleValue(ii, iiwidth, x, y, this->xc, this->yc+(this->height*2/3), this->width, this->height/3);
      break;
    case 4:
      return rectangleValue(ii, iiwidth, x, y, this->xc+(width/2), this->yc, this->width/2, this->height/2)+rectangleValue(ii, iiwidth, x, y, this->xc, this->yc+(height/2), this->width/2, this->height/2)-rectangleValue(ii, iiwidth, x, y, this->xc+(width/2), this->yc+(height/2), this->width/2, this->height/2)-rectangleValue(ii, iiwidth, x, y, this->xc, this->yc, this->width/2, this->height/2);
      break;
    default:
      std::cout << "Error: feature type " << this->type << " does not exist. Feature type is in range [0, 4]." << std::endl;
      exit(-1);
      break;
  }
}

void Feature::scale(float s) {
  this->width = (this->width)*s;
  this->height = (this->height)*s;
  this->xc = (this->xc)*s;
  this->yc = (this->yc)*s;
}
