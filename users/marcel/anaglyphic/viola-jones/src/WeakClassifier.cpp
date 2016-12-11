#include "WeakClassifier.h"
#include <math.h>
#include <algorithm>
#include <string>
#include <sstream>
#include <vector>
#include <iostream>

struct score {
  float value;
  bool label;
  float weight;
};

static bool compare(const score &a, const score &b) {
  return a.value < b.value;
}

static std::string f2s(float f) {
  std::ostringstream ss;
  ss << f;
  std::string s(ss.str());
  return s;
}

WeakClassifier::WeakClassifier(Feature *f) {
  this->f = f;
}

WeakClassifier::WeakClassifier(Feature *f, float threshold, bool polarity) {
  this->f = f;
  this->threshold = threshold;
  this->polarity = polarity;
}

float WeakClassifier::find_optimum_threshold(float *fvalues, int fsize, int nfsize, float *weights) {
  score *scores = new score[fsize+nfsize];
  for (int i=0; i<fsize; i++) {
    scores[i].value = fvalues[i];
    scores[i].label = true;
    scores[i].weight = weights[i];
  }
  for (int j=0; j<nfsize; j++) {
    scores[j+fsize].value = fvalues[j+fsize];
    scores[j+fsize].label = false;
    scores[j+fsize].weight = weights[j+fsize];
  }
  std::sort(scores, scores+fsize+nfsize, compare);
  wsum *ws = new wsum[fsize+nfsize];
  float tp = 0;
  float tn = 0;
  if (scores[0].label == false) {
    tn = scores[0].weight;
    ws[0].sn = scores[0].weight;
    ws[0].sp = 0;
  }
  else {
    tp = scores[0].weight;
    ws[0].sp = scores[0].weight;
    ws[0].sn = 0;
  }
  for (int k=1; k<(fsize+nfsize); k++) {
    if (scores[k].label == false) {
      tn += scores[k].weight;
      ws[k].sn = ws[k-1].sn + scores[k].weight;
      ws[k].sp = ws[k-1].sp;
    }
    else {
      tp += scores[k].weight;
      ws[k].sp = ws[k-1].sp + scores[k].weight;
      ws[k].sn = ws[k-1].sn;
    }
  }
  float minerror = 1;
  float errorp;
  float errorm;
  for (int l=0; l<(fsize+nfsize); l++) {
    errorp = ws[l].sp + tn - ws[l].sn;
    errorm = ws[l].sn + tp - ws[l].sp;
    if (errorp < errorm) {
      if (errorp < minerror) {
        minerror = errorp;
        this->threshold = scores[l].value;
        this->polarity = false;
      }
    }
    else {
     if (errorm < minerror) {
        minerror = errorm;
        this->threshold = scores[l].value;
        this->polarity = true;
      }
    }
  }
  delete[] scores;
  delete[] ws;
  return minerror;
}

Feature* WeakClassifier::getFeature() {
  return this->f;
}

int WeakClassifier::classify(const float *img, int imwidth, int x, int y, float mean, float stdev) {
  float fval = this->f->getValue(img, imwidth, x, y);
  // if the number of rectangles is odd
  if (this->f->getType() == 2 || this->f->getType() == 3) fval = (fval+(this->f->getWidth()*this->f->getHeight()*mean/3));
  if (stdev != 0) fval = fval/stdev;
  if (fval < this->threshold) {
    if (this->polarity) return 1;
    else return -1;
  }
  else {
    if (this->polarity) return -1;
    else return 1;
  }
}

void WeakClassifier::scale(float s) {
  f->scale(s);
  this->threshold = (this->threshold)*pow(s, 2);
}

std::string WeakClassifier::toString() {
  std::string s = f->toString()+" "+f2s(this->threshold);
  if (this->polarity == false) s += " 0";
  else s += " 1";
  return s;
}
