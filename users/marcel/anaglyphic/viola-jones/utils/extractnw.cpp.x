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

using namespace std;

float** normalize_image(float **img, int width, int height) {
  float mean = 0;
  float stdev = 0;
  int x, y;

  for (x = 0; x < width; x++) {
    for (y = 0; y < height; y++) {
      mean += img[x][y];
    }
  }
  mean = mean/(width*height);

  for (x = 0; x < width; x++) {
    for (y = 0; y < height; y++) {
      stdev += pow(img[x][y]-mean, 2);
    }
  }
  stdev = stdev/(width*height);
  stdev = sqrt(stdev);
  if (stdev == 0) stdev = 1;
  float **nimg = new float*[width];
  for (x = 0; x < width; x++) {
    nimg[x] = new float[height];
    for (y = 0; y < height; y++) {
      nimg[x][y] = (img[x][y]-mean)/stdev;
    }
  }
  return nimg;
}

float** resizeToHalf(float **img, int width, int height) {
  float **himg = new float*[width/2];
  for (int k=0; k<width/2; k++) himg[k] = new float[height/2];
  for (int i=1; i<width-1; i+=2) {
    for (int j=1; j<height-1; j+=2) {
      himg[i/2][j/2] = (img[i][j] + img[i+1][j] + img[i][j+1] + img[i+1][j+1]) / 4;
    }
  }

  // delete previous image
  for (int l=0; l<width; l++) delete[] img[l];
  delete[] img;
  return himg;
}

void printUsage(char* prog) {
  cout << "usage: " << prog << " --base-resolution BR IMAGE.PNG" << endl;
}

int main(int argc, char** argv) {
  int baseres = 21;
  vector<string> nofacesurl;
  bool normalize = false;

  if (argc < 3) {
    printUsage(argv[0]);
    return -1;
  }

  // Read input arguments
  for (int i=1; i<argc; i++) {
    if (strcmp(argv[i], "--base-resolution") == 0) {
      baseres = stoi(argv[i+1]);
      i++;
    }
    else if (strcmp(argv[i], "--normalize") == 0) {
      normalize = true;
    }
    else nofacesurl.push_back(argv[i]);
  }

  if (nofacesurl.size() == 0) {
    printUsage(argv[0]);
    return -1;
  }

  png::rgb_pixel p;
  float **grayscale;
  size_t x, y;
  int z, x1, y1, x2, y2;
  float **subwindow;
  int currentw, currenth;
  bool firstsize = true;

  for (vector<string>::iterator it = nofacesurl.begin(); it != nofacesurl.end(); ++it) {
    // Load PNG image
    png::image<png::rgb_pixel> img((*it));
    firstsize = true;

    currentw = img.get_width();
    currenth = img.get_height();
    // Convert RGB image to grayscale
    grayscale = new float*[img.get_width()];
    for (x = 0; x < img.get_width(); ++x) {
      grayscale[x] = new float[img.get_height()];
      for (y = 0; y < img.get_height(); ++y) {
        p = img.get_pixel(x, y);
        grayscale[x][y] = (0.21*p.red)+(0.71*p.green)+(0.07*p.blue);
      }
    }

    subwindow = new float*[baseres];
    for (z=0; z<baseres; z++) subwindow[z] = new float[baseres];

    while (currentw > baseres && currenth > baseres) {
      if (!firstsize) grayscale = resizeToHalf(grayscale, currentw*2, currenth*2);
      else firstsize = false;

      for (x1=0; x1<currentw-baseres; x1+=(baseres/2)) {
        for (y1=0; y1<currenth-baseres; y1+=(baseres/2)) {
          for (x2=0; x2<baseres; x2++) {
            for (y2=0; y2<baseres; y2++) {
              subwindow[x2][y2] = grayscale[x1+x2][y1+y2];
            }
          }
          if (normalize) subwindow = normalize_image(subwindow, baseres, baseres);
          for (y2=0; y2<baseres; y2++) {
            for (x2=0; x2<baseres; x2++) {
              if (y2 == 0 && x2 == 0) cout << subwindow[x2][y2];
              else cout << " " << subwindow[x2][y2];
            }
          }
          cout << endl;
        }
      }
      currentw = currentw/2;
      currenth = currenth/2;
    }
  }

  return 0;
}
