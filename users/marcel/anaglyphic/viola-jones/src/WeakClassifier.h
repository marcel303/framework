//
//  Created by Alejandro Pérez on 27/04/13.
//  Copyright (c) 2013 Alejandro Pérez. All rights reserved.
//

#ifndef WEAKCLASSIFIER_H
#define WEAKCLASSIFIER_H

#include "Feature.h"
#include <string>
#include <vector>

/*
For each feature, the examples are sorted based on feature value. The AdaBoost optimal threshold for that feature can then be computed in a single pass over this sorted list. For each element in the sorted list, four sums are maintained and evaluated:
*/

struct wsum {
  // the sum of positive weights below the current example S+
  float sp;
  // the sum of negative weights below the current example S−
  float sn;
};

class WeakClassifier {
  public:
    WeakClassifier(Feature *f);
    WeakClassifier(Feature *f, float threshold, bool polarity);
    float find_optimum_threshold(float *fvalues, int fsize, int nfsize, float *weights);
    Feature* getFeature();
    int classify(const float *img, int imwidth, int x, int y, float mean, float stdev);
    void scale(float s);
    std::string toString();
  protected:
    Feature *f;
    float threshold;
    bool polarity;
};

#endif
