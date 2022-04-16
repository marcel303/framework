#if 0 // todo : dialog

#include <stdio.h>
#include <string.h>
#include "dialog.h"
#include "main.h"
#include "map.h"

//---------------------------------------------------------------------------

void dlg_init() {

	gui_fg_color = makecol(63, 127, 255);
        gui_bg_color = makecol(0, 0, 0);

}

//---------------------------------------------------------------------------

bool dlg_message(char* message, int type) {

	switch(type) {
        	case DLG_OK:
                	alert(message, 0, 0, "&OK", 0, 'o', 0);
                	return false;
                break;
                case DLG_YESNO:
                	return alert(message, 0, 0, "&Yes", "&No", 'y', 'n')==1?true:false;
                	return false;
                break;
        }

        return false;

}

//---------------------------------------------------------------------------

#define D_CFG_SIZE		30
#define D_CFG_BOX		0
#define D_CFG_OK		1
#define D_CFG_DEFAULT		2
#define D_CFG_TXT_LAND		3
#define D_CFG_TXT_WATER		4
#define D_CFG_TXT_ROCK		5
#define D_CFG_TXT_FILTER_SIZE	6
#define D_CFG_TXT_PLAYERS	7
#define D_CFG_SLD_LAND		8
#define D_CFG_SLD_WATER		9
#define D_CFG_SLD_ROCK		10
#define D_CFG_SLD_FILTER_SIZE	11
#define D_CFG_SLD_PLAYERS	12
#define D_CFG_PERC_LAND		13
#define D_CFG_PERC_WATER	14
#define D_CFG_PERC_ROCK		15
#define D_CFG_PERC_FILTER_SIZE	16
#define D_CFG_PERC_PLAYERS	17
#define D_CFG_CHK_MEDIAN	18
#define D_CFG_CHK_MIN		19
#define D_CFG_CHK_MAX		20
#define D_CFG_CHK_DIFF		21
#define D_CFG_CHK_MEAN		22
#define D_CFG_LB_ALGO		23
#define D_CFG_END		24

#define CHECK(_x, _m)	_x |= _m;

#define UNCHECK(_x, _m) \
	if (_x & _m) \
        	_x -= _m; \

static DIALOG d_cfg[D_CFG_SIZE];
static char perc_txt[5][4];

static void update_perc_txt() {

	sprintf(perc_txt[0], "%03d", d_cfg[D_CFG_SLD_LAND].d2);
	sprintf(perc_txt[1], "%03d", d_cfg[D_CFG_SLD_WATER].d2);
	sprintf(perc_txt[2], "%03d", d_cfg[D_CFG_SLD_ROCK].d2);
        int s = (d_cfg[D_CFG_SLD_FILTER_SIZE].d2+1)*2+1;
	sprintf(perc_txt[3], "%dx%d", s, s);
        sprintf(perc_txt[4], "%d", d_cfg[D_CFG_SLD_PLAYERS].d2);

        d_cfg[D_CFG_PERC_LAND].flags |= D_DIRTY;
        d_cfg[D_CFG_PERC_WATER].flags |= D_DIRTY;
        d_cfg[D_CFG_PERC_ROCK].flags |= D_DIRTY;
        d_cfg[D_CFG_PERC_FILTER_SIZE].flags |= D_DIRTY;
        d_cfg[D_CFG_PERC_PLAYERS].flags |= D_DIRTY;

}

static int cb_sld_water(void* dp3, int d2) {

	update_perc_txt();

	return D_REDRAW;

}

static char *cb_lb_algo(int index, int *list_size) {

	if (index < 0) {
        	*list_size = 2;
                return 0;
	} else if (index == 0)
        	return "offset-square";
	else
        	return "diamond-square";

}

static int d_defbutton_proc(int msg, DIALOG* d, int c) {

	int ret = d_button_proc(msg, d, c);

	if (d->flags & D_SELECTED) {
        	d->flags -= D_SELECTED;
                d->flags |= D_DIRTY;
	        d_cfg[D_CFG_SLD_LAND].d2 = 18;
	        d_cfg[D_CFG_SLD_WATER].d2 = 40;
	        d_cfg[D_CFG_SLD_ROCK].d2 = 31;
	        d_cfg[D_CFG_SLD_FILTER_SIZE].d2 = 1;
	        d_cfg[D_CFG_SLD_PLAYERS].d2 = 3;
                CHECK(d_cfg[D_CFG_CHK_MEDIAN].flags, D_SELECTED);
                UNCHECK(d_cfg[D_CFG_CHK_MIN].flags, D_SELECTED);
                UNCHECK(d_cfg[D_CFG_CHK_MAX].flags, D_SELECTED);
                UNCHECK(d_cfg[D_CFG_CHK_DIFF].flags, D_SELECTED);
                CHECK(d_cfg[D_CFG_CHK_MEAN].flags, D_SELECTED);
                for (int i=0; i<D_CFG_SIZE; i++)
                	d_cfg[i].flags |= D_DIRTY;
                update_perc_txt();
        }

        return ret;

}

void dlg_config() {

        memset(d_cfg, 0, sizeof(DIALOG)*D_CFG_SIZE);

        int w = 350;
        int h = 240;

        #define SET(_i, _proc, _x, _y, _w, _h) \
        	d_cfg[_i].proc = _proc; \
        	d_cfg[_i].x = _x; \
        	d_cfg[_i].y = _y; \
        	d_cfg[_i].w = _w; \
        	d_cfg[_i].h = _h;

	SET(D_CFG_BOX, d_shadow_box_proc, 0, 0, w, h);

        SET(D_CFG_OK, d_button_proc, 5, 5, 45, 20);
        char* msg_ok = "&OK";
        d_cfg[D_CFG_OK].key = 'o';
        d_cfg[D_CFG_OK].dp = msg_ok;
        d_cfg[D_CFG_OK].flags |= D_EXIT;

        SET(D_CFG_DEFAULT, d_defbutton_proc, 55, 5, 100, 20);
        char* msg_default = "&default";
        d_cfg[D_CFG_DEFAULT].key = 'd';
        d_cfg[D_CFG_DEFAULT].dp = msg_default;

        SET(D_CFG_TXT_LAND, d_text_proc, 5, 32, 0, 0);
        char* msg_land = "land";
        d_cfg[D_CFG_TXT_LAND].dp = msg_land;

        SET(D_CFG_TXT_WATER, d_text_proc, 5, 57, 0, 0);
        char* msg_water = "water";
        d_cfg[D_CFG_TXT_WATER].dp = msg_water;

        SET(D_CFG_TXT_ROCK, d_text_proc, 5, 82, 0, 0);
        char* msg_rock = "rock";
        d_cfg[D_CFG_TXT_ROCK].dp = msg_rock;

        SET(D_CFG_TXT_FILTER_SIZE, d_text_proc, 5, 207, 0, 0);
        char* msg_filter_size = "filter size";
        d_cfg[D_CFG_TXT_FILTER_SIZE].dp = msg_filter_size;

        SET(D_CFG_TXT_PLAYERS, d_text_proc, 195, 77, 0, 0);
        char* msg_players = "players";
        d_cfg[D_CFG_TXT_PLAYERS].dp = msg_players;

        float total = config.w+config.l+config.r;
        if (!total)
        	total = 1.0;

        SET(D_CFG_SLD_LAND, d_slider_proc, 55, 30, 100, 12);
        d_cfg[D_CFG_SLD_LAND].d1 = 100;
        d_cfg[D_CFG_SLD_LAND].d2 = int((config.r-config.l)*100.0/total);
        d_cfg[D_CFG_SLD_LAND].dp2 = cb_sld_water;

        SET(D_CFG_SLD_WATER, d_slider_proc, 55, 55, 100, 12);
        d_cfg[D_CFG_SLD_WATER].d1 = 100;
        d_cfg[D_CFG_SLD_WATER].d2 = int((config.l-config.w)*100.0/total);
        d_cfg[D_CFG_SLD_WATER].dp2 = cb_sld_water;

        SET(D_CFG_SLD_ROCK, d_slider_proc, 55, 80, 100, 12);
        d_cfg[D_CFG_SLD_ROCK].d1 = 100;
        d_cfg[D_CFG_SLD_ROCK].d2 = int((1.0-config.r)*100.0/total);
        d_cfg[D_CFG_SLD_ROCK].dp2 = cb_sld_water;

        SET(D_CFG_SLD_FILTER_SIZE, d_slider_proc, 105, 205, 50, 12);
        d_cfg[D_CFG_SLD_FILTER_SIZE].d1 = 2;
        d_cfg[D_CFG_SLD_FILTER_SIZE].d2 = ((config.filt_size-1)>>1)-1;
        d_cfg[D_CFG_SLD_FILTER_SIZE].dp2 = cb_sld_water;

        SET(D_CFG_SLD_PLAYERS, d_slider_proc, 245, 75, 50, 12);
        d_cfg[D_CFG_SLD_PLAYERS].d1 = 5;
        d_cfg[D_CFG_SLD_PLAYERS].d2 = config.players;
        d_cfg[D_CFG_SLD_PLAYERS].dp2 = cb_sld_water;

        SET(D_CFG_PERC_LAND, d_text_proc, 160, 32, 0, 0);
        d_cfg[D_CFG_PERC_LAND].dp = perc_txt[0];

        SET(D_CFG_PERC_WATER, d_text_proc, 160, 57, 0, 0);
        d_cfg[D_CFG_PERC_WATER].dp = perc_txt[1];

        SET(D_CFG_PERC_ROCK, d_text_proc, 160, 82, 0, 0);
        d_cfg[D_CFG_PERC_ROCK].dp = perc_txt[2];

        SET(D_CFG_PERC_FILTER_SIZE, d_text_proc, 160, 207, 0, 0);
        d_cfg[D_CFG_PERC_FILTER_SIZE].dp = perc_txt[3];

        SET(D_CFG_PERC_PLAYERS, d_text_proc, 300, 77, 0, 0);
        d_cfg[D_CFG_PERC_PLAYERS].dp = perc_txt[4];

        SET(D_CFG_CHK_MEDIAN, d_check_proc, 5, 105, 150, 15);
        if (config.filt_median)
		d_cfg[D_CFG_CHK_MEDIAN].flags |= D_SELECTED;
        char* msg_median = "median filter";
        d_cfg[D_CFG_CHK_MEDIAN].dp = msg_median;
        d_cfg[D_CFG_CHK_MEDIAN].d1 = 1;

        SET(D_CFG_CHK_MIN, d_check_proc, 5, 125, 150, 15);
        if (config.filt_min)
		d_cfg[D_CFG_CHK_MIN].flags |= D_SELECTED;
        char* msg_min = "min filter";
        d_cfg[D_CFG_CHK_MIN].dp = msg_min;
        d_cfg[D_CFG_CHK_MIN].d1 = 1;

        SET(D_CFG_CHK_MAX, d_check_proc, 5, 145, 150, 15);
        if (config.filt_max)
		d_cfg[D_CFG_CHK_MAX].flags |= D_SELECTED;
        char* msg_max = "max filter";
        d_cfg[D_CFG_CHK_MAX].dp = msg_max;
        d_cfg[D_CFG_CHK_MAX].d1 = 1;

        SET(D_CFG_CHK_DIFF, d_check_proc, 5, 165, 150, 15);
        if (config.filt_diff)
		d_cfg[D_CFG_CHK_DIFF].flags |= D_SELECTED;
        char* msg_diff = "diff filter";
        d_cfg[D_CFG_CHK_DIFF].dp = msg_diff;
        d_cfg[D_CFG_CHK_DIFF].d1 = 1;

        SET(D_CFG_CHK_MEAN, d_check_proc, 5, 185, 150, 15);
        if (config.filt_mean)
		d_cfg[D_CFG_CHK_MEAN].flags |= D_SELECTED;
        char* msg_mean = "mean filter";
        d_cfg[D_CFG_CHK_MEAN].dp = msg_mean;
        d_cfg[D_CFG_CHK_MEAN].d1 = 1;

        SET(D_CFG_LB_ALGO, d_list_proc, 195, 30, 150, 40);
	d_cfg[D_CFG_LB_ALGO].dp = cb_lb_algo;
        int algo = 0;
        if (config.algo == MAP_OFFSET_SQUARE)
        	algo = 0;
	else if (config.algo == MAP_DIAMOND_SQUARE)
        	algo = 1;
	d_cfg[D_CFG_LB_ALGO].d1 = algo;

        d_cfg[D_CFG_END].proc = 0;

        set_dialog_color(d_cfg, gui_fg_color, gui_bg_color);
        position_dialog(d_cfg, (SCREEN_W-w)>>1, (SCREEN_H-h)>>1);
        update_perc_txt();

        int ret = popup_dialog(d_cfg, -1);

	// apply changes

        if (ret == D_CFG_OK) {

		int algo = d_cfg[D_CFG_LB_ALGO].d1;
                if (algo == 0)
                	algo = MAP_OFFSET_SQUARE;
                else if (algo == 1)
                	algo = MAP_DIAMOND_SQUARE;
		config.algo = algo;
        	int total = d_cfg[D_CFG_SLD_WATER].d2+d_cfg[D_CFG_SLD_LAND].d2+d_cfg[D_CFG_SLD_ROCK].d2;
                if (!total)
                	total = 1;
		config.w = 0.0;
		config.l = d_cfg[D_CFG_SLD_WATER].d2/float(total);
		config.r = (d_cfg[D_CFG_SLD_WATER].d2+d_cfg[D_CFG_SLD_LAND].d2)/float(total);
        	config.filt_median 	= (d_cfg[D_CFG_CHK_MEDIAN].flags&D_SELECTED)?true:false;
        	config.filt_min		= (d_cfg[D_CFG_CHK_MIN].flags&D_SELECTED)?true:false;
        	config.filt_max		= (d_cfg[D_CFG_CHK_MAX].flags&D_SELECTED)?true:false;
        	config.filt_diff	= (d_cfg[D_CFG_CHK_DIFF].flags&D_SELECTED)?true:false;
        	config.filt_mean	= (d_cfg[D_CFG_CHK_MEAN].flags&D_SELECTED)?true:false;
                config.filt_size	= (d_cfg[D_CFG_SLD_FILTER_SIZE].d2+1)*2+1;
                config.players		= d_cfg[D_CFG_SLD_PLAYERS].d2;

        }

}

#endif
