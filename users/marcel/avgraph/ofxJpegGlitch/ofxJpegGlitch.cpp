//
//  ofxJpegGlitch.cpp
//
//  Created by ISHII 2bit
//

#include "Log.h"
#include "ofxJpegGlitch.h"

typedef unsigned char Byte;

const static float randomMax = 1000000.0f;

void ofxJpegGlitch::setup(int data, int qn, int dht) {
    dataBlock = data;
    qnBlock   = qn;
    dhtBlock  = dht;
}

void ofxJpegGlitch::setJpegBuffer(ofBuffer &buf) {
    this->buf = buf;
}

MarkerType ofxJpegGlitch::calcMarkerType(unsigned char *bytes, int cur) {
    unsigned char b0 = bytes[cur];
    if(b0 == 0xFF) {
        unsigned char b = bytes[cur + 1];
        switch(b) {
            case SOI:  return MT_SOI;
            case EOI:  return MT_EOI;
            case APP0: return MT_APP0;
            case APP1: return MT_APP1;
            case APP2: return MT_APP2;
            case APP3: return MT_APP3;
            case APP4: return MT_APP4;
            case APP5: return MT_APP5;
            case APP6: return MT_APP6;
            case APP7: return MT_APP7;
            case APP8: return MT_APP8;
            case APP9: return MT_APP9;
            case APPa: return MT_APPa;
            case APPb: return MT_APPb;
            case APPc: return MT_APPc;
            case APPd: return MT_APPd;
            case APPe: return MT_APPe;
            case APPf: return MT_APPf;
            case DQT:  return MT_DQT;
            case SOF0: return MT_SOF0;
            case SOF1: return MT_SOF1;
            case SOF2: return MT_SOF2;
            case SOF3: return MT_SOF3;
//            case SOF4: return MT_SOF4;
            case SOF5: return MT_SOF5;
            case SOF6: return MT_SOF6;
            case SOF7: return MT_SOF7;
//            case SOF8: return MT_SOF8;
            case SOF9: return MT_SOF9;
            case SOFa: return MT_SOFa;
            case SOFb: return MT_SOFb;
//            case SOFc: return MT_SOFc;
            case SOFd: return MT_SOFd;
            case SOFe: return MT_SOFe;
            case SOFf: return MT_SOFf;
            case DHT:  return MT_DHT;
            case SOS:  return MT_SOS;
            case DRI:  return MT_DRI;
            case COM:  return MT_COM;
            case RST0: return MT_RST0;
            case RST1: return MT_RST1;
            case RST2: return MT_RST2;
            case RST3: return MT_RST3;
            case RST4: return MT_RST4;
            case RST5: return MT_RST5;
            case RST6: return MT_RST6;
            case RST7: return MT_RST7;
            default: return MT_Unknown;
        }
    }
    return MT_Unknown;
}

int ofxJpegGlitch::calcLength(unsigned char *bytes, int cur) {
    return bytes[cur] * 256 + bytes[cur + 1];
}

void ofxJpegGlitch::glitch() {
    unsigned char* bytes = (unsigned char *)buf.getBinaryBuffer();
    int cur = 0;
    MarkerType startMarker = calcMarkerType(bytes, cur);
    if(startMarker != MT_SOI) {
        LOG_ERR("this isn't jpeg", 0);
        return;
    }
    cur += 2;
    int mcuNumber = 0;
    int bpp = 0;
    int width = 0;
    int height = 0;
    int resetMarkerNum = 0;
    
    while(cur < buf.size()) {
        MarkerType marker = calcMarkerType(bytes, cur);
        cur += 2;
        
        int length = 0;
        switch (marker) {
            case MT_DRI: {
                length = calcLength(bytes, cur);
                mcuNumber = bytes[cur + 2] * 256 + bytes[cur + 3];
                break;
            }
            case MT_DQT: {
                length = calcLength(bytes, cur);
                int offset = 2;
                int Pqn = 0;
                int Tqn = 0;
                int Qn[64];
                while(offset < length) {
                    Pqn = (*(bytes + cur + offset) & 0xF0) >> 4;
                    Tqn = *(bytes + cur + offset) & 0x0F;
                    if(Pqn == 0) {
                        for(int i = 0; i < 64; i++) {
                            Qn[i] = *(bytes + cur + offset + i);
                            // RANDOMIZE
                            if(ofRandom(0.0f, randomMax) < qnBlock) {
                                *(bytes + cur + offset + i) = rand() % 256;
                            }
                        }
                        offset += 64 + 1;
                    } else {
                        for(int i = 0; i < 64; i++) {
                            Qn[i] = *(bytes + cur + offset + 2 * i);
                            Qn[i] = Qn[i] * 256 + *(bytes + cur + offset + 2 * i + 1);
                            // RANDOMIZE
                            if(ofRandom(0.0f, randomMax) < qnBlock) {
                                *(bytes + cur + offset + i) = rand() % 256;
                            }
                        }
                        offset += 128 + 1;
                    }
                }
                break;
            }
            case MT_DHT: {
                length = calcLength(bytes, cur);
                int offset = 2;
                int Tcn = 0;
                int Thn = 0;
                Byte Ln[16];
                Byte Vnn[16][256];
                while(offset < length) {
                    Tcn = (*(bytes + cur + offset) & 0xF0) >> 4;
                    Thn = *(bytes + cur + offset) & 0x0F;
                    offset += 1;
                    memcpy(Ln, bytes + cur + offset, 16);
                    int vOffset = 0;
                    for(int i = 0; i < 16; i++) {
                        memcpy(Vnn[i], bytes + cur + offset + vOffset, Ln[i]);
                        for(int i = 0; i < Ln[i]; i++) {
                            if(ofRandom(0.0f, randomMax) < dhtBlock) {
                                *(bytes + cur + offset + vOffset) = rand() % 256;
                            }
                        }
                        vOffset += Ln[i];
                    }
                    offset += vOffset;
                }
                break;
            }
            case MT_SOS: {
                length = calcLength(bytes, cur);
                break;
            }
            case MT_APP0: case MT_APP1: case MT_APP2: case MT_APP3:
            case MT_APP4: case MT_APP5: case MT_APP6: case MT_APP7:
            case MT_APP8: case MT_APP9: case MT_APPa: case MT_APPb:
            case MT_APPc: case MT_APPd: case MT_APPe: case MT_APPf: {
                length = calcLength(bytes, cur);
                break;
            }
            case MT_SOF0: case MT_SOF1: case MT_SOF2: case MT_SOF3:
            case MT_SOF4: case MT_SOF5: case MT_SOF6: case MT_SOF7:
            case MT_SOF8: case MT_SOF9: case MT_SOFa: case MT_SOFb:
            case MT_SOFc: case MT_SOFd: case MT_SOFe: case MT_SOFf: {
                length = calcLength(bytes, cur);
                bpp    = bytes[cur + 2];
                width  = bytes[cur + 3] * 256 + bytes[cur + 4];
                height = bytes[cur + 5] * 256 + bytes[cur + 6];
                break;
            }
            default: {
                length = calcLength(bytes, cur);
                break;
            }
        }
        cur += length;
        if(marker == MT_SOS) {
            break;
        }
    }
    
    while(cur < buf.size()) {
        if(bytes[cur] == 0xFF && bytes[cur] != 0x0) {
            cur += 2;
        } else {
            // RANDOMIZE
            if(*(bytes + cur) != 0xFF && (ofRandom(0.0f, randomMax) < dataBlock)) {
                *(bytes + cur) = rand() % 255;
            }
            
            cur++;
        }
    }
    bImageLoaded = false;
}
