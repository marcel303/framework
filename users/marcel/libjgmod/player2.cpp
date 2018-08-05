/*
 *
 *                _______  _______  __________  _______  _____ 
 *               /____  / / _____/ /         / / ___  / / ___ \
 *               __  / / / / ____ / //   // / / /  / / / /  / /
 *             /  /_/ / / /__/ / / / /_/ / / / /__/ / / /__/ /
 *            /______/ /______/ /_/     /_/ /______/ /______/
 *
 *
 *
 *
 *  Protracker effects and misc stuff are located here */

#include <math.h>
#include "framework-allegro2.h"
#include "jgmod.h"
#include "jshare.h"

#define speed_ratio     mi.speed_ratio / 100
#define pitch_ratio     mi.pitch_ratio / 100

#define LOGFAC 32


extern volatile const int mod_finetune[];

int find_lower_period(int period, int times);

// for converting linear period to frequency
static const unsigned long lintab[768] =
{   535232,534749,534266,533784,533303,532822,532341,531861,
    531381,530902,530423,529944,529466,528988,528511,528034,
    527558,527082,526607,526131,525657,525183,524709,524236,
    523763,523290,522818,522346,521875,521404,520934,520464,
    519994,519525,519057,518588,518121,517653,517186,516720,
    516253,515788,515322,514858,514393,513929,513465,513002,
    512539,512077,511615,511154,510692,510232,509771,509312,
    508852,508393,507934,507476,507018,506561,506104,505647,
    505191,504735,504280,503825,503371,502917,502463,502010,
    501557,501104,500652,500201,499749,499298,498848,498398,
    497948,497499,497050,496602,496154,495706,495259,494812,
    494366,493920,493474,493029,492585,492140,491696,491253,
    490809,490367,489924,489482,489041,488600,488159,487718,
    487278,486839,486400,485961,485522,485084,484647,484210,
    483773,483336,482900,482465,482029,481595,481160,480726,
    480292,479859,479426,478994,478562,478130,477699,477268,
    476837,476407,475977,475548,475119,474690,474262,473834,
    473407,472979,472553,472126,471701,471275,470850,470425,
    470001,469577,469153,468730,468307,467884,467462,467041,
    466619,466198,465778,465358,464938,464518,464099,463681,
    463262,462844,462427,462010,461593,461177,460760,460345,
    459930,459515,459100,458686,458272,457859,457446,457033,
    456621,456209,455797,455386,454975,454565,454155,453745,
    453336,452927,452518,452110,451702,451294,450887,450481,
    450074,449668,449262,448857,448452,448048,447644,447240,
    446836,446433,446030,445628,445226,444824,444423,444022,
    443622,443221,442821,442422,442023,441624,441226,440828,
    440430,440033,439636,439239,438843,438447,438051,437656,
    437261,436867,436473,436079,435686,435293,434900,434508,
    434116,433724,433333,432942,432551,432161,431771,431382,
    430992,430604,430215,429827,429439,429052,428665,428278,
    427892,427506,427120,426735,426350,425965,425581,425197,
    424813,424430,424047,423665,423283,422901,422519,422138,
    421757,421377,420997,420617,420237,419858,419479,419101,
    418723,418345,417968,417591,417214,416838,416462,416086,
    415711,415336,414961,414586,414212,413839,413465,413092,
    412720,412347,411975,411604,411232,410862,410491,410121,
    409751,409381,409012,408643,408274,407906,407538,407170,
    406803,406436,406069,405703,405337,404971,404606,404241,
    403876,403512,403148,402784,402421,402058,401695,401333,
    400970,400609,400247,399886,399525,399165,398805,398445,
    398086,397727,397368,397009,396651,396293,395936,395579,
    395222,394865,394509,394153,393798,393442,393087,392733,
    392378,392024,391671,391317,390964,390612,390259,389907,
    389556,389204,388853,388502,388152,387802,387452,387102,
    386753,386404,386056,385707,385359,385012,384664,384317,
    383971,383624,383278,382932,382587,382242,381897,381552,
    381208,380864,380521,380177,379834,379492,379149,378807,

    378466,378124,377783,377442,377102,376762,376422,376082,
    375743,375404,375065,374727,374389,374051,373714,373377,
    373040,372703,372367,372031,371695,371360,371025,370690,
    370356,370022,369688,369355,369021,368688,368356,368023,
    367691,367360,367028,366697,366366,366036,365706,365376,
    365046,364717,364388,364059,363731,363403,363075,362747,
    362420,362093,361766,361440,361114,360788,360463,360137,
    359813,359488,359164,358840,358516,358193,357869,357547,
    357224,356902,356580,356258,355937,355616,355295,354974,
    354654,354334,354014,353695,353376,353057,352739,352420,
    352103,351785,351468,351150,350834,350517,350201,349885,
    349569,349254,348939,348624,348310,347995,347682,347368,
    347055,346741,346429,346116,345804,345492,345180,344869,
    344558,344247,343936,343626,343316,343006,342697,342388,
    342079,341770,341462,341154,340846,340539,340231,339924,
    339618,339311,339005,338700,338394,338089,337784,337479,
    337175,336870,336566,336263,335959,335656,335354,335051,
    334749,334447,334145,333844,333542,333242,332941,332641,
    332341,332041,331741,331442,331143,330844,330546,330247,
    329950,329652,329355,329057,328761,328464,328168,327872,
    327576,327280,326985,326690,326395,326101,325807,325513,
    325219,324926,324633,324340,324047,323755,323463,323171,
    322879,322588,322297,322006,321716,321426,321136,320846,
    320557,320267,319978,319690,319401,319113,318825,318538,
    318250,317963,317676,317390,317103,316817,316532,316246,
    315961,315676,315391,315106,314822,314538,314254,313971,
    313688,313405,313122,312839,312557,312275,311994,311712,
    311431,311150,310869,310589,310309,310029,309749,309470,
    309190,308911,308633,308354,308076,307798,307521,307243,
    306966,306689,306412,306136,305860,305584,305308,305033,
    304758,304483,304208,303934,303659,303385,303112,302838,
    302565,302292,302019,301747,301475,301203,300931,300660,
    300388,300117,299847,299576,299306,299036,298766,298497,
    298227,297958,297689,297421,297153,296884,296617,296349,
    296082,295815,295548,295281,295015,294749,294483,294217,
    293952,293686,293421,293157,292892,292628,292364,292100,
    291837,291574,291311,291048,290785,290523,290261,289999,
    289737,289476,289215,288954,288693,288433,288173,287913,
    287653,287393,287134,286875,286616,286358,286099,285841,
    285583,285326,285068,284811,284554,284298,284041,283785,
    283529,283273,283017,282762,282507,282252,281998,281743,
    281489,281235,280981,280728,280475,280222,279969,279716,
    279464,279212,278960,278708,278457,278206,277955,277704,
    277453,277203,276953,276703,276453,276204,275955,275706,
    275457,275209,274960,274712,274465,274217,273970,273722,
    273476,273229,272982,272736,272490,272244,271999,271753,
    271508,271263,271018,270774,270530,270286,270042,269798,
    269555,269312,269069,268826,268583,268341,268099,267857
};


// for converting note to amiga period
static volatile const ushort logtab[]={
	LOGFAC*907,LOGFAC*900,LOGFAC*894,LOGFAC*887,LOGFAC*881,LOGFAC*875,LOGFAC*868,LOGFAC*862,
	LOGFAC*856,LOGFAC*850,LOGFAC*844,LOGFAC*838,LOGFAC*832,LOGFAC*826,LOGFAC*820,LOGFAC*814,
	LOGFAC*808,LOGFAC*802,LOGFAC*796,LOGFAC*791,LOGFAC*785,LOGFAC*779,LOGFAC*774,LOGFAC*768,
	LOGFAC*762,LOGFAC*757,LOGFAC*752,LOGFAC*746,LOGFAC*741,LOGFAC*736,LOGFAC*730,LOGFAC*725,
	LOGFAC*720,LOGFAC*715,LOGFAC*709,LOGFAC*704,LOGFAC*699,LOGFAC*694,LOGFAC*689,LOGFAC*684,
	LOGFAC*678,LOGFAC*675,LOGFAC*670,LOGFAC*665,LOGFAC*660,LOGFAC*655,LOGFAC*651,LOGFAC*646,
	LOGFAC*640,LOGFAC*636,LOGFAC*632,LOGFAC*628,LOGFAC*623,LOGFAC*619,LOGFAC*614,LOGFAC*610,
	LOGFAC*604,LOGFAC*601,LOGFAC*597,LOGFAC*592,LOGFAC*588,LOGFAC*584,LOGFAC*580,LOGFAC*575,
	LOGFAC*570,LOGFAC*567,LOGFAC*563,LOGFAC*559,LOGFAC*555,LOGFAC*551,LOGFAC*547,LOGFAC*543,
	LOGFAC*538,LOGFAC*535,LOGFAC*532,LOGFAC*528,LOGFAC*524,LOGFAC*520,LOGFAC*516,LOGFAC*513,
	LOGFAC*508,LOGFAC*505,LOGFAC*502,LOGFAC*498,LOGFAC*494,LOGFAC*491,LOGFAC*487,LOGFAC*484,
	LOGFAC*480,LOGFAC*477,LOGFAC*474,LOGFAC*470,LOGFAC*467,LOGFAC*463,LOGFAC*460,LOGFAC*457,
	LOGFAC*453,LOGFAC*450,LOGFAC*447,LOGFAC*443,LOGFAC*440,LOGFAC*437,LOGFAC*434,LOGFAC*431
};

volatile const int noteperiod[] = {
    6848, 6464, 6096, 5760, 5424, 5120, 4832, 4560, 4304, 4064, 3840, 3628
    };

// for vibrato and tremolo
static volatile const int vib_table[] = {
    0,   24,  49,  74,  97,  120, 141, 161,
    180, 197, 212, 224, 235, 244, 250, 253,
    255, 253, 250, 244, 235, 224, 212, 197,
    180, 161, 141, 120, 97,  74,  49,  24};

int JGMOD_PLAYER::interpolate(int p, int p1, int p2, int v1, int v2)
{
    int dp, dv, di;

    if(p1 == p2)
        return v1;

    dv = v2 - v1;
    dp = p2 - p1;
    di = p  - p1;

    return v1 + ((di*dv) / dp);
}

int JGMOD_PLAYER::find_lower_period(int period, int times)
{
    int result = period;
    int index = 0;

    if (mi.flag & XM_MODE)
        {
        while (result >= period)
            {
            index++;
            if ( note2period (index, 0) <= period )
                {
                index += times;
                result = note2period (index, 0);
                }
            }
        }
    else
        {
        while (result >= period)
            {
            if ( (noteperiod[index%12] >> (index/12)) <= period)
                {
                index += times;
                result = noteperiod[index%12] >> (index/12);
                }

            index++;
            }
        }


    return result;

}

// change the volume from scale of 64 to 255
int JGMOD_PLAYER::calc_volume (int chn)
{
    int temp;

    temp = ci[chn].temp_volume * mi.global_volume * ci[chn].volfade; // 0...2^27
    temp >>= 19;                                    // 0...256
    temp *= mod_volume * ci[chn].volenv.v;          // 0...4177920
    temp >>= 14;                                    // 0...255

    return temp;
}

int JGMOD_PLAYER::calc_pan (int chn)
{
    int temp;

    temp = ci[chn].temp_pan + ((ci[chn].panenv.v - 32) * (128 - ABS(ci[chn].temp_pan - 128)) / 32);
    if (temp > 255)
        temp = 255;
    else if (temp < 0)
        temp = 0;

    return temp;
}

// return the position of the note
NOTE_INFO *JGMOD_PLAYER::get_note (JGMOD *j, int pat, int pos, int chn)
{
    PATTERN_INFO *pi;
    NOTE_INFO *ni;

    pi = j->pi + pat;
    ni = pi->ni + (pos*j->no_chn + chn);

    return ni;
}

int JGMOD_PLAYER::note2period (int note, int c2spd)
{
    if (mi.flag & LINEAR_MODE)
        {
        if (note < 0)
            note = 0;
        else if (note > 118)
            note = 118;

        return (7680 - (note*64) - (c2spd/2) + 64);
        }
    else if (mi.flag & PERIOD_MODE)
        {
        uchar n, o;
        ushort p1, p2, i;

        if (note < 0)
            note = 0;
        else if (note>118)
            note = 118;

        n = note % 12;
        o = note / 12;
        i = (n<<3) + (c2spd>> 4);

        p1 = logtab[i];
        p2 = logtab[i+1];

        return (interpolate (c2spd/16, 0, 15, p1, p2) >> o);
        }
    else
        {
        int temp;

        if (c2spd == 0)
            c2spd = 4242;

        temp =  note * c2spd / 8363;
        return (NTSC << 2) / temp;
        }

}

int JGMOD_PLAYER::get_jgmod_sample_no (int instrument_no, int note_no)
{
    INSTRUMENT_INFO *ii;

    if (mi.flag & XM_MODE)
        {
        if (note_no > 95 || note_no < 0)
            return (jgmod_player.of->no_sample - 1);

        if (instrument_no >= jgmod_player.of->no_instrument || instrument_no < 0)
            return (jgmod_player.of->no_sample - 1);

        ii = jgmod_player.of->ii + instrument_no;
        if (ii->sample_number[note_no] >= jgmod_player.of->no_sample)
            return (jgmod_player.of->no_sample - 1);
        else if (ii->sample_number[note_no] < 0)
            return (jgmod_player.of->no_sample - 1);
        else
            return ii->sample_number[note_no];
        }
    else
        return instrument_no;

}

int JGMOD_PLAYER::period2pitch (int period)
{
    if ( (jgmod_player.of->flag & XM_MODE) && (jgmod_player.of->flag & LINEAR_MODE) )
        {
        int temp;
        //char buf[108];

        //asm ("fnsave %0" : "=m" (buf) : );   // save fpu registers
        //temp = floor(8363.0 * pow ( 2, (4608 - period) / 768.0 ));
        //asm ("frstor %0" : : "m" (buf));    // restore fpu registers    

        temp = lintab[period % 768] >> (period / 768);
        //temp >>= 2;
        return temp;
        }
    else if ( (jgmod_player.of->flag & XM_MODE) && (jgmod_player.of->flag & PERIOD_MODE) )
        return 14317456L / period;
    else
        return ((NTSC << 2) / period);
}

void JGMOD_PLAYER::do_position_jump (int extcommand)
{
    if (jgmod_player.enable_lasttrk_loop == TRUE)
        {
        mi.new_trk = extcommand + 1;

        if (mi.loop == TRUE)    
            if (mi.new_trk > jgmod_player.of->no_trk)
                mi.new_trk = 1;

        if (!mi.new_pos)
            mi.new_pos = 1;
        }
    else
        {
        if (mi.trk < (jgmod_player.of->no_trk-1))
            {
            mi.new_trk = extcommand + 1;

            if (!mi.new_pos)
                mi.new_pos = 1;
            }
        }


}

void JGMOD_PLAYER::do_pattern_break (int extcommand)
{
    PATTERN_INFO *pi;

    if (!mi.new_trk)
        mi.new_trk = mi.trk+2;

    mi.new_pos = (extcommand >> 4) * 10 + (extcommand & 0xF) + 1;

    if (mi.loop == TRUE)
        if (mi.new_trk > jgmod_player.of->no_trk)
            mi.new_trk = 1;

    pi = jgmod_player.of->pi + *(jgmod_player.of->pat_table + mi.new_trk-1);

    if ( (mi.new_pos-1) >= pi->no_pos)
         mi.new_pos -= 1;
}

void JGMOD_PLAYER::do_pro_tempo_bpm (int extcommand)
{
    if (extcommand == 0)
        {}
    else if (extcommand <= 32)
        mi.tempo = extcommand;
    else
        {
        mi.bpm = extcommand;
        //remove_int2 (mod_interrupt_proc, this);
        install_int_ex2 (jgmod_player.mod_interrupt_proc, BPM_TO_TIMER (mi.bpm * 24 * speed_ratio), &jgmod_player);
        }

}

void JGMOD_PLAYER::do_pattern_loop (int chn, int extcommand)
{
    if ( (extcommand & 0xF) == 0)
        ci[chn].loop_start = mi.pos;
    else
        {
        if (ci[chn].loop_times > 0)
            ci[chn].loop_times--;
        else
            ci[chn].loop_times = extcommand & 0xF;

        if (ci[chn].loop_times > 0)
            ci[chn].loop_on = TRUE;
        else
            ci[chn].loop_start = mi.pos+1;
        }
}

void JGMOD_PLAYER::parse_pro_pitch_slide_up (int chn, int extcommand)
{
    ci[chn].pro_pitch_slide_on = TRUE;

    if (extcommand) 
        ci[chn].pro_pitch_slide = (extcommand << 2);

    ci[chn].pro_pitch_slide = -ABS(ci[chn].pro_pitch_slide);
}

void JGMOD_PLAYER::parse_pro_pitch_slide_down (int chn, int extcommand)
{
    ci[chn].pro_pitch_slide_on = TRUE;

    if (extcommand) 
        ci[chn].pro_pitch_slide = (extcommand << 2);

    ci[chn].pro_pitch_slide = ABS(ci[chn].pro_pitch_slide);
}

void JGMOD_PLAYER::parse_pro_volume_slide (int chn, int extcommand)
{
    if (extcommand & 0xF0)
        ci[chn].pro_volume_slide = ((extcommand & 0xF0) >> 4);
    else if (extcommand & 0xF)
        ci[chn].pro_volume_slide = -(extcommand & 0xF);
}

void JGMOD_PLAYER::parse_vibrato (int chn, int extcommand, int shift)
{
    if (!ci[chn].period)
        return;

    ci[chn].vibrato_on = TRUE;
    ci[chn].vibrato_shift = shift;

    if (extcommand >> 4)
        ci[chn].vibrato_speed = (extcommand & 0xF0) >> 2;
                    
    if (extcommand & 0xF)
        ci[chn].vibrato_depth = (extcommand & 0xF);

}

void JGMOD_PLAYER::do_vibrato (int chn)
{
    int temp=0;
    int q;

    q = (ci[chn].vibrato_pointer >> 2) & 0x1F;

    if ( (ci[chn].vibrato_waveform & 3) == 0 )
        temp = vib_table[q];
    else if ( (ci[chn].vibrato_waveform & 3) == 1)
        {
        q <<= 3;
        if (ci[chn].vibrato_pointer < 0)
            q = 255 - q;

        temp = q;
        }
    else if ( (ci[chn].vibrato_waveform & 3) == 2)
        temp = 255;
    else
        temp = rand() % 255;

    temp *= ci[chn].vibrato_depth;
    temp >>= ci[chn].vibrato_shift;

    if (ci[chn].vibrato_pointer >= 0)
        temp += ci[chn].period;
    else
        temp = ci[chn].period - temp;

    ci[chn].temp_period = temp;

    if (mi.tick)
        ci[chn].vibrato_pointer += ci[chn].vibrato_speed;

}

void JGMOD_PLAYER::parse_tremolo (int chn, int extcommand, int shift)
{
    ci[chn].tremolo_on = TRUE;
    ci[chn].tremolo_shift = shift;

    if (extcommand & 0xF0)
        ci[chn].tremolo_speed = (extcommand & 0xF0) >> 2;

    if (extcommand & 0xF)
        ci[chn].tremolo_depth = (extcommand & 0xF);

}

void JGMOD_PLAYER::do_tremolo (int chn)
{
    int q;
    int temp;

    q = (ci[chn].tremolo_pointer >> 2) & 0x1F;

    if ( (ci[chn].tremolo_waveform & 3) == 0)
        temp = vib_table[q];
    else if ( (ci[chn].tremolo_waveform & 3) == 1)
        {
        q <<= 3;
        if (ci[chn].tremolo_pointer < 0)
            q = 255 -q;
        
        temp = q;
        }
    else if ( (ci[chn].tremolo_waveform & 3) == 2)
        temp = 255;
    else
        temp = rand() % 255;
        
    temp *= ci[chn].tremolo_depth;
    temp >>= ci[chn].tremolo_shift;

    if (ci[chn].tremolo_pointer >= 0)
        ci[chn].temp_volume = ci[chn].volume + temp;
    else
        ci[chn].temp_volume = ci[chn].volume - temp;

    if (mi.tick)
        ci[chn].tremolo_pointer += ci[chn].tremolo_speed;

}

void JGMOD_PLAYER::parse_slide2period (int chn, int extcommand, int note)
{
    if (!ci[chn].period)
        return;

    if (extcommand > 0)
        ci[chn].slide2period_spd = (extcommand << 2);

    if (note > 0)
        ci[chn].slide2period = note2period (note-1, ci[chn].c2spd);

    ci[chn].slide2period_spd = ABS (ci[chn].slide2period_spd);
    if (ci[chn].slide2period > ci[chn].period)
        ci[chn].slide2period_spd = -ABS (ci[chn].slide2period_spd);

    if (!ci[chn].slide2period)
        return;

    ci[chn].slide2period_on = TRUE;
}

void JGMOD_PLAYER::do_slide2period (int chn)
{
    ci[chn].period -= ci[chn].slide2period_spd;

    if (ci[chn].slide2period_spd > 0)       // sliding down
        {
        if (ci[chn].period < ci[chn].slide2period)
            ci[chn].period = ci[chn].slide2period;
        }
    else
        {
        if (ci[chn].period > ci[chn].slide2period)
            ci[chn].period = ci[chn].slide2period;
        }


    ci[chn].temp_period = ci[chn].period;

}

void JGMOD_PLAYER::parse_pro_arpeggio (int chn, int extcommand)
{
    if (!ci[chn].period)
        return;

    if (extcommand)
        {
        ci[chn].arpeggio = extcommand;
        ci[chn].arpeggio_on = TRUE;
        }

}

void JGMOD_PLAYER::do_arpeggio (int chn)
{
    if ( (mi.tick % 3) == 0)
        ci[chn].temp_period = ci[chn].period;
    else if ( (mi.tick % 3) == 1)
        ci[chn].temp_period = find_lower_period (ci[chn].period, (ci[chn].arpeggio & 0xF0) >> 4);
    else if ( (mi.tick % 3) == 2)
        ci[chn].temp_period = find_lower_period (ci[chn].period, ci[chn].arpeggio & 0xF);

}

void JGMOD_PLAYER::do_delay_sample (int chn)
{
    if (mi.tick == ci[chn].delay_sample)
        {
        ci[chn].kick = TRUE;
        ci[chn].delay_sample = 0;
        }
    else
        ci[chn].kick = FALSE;

}

void JGMOD_PLAYER::parse_old_note (int chn, int note, int sample_no)
{
    SAMPLE_INFO *si;

    if (note > 0 && sample_no >= 0)
        {
        si = jgmod_player.of->si+sample_no;
        ci[chn].sample = sample_no;
        ci[chn].volume = si->volume;
        ci[chn].c2spd = si->c2spd;
        ci[chn].period = note2period (note, ci[chn].c2spd);
        ci[chn].kick = TRUE;
        }
    else if ( (note > 0) && (sample_no < 0) )  // only pitch specified
        {
        si = jgmod_player.of->si + ci[chn].sample;
        ci[chn].period = note2period (note, ci[chn].c2spd);
        ci[chn].kick = TRUE;
        }
    else if ( (note <= 0) && (sample_no >= 0) ) // only sample_spedified
        {
        if ( (ci[chn].sample != sample_no) && (ci[chn].period > 0))
            ci[chn].kick = TRUE;

        si = jgmod_player.of->si + sample_no;
        ci[chn].sample = sample_no;
        ci[chn].volume = si->volume;
        ci[chn].c2spd = si->c2spd;
        }

}

void JGMOD_PLAYER::parse_extended_command (int chn, int extcommand)
{
    switch (extcommand >> 4)
        {
        case 1 :    // fine pitch slide up
            {
            if (!ci[chn].period)
                break;

            if (extcommand & 0xF)
                ci[chn].pro_fine_pitch_slide = ((extcommand & 0xF) << 2);
            else if ( ((extcommand & 0xF) == 0) && (ci[chn].pro_fine_pitch_slide > 0))
                break;

            ci[chn].pro_fine_pitch_slide = -ABS(ci[chn].pro_fine_pitch_slide);
            ci[chn].period += ci[chn].pro_fine_pitch_slide;
            break;
            }
        case 2 :    // fine pitch slide down
            {
            if (!ci[chn].period)
                break;

            if (extcommand & 0xF)
                ci[chn].pro_fine_pitch_slide = ((extcommand & 0xF) << 2);
            else if ( ((extcommand & 0xF) == 0) && (ci[chn].pro_fine_pitch_slide < 0))
                break;

            ci[chn].pro_fine_pitch_slide = ABS(ci[chn].pro_fine_pitch_slide);
            ci[chn].period += ci[chn].pro_fine_pitch_slide;
            break;
            }
        case 3:     // glissando
            {
            if (extcommand & 0xF)
                ci[chn].glissando = TRUE;
            else
                ci[chn].glissando = FALSE;

            break;
            }
        case 4 :    // set vibrato waveform
            {
            ci[chn].vibrato_waveform = (extcommand & 0xF);
            break;
            }
        case 7 :    // set tremolo waveform
            {
            ci[chn].tremolo_waveform = (extcommand & 0xF);
            break;
            }
        case 8 :    // set 16 position panning
            {
            ci[chn].pan = (extcommand & 0xF) * 17;
            break;
            }
        case 9 :    // retrigger sample
            {
            ci[chn].retrig = (extcommand & 0xF);
            break;
            }
        case 10 :   // fine volume slide up
            {
            ci[chn].volume += (extcommand & 0xF);
            break;
            }
        case 11 :   // fine volume slide down
            {
            ci[chn].volume -= (extcommand & 0xF);
            break;
            }
        case 12 :   // cut sample
            {
            ci[chn].cut_sample = (extcommand & 0xF);
            if (ci[chn].cut_sample == 0)
                ci[chn].volume = 0;
            break;
            }
        case 13 :   // delay sample
            {
            ci[chn].delay_sample = (extcommand & 0xF);
            break;
            }
        case 16 :   // stereo control
            {
            ci[chn].pan = (extcommand & 0xF);

            if (ci[chn].pan < 8)
                ci[chn].pan += 8;
            else
                ci[chn].pan -= 8;

            ci[chn].pan *= 17;
            break;
            }
        case 17 :   // xm fine porta down
            {
            if (!ci[chn].period)
                break;

            if (extcommand & 0xF)
                ci[chn].xm_fine_pitch_slide_down = -((extcommand & 0xF) << 2);

            ci[chn].period += ci[chn].xm_fine_pitch_slide_down;

            break;
            }
        case 18 :   // xm fine porta up
            {
            if (!ci[chn].period)
                break;

            if (extcommand & 0xF)
                ci[chn].xm_fine_pitch_slide_up = ((extcommand & 0xF) << 2);

            ci[chn].period += ci[chn].xm_fine_pitch_slide_up;

            break;
            }

        case 19 :   // xm fine volume slide up
            {
            if (extcommand & 0xF)
                ci[chn].xm_fine_volume_slide_up = (extcommand & 0xF);

            ci[chn].temp_volume += ci[chn].xm_fine_volume_slide_up;
            ci[chn].volume += ci[chn].xm_fine_volume_slide_up;

            break;
            }
        case 20 :   // xm fine volume slide down
            {
            if (extcommand & 0xF)
                ci[chn].xm_fine_volume_slide_down = -(extcommand & 0xF);

            ci[chn].temp_volume += ci[chn].xm_fine_volume_slide_down;
            ci[chn].volume += ci[chn].xm_fine_volume_slide_down;

            break;
            }
        }
}
