//
//  Created by Alejandro Pérez on 27/04/13.
//  Copyright (c) 2013 Alejandro Pérez. All rights reserved.
//

#ifndef STRONGCLASSIFIER_H
#define STRONGCLASSIFIER_H

#include "WeakClassifier.h"
#include <vector>
#include <string>

class StrongClassifier {
  public:
    StrongClassifier();
    StrongClassifier(std::vector<WeakClassifier*> wc, float *weight);
    StrongClassifier(std::vector<WeakClassifier*> wc, float *weight, float threshold);
    bool classify(const float *img, int imwidth, int x, int y, float mean, float stdev);
    std::string toString();
    void add(WeakClassifier* wc, float weight);
    void scale(float s);
    void optimise_threshold(std::vector<float*> &positive_set, int base_resolution, float maxfnr);
    float fnr(std::vector<float*> &positive_set, int base_resolution);
    float fpr(std::vector<float*> &negative_set, int base_resolution);
    void strictness(float p);
    std::vector<WeakClassifier*> getWeakClassifiers();
  protected:
    std::vector<WeakClassifier*> wc;
    std::vector<float> weight;
    float threshold;
};

#endif
