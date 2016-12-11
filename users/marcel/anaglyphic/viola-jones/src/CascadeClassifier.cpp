//
//  Created by Alejandro Pérez on 20/11/13.
//  Copyright (c) 2013 Alejandro Pérez. All rights reserved.
//

#include "CascadeClassifier.h"
#include "StrongClassifier.h"
#include "WeakClassifier.h"
#include "Feature.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>

CascadeClassifier::CascadeClassifier(int baseres) {
  this->baseres = baseres;
}

CascadeClassifier::CascadeClassifier(std::vector<StrongClassifier*> sc, int baseres) {
  this->sc = sc;
  this->baseres = baseres;
}

CascadeClassifier::CascadeClassifier(std::string furl) {
  std::ifstream ifs(furl.c_str());
  std::vector<StrongClassifier*> vs;
  std::vector<WeakClassifier*> *vw;
  StrongClassifier *sclass;
  WeakClassifier *wclass;
  Feature *feat;
  int nline = 0;
  int br, ns, nw, fx, fy, fw, fh, ft, wp, i;
  float sthr, wthr;
  float *aux;
  
  if (!ifs.is_open())
	  return;

  if (nline == 0) {
    ifs >> br >> ns;
    this->baseres = br;
  }
  while (vs.size() < ns) {
    ifs >> nw >> sthr;
    aux = new float[nw];
    vw = new std::vector<WeakClassifier*>();
    i = 0;
    while (vw->size() < nw) {
      ifs >> aux[i++] >> ft >> fw >> fh >> fx >> fy >> wthr >> wp;
      feat = new Feature(ft, fx, fy, fw, fh);
      if (wp == 0) wclass = new WeakClassifier(feat, wthr, false);
      else wclass = new WeakClassifier(feat, wthr, true);
      vw->push_back(wclass);
    }
    sclass = new StrongClassifier(*vw, aux, sthr);
    vs.push_back(sclass);
  }
  this->sc = vs;
}

static std::string i2s(int i) {
  std::ostringstream oss;
  oss << i;
  std::string s(oss.str());
  return s;
}

std::string CascadeClassifier::toString() {
  /* CASCADE CLASSIFIER OUTPUT FORMAT:
  21 6 # baseres cascadesize
  # for each strong classifier (total=cascadesize)
    3 1.275 # nweakclass sthreshold
    # for each weak classifier (total = nweakclass)
    0 1 1 4 4 0.45 1 # ftype fx fy fwidth fheight wthreshold wpolarity
    # end for
  # end for
  */
  std::string s = i2s(this->baseres)+" "+i2s(sc.size())+"\n";
  for (std::vector<StrongClassifier*>::iterator it = sc.begin(); it != sc.end(); ++it) {
    s += (*it)->toString();
  }
  return s;
}

void CascadeClassifier::push_back(StrongClassifier *sc) {
  this->sc.push_back(sc);
}

void CascadeClassifier::pop_back() {
  this->sc.pop_back();
}

bool CascadeClassifier::classify(const float *img, int imwidth, int x, int y, float mean, float stdev) {
  for (std::vector<StrongClassifier*>::iterator it = sc.begin(); it != sc.end(); ++it) {
    if ((*it)->classify(img, imwidth, x, y, mean, stdev) == false) return false;
  }
  return true;
}

bool CascadeClassifier::save(std::string furl) {
  std::ofstream ofs(furl);
  ofs << this->toString();
  ofs.close();
  return true;
}

void CascadeClassifier::strictness(float p) {
  for (std::vector<StrongClassifier*>::iterator it = sc.begin(); it != sc.end(); ++it) {
    (*it)->strictness(p);
  }
}

float CascadeClassifier::fnr(std::vector<float*> &positive_set) {
  int fn = 0;
  for (int i=0; i<positive_set.size(); i++) if (this->classify(positive_set[i], this->baseres, 0, 0, 0, 1) == false) fn++;
  return float(fn)/float(positive_set.size());
}

float CascadeClassifier::fpr(std::vector<float*> &negative_set) {
  int fp = 0;
  for (int i=0; i<negative_set.size(); i++) if (this->classify(negative_set[i], this->baseres, 0, 0, 0, 1) == true) fp++;
  return float(fp)/float(negative_set.size());
}

void CascadeClassifier::scale(float s) {
  this->baseres = (this->baseres)*s;
  for (std::vector<StrongClassifier*>::iterator it = sc.begin(); it != sc.end(); ++it) {
    (*it)->scale(s);
  }
}

int CascadeClassifier::getBaseResolution() {
  return this->baseres;
}

std::vector<StrongClassifier*> CascadeClassifier::getStrongClassifiers() {
  return this->sc;
}
