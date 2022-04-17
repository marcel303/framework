#include <math.h>
#include <stdlib.h>
#include "main.h"
#include "map.h"

//---------------------------------------------------------------------------

#define SIMPLE_ISLANDS	1    // islands tend to get circular anyways, so simply adding a circle instead of a random blob is also possible

#define SWAP() \
	{ \
		subdiv_t* tmp = tmp1; \
		tmp1 = tmp2; \
		tmp2 = tmp; \
	}

//---------------------------------------------------------------------------

map_t* map = 0; // map

static subdiv_t* tmp1; // temp heightmaps
static subdiv_t* tmp2;

static diamond_square_t map_ds; // heightmap generators
static offset_square_t map_os;

//---------------------------------------------------------------------------

static void subdiv2map(subdiv_t* subdiv, map_t* map);

//---------------------------------------------------------------------------
// (GRAPHICS) FILTERS

static void filt_median(subdiv_t* src, subdiv_t* dst, int size);
static void filt_highpass(subdiv_t* src, subdiv_t* dst, int size);
static void filt_min(subdiv_t* src, subdiv_t* dst, int size);
static void filt_max(subdiv_t* src, subdiv_t* dst, int size);
static void filt_sub(subdiv_t* src, subdiv_t* dst, int size);
static void filt_mean(subdiv_t* src, subdiv_t* dst, int size);

//---------------------------------------------------------------------------
// PLAYER STARTING POSITIONS, MINERALS, ETC

static void add_players(int n, map_t* map, subdiv_t* subdiv);
//static void add_minerals(int n);
static float random_positions(int n, int type, int* x, int* y);

//---------------------------------------------------------------------------
// ISLAND GENERATOR (AS DESCRIBED IN PIXELATE)

typedef struct island_point_t {
	short x, y;
	struct island_point_t* prev;
	struct island_point_t* next;
} island_point_t;

static island_point_t* island_point_h = 0;

static void add_island(map_t* map, subdiv_t* subdiv, int x, int y, int c);
static void do_island_point(int n);
static void mark_island_point(int x, int y);
static void add_island_point(int x, int y);
static void remove_island_point(island_point_t* point);

//---------------------------------------------------------------------------

void map_gen(int type, int size) {

	subdiv_t* tmp;

	if (type == MAP_DIAMOND_SQUARE)
		tmp = &map_ds;
	else if (type == MAP_OFFSET_SQUARE)
		tmp = &map_os;
	else {

		// invalid type

		if (map)
			delete map;
		map = 0;

		return;

	}

	// generate height map

	tmp->setSize(size);
	tmp->generate();
	tmp->rerange(0, 255);

	// create map

	if (map)
		delete map;
	map = new map_t;
	map->setSize(size);

	// filter landscape

	tmp1 = new subdiv_t; tmp1->setSize(size);
	tmp2 = new subdiv_t; tmp2->setSize(size);

	for (int i=0; i<tmp->size; i++)
		memcpy(tmp1->h[i], tmp->h[i], sizeof(int)*tmp->size);

	// apply mean filter (blur)

	if (config.filt_mean) {
		filt_mean(tmp1, tmp2, config.filt_size);
		SWAP();
	}

	// convert height map to map

	subdiv2map(tmp1, map);

	// add some minerals

	if (0) {
		for (int i=0; i<5; i++)
			add_island(map, tmp1, rand()&(map->size-1), rand()&(map->size-1), 255);
	}

	// add some players

	add_players(config.players, map, tmp1);

	// do mean filter again, because of added islands

	if (config.filt_mean) {
		filt_mean(tmp1, tmp2, config.filt_size);
		SWAP();
	}

	// apply some other filters

	if (config.filt_median)	{ filt_median(tmp1, tmp2, config.filt_size);   SWAP(); }
	if (0)                  { filt_highpass(tmp1, tmp2, config.filt_size); SWAP(); }
	if (config.filt_min)	{ filt_min(tmp1, tmp2, config.filt_size);      SWAP(); }
	if (config.filt_max)	{ filt_max(tmp1, tmp2, config.filt_size);      SWAP(); }
	if (config.filt_diff)	{ filt_sub(tmp1, tmp2, config.filt_size);      SWAP(); }

	// convert height map to map

	subdiv2map(tmp1, map);

	delete tmp1;
	delete tmp2;

}

static void subdiv2map(subdiv_t* subdiv, map_t* map) {

	// calc ranges

	int w1 = 0;
	int w2 = int(config.l*255.0)-1;
	int l1 = int(config.l*255.0);
	int l2 = int(config.r*255.0)-1;
	int r1 = int(config.r*255.0);
	int r2 = 255;

	float iw = 1.0/(w2-w1);
	float il = 1.0/(l2-l1);
	float ir = 1.0/(r2-r1);

	// rerange height values

	subdiv->rerange(0, 255);

	// convert height values to tile types

	for (int i=0; i<map->size; i++) {

		tile_t* tile = map->tile[i];
		int* h = subdiv->h[i];

		for (int j=0; j<map->size; j++) {

			// water

			if (*h >= w1 && *h <= w2) {
				tile->type = TILE_WATER;
					tile->l = int((0.25+(*h-w1)*iw*0.25)*255.0);
			}

			// land

			else if (*h >= l1 && *h <= l2) {
				tile->type = TILE_LAND;
					tile->l = int((0.5+(*h-l1)*il*0.5)*255.0);
			}

			// rock

			else if (*h >= r1 && *h <= r2) {
				tile->type = TILE_ROCK;
				tile->l = int((1.0-(*h-r1)*ir*0.5)*255.0);
			}

			tile++;
			h++;

		}
	}

}

//---------------------------------------------------------------------------

#define HVAL(_x, _y)    h[(_x)+s2+((_y)+s2)*size]
#define SVAL(_x, _y)    src->h[(_x)&m][(_y)&m]
#define DVAL(_x, _y)    dst->h[(_x)&m][(_y)&m]

#define FILT_START() \
	int* h = new int[size*size]; \
	int s2 = (size-1)>>1; \
        int m = src->size-1; \
	for (int i=0; i<src->size; i++) { \
		for (int j=0; j<src->size; j++) { \
 \
 			int *th = h; \
			for (int y=-s2; y<=s2; y++) \
				for (int x=-s2; x<=s2; x++) \
					*th++ = SVAL(i+x, j+y);

#define FILT_END() \
		} \
	} \
  	delete[] h;

//---------------------------------------------------------------------------

static int cb_median(const void* p1, const void* p2) {

	return *((const int* )p2)-*((const int* )p1);

}

//---------------------------------------------------------------------------

static void filt_median(subdiv_t* src, subdiv_t* dst, int size) {

	// ever wondered what that median filter of your favorite image editor does?

	FILT_START();

	qsort(h, size*size, sizeof(int), cb_median);
	dst->h[i][j] = h[(size*size-1)>>1];

	FILT_END();

}

//---------------------------------------------------------------------------

static void filt_highpass(subdiv_t* src, subdiv_t* dst, int size) {

	// sharpens with a so called highpass filter

	FILT_START();

	int c = HVAL(0, 0)*5-
		HVAL(0, -1)-
		HVAL(-1, 0)-
		HVAL(1, 0)-
		HVAL(0, 1);

	if (c < 0)
		c = 0;
	if (c > 255)
		c = 255;

	dst->h[i][j] = c;

	FILT_END();

}

//---------------------------------------------------------------------------

static void filt_min(subdiv_t* src, subdiv_t* dst, int size) {

	// preferres min values

	FILT_START();

	int min = h[0];
	for (int k=1; k<size*size; k++)
		if (h[k] < min)
				min = h[k];
	dst->h[i][j] = min;

	FILT_END();

}

//---------------------------------------------------------------------------

static void filt_max(subdiv_t* src, subdiv_t* dst, int size) {

	// preferres max values

	FILT_START();

	int max = h[0];
	for (int k=1; k<size*size; k++)
		if (h[k] > max)
			max = h[k];

	dst->h[i][j] = max;

	FILT_END();

}

//---------------------------------------------------------------------------

static void filt_sub(subdiv_t* src, subdiv_t* dst, int size) {

	// this is a cool filter to apply to images!

	FILT_START();

	int min, max;
	min = max = h[0];

	for (int k=1; k<size*size; k++)
		if (h[k] < min)
			min = h[k];
	else if (h[k] > max)
		max = h[k];

	dst->h[i][j] = max-min;

	FILT_END();

}

//---------------------------------------------------------------------------

static void filt_mean(subdiv_t* src, subdiv_t* dst, int size) {

	// mean or blur filter

	FILT_START();

	int total = 0;
	for (int k=0; k<size*size; k++)
		total += h[k];

	dst->h[i][j] = total/(size*size);

	FILT_END();

}

//---------------------------------------------------------------------------

static void add_players(int n, map_t* map, subdiv_t* subdiv) {

	map->setPlayers(n);

	if (n <= 0)
		return;

	int* x = new int[n];
	int* y = new int[n];

	int* tx = new int[n];
	int* ty = new int[n];

	float min = -1.0;

	for (int i=0; i<100; i++) {
		float d = random_positions(n, TILE_LAND, tx, ty);
		if (d < min || min < 0.0) {
			min = d;
			for (int j=0; j<n; j++) {
				x[j] = tx[j];
					y[j] = ty[j];
			}
		}
	}

	delete[] tx;
	delete[] ty;

	int c = int((config.l+config.r)*0.5*255.0);
	for (int i=0; i<n; i++) {
		add_island(map, subdiv, x[i], y[i], c);
			map->player[i].x = x[i];
			map->player[i].y = y[i];
	}

	delete[] x;
	delete[] y;

}

//---------------------------------------------------------------------------

//static void add_minerals(int n) {
//}

//---------------------------------------------------------------------------

static float random_positions(int n, int type, int* x, int* y) {

	for (int i=0; i<n; i++) {

		bool success;
		int tx, ty;

		do {

			tx = 20+(rand()&(map->size-1-40));
			ty = 20+(rand()&(map->size-1-40));

			if (map->tile[tx][ty].type == type)
				success = true;
			else
				success = false;

		} while (!success);

		x[i] = tx;
		y[i] = ty;

	}

	float d = 0;

	float is = sqrt(2)*map->size;

	for (int i=0; i<n; i++) {
		for (int j=0; j<n; j++) {

			if (i != j) {

				float dx = x[i]-x[j];
				float dy = y[i]-y[j];
				float td = sqrt(dx*dx+dy*dy)*is;

				if (!td)
					td = 0.000001;

				d += 1.0/td;

			}
		}
	}

	return d;

}

//---------------------------------------------------------------------------

static void add_island(map_t* map, subdiv_t* subdiv, int x, int y, int c) {

	if (SIMPLE_ISLANDS) {

		// this draws a circle, really ;)

		#define SUBDIV_POINT(_x, _y) subdiv->h[(_x)&mask][(_y)&mask]

		int r = 15;		// circle radius
		int mask = map->size-1;	// FIXME: is it faster this way?

		// skip top and bottom line

		for (int cy=-r+1; cy<r; cy++) {

			// NOTE: maths:
			// x*x + y*y = r*r
			// x*x = r*r - y*y
			// x = sqrt(r*r - y*y)

			int w = (int)sqrt(r*r-cy*cy);
			int x1 = x-w;
			int x2 = x+w;
			for (int i=x1; i<=x2; i++)
				SUBDIV_POINT(i, y+cy) = c;

		}

	} else {

		// this is the algo in pixelate (not a great implementation, but hey,
		// i'm in a hurry :)

		int n = (rand()&3)+4;

		add_island_point(x, y);

		do_island_point(n);

		for (int i=0; i<map->size; i++) {
			for (int j=0; j<map->size; j++) {
				if (map->tile[i][j].flags & MAP_F_ISLAND)
				subdiv->h[i][j] = c;
			}
		}

	}

}

//---------------------------------------------------------------------------

static void do_island_point(int n) {

	if (n <= 0 || !island_point_h)
		return;

	short x = island_point_h->x;
	short y = island_point_h->y;
	remove_island_point(island_point_h);

	char s = (rand()&15)+5;       // size (3..10)
	char pt = (rand()%(s/2))+s/4; // offspring (0..s/2-1)

	if (rand()&1) {

		// horizontal

		for (int i=0; i<s; i++)
			mark_island_point(x-s/2+i, y);

		for (int i=0; i<pt; i++) {
			add_island_point(x-s/2+rand()%s, y);
			do_island_point(n-1);
		}

	} else {

		// vertical

		for (int i=0; i<s; i++)
			mark_island_point(x, y-s/2+i);

		for (int i=0; i<pt; i++) {
			add_island_point(x, y-s/2+rand()%s);
			do_island_point(n-1);
		}

	}

}

//---------------------------------------------------------------------------

static void mark_island_point(int x, int y) {

	// comments are useless here - so complex, so brilliant - you'll never be able to understand this..

	x = x&(map->size-1);
	y = y&(map->size-1);

	map->tile[x][y].flags |= MAP_F_ISLAND;

	// well, you won't understand why this is a seperate function, anyways (??)

}

//---------------------------------------------------------------------------

static void add_island_point(int x, int y) {

	x = x&(map->size-1);
	y = y&(map->size-1);

	island_point_t* tmp = new island_point_t;
	tmp->x = x;
	tmp->y = y;
	tmp->prev = 0;
	tmp->next = island_point_h;
	island_point_h = tmp;

	map->tile[x][y].flags |= MAP_F_ISLAND;

}

//---------------------------------------------------------------------------

static void remove_island_point(island_point_t* point) {

	if (point->prev)
		point->prev->next = point->next;
	if (point->next)
		point->next->prev = point->prev;
	if (point == island_point_h)
		island_point_h = point->next;

	delete point;

}

//---------------------------------------------------------------------------

BITMAP* gen_radarmap(map_t* map, int size, bool trans) {

	BITMAP* tmp = create_bitmap(map->size, map->size);
	BITMAP* bmp = create_bitmap(size, size);

	int mask = map->size-1;
	int c_water = makecol(0, 0, 255);
	int c_land = makecol(255, 0, 0);
	int c_rock = makecol(127, 127, 127);

	for (int i=0; i<map->size; i++) {
		uint32_t* tline = tmp->line[i];
		for (int j=0; j<map->size; j++) {
			int min = map->tile[i][j].type;
			int max = map->tile[i][j].type;
			for (int k=-1; k<=1; k++)
				for (int l=-1; l<=1; l++) {
					int v = map->tile[(i+k)&mask][(j+l)&mask].type;
					if (v < min)
						min = v;
					else if (v > max)
						max = v;
				}
			if (min != max)
				*tline = 255;
			else
				*tline = 0;
			tline++;
		}
	}

	// scale with anti-alias

	for (int j=0; j<size; j++) {
		uint32_t* bline = bmp->line[j];
		for (int i=0; i<size; i++) {
			int x1 = i*tmp->w/bmp->w;
			int y1 = j*tmp->h/bmp->h;
			int x2 = (i+1)*tmp->w/bmp->w-1;
			int y2 = (j+1)*tmp->h/bmp->h-1;
			int c = 0;
			for (int k=x1; k<=x2; k++)
				for (int l=y1; l<=y2; l++)
					c += getpixel(tmp, k, l);
			int h = c/((x2-x1+1)*(y2-y1+1));
			if (h)
				*bline = makecol(h, h, h);
			else {
				if (!trans || !((i+j)&1)) {
					int type = map->tile[y1][x1].type;
					if (type == TILE_WATER)
						*bline = c_water;
					else if (type == TILE_LAND)
						*bline = c_land;
					else if (type == TILE_ROCK)
						*bline = c_rock;
				} else
					*bline = 0;
			}
			bline++;
		}
	}

	// draw player starting positions

	for (int i=0; i<map->players; i++)
		circle(bmp, map->player[i].y*size/map->size, map->player[i].x*size/map->size, 2, makecol(255, 255, 0));

	rect(bmp, 0, 0, size-1, size-1, makecol(255, 255, 255));

	destroy_bitmap(tmp);

	return bmp;

}
