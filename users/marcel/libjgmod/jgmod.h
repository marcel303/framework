#ifndef JGMOD_H
#define JGMOD_H

#include "port.h"

#ifndef JGM_ID
    #define JGM_ID  DAT_ID('J','G','M',' ')
#endif

#ifndef null
#define null    0
#endif

#ifndef uchar
#define uchar   unsigned char
#endif

#ifndef ushort
#define ushort  unsigned short
#endif

#ifndef uint
#define uint    unsigned int
#endif

#ifndef NULL
#define NULL 0
#endif

#ifndef TRUE
#define TRUE -1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define JGMOD_AUTHOR        "Guan Foo Wah"
#define JGMOD_VERSION       0
#define JGMOD_SUB_VERSION   99
#define JGMOD_VERSION_STR   "0.99"
#define JGMOD_DATE_STR      "15 October 2001"
#define JGMOD_DATE          20021015        /* yyyymmdd */

#define NTSC                3579546L
#define JGMOD_PRIORITY      192
#define MAX_ALLEG_VOICE     64
#define LOOP_OFF            0
#define LOOP_ON             1
#define LOOP_BIDI           2

#define ENV_ON              1
#define ENV_SUS             2
#define ENV_LOOP            4

#define XM_MODE             1
#define PERIOD_MODE         2
#define LINEAR_MODE         4


#define PTEFFECT_0          0
#define PTEFFECT_1          1
#define PTEFFECT_2          2
#define PTEFFECT_3          3
#define PTEFFECT_4          4
#define PTEFFECT_5          5
#define PTEFFECT_6          6
#define PTEFFECT_7          7
#define PTEFFECT_8          8
#define PTEFFECT_9          9
#define PTEFFECT_A          10
#define PTEFFECT_B          11
#define PTEFFECT_C          12
#define PTEFFECT_D          13
#define PTEFFECT_E          14
#define PTEFFECT_F          15

#define S3EFFECT_A          16
#define S3EFFECT_D          17
#define S3EFFECT_E          18
#define S3EFFECT_F          19
#define S3EFFECT_I          20
#define S3EFFECT_J          21
#define S3EFFECT_K          22
#define S3EFFECT_L          23
#define S3EFFECT_Q          24
#define S3EFFECT_R          25
#define S3EFFECT_T          26
#define S3EFFECT_U          27
#define S3EFFECT_V          28
#define S3EFFECT_X          29

#define XMEFFECT_1          30
#define XMEFFECT_2          31
#define XMEFFECT_5          32
#define XMEFFECT_6          33
#define XMEFFECT_A          34
#define XMEFFECT_H          35
#define XMEFFECT_K          36
#define XMEFFECT_L          37
#define XMEFFECT_P          38
#define XMEFFECT_X          39


//this is used in get_mod_info() function.
#define MOD15_TYPE          1
#define MOD31_TYPE          2
#define S3M_TYPE            3
#define XM_TYPE             4
#define IT_TYPE             5
#define JGM_TYPE            6
#define UNREAL_S3M_TYPE     7
#define UNREAL_XM_TYPE      8
#define UNREAL_IT_TYPE      9

//-- Header ------------------------------------------------------------------
typedef struct ENVELOPE_INFO
{
    int env[12];
    int pos[12];
    int flg;
    int pts;
    int loopbeg;
    int loopend;
    int susbeg;
    int susend;
    int a;
    int b;
    int p;
    int v;

}ENVELOPE_INFO;


typedef struct CHANNEL_INFO
{
    ENVELOPE_INFO volenv;
    ENVELOPE_INFO panenv;

    int instrument;
    int sample;
    int volume;
    int note;
    int period;
    int c2spd;
    int transpose;
    int pan;
    int kick;       // TRUE if sample needs to be restarted
    int keyon;
    int volfade;    // volume fadeout
    int instfade;   // how much volume to subtract from volfade

    int temp_volume;
    int temp_period;
    int temp_pan;

    int pan_slide_common;
    int pan_slide;
    int pan_slide_left;
    int pan_slide_right;

    int pro_pitch_slide_on;
    int pro_pitch_slide;
    int pro_fine_pitch_slide;
    int s3m_pitch_slide_on;
    int s3m_pitch_slide;
    int s3m_fine_pitch_slide;
    int xm_pitch_slide_up_on;
    int xm_pitch_slide_up;
    int xm_pitch_slide_down_on;
    int xm_pitch_slide_down;
    int xm_fine_pitch_slide_up;
    int xm_fine_pitch_slide_down;
    int xm_extra_fine_pitch_slide_up;
    int xm_x_up;
    int xm_x_down;

    int pro_volume_slide;
    int s3m_volume_slide_on;
    int s3m_fine_volume_slide;
    int s3m_volume_slide;
    int xm_volume_slide_on;
    int xm_volume_slide;
    int xm_fine_volume_slide_up;
    int xm_fine_volume_slide_down;

    int loop_on;
    int loop_times;
    int loop_start;

    int tremolo_on;
    int tremolo_waveform;
    char tremolo_pointer;
    int tremolo_speed;
    int tremolo_depth;
    int tremolo_shift;
    
    int vibrato_on;    
    int vibrato_waveform;
    char vibrato_pointer;
    int vibrato_speed;
    int vibrato_depth;
    int vibrato_shift;

    int slide2period_on;
    int slide2period_spd;
    int slide2period;

    int arpeggio_on;
    int arpeggio;

    int tremor_on;
    int tremor_count;
    int tremor_set;

    int delay_sample;
    int cut_sample;
    int glissando;
    int retrig;
    int s3m_retrig_on;
    int s3m_retrig;
    int s3m_retrig_slide;

    int sample_offset_on;
    int sample_offset;

    int global_volume_slide_on;
    int global_volume_slide;

}CHANNEL_INFO; 

typedef struct MUSIC_INFO
{
    int max_chn;
    int no_chn;

    int tick;
    int pos;
    int pat;
    int trk;
    int flag;

    int bpm;
    int tempo;
    int speed_ratio;
    int pitch_ratio;
    int global_volume;
    
    int new_pos;        // for pattern break
    int new_trk;        // or position jump
    int pattern_delay;  // pattern delay

    int skip_pos;       // for next_pattern
    int skip_trk;       // or prev_pattern
    int loop;           // replay the music if ended
    int pause;          // for pause function
    int forbid;
    int is_playing;

}MUSIC_INFO; 

typedef struct NOTE_INFO
{
    int sample;
    int note;
    int volume;
    int command;
    int extcommand;
}NOTE_INFO;


typedef struct SAMPLE_INFO
{
    int lenght;
    int c2spd;
    int transpose;
    int volume;
    int pan;
    int repoff;
    int replen;
    int loop;

    int vibrato_type;
    int vibrato_spd;
    int vibrato_depth;
    int vibrato_rate;

}SAMPLE_INFO;


typedef struct INSTRUMENT_INFO
{
    int sample_number[96];

    int volenv[12];
    int volpos[12];
    int no_volenv;
    int vol_type;
    int vol_susbeg;
    int vol_susend;
    int vol_begin;
    int vol_end;

    int panenv[12];
    int panpos[12];
    int no_panenv;
    int pan_type;
    int pan_susbeg;
    int pan_susend;
    int pan_begin;
    int pan_end;

    int volume_fadeout;

}INSTRUMENT_INFO;


typedef struct PATTERN_INFO
{
    NOTE_INFO *ni;
    int no_pos;
}PATTERN_INFO;

typedef struct JGMOD
{
    char name[29];
    SAMPLE_INFO *si;
    PATTERN_INFO *pi;
    INSTRUMENT_INFO *ii;
    SAMPLE *s;

    int no_trk;
    int no_pat;
    int pat_table[256];
    int panning[MAX_ALLEG_VOICE];
    int flag;

    int tempo;
    int bpm;

    int restart_pos;       
    int no_chn;
    int no_instrument;
    int no_sample;
    int global_volume;

}JGMOD;

typedef struct JGMOD_INFO
{
    int type;
    char type_name[20];
    char name[29];

}JGMOD_INFO;

// todo : add setError method, remove sprintf calls

typedef struct JGMOD_PLAYER
{
	int mod_init;
	SAMPLE *fake_sample;
	
	JGMOD *of;
	volatile MUSIC_INFO mi;
	volatile int voice_table[MAX_ALLEG_VOICE];
	volatile CHANNEL_INFO ci[MAX_ALLEG_VOICE];
	volatile int mod_volume;
	
	// load_s3m.cpp
	int fast_loading;
	
	// mod.cpp
	int enable_lasttrk_loop;
	int enable_m15;
	
	JGMOD_PLAYER()
	{
		mod_init = FALSE;
		fake_sample = nullptr;
		
		of = nullptr;
		for (int i = 0; i < MAX_ALLEG_VOICE; ++i)
			voice_table[i] = -1;
		mod_volume = 255;
		
		// load_s3m.cpp
		fast_loading = TRUE;
		
		// mod.cpp
		enable_lasttrk_loop = TRUE;
		enable_m15 = FALSE;
	}
	
	// main api
	int init(int no_voices);
	void shut (void);
	
	static JGMOD *load_mod (char *filename, int fast_loading = TRUE, int enable_m15 = FALSE);
	static void mod_interrupt_proc (void * data);
	void mod_interrupt (void);
	void play (JGMOD *j, int loop);
	void next_track (void);
	void prev_track (void);
	void goto_track (int new_track);
	void stop (void);
	int is_playing (void);
	void pause (void);
	void resume (void);
	int is_paused (void);
	void destroy_mod();
	static void destroy_mod (JGMOD *j); // todo : shouldn't be a member
	void set_volume (int volume);
	int get_volume (void);
	SAMPLE *get_jgmod_sample (JGMOD *j, int sample_no);
	void set_speed (int speed);
	void set_pitch (int pitch);
	void toggle_pause_mode (void);
	
	static int get_info (char *filename, JGMOD_INFO *ji, int enable_m15 = FALSE);
	
	// -- located in player2.c ---------------------------------------------------
	int find_lower_period(int period, int times);
	static NOTE_INFO *get_note (JGMOD *j, int pat, int pos, int chn);
	int calc_pan (int chn);
	int calc_volume (int volume);
	int note2period (int note, int c2spd);
	int get_jgmod_sample_no (int instrument_no, int note_no);
	int period2pitch (int period);
	static int interpolate(int p, int p1, int p2, int v1, int v2);

	void parse_extended_command (int chn, int extcommand);
	void parse_old_note (int chn, int note, int sample_no);

	void parse_pro_pitch_slide_down (int chn, int extcommand);
	void parse_pro_pitch_slide_up (int chn, int extcommand);
	void parse_pro_volume_slide (int chn, int extcommand);
	void parse_vibrato (int chn, int extcommand, int shift);
	void parse_tremolo (int chn, int extcommand, int shift);
	void parse_slide2period (int chn, int extcommand, int note);
	void parse_pro_arpeggio (int chn, int extcommand);

	void do_position_jump (int extcommand);
	void do_pattern_break (int extcommand);
	void do_pro_tempo_bpm (int extcommand);
	void do_pattern_loop (int chn, int extcommand);
	void do_vibrato (int chn);
	void do_tremolo (int chn);
	void do_slide2period (int chn);
	void do_arpeggio (int chn);
	void do_delay_sample (int chn);

	// -- located in player3.c ---------------------------------------------------
	void parse_volume_command (int chn, int volume, int note);
	void parse_note_command (int chn, int note);

	void parse_s3m_volume_slide (int chn, int extcommand);
	void parse_s3m_portamento_down (int chn, int extcommand);
	void parse_s3m_portamento_up (int chn, int extcommand);
	void parse_s3m_arpeggio (int chn, int extcommand);
	void parse_s3m_panning (int chn, int extcommand);
	void parse_tremor (int chn, int extcommand);
	void parse_s3m_retrig (int chn, int extcommand);

	void do_global_volume (int extcommand);
	void do_s3m_set_tempo (int extcommand);
	void do_s3m_set_bpm (int extcommand);
	void do_s3m_volume_slide (int chn);
	void do_s3m_portamento (int chn);
	void do_tremor (int chn);
	void do_s3m_retrig (int chn);

	// -- located in player4.c ---------------------------------------------------
	void parse_new_note (int chn, int note, int sample_no);

	void parse_xm_volume_slide (int chn, int extcommand);
	void parse_xm_pitch_slide_up (int chn, int extcommand);
	void parse_xm_pitch_slide_down (int chn, int extcommand);
	void parse_xm_pan_slide (int chn, int extcommand);
	void parse_global_volume_slide (int chn, int extcommand);
	void parse_xm_set_envelop_position (volatile ENVELOPE_INFO *t, int extcommand);

	void do_xm_volume_slide (int chn);
	void do_xm_pitch_slide_up (int chn);
	void do_xm_pitch_slide_down (int chn);
	void do_xm_pan_slide (int chn);
	void do_global_volume_slide(int chn);
	void do_xm_x (int chn, int extcommand);

	static void process_envelope (volatile ENVELOPE_INFO *t, int v, int keyon);
	static void start_envelope (volatile ENVELOPE_INFO *t, int *env, int *pos, int flg, int pts, int loopbeg, int loopend, int susbeg, int susend);

	// mod.cpp internal
	static int detect_m31 (char *filename);
	static int detect_m15 (char *filename);
	static int detect_s3m (char *filename);
	static int detect_xm (char *filename);
	static int detect_it(char *filename);
	static int detect_jgm (char *filename);
	static int detect_unreal_it (char *filename);
	static int detect_unreal_xm (char *filename);
	static int detect_unreal_s3m (char *filename);
	static JGMOD *load_m (char *filename, int no_inst);
	static JGMOD *load_s3m (char *filename, int start_offset, int fast_loading);
	static JGMOD *load_it (char *filename, int start_offset);
	static JGMOD *load_xm (char *filename, int start_offset);
	static JGMOD *load_jgm (JGMOD_FILE *f);
	static void *jgmod_calloc (int size);
	static int get_jgm_info(JGMOD_FILE *f, JGMOD_INFO *ji);
	static int get_it_info(char *filename, int start_offset, JGMOD_INFO *ji);
	static int get_s3m_info (char *filename, int start_offset, JGMOD_INFO *ji);
	static int get_xm_info (char *filename, int start_offset, JGMOD_INFO *ji);
	static int get_m_info(char *filename, int no_inst, JGMOD_INFO *ji);
}JGMOD_PLAYER;

//-- externs -----------------------------------------------------------------

// todo : remove global
extern char jgmod_error[80];

void setError(const char * format, ...);

#endif  // for JGMOD_H
