//
//  Created by Alejandro Pérez on 27/04/13.
//  Copyright (c) 2013 Alejandro Pérez. All rights reserved.
//

#ifndef CASCADECLASSIFIER_H
#define CASCADECLASSIFIER_H

#include "CascadeClassifier.h"
#include "StrongClassifier.h"
#include <vector>
#include <string>

class CascadeClassifier {
  public:
    CascadeClassifier(int baseres);
    CascadeClassifier(std::vector<StrongClassifier*> sc, int baseres);
    CascadeClassifier(std::string furl);
    bool classify(const float *img, int imwidth, int x, int y, float mean, float stdev);
    std::string toString();
    void push_back(StrongClassifier *sc);
    void pop_back();
    bool save(std::string furl);
    float fnr(std::vector<float*> &positive_set);
    float fpr(std::vector<float*> &negative_set);
    void strictness(float p);
    int getBaseResolution();
    void scale(float s);
    std::vector<StrongClassifier*> getStrongClassifiers();
  protected:
    std::vector<StrongClassifier*> sc;
    int baseres;
};

#endif
