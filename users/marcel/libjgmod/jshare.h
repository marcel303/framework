#ifndef JSHARE_H
#define JSHARE_H

// -- located in player2.c ---------------------------------------------------
NOTE_INFO *get_note (JGMOD *j, int pat, int pos, int chn);
int calc_pan (int chn);
int calc_volume (int volume);
int note2period (int note, int c2spd);
int get_jgmod_sample_no (int instrument_no, int note_no);
int period2pitch (int period);
int interpolate(int p, int p1, int p2, int v1, int v2);

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

void process_envelope (volatile ENVELOPE_INFO *t, int v, int keyon);
void start_envelope (volatile ENVELOPE_INFO *t, int *env, int *pos, int flg, int pts,
    int loopbeg, int loopend, int susbeg, int susend);

#endif
