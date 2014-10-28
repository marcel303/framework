#pragma once

#define MAX_CHANNELS 4

//const static int i255 = 1.0f / 255.0f;

#define FTOI(x) int(Round((x) * 255.0f))
//#define FTOI(x) int(x * 255.0f)
#define ITOF(x) ((x) / 255.0f)
//#define ITOF(x) ((x) * i255)

typedef float sample_t;
