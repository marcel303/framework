#ifndef RESTYPES_H
#define RESTYPES_H
#pragma once

#include "Res.h"

class Res;
class ResBaseTex;
class ResFont;
class ResIB;
class ResPS;
//class ResRT;
class ResShader;
class ResSnd;
class ResSndSrc;
class ResTex;
class ResTexCD;
class ResTexCF;
class ResTexCR;
class ResTexD;
class ResTexR;
class ResTexRectR;
class ResVB;
class ResVS;

typedef ResPtr<Res> ShRes;
typedef ResPtr<ResBaseTex> ShBaseTex;
typedef ResPtr<ResFont> ShFont;
typedef ResPtr<ResIB> ShIB;
typedef ResPtr<ResPS> ShPS;
//typedef ResPtr<ResRT> ShRT;
typedef ResPtr<ResShader> ShShader;
typedef ResPtr<ResSnd> ShSnd;
typedef ResPtr<ResSndSrc> ShSndSrc;
typedef ResPtr<ResTex> ShTex;
typedef ResPtr<ResTexCD> ShTexCD;
typedef ResPtr<ResTexCF> ShTexCF;
typedef ResPtr<ResTexCR> ShTexCR;
typedef ResPtr<ResTexD> ShTexD;
typedef ResPtr<ResTexR> ShTexR;
typedef ResPtr<ResTexRectR> ShTexRectR;
typedef ResPtr<ResVB> ShVB;
typedef ResPtr<ResVS> ShVS;

#endif
