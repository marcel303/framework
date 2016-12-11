//
//  Created by Alejandro Pérez on 27/04/13.
//  Copyright (c) 2013 Alejandro Pérez. All rights reserved.
//

#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <math.h>
#include <png++/png.hpp>
#include <vector>
#include <sstream>
#include <fstream>

using namespace std;

png::image<png::gray_pixel> nimg2png(float *img, int width, int height) {
  png::image<png::gray_pixel> image(width, height);
  png::gray_pixel p;
  float min = img[0];
  float max = img[0];

  for (int x=0; x<width; x++) {
    for (int y=0; y<height; y++) {
      if (img[(y*width)+x] < min) min = img[(y*width)+x];
      if (img[(y*width)+x] > max) max = img[(y*width)+x];
    }
  }
  max = max - min;
  for (int x=0; x<width; x++) {
    for (int y=0; y<height; y++) {
      p = (img[(y*width)+x]-min)*(255/max);
      image.set_pixel(x, y, p);
    }
  }
  return image;
}

float* rotate_90deg(float *img, int width, int height) {
  float *img90deg = new float[width*height];

  for (int y=0; y<height; y++) {
    for (int x=0; x<width; x++) {
      img90deg[height-1-y+(x*height)] = img[(y*width)+x];
    }
  }

  return img90deg;
}

float* rotate_180deg(float *img, int width, int height) {
  float *img180deg = new float[width*height];

  for (int y=0; y<height; y++) {
    for (int x=0; x<width; x++) {
      img180deg[((height-1-y)*width)+width-1-x] = img[(y*width)+x];
    }
  }

  return img180deg;
}

float* rotate_270deg(float *img, int width, int height) {
  float *img270deg = new float[width*height];

  for (int y=0; y<height; y++) {
    for (int x=0; x<width; x++) {
      img270deg[y+((width-x-1)*height)] = img[(y*width)+x];
    }
  }

  return img270deg;
}

float *parse_float_string(string image, int width, int height) {
  float *img = new float[width*height];
  int x=0, y=0;
  float value;
  istringstream iss(image);

  while (iss >> value) {
    if (y == height) {
      cout << endl << "WARNING: image size differs from " << width << "x" << height << " (ignoring sample)" << endl;
      return NULL;
    }
    img[(y*width)+(x++)] = value;
    if (x == width) {
      x = 0;
      y++;
    }
  }
  if (x != 0 || y != height) {
    cout << endl << "WARNING: image size differs from " << width << "x" << height << " (ignoring sample)" << endl;
    return NULL;
  }
  return img;
}

float* mirror_image(float* image, int width, int height) {
  float *vmirror;
  int i, j, x, y;
  vmirror = new float[width*height];
  for (y=0; y<height; y++) {
    for (x=0; x<width; x++) {
      vmirror[(y*width)+width-1-x] = image[(y*width)+x];
    }
  }
  return vmirror;
}

void printUsage(char* prog) {
  cout << "usage: " << prog << " --base-resolution B IMAGESFILE" << endl;
}

int main(int argc, char** argv) {
  int base_resolution = 21;
  string fileurl = "";
  float *img;
  png::image<png::gray_pixel> image;
  bool mirror = false;
  int rotation = 0;

  if (argc < 3) {
    printUsage(argv[0]);
    return -1;
  }

  // Read input arguments
  for (int i=1; i<argc; i++) {
    if (strcmp(argv[i], "--base-resolution") == 0) {
      base_resolution = stoi(argv[i+1]);
      i++;
    }
    else if (strcmp(argv[i], "--mirror") == 0) {
      mirror = true;
    }
    else if (strcmp(argv[i], "--rotate") == 0) {
      rotation = stoi(argv[i+1]);
      i++;
    }
    else fileurl = argv[i];
  }

  if (fileurl.compare("") == 0) {
    printUsage(argv[0]);
    return -1;
  }

  ifstream file(fileurl.c_str());
  string sline = "";
  int j=0;
  while (getline(file, sline)) {
    img = parse_float_string(sline, base_resolution, base_resolution);
    if (rotation == 90) img = rotate_90deg(img, base_resolution, base_resolution);
    else if (rotation == 180) img = rotate_180deg(img, base_resolution, base_resolution);
    else if (rotation == 270) img = rotate_270deg(img, base_resolution, base_resolution);
    if (img != NULL) {
      image = nimg2png(img, base_resolution, base_resolution);
      image.write(to_string(j++)+".png");
      if (mirror) {
        img = mirror_image(img, base_resolution, base_resolution);
        image = nimg2png(img, base_resolution, base_resolution);
        image.write(to_string(j++)+".png");
      }
    }
  }
  file.close();
  return 0;
}
