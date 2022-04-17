#include <stdlib.h>
#include <string.h>
#include "subdiv.h"

//---------------------------------------------------------------------------

#define RAND1(x, y) ((y) ? (x)+rand()%(y)-((y)/2) : (x))
#define RAND2(x, y) ((x)+(rand()&(y))-((y)>>1))

//---------------------------------------------------------------------------

subdiv_t :: subdiv_t() {

	h = 0;
	size = 0;

}

//---------------------------------------------------------------------------

subdiv_t :: ~subdiv_t() {

	setSize(0);

}

//---------------------------------------------------------------------------

void subdiv_t :: setSize(int aSize) {

	if (aSize == size)
		return;

	if (h) {
		for (int i=0; i<size; i++)
			delete[] h[i];
		delete[] h;
		h = 0;
		size = 0;
	}

	if (aSize <= 0)
		return;

	size = aSize;
	h = new int*[size];
	for (int i=0; i<size; i++)
		h[i] = new int[size];

	clear();

}

//---------------------------------------------------------------------------

void subdiv_t :: clear() {

	if (size <= 0)
		return;

	for (int i=0; i<size; i++)
		memset(h[i], 0, sizeof(int)*size);

}

//---------------------------------------------------------------------------

void subdiv_t :: getMinMax(int* aMin, int* aMax) {

	int min = h[0][0];
	int max = h[0][0];
	for (int i=0; i<size; i++) {
		int* tmp = h[i];
		for (int* tmp2=h[i]+size; tmp<tmp2;) {
			if (*tmp < min)
				min = *tmp;
			else if (*tmp > max)
				max = *tmp;
			tmp++;
			if (*tmp < min)
				min = *tmp;
			else if (*tmp > max)
				max = *tmp;
			tmp++;
			if (*tmp < min)
				min = *tmp;
			else if (*tmp > max)
				max = *tmp;
			tmp++;
			if (*tmp < min)
				min = *tmp;
			else if (*tmp > max)
				max = *tmp;
			tmp++;
		}
	}

	*aMin = min;
	*aMax = max;

}

//---------------------------------------------------------------------------

int subdiv_t :: generate() {

	return 1;

}

//---------------------------------------------------------------------------

void subdiv_t :: rerange(int aMin, int aMax) {

	int min, max;
        getMinMax(&min, &max);
	if (min==max)
		return;
	int tmp = ((aMax-aMin)<<16)/(max-min);

#define RESCALE(v) \
	aMin+(((v-min)*tmp)>>16)

	for (int i=0; i<size; i++) {
		int* line = h[i];
			for (int* line2=line+size; line<line2;) {
				*line = RESCALE(*line);
				line++;
				*line = RESCALE(*line);
				line++;
			}
	}

}

//---------------------------------------------------------------------------

diamond_square_t :: diamond_square_t() : subdiv_t() {
}

//---------------------------------------------------------------------------

diamond_square_t :: ~diamond_square_t() {
}

//---------------------------------------------------------------------------

int diamond_square_t :: avgyvals(int i, int j, int strut, int dim) {

	if (i == 0)
		return ((h[i][(j-strut)&(size-1)] +
			h[i][(j+strut)&(size-1)] +
			h[(i+strut)&(size-1)][j]) / 3);
	else if (i == dim-1)
		return ((h[i][(j-strut)&(size-1)] +
			h[i][(j+strut)&(size-1)] +
			h[(i-strut)&(size-1)][j]) / 3);
	else if (j == 0)
		return ((h[(i-strut)&(size-1)][j] +
			h[(i+strut)&(size-1)][j] +
			h[i][(j+strut)&(size-1)]) / 3);
	else if (j == dim-1)
		return ((h[(i-strut)&(size-1)][j] +
			h[(i+strut)&(size-1)][j] +
			h[i][(j-strut)&(size-1)]) / 3);
	else
		return ((h[(i-strut)&(size-1)][j] +
			h[(i+strut)&(size-1)][j] +
			h[i][(j-strut)&(size-1)] +
			h[i][(j+strut)&(size-1)])>>2);

}

//---------------------------------------------------------------------------
    
int diamond_square_t :: avgyvals2(int i, int j, int strut/*, int dim*/) {

	int tstrut = strut/2;

	return ((h[(i-tstrut)&(size-1)][(j-tstrut)&(size-1)] +
		h[(i-tstrut)&(size-1)][(j+tstrut)&(size-1)] +
		h[(i+tstrut)&(size-1)][(j-tstrut)&(size-1)] +
		h[(i+tstrut)&(size-1)][(j+tstrut)&(size-1)])>>2);

}

//---------------------------------------------------------------------------
    
int diamond_square_t :: generate() {

	int i, j;
	int strut, tstrut;
	int oddline;

	int dim = size;

	clear();
    
	// initialize things
	strut = dim>>1;
    
	// create fractal surface from seeded values
	tstrut = strut;
	while (tstrut > 0) {
		oddline = 0;
		for (i=0; i<dim; i+=tstrut) {
			oddline = (!oddline);
			#if 1
			for (j=0; j<dim; j+=tstrut) {
				if ((oddline) && (j==0)) j+=tstrut;
					h[i][j] = RAND1(avgyvals(i, j, tstrut, dim), tstrut);
				j += tstrut;
			}
			#endif
		}
		#if 1
		if (tstrut/2 != 0) {
			for (i=tstrut/2; i < dim; i+=tstrut) {
				for (j=tstrut/2; j < dim; j+=tstrut) {
					h[i][j] = RAND1(avgyvals2(i, j, tstrut/*, dim*/), tstrut);
				}
			}
		}
		#endif
		tstrut /= 2;
	}

	return 1;

}

//---------------------------------------------------------------------------

offset_square_t :: offset_square_t() : subdiv_t() {
}

//---------------------------------------------------------------------------

offset_square_t :: ~offset_square_t() {
}

//---------------------------------------------------------------------------

int offset_square_t :: generate() {

	int row_offset = 0;  // start at zero for first row

	clear();

	int smask = size-1;
    
	for (int square_size=size; square_size>1; square_size>>=1) {

		int random_range = square_size;

		for (int x1=row_offset; x1<size; x1+=square_size) {

			for (int y1=row_offset; y1<size; y1+=square_size) {

				// Get the four corner points.

				int x2 = (x1+square_size)&smask;
				int y2 = (y1+square_size)&smask;

				int i1 = h[x1][y1];
				int i2 = h[x2][y1];
				int i3 = h[x1][y2];
				int i4 = h[x2][y2];

				// Obtain new points by averaging the corner points.

				int p1 = ((i1 * 9) + (i2 * 3) + (i3 * 3) + (i4)) >> 4;
				int p2 = ((i1 * 3) + (i2 * 9) + (i3) + (i4 * 3)) >> 4;
				int p3 = ((i1 * 3) + (i2) + (i3 * 9) + (i4 * 3)) >> 4;
				int p4 = ((i1) + (i2 * 3) + (i3 * 3) + (i4 * 9)) >> 4;

				// Add a random offset to each new point.

				p1 = RAND2(p1, random_range);
				p2 = RAND2(p2, random_range);
				p3 = RAND2(p3, random_range);
				p4 = RAND2(p4, random_range);

				// Write out the generated points.

				int x3 = (x1 + (square_size>>2))&smask;
				int y3 = (y1 + (square_size>>2))&smask;
				x2 = (x3 + (square_size>>1))&smask;
				y2 = (y3 + (square_size>>1))&smask;

				h[x3][y3] = p1;
				h[x2][y3] = p2;
				h[x3][y2] = p3;
				h[x2][y2] = p4;

			}
		}

		row_offset = square_size>>2;  // set offset for next row

	}

	return 1;

}
