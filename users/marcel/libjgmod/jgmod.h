#ifndef JGMOD_H
#define JGMOD_H

#ifndef JGM_ID
    #define JGM_ID  DAT_ID('J','G','M',' ')
#endif

#define JGMOD_AUTHOR        "Guan Foo Wah"
#define JGMOD_VERSION       0
#define JGMOD_SUB_VERSION   99
#define JGMOD_VERSION_STR   "0.99"
#define JGMOD_DATE_STR      "15 October 2001"
#define JGMOD_DATE          20021015        /* yyyymmdd */

#define JGMOD_NTSC          3579546L
#define JGMOD_PRIORITY      192
#define JGMOD_MAX_VOICES    64
#define JGMOD_MAX_ENVPTS    25
#define JGMOD_MAX_INSTKEYS  120
#define JGMOD_LOOP_OFF      0
#define JGMOD_LOOP_ON       1
#define JGMOD_LOOP_BIDI     2

#define JGMOD_ENV_ON        1
#define JGMOD_ENV_SUS       2
#define JGMOD_ENV_LOOP      4

#define JGMOD_MODE_XM       1
#define JGMOD_MODE_PERIOD   2
#define JGMOD_MODE_LINEAR   4
#define JGMOD_MODE_IT       8
#define JGMOD_MODE_IT_INST  16


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

// Allegro forward declarations
struct SAMPLE;

// Allegro <-> framework forward declarations
struct AllegroTimerApi;
struct AllegroVoiceApi;

// JGMOD forward declarations
struct CHANNEL_INFO;
struct ENVELOPE_INFO;
struct INSTRUMENT_INFO;
struct JGMOD;
struct JGMOD_INFO;
struct JGMOD_PLAYER;
struct MUSIC_INFO;
struct NOTE_INFO;
struct PATTERN_INFO;
struct SAMPLE_INFO;

// this is used in jgmod_get_info() function.
enum JGMOD_TYPE
{
	JGMOD_TYPE_INVALID,
	JGMOD_TYPE_MOD15,
	JGMOD_TYPE_MOD31,
	JGMOD_TYPE_S3M,
	JGMOD_TYPE_XM,
	JGMOD_TYPE_IT,
	JGMOD_TYPE_UNREAL_S3M,
	JGMOD_TYPE_UNREAL_XM,
	JGMOD_TYPE_UNREAL_IT,
};

struct ENVELOPE_INFO
{
    int env[JGMOD_MAX_ENVPTS];
    int pos[JGMOD_MAX_ENVPTS];
	
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
};

struct CHANNEL_INFO
{
    ENVELOPE_INFO volenv;
    ENVELOPE_INFO panenv;

	int  channel_volume;
    int  instrument;
    int  sample;
    int  volume;
    int  note;
    int  period;
    int  c2spd;
    int  transpose;
    int  pan;
    bool kick;      // TRUE if sample needs to be restarted
    bool keyon;     // FALSE if the key is pressed (sustain). fixme : this seems opposite of what the name intuitively would suggest..
    int  volfade;    // volume fadeout
    int  instfade;   // how much volume to subtract from volfade

    int  temp_volume;
    int  temp_period;
    int  temp_pan;

    int  pan_slide_common;
    int  pan_slide;
    int  pan_slide_left;
    int  pan_slide_right;

    bool pro_pitch_slide_on;
    int  pro_pitch_slide;
    int  pro_fine_pitch_slide;
    bool s3m_pitch_slide_on;
    int  s3m_pitch_slide;
    int  s3m_fine_pitch_slide;
    bool xm_pitch_slide_up_on;
    int  xm_pitch_slide_up;
    bool xm_pitch_slide_down_on;
    int  xm_pitch_slide_down;
    int  xm_fine_pitch_slide_up;
    int  xm_fine_pitch_slide_down;
    int  xm_extra_fine_pitch_slide_up;
    int  xm_x_up;
    int  xm_x_down;

    int  pro_volume_slide;
    bool s3m_volume_slide_on;
    int  s3m_fine_volume_slide;
    int  s3m_volume_slide;
    bool xm_volume_slide_on;
    int  xm_volume_slide;
    int  xm_fine_volume_slide_up;
    int  xm_fine_volume_slide_down;

    bool loop_on;
    int  loop_times;
    int  loop_start;

    bool tremolo_on;
    int  tremolo_waveform;
    char tremolo_pointer;
    int  tremolo_speed;
    int  tremolo_depth;
    int  tremolo_shift;
    
    bool vibrato_on;    
    int  vibrato_waveform;
    char vibrato_pointer;
    int  vibrato_speed;
    int  vibrato_depth;
    int  vibrato_shift;

    bool slide2period_on;
    int  slide2period_spd;
    int  slide2period;

    bool arpeggio_on;
    int  arpeggio;

    bool tremor_on;
    int  tremor_count;
    int  tremor_set;

    int  delay_sample;
    int  cut_sample;
    bool glissando;
	int  retrig;
    bool s3m_retrig_on;
    int  s3m_retrig;
    int  s3m_retrig_slide;

    bool sample_offset_on;
    int  sample_offset;

    bool global_volume_slide_on;
    int  global_volume_slide;
};

struct MUSIC_INFO
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

    int  skip_pos;      // for next_pattern
    int  skip_trk;      // or prev_pattern
	bool loop;          // replay the music if ended
	bool pause;         // for pause function
    bool forbid;
    bool is_playing;
};

struct NOTE_INFO
{
    int sample;
    int note;
    int volume;
    int command;
    int extcommand;
	
    int it_note;
};

struct SAMPLE_INFO
{
	char name[64];
	
    int lenght;
    int c2spd;
    int transpose;
    int volume;        // default volume for this sample when triggered by note
    int global_volume; // volume used during mixing for every instance of this sample
    int pan;
    int repoff;
    int replen;
    int loop;

    int vibrato_type;
    int vibrato_spd;
    int vibrato_depth;
    int vibrato_rate;
};

struct INSTRUMENT_INFO
{
	char name[64];
	
	int key_to_note[JGMOD_MAX_INSTKEYS];
    int sample_number[JGMOD_MAX_INSTKEYS];

    int volenv[JGMOD_MAX_ENVPTS];
    int volpos[JGMOD_MAX_ENVPTS];
    int no_volenv;
    int vol_type;
    int vol_susbeg;
    int vol_susend;
    int vol_begin;
    int vol_end;

    int panenv[JGMOD_MAX_ENVPTS];
    int panpos[JGMOD_MAX_ENVPTS];
    int no_panenv;
    int pan_type;
    int pan_susbeg;
    int pan_susend;
    int pan_begin;
    int pan_end;

    int volume_fadeout;
};

struct PATTERN_INFO
{
    NOTE_INFO * ni;
    int no_pos;
};

struct JGMOD
{
    char name[29];
	
    SAMPLE_INFO * si;
    PATTERN_INFO * pi;
    INSTRUMENT_INFO * ii;
    SAMPLE * s;

    int no_trk;
    int no_pat;
    int pat_table[256];
    int panning[JGMOD_MAX_VOICES];
    int channel_volume[JGMOD_MAX_VOICES];
    bool channel_disabled[JGMOD_MAX_VOICES];
    int flag;

    int tempo;
    int bpm;

    int restart_pos;       
    int no_chn;
    int no_instrument;
    int no_sample;
    int global_volume;
    int mixing_volume;
};

struct JGMOD_INFO
{
    JGMOD_TYPE type;
    char type_name[20];
    char name[29];
};

struct JGMOD_PLAYER
{
	bool is_init;
	SAMPLE * fake_sample;
	
	JGMOD * of = nullptr;
	
	AllegroTimerApi * timerApi = nullptr;
	AllegroVoiceApi * voiceApi = nullptr;
	
	volatile MUSIC_INFO mi;
	volatile int voice_table[JGMOD_MAX_VOICES];
	volatile CHANNEL_INFO ci[JGMOD_MAX_VOICES];
	volatile int mod_volume;
	
	volatile bool enable_lasttrk_loop;
	
	JGMOD_PLAYER();
	
	int init(int no_voices, AllegroTimerApi * timerApi, AllegroVoiceApi * voiceApi);
	void shut ();
	
	void play (JGMOD *j, bool loop, int speed = 100, int pitch = 100);
	void next_track ();
	void prev_track ();
	void goto_track (int new_track);
	void stop ();
	bool is_playing () const;
	void pause ();
	void resume ();
	bool is_paused () const;
	void destroy_mod();
	void set_volume (int volume);
	int get_volume () const;
	SAMPLE *get_jgmod_sample (JGMOD *j, int sample_no);
	void set_loop (bool loop);
	void set_speed (int speed);
	void set_pitch (int pitch);
	void toggle_pause_mode ();
	
protected:
	static void mod_interrupt_proc (void * data);
	void mod_interrupt ();
	
	// -- located in player2.c ---------------------------------------------------
	int find_lower_period(int period, int times) const;
	static NOTE_INFO * get_note (JGMOD * j, int pat, int pos, int chn);
	int calc_pan (int chn) const;
	int calc_volume (int volume) const;
	int note2period (int note, int c2spd) const;
	int get_jgmod_sample_no (int instrument_no, int note_no) const;
	int period2pitch (int period) const;
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
	void parse_xm_set_envelop_position (volatile ENVELOPE_INFO * t, int extcommand);

	void do_xm_volume_slide (int chn);
	void do_xm_pitch_slide_up (int chn);
	void do_xm_pitch_slide_down (int chn);
	void do_xm_pan_slide (int chn);
	void do_global_volume_slide(int chn);
	void do_xm_x (int chn, int extcommand);

	static void process_envelope (volatile ENVELOPE_INFO * t, int v, bool keyon);
	static void start_envelope (volatile ENVELOPE_INFO * t, const int *env, const int *pos, int flg, int pts, int loopbeg, int loopend, int susbeg, int susend);
	
	// -- located in player5.c ---------------------------------------------------
	void parse_it_note (int chn, int key, int note, int sample_no);
};

//-- externs -----------------------------------------------------------------

void jgmod_seterror(const char * format, ...);
const char * jgmod_geterror();

JGMOD *jgmod_load (const char * filename, bool fast_loading = true, bool enable_m15 = false);
void jgmod_destroy (JGMOD * j);

int jgmod_get_info (const char *filename, JGMOD_INFO * ji, bool enable_m15 = false);

#endif  // for JGMOD_H
