//
//  JpegConstant.h
//
//  Created by ISHII 2bit
//  Copyright (c) 2014年 buffer Renaiss co., ltd. All rights reserved.
//

static const unsigned char SOI  = 0xD8;

static const unsigned char RST0 = 0xD0;
static const unsigned char RST1 = 0xD1;
static const unsigned char RST2 = 0xD2;
static const unsigned char RST3 = 0xD3;
static const unsigned char RST4 = 0xD4;
static const unsigned char RST5 = 0xD5;
static const unsigned char RST6 = 0xD6;
static const unsigned char RST7 = 0xD7;

static const unsigned char EOI  = 0xD9;

static const unsigned char APP0 = 0xE0;
static const unsigned char APP1 = 0xE1;
static const unsigned char APP2 = 0xE2;
static const unsigned char APP3 = 0xE3;
static const unsigned char APP4 = 0xE4;
static const unsigned char APP5 = 0xE5;
static const unsigned char APP6 = 0xE6;
static const unsigned char APP7 = 0xE7;

static const unsigned char APP8 = 0xE8;
static const unsigned char APP9 = 0xE9;
static const unsigned char APPa = 0xEA;
static const unsigned char APPc = 0xEB;
static const unsigned char APPb = 0xEC;
static const unsigned char APPd = 0xED;
static const unsigned char APPe = 0xEE;
static const unsigned char APPf = 0xEF;

static const unsigned char DQT  = 0xDB;

static const unsigned char DHT  = 0xC4;
static const unsigned char SOF0 = 0xC0;
static const unsigned char SOF1 = 0xC1;
static const unsigned char SOF2 = 0xC2;
static const unsigned char SOF3 = 0xC3;
//static const unsigned char SOF4 = 0xC4;
static const unsigned char SOF5 = 0xC5;
static const unsigned char SOF6 = 0xC6;
static const unsigned char SOF7 = 0xC7;
//static const unsigned char SOF8 = 0xC8;
static const unsigned char SOF9 = 0xC9;
static const unsigned char SOFa = 0xCA;
static const unsigned char SOFb = 0xCB;
//static const unsigned char SOFc = 0xCC;
static const unsigned char SOFd = 0xCD;
static const unsigned char SOFe = 0xCE;
static const unsigned char SOFf = 0xCF;
static const unsigned char SOS  = 0xDA;
static const unsigned char DRI  = 0xDD;

static const unsigned char COM  = 0xFE;

typedef enum {
    // 0
    MT_Unknown,
    MT_SOI,
    MT_EOI,
    MT_APP0,
    MT_APP1,
    
    // 5
    MT_APP2,
    MT_APP3,
    MT_APP4,
    MT_APP5,
    MT_APP6,
    
    // 10
    MT_APP7,
    MT_APP8,
    MT_APP9,
    MT_APPa,
    MT_APPb,
    
    // 15
    MT_APPc,
    MT_APPd,
    MT_APPe,
    MT_APPf,
    MT_DQT,
    
    // 20
    MT_SOF0,
    MT_SOF1,
    MT_SOF2,
    MT_SOF3,
    MT_SOF4, // unuse 24
    
    // 25
    MT_SOF5,
    MT_SOF6,
    MT_SOF7,
    MT_SOF8, // unuse 29
    MT_SOF9,
    
    // 30
    MT_SOFa,
    MT_SOFb,
    MT_SOFc, // unuse 32
    MT_SOFd,
    MT_SOFe,
    
    // 35
    MT_SOFf,
    MT_DHT,
    MT_SOS, // 37
    MT_DRI,
    MT_COM,
    
    // 40
    MT_RST0,
    MT_RST1,
    MT_RST2,
    MT_RST3,
    MT_RST4,
    
    // 45
    MT_RST5,
    MT_RST6,
    MT_RST7,
    MT_Num
} MarkerType;