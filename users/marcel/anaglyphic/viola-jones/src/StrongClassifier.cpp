//
//  Created by Alejandro Pérez on 27/04/13.
//  Copyright (c) 2013 Alejandro Pérez. All rights reserved.
//

#include "StrongClassifier.h"
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

StrongClassifier::StrongClassifier() {
  this->threshold = 0;
}

StrongClassifier::StrongClassifier(std::vector<WeakClassifier*> wc, float *weight) {
  this->wc = wc;
  this->threshold = 0;
  for (int i=0; i<wc.size(); i++) this->weight.push_back(weight[i]);
}

StrongClassifier::StrongClassifier(std::vector<WeakClassifier*> wc, float *weight, float threshold) {
  this->wc = wc;
  for (int i=0; i<wc.size(); i++) this->weight.push_back(weight[i]);
  this->threshold = threshold;
}

static std::string f2s(float f) {
  std::ostringstream oss;
  oss << f;
  std::string s(oss.str());
  return s;
}

static std::string i2s(int i) {
  std::ostringstream oss;
  oss << i;
  std::string s(oss.str());
  return s;
}

std::string StrongClassifier::toString() {
  std::string s = i2s(wc.size())+" "+f2s(this->threshold)+"\n";
  int i=0;
  for (std::vector<WeakClassifier*>::iterator it = wc.begin(); it != wc.end(); ++it) {
    s += f2s(weight[i++])+" "+(*it)->toString()+"\n";
  }
  return s;
}

bool StrongClassifier::classify(const float *img, int imwidth, int x, int y, float mean, float stdev) {
  float score = 0;
  int i = 0;
  for (std::vector<WeakClassifier*>::iterator it = wc.begin(); it != wc.end(); ++it) {
    score += weight[i++]*((*it)->classify(img, imwidth, x, y, mean, stdev));
  }
  if (score >= this->threshold) return true;
  else return false;
}

void StrongClassifier::add(WeakClassifier* wc, float weight) {
  this->wc.push_back(wc);
  this->weight.push_back(weight);
}

float StrongClassifier::fnr(std::vector<float*> &positive_set, int base_resolution) {
  int fn = 0;
  for (int i=0; i<positive_set.size(); i++) if (this->classify(positive_set[i], base_resolution, 0, 0, 0, 1) == false) fn++;
  return float(fn)/float(positive_set.size());
}

float StrongClassifier::fpr(std::vector<float*> &negative_set, int base_resolution) {
  int fp = 0;
  for (int i=0; i<negative_set.size(); i++) if (this->classify(negative_set[i], base_resolution, 0, 0, 0, 1) == true) fp++;
  return float(fp)/float(negative_set.size());
}

void StrongClassifier::scale(float s) {
  for (std::vector<WeakClassifier*>::iterator it = this->wc.begin(); it != this->wc.end(); ++it) {
    (*it)->scale(s);
  }
}

void StrongClassifier::optimise_threshold(std::vector<float*> &positive_set, int base_resolution, float maxfnr) {
  int wf;
  float thr;
  float *scores = new float[positive_set.size()];
  for (int i=0; i<positive_set.size(); i++) {
    scores[i] = 0;
    wf = 0;
    for (std::vector<WeakClassifier*>::iterator it = wc.begin(); it != wc.end(); ++it, wf++)
      scores[i] += (this->weight[wf])*((*it)->classify(positive_set[i], base_resolution, 0, 0, 0, 1));
  }
  std::sort(scores, scores+positive_set.size());
  int maxfnrind = maxfnr*positive_set.size();
  if (maxfnrind >= 0 && maxfnrind < positive_set.size()) {
    thr = scores[maxfnrind];
    while (maxfnrind > 0 && scores[maxfnrind] == thr) maxfnrind--;
    this->threshold = scores[maxfnrind];
  }
  delete[] scores;
}

std::vector<WeakClassifier*> StrongClassifier::getWeakClassifiers() {
  return this->wc;
}

void StrongClassifier::strictness(float p) {
  this->threshold = (this->threshold)*p;
}
