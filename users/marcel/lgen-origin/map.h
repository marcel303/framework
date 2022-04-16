#ifndef __map_h__
#define __map_h__

//---------------------------------------------------------------------------

#include <string.h>
#include "subdiv.h"

//---------------------------------------------------------------------------

#define MAP_DIAMOND_SQUARE	1
#define MAP_OFFSET_SQUARE	2

#define TILE_WATER		0
#define TILE_LAND		1
#define TILE_ROCK		2

#define MAP_F_ISLAND		0x01

//---------------------------------------------------------------------------

typedef struct {
	char type;
	int dir;
        int image;
        float h;
        int l;
        char flags;
} tile_t;

//---------------------------------------------------------------------------

typedef struct {
	int x, y;
} player_t;

//---------------------------------------------------------------------------

class map_t {
 public:
	map_t() {

        	tile = 0;
                size = 0;
                player = 0;
                players = 0;

        }

        ~map_t() {

        	setSize(0);

        }

 public:
	tile_t** tile;
        int size;
        player_t* player;
        int players;

 public:
	void setSize(int aSize) {

        	if (aSize == size)
                	return;

        	if (size > 0) {
	        	for (int i=0; i<size; i++)
        	        	delete[] tile[i];
			delete[] tile;
                        tile = 0;
                        size = 0;
		}

                if (aSize <= 0)
                	return;

		tile = new tile_t*[aSize];
                for (int i=0; i<aSize; i++) {
                	tile[i] = new tile_t[aSize];
                        memset(tile[i], 0, sizeof(tile_t)*aSize);
		}
		size = aSize;

        }

        void setPlayers(int n) {

        	if (player)
                	delete[] player;
		player = 0;
                players = 0;

                if (n <= 0)
                	return;

		player = new player_t[n];
                memset(player, 0, sizeof(player_t)*n);
                players = n;

        }

};

//---------------------------------------------------------------------------

extern map_t* map;

//---------------------------------------------------------------------------

extern void map_gen(int type, int size);
extern BITMAP* gen_radarmap(map_t* map, int size, bool trans);

//---------------------------------------------------------------------------

#endif
