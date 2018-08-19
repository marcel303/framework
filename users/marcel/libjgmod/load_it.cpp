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
 *  IT loader. Unfinished. But you can still keep dreaming on !! */

#include "jgmod.h"
#include "jshare.h"
#include "file_io.h"

#include "framework-allegro2.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/*

todo :

- add IT resonant filter

- add cubic interpolation support

- add instrument support
	- easy enough to load
	- requires NNA support to be fully functional
	- not sure to what degree current player routines cover envelopes
	- requires multiple-sample support, different sample may be selected per note
 
- add NNA and virtual channel support

- add support for disabled channels

- add sustain loop support

*/

//#define JG_debug

extern volatile const int noteperiod[];

namespace jgmod
{

int get_mod_no_pat (int *table, int max_trk);

// to detect unreal IT files
int detect_unreal_it (const char *filename)
{
    JGMOD_FILE *f;
    char id[4];
    int index;
    int start_offset = 0;

    f = jgmod_fopen (filename, "rb");
    if (f == nullptr)
        return 0;

    jgmod_fread (id, 4, f);
    if (memcmp (id, "Áƒ*ž", 4) != 0)    //detect a umx file
        {
        jgmod_fclose (f);
        return -1;
        }

    id[0] = jgmod_getc(f);
    id[1] = jgmod_getc(f);
    id[2] = jgmod_getc(f);
    id[3] = jgmod_getc(f);
    start_offset = 8;

    for (index=0; index<500; index++)
        {
        if (memcmp (id, "IMPM", 4) == 0)    //detect a S3M file
            return (start_offset - 4);

        id[0] = id[1];
        id[1] = id[2];
        id[2] = id[3];
        id[3] = jgmod_getc(f);
        start_offset++;        
        }

    return -1;
}

// convert the it note table into hz
void convert_it_pitch (int *pitch)
{
    int octave;

	// 255 = note cut
	// 254 = note off
	
	// -1 sets volume to 0
	// -2 sets keyon to true. if no envolume, also volume to 0. also enables volfade to zero -> this is key off?
	// -3 sets keyon to false
	
	if (*pitch == 255)  // note off
        {
        *pitch = -3; // todo : not correct!
        return;
        }
    if (*pitch == 254)  // note cut
        {
        *pitch = -2;
        return;
        }
	if (*pitch == 253) // note none
		{
		*pitch = 0;
        return;
		}
	if (*pitch > 119) // note fade
		{
		*pitch = -3; // todo : not correct!
        return;
		}
	
	assert(*pitch >= 0);
    octave = *pitch / 12 - 1;    //pitch / 16
    if (octave < 0)
    	*pitch = noteperiod[(*pitch % 12)] << (-octave);
    else
    	*pitch = noteperiod[(*pitch % 12)] >> octave;

    if (*pitch != 0)
        *pitch = JGMOD_NTSC / *pitch;
}

int detect_it(const char *filename)
{
    JGMOD_FILE *f;
    char id[4];

    f =  jgmod_fopen (filename, "rb");
    if (f == nullptr)
        return 0;

    jgmod_fread (id, 4, f);
    if (memcmp (id, "IMPM", 4) == 0)    //detect successful
        return 1;

    jgmod_fclose (f);
    return -1;
}


int get_it_info(const char *filename, int start_offset, JGMOD_INFO *ji)
{
    JGMOD_FILE *f;

    f = jgmod_fopen (filename, "rb");
    if (f == nullptr)
        {
        jgmod_seterror ("Unable to open %s", filename);
        return -1;
        }

    if (start_offset ==0)
        {
        sprintf (ji->type_name, "IT");
        ji->type = JGMOD_TYPE_IT;
        }
    else
        {
        sprintf (ji->type_name, "Unreal IT (UMX)");
        ji->type = JGMOD_TYPE_UNREAL_IT;
        }

    jgmod_skip (f, 4 + start_offset);
    jgmod_fread (ji->name, 26, f);
    jgmod_fclose (f);
    return 1;
}

//convert it commands to protracker like commands
static void convert_it_command (int *command, int *extcommand, int & last_special)
{
	const char c = (*command) + 0x40;
	
	//printf("command: %c (%d) %02x\n", c, *command, *extcommand);
	
// todo : review this list !!

#define experimentalCommands false

    if (c == 'A')                      // it set tempo
        *command = S3EFFECT_A;
    else if (c == 'B')                 // pattern jump
        *command = PTEFFECT_B;
    else if (c == 'C')                 // pattern break
        *command = PTEFFECT_D;
    else if (c == 'D')                 // s3m volume slide
        *command = S3EFFECT_D;
    else if (c == 'E')                 // todo : ? : S3M portamento down
        *command = S3EFFECT_E;
    else if (c == 'F')                 // todo : ? : S3M portamento up
        *command = S3EFFECT_F;
    else if (c == 'G')                 // slide to note
        *command = PTEFFECT_3;
    else if (c == 'H')                 // vibrato
        *command = PTEFFECT_4;
    else if (c == 'I')                 // s3m tremor
        *command = S3EFFECT_I;
    else if (c == 'J')                // s3m arpeggio
        *command = S3EFFECT_J;
    else if (c == 'K')                // vibrato+volume slide
        *command = S3EFFECT_K;
    else if (c == 'L')                // porta to note
        *command = S3EFFECT_L;
#if experimentalCommands
	else if (c == 'M')
	{
		// set channel volume 0..64
		
		printf("IT command not implemented: '%c': set channel volume\n", c);
		
		*command = 0;
		*extcommand = 0;
	}
	else if (c == 'N')
	{
		// channel volume slide. works like Dxy but operates on the channel volume instead
		
		printf("IT command not implemented: '%c': channel volume slide\n", c);
		
		*command = 0;
		*extcommand = 0;
	}
#endif
    else if (c == 'O')                // set sample offset
        *command = PTEFFECT_9;
#if experimentalCommands
	else if (c == 'P')
	{
		// PFx = panning fine slide right
		// PxF = panning fine slide right
		
		// P0x = panning slide right
		// Px0 = panning slide left
		
		printf("IT command not implemented: '%c': channel panning\n", c);
		
		*command = 0;
		*extcommand = 0;
	}
#endif
    else if (c == 'Q')                // retrigger
        *command = S3EFFECT_Q;
    else if (c == 'R')                // s3m tremolo
        *command = S3EFFECT_R;
    else if (c == 'T')                // set bpm
        *command = S3EFFECT_T;
    else if (c == 'U')                // fine vibrato
        *command = S3EFFECT_U;
    else if (c == 'V')                // set global volume
        *command = S3EFFECT_V;
    else if (c == 'X')                // set panning
        *command = S3EFFECT_X;
    else if (c == 'S')
    {
    	if (*extcommand == 0x00)
    	{
			*extcommand = last_special;
		}
		
		last_special = *extcommand;
		
    	const int no = (*extcommand & 0xF0 ) >> 4;
    	const int value = (*extcommand) & 0x0F;
		
    	if (no == 1)           // glissando
        {
			*command = PTEFFECT_E;
			*extcommand = (*extcommand & 0xF) | 0x30;
        }
		else if (no == 2)      // set finetune
        {
			*command = PTEFFECT_E;
			*extcommand = (*extcommand & 0xF) | 0x50;
		}
        else if (no == 3)     // set vibrato waveform
        {
			*command = PTEFFECT_E;
			*extcommand = (*extcommand & 0xF) | 0x40;
		}
        else if (no == 4)     // set tremolo waveform
        {
			*command = PTEFFECT_E;
			*extcommand = (*extcommand & 0xF) | 0x70;
		}
        else if (no == 8)     // set 16 pan position
        {
        	*command = PTEFFECT_E;
		}
    	else if (no == 0xA)   // stereo control
        {
			*command = PTEFFECT_E;
			*extcommand = (*extcommand & 0xF) | 0x100;
        }
    	else if (no == 0xB)   // pattern loop
        {
			*command = PTEFFECT_E;
			*extcommand = (*extcommand & 0xF) | 0x60;
        }
    	else if (no == 0xC)   // note cut
    	{
        	*command = PTEFFECT_E;
		}
		else if (no ==  0xD)   // note delay
		{
			*command = PTEFFECT_E;
		}
		else if (no ==  0xE)   // pattern delay
		{
			*command = PTEFFECT_E;
		}
		else if (no == 0x9 && value == 1)
		{
			// S91 : set surround sound. not supported yet
			*command = 0;
			*extcommand = 0;
		}
		else
		{
		#ifdef JG_debug
			printf("unknown IT 'S' effect type: %d, %d\n", no, value);
		#endif
			
			*command = 0;
			*extcommand = 0;
		}
	}
    else
	{
	#ifdef JG_debug
		printf("unknown IT command: '%c'\n", c);
	#endif
		
        *command = 0;
        *extcommand = 0;
	}
}

JGMOD *load_it (const char *filename, int start_offset)
{
    JGMOD_FILE *f;
    JGMOD *j;

    f =  jgmod_fopen (filename, "rb");
    if (f == nullptr)
        return nullptr;

    j = (JGMOD*)jgmod_calloc ( sizeof (JGMOD));
    if (j == nullptr)
        {
        jgmod_seterror ("Unable to allocate enough memory for JGMOD structure");
        jgmod_fclose (f);
        return nullptr;
        }

    jgmod_skip (f, start_offset);
	
	// decode with the help of https://github.com/schismtracker/schismtracker/wiki/ITTECH.TXT
	// https://bitbucket.org/jthlim/impulsetracker/src/default/ReleaseDocumentation/ITTECH.TXT
	// effect reference: https://bitbucket.org/jthlim/impulsetracker/src/default/ReleaseDocumentation/IT.TXT
	
    char header[4];
    jgmod_fread(header, 4, f);
	if (strncmp(header, "IMPM", 4))
		return nullptr;
	
    jgmod_fread (j->name, 26, f);
	
#ifdef JG_debug
    printf ("%s\n", j->name);
#endif
	
    jgmod_skip (f, 2); // Pattern row hilight information. Only relevant for pattern editing situations.
	
    j->no_trk = jgmod_igetw(f);
    const uint16_t num_instruments = jgmod_igetw(f);
    j->no_sample = jgmod_igetw(f);
    j->no_pat = jgmod_igetw(f);
	
#ifdef JG_debug
    printf("no_trk: %d, no_inst: %d, no_sample: %d, no_pat: %d\n",
    	j->no_trk,
    	num_instruments,
    	j->no_sample,
    	j->no_pat);
#endif

    j->si = (SAMPLE_INFO*)jgmod_calloc (sizeof (SAMPLE_INFO) * j->no_sample);
    j->s  = (SAMPLE*)jgmod_calloc (sizeof (SAMPLE) * j->no_sample);
	
    const uint16_t created_with = jgmod_igetw(f); // Created with tracker. Impulse Tracker y.xx = 0yxxh
    const uint16_t created_version = jgmod_igetw(f); // Compatible with tracker with version greater than value. (ie. format version)
    (void)created_with;
    (void)created_version;
	
    const int flags = jgmod_igetw(f);
/*
     Flags:    Bit 0: On = Stereo, Off = Mono
                Bit 1: Vol0MixOptimizations - If on, no mixing occurs if
                       the volume at mixing time is 0 (redundant v1.04+)
                Bit 2: On = Use instruments, Off = Use samples.
                Bit 3: On = Linear slides, Off = Amiga slides.
                Bit 4: On = Old Effects, Off = IT Effects
                        Differences:
                       - Vibrato is updated EVERY frame in IT mode, whereas
                          it is updated every non-row frame in other formats.
                          Also, it is two times deeper with Old Effects ON
                       - Command Oxx will set the sample offset to the END
                         of a sample instead of ignoring the command under
                         old effects mode.
                       - (More to come, probably)
                Bit 5: On = Link Effect G's memory with Effect E/F. Also
                            Gxx with an instrument present will cause the
                            envelopes to be retriggered. If you change a
                            sample on a row with Gxx, it'll adjust the
                            frequency of the current note according to:

                              NewFrequency = OldFrequency * NewC5 / OldC5;
                Bit 6: Use MIDI pitch controller, Pitch depth given by PWD
                Bit 7: Request embedded MIDI configuration
                       (Coded this way to permit cross-version saving)
*/
	enum Flag
	{
		kFlag_Stereo = 1 << 0,
		kFlag_UseInstruments = 1 << 2,
		kFlag_LinearSlides = 1 << 3,
		kFlag_OldEffects = 1 << 4
	};
	
	if (flags & kFlag_UseInstruments)
	{
		jgmod_seterror("IT instruments are not yet supported");
		return nullptr;
	}
	
	if (flags & kFlag_LinearSlides)
	{
	#ifdef JG_debug
		printf("linear sliders not yet supported properly\n");
	#endif
	
		//j->flag |= JGMOD_MODE_LINEAR;
	}
	
	const uint16_t special = jgmod_igetw(f);
	(void)special;
/*
      Special:  Bit 0: On = song message attached.
                       Song message:
                        Stored at offset given by "Message Offset" field.
                        Length = MsgLgth.
                        NewLine = 0Dh (13 dec)
                        EndOfMsg = 0

                       Note: v1.04+ of IT may have song messages of up to
                             8000 bytes included.
                Bit 1: Reserved
                Bit 2: Reserved
                Bit 3: MIDI configuration embedded
                Bit 4-15: Reserved
*/
	enum Special
	{
		kSpecial_SongMessageAttached = 1 << 0
	};
	
	// note : JGMOD assumed global_volume 0->64
	j->global_volume = jgmod_getc(f) / 2; // Global volume. (0->128) All volumes are adjusted by this.
	const uint8_t mixing_volume = jgmod_getc(f); // Mix volume (0->128) During mixing, this value controls the magnitude of the wave being mixed.
	j->mixing_volume = mixing_volume;
#ifdef JG_debug
	printf("mixing volume: %d\n", mixing_volume);
#endif
	
	const uint8_t initial_speed = jgmod_getc(f); // Initial Speed of song.
	const uint8_t initial_tempo = jgmod_getc(f); // Initial Tempo of song.
	j->tempo = initial_speed;
	j->bpm = initial_tempo;
	
	const uint8_t panning_separation = jgmod_getc(f); // Panning separation between channels (0->128, 128 is max sep.)
	const uint8_t pitch_wheel_depth = jgmod_getc(f); // Pitch wheel depth for MIDI controllers.
	(void)panning_separation;
	(void)pitch_wheel_depth;
	
	// message stored with Impulse Tracker file. up to 8000 chars.
	const uint16_t message_length = jgmod_igetw(f);
	const uint32_t message_offset = jgmod_igetl(f);
	(void)message_length;
	(void)message_offset;
	
	jgmod_skip(f, 4); // Reserved.
	
	/*
		Each byte contains a panning value for a channel. Ranges from
		 0 (absolute left) to 64 (absolute right). 32 = central pan,
		 100 = Surround sound.
		 +128 = disabled channel (notes will not be played, but note
								  that effects in muted channels are
								  still processed)
	*/
	uint8_t channel_panning[64];
	jgmod_fread(channel_panning, 64, f);
	
	for (int i = 0; i < 64; ++i)
	{
		const int panning = channel_panning[i] & (~128);
		const bool disabled = channel_panning[i] & 128;
		
		if (panning < 64)
			j->panning[i] = int(channel_panning[i]) * 255 / 64;
		else if (panning == 100) // surround mode
			j->panning[i] = 128;
		
		j->channel_disabled[i] = disabled;
		
	#ifdef JG_debug
		//if (disabled)
		//	printf("channel disabled for channel %d\n", i);
	#endif
	}
	
	// Volume for each channel. Ranges from 0->64
	uint8_t channel_volume[64];
	jgmod_fread(channel_volume, 64, f);
	for (int i = 0; i < 64; ++i)
		j->channel_volume[i] = channel_volume[i];
	
	uint8_t orders[j->no_trk];
	jgmod_fread(orders, j->no_trk, f);
	for (auto i = 0; i < j->no_trk; ++i)
		j->pat_table[i] = orders[i];
	
	int actual_no_trk = 0;
    for (int index = 0; index < j->no_trk; index++)
	{
		if (j->pat_table[index] < j->no_pat)
			j->pat_table[actual_no_trk++] = j->pat_table[index];
	}
    j->no_trk = actual_no_trk;
	
	uint32_t instrument_offsets[num_instruments];
	for (auto i = 0; i < num_instruments; ++i)
		instrument_offsets[i] = jgmod_igetl(f);
	
	uint32_t sample_headers[j->no_sample];
	for (auto i = 0; i < j->no_sample; ++i)
		sample_headers[i] = jgmod_igetl(f);

	uint32_t pattern_offsets[j->no_pat];
	for (auto i = 0; i < j->no_pat; ++i)
		pattern_offsets[i] = jgmod_igetl(f);
	
	// load samples
	
	for (auto i = 0; i < j->no_sample; ++i)
	{
		SAMPLE_INFO * si = j->si + i;
		SAMPLE * s = j->s + i;
		
		if (!sample_headers[i])
			continue;
		
		jgmod_fseek(&f, filename, sample_headers[i]);
		
		char header[4];
		jgmod_fread(header, 4, f);
		
		if (strncmp(header, "IMPS", 4))
			return nullptr;
		
		char dos_filename[12];
		jgmod_fread(dos_filename, 12, f);
		
		jgmod_getc(f); // 0x00
		
		uint8_t global_volume = jgmod_getc(f); // Global volume for instrument, ranges from 0->64
		si->global_volume = global_volume;
		
		/*
		Flags:
			Bit 0. On = sample associated with header.
			Bit 1. On = 16 bit, Off = 8 bit.
			Bit 2. On = stereo, Off = mono. Stereo samples not supported yet
			Bit 3. On = compressed samples.
			Bit 4. On = Use loop
			Bit 5. On = Use sustain loop
			Bit 6. On = Ping Pong loop, Off = Forwards loop
			Bit 7. On = Ping Pong Sustain loop, Off = Forwards Sustain loop
		*/
		enum SampleFlag
		{
			kSampleFlag_HasSampleData    = 1 << 0,
			kSampleFlag_16bit            = 1 << 1,
			kSampleFlag_Compressed       = 1 << 3,
			kSampleFlag_Loop             = 1 << 4,
			kSampleFlag_SustainLoop      = 1 << 5,
			kSampleFlag_LoopBidir        = 1 << 6,
			kSampleFlag_SustainLoopBidir = 1 << 7
		};
		const uint8_t sample_flags = jgmod_getc(f);
		
		if (sample_flags & kSampleFlag_Compressed)
		{
			// todo : integrate https://github.com/schismtracker/schismtracker/blob/master/fmt/compression.c ?
			jgmod_seterror("Compress IT samples are not supported");
			return nullptr;
		}
		
		const uint8_t default_volume = jgmod_getc(f); // Default volume for instrument.
		si->volume = default_volume;
		si->transpose = 0;
		
		char sample_name[26];
		jgmod_fread(sample_name, 26, f);
	#ifdef JG_debug
		//printf("sample name: %s\n", sample_name);
	#endif
		// todo : safe strcpy
		strcpy(si->name, sample_name);
		
		/*
		Conversion flags:
			Bit 0:
			 Off: Samples are unsigned   } IT 2.01 and below use unsigned samples
			  On: Samples are signed     } IT 2.02 and above use signed samples
			Bit 1:
			 Off: Intel lo-hi byte order for 16-bit samples    } Safe to ignore
			 On: Motorola hi-lo byte order for 16-bit samples  } these values...
			Bit 2:                                             }
			 Off: Samples are stored as PCM values             }
			  On: Samples are stored as Delta values           }
			Bit 3:                                             }
			  On: Samples are stored as byte delta values      }
				  (for PTM loader)                             }
			Bit 4:                                             }
			  On: Samples are stored as TX-Wave 12-bit values  }
			Bit 5:                                             }
			  On: Left/Right/All Stereo prompt                 }
			Bit 6: Reserved
			Bit 7: Reserved
        */
        enum ConversionFlag
        {
			kConversionFlag_Signed = 1 << 0
		};
		const uint8_t convert_flags = jgmod_getc(f);
		
		// panning
		const uint8_t default_pan = jgmod_getc(f); // Default Pan. Bits 0->6 = Pan value, Bit 7 ON to USE (opposite of inst).
		if (default_pan & (1 << 7))
			si->pan = (default_pan & 63) * 255 / 63;
		else
			si->pan = 127;
		
		// sample length and loop points
		const uint32_t sample_length = jgmod_igetl(f); // Length of sample in no. of samples NOT no. of bytes.
		const uint32_t sample_loop_begin = jgmod_igetl(f); // Start of loop (no of samples in, not bytes).
		const uint32_t sample_loop_end = jgmod_igetl(f); // Sample no. AFTER end of loop.
		
		si->lenght = sample_length;
		
		if (sample_flags & kSampleFlag_Loop)
		{
			assert(sample_loop_begin < si->lenght);
			assert(sample_loop_end > sample_loop_begin);
			
			si->repoff = sample_loop_begin;
			
			if (sample_loop_end > 0)
				si->replen = sample_loop_end;
			else
				si->replen = si->lenght;
		}
		
		const uint32_t c5_speed = jgmod_igetl(f); // Number of bytes a second for C-5 (ranges from 0->9999999).
		assert(c5_speed >= 0 && c5_speed <= 9999999);
		si->c2spd = c5_speed;
		
		const uint32_t sustain_loop_begin = jgmod_igetl(f); // Start of sustain loop.
		const uint32_t sustain_loop_end = jgmod_igetl(f); // Sample no. AFTER end of sustain loop.
		(void)sustain_loop_begin;
		(void)sustain_loop_end;
		
		if (sample_flags & kSampleFlag_SustainLoop)
		{
			// note : sustain loops apply as long as no note-off has been encountered
			//        so if they're present, they will preceede over normal loops until
			//        a note is released. if sustain loop is enabled, use it as the sample
			//        loop for now. this is the closest we can get to the IT's behavior
			//        with the current voice routines
			
		#ifdef JG_debug
			printf("warning: sustain loop not properly implemented yet\n");
		#endif
			
			assert(sustain_loop_begin < si->lenght);
			assert(sustain_loop_end > sustain_loop_begin);
			
			si->repoff = sustain_loop_begin;
			
			if (sustain_loop_end > 0)
				si->replen = sustain_loop_end;
			else
				si->replen = si->lenght;
		}
		
		const uint32_t sample_data_offset = jgmod_igetl(f); // 'Long' Offset of sample in file.
		
		const uint8_t vibrato_speed = jgmod_getc(f); // Vibrato Speed, ranges from 0->64.
		const uint8_t vibrato_depth = jgmod_getc(f); // Vibrato Depth, ranges from 0->64.
		const uint8_t vibrate_rate = jgmod_getc(f); // Vibrato Rate, rate at which vibrato is applied (0->64).
		(void)vibrato_speed;
		(void)vibrato_depth;
		(void)vibrate_rate;
		/*
		Vibrato waveform type:
			0=Sine wave
			1=Ramp down
			2=Square wave
			3=Random (speed is irrelevant)
		*/
		const uint8_t vibrate_waveform_type = jgmod_getc(f);
		(void)vibrate_waveform_type;
	#if 1
		si->vibrato_spd = vibrato_speed;
		si->vibrato_depth = vibrato_depth;
		si->vibrato_rate = vibrate_rate;
		si->vibrato_type = vibrate_waveform_type;
		
	#ifdef JG_debug
		if (vibrato_depth != 0)
			printf("sample vibrato is not yet supported\n");
	#endif
	#endif
		
		s->freq = si->c2spd;
		
		#ifdef ALLEGRO_DATE
        s->stereo = FALSE;
        #endif
		
		s->len = si->lenght;
        s->priority = JGMOD_PRIORITY;
        s->loop_start = si->repoff;
        s->loop_end = si->replen;
        s->param = -1;
		
		if (sample_flags & kSampleFlag_SustainLoop)
		{
			if (sample_flags & kSampleFlag_SustainLoopBidir)
				si->loop = JGMOD_LOOP_BIDI;
			else
				si->loop = JGMOD_LOOP_ON;
		}
		else if (sample_flags & kSampleFlag_Loop)
		{
			if (sample_flags & kSampleFlag_LoopBidir)
				si->loop = JGMOD_LOOP_BIDI;
			else
				si->loop = JGMOD_LOOP_ON;
		}
		else
			si->loop = JGMOD_LOOP_OFF;
			
        // load sample data
		
		if ((sample_flags & kSampleFlag_HasSampleData) && sample_data_offset != 0)
		{
			jgmod_fseek(&f, filename, sample_data_offset);
			
			if (sample_flags & kSampleFlag_16bit)
			{
				s->data = jgmod_calloc (s->len*2);
				s->bits = 16;
				
				uint16_t * data = (uint16_t*)s->data;
				
				for (int i = 0; i < s->len; ++i)
				{
					data[i] = jgmod_igetw(f);
				}
				
				if (convert_flags & kConversionFlag_Signed)
					for (int i = 0; i < s->len; ++i)
						data[i] ^= 0x8000;
			}
			else
			{
				s->data = jgmod_calloc (s->len);
				s->bits = 8;
				
				uint8_t * data = (uint8_t*)s->data;
				
				for (int i = 0; i < s->len; ++i)
				{
					data[i] = jgmod_getc(f);
				}
				
				if (convert_flags & kConversionFlag_Signed)
					for (int i = 0; i < s->len; ++i)
						data[i] ^= 0x80;
			}
		}
	}
	
	// read patterns
	
	j->pi = (PATTERN_INFO*)jgmod_calloc (sizeof(PATTERN_INFO) * j->no_pat);
	
	j->no_chn = 64;
	
	for (auto i = 0; i < j->no_pat; ++i)
	{
		if (!pattern_offsets[i])
		{
			PATTERN_INFO * pi = j->pi + i;
			
			pi->no_pos = 1;
			pi->ni = (NOTE_INFO*)jgmod_calloc (sizeof(NOTE_INFO) * pi->no_pos * j->no_chn);
			continue;
		}
		
		jgmod_fseek(&f, filename, pattern_offsets[i]);
		
		PATTERN_INFO * pi = j->pi + i;
		
		const uint16_t packed_length = jgmod_igetw(f); // Length of packed pattern, not including the 8 byte header. Note that the pattern + the 8 byte header will ALWAYS be less than 64k
		const uint16_t num_rows = jgmod_igetw(f); // Number of rows in this pattern (Ranges from 32->200).
		
		pi->no_pos = num_rows;
		pi->ni = (NOTE_INFO*)jgmod_calloc (sizeof(NOTE_INFO) * pi->no_pos * j->no_chn);
		
		jgmod_skip(f, 4);
		
		// read packed pattern data
		
		uint8_t channel_mask[64];
		memset(channel_mask, 0, sizeof(channel_mask));
		
		int channel_note[64] = { };
		int channel_instrument[64] = { };
		int channel_command[64] = { };
		int channel_command_param[64] = { };
		int channel_special[64] = { };
		
		int row_index = 0;
		
		while (row_index < num_rows)
		{
			const uint8_t channel_variable = jgmod_getc(f);
			
			if (channel_variable == 0)
			{
				row_index++;
				continue;
			}
			
			const uint8_t channel = (channel_variable - 1) & 63;
			
			if (channel_variable & 128)
			{
				channel_mask[channel] = jgmod_getc(f);
			}
			
			//
			
			NOTE_INFO * ni = pi->ni + row_index * j->no_chn + channel;
			
			if (channel_mask[channel] & 1) // read note. (byte value)
			{
				//Note ranges from 0->119 (C-0 -> B-9)
                // 255 = note off, 254 = notecut
                // Others = note fade (already programmed into IT's player
                //                     but not available in the editor)
				
				ni->note = jgmod_getc(f);
				convert_it_pitch (&ni->note);
				
                channel_note[channel] = ni->note;
			}
			
			if (channel_mask[channel] & 2) // read instrument (byte value)
			{
				// Instrument ranges from 1->99
				
				ni->sample = jgmod_getc(f);
				
				channel_instrument[channel] = ni->sample;
			}
			
			if (channel_mask[channel] & 4) // read volume/panning (byte value)
			{
                // Volume ranges from 0->64
                // Panning ranges from 0->64, mapped onto 128->192
                // Prepare for the following also:
                //  65->74 = Fine volume up
                //  75->84 = Fine volume down
                //  85->94 = Volume slide up
                //  95->104 = Volume slide down
                //  105->114 = Pitch Slide down
                //  115->124 = Pitch Slide up
                //  193->202 = Portamento to
                //  203->212 = Vibrato
				
                /*
				Effects 65 is equivalent to D0F, 66 is equivalent to D1F -> 74 = D9F
				Similarly for 75-84 (DFx), 85-94 (Dx0), 95->104 (D0x).

				(Fine) Volume up/down all share the same memory (NOT shared with Dxx
				in the effect column tho).

				Pitch slide up/down affect E/F/(G)'s memory - a Pitch slide
				up/down of x is equivalent to a normal slide by x*4

				Portamento to (Gx) affects the memory for Gxx and has the equivalent
				slide given by this table:

				SlideTable      DB      1, 4, 8, 16, 32, 64, 96, 128, 255

				Vibrato uses the same 'memory' as Hxx/Uxx.
                */
				
                const uint8_t volume_and_panning = jgmod_getc(f);
				
				if (volume_and_panning <= 64)
				{
					ni->volume = volume_and_panning + 0x10;
					
					channel_volume[channel] = ni->volume;
				}
				else
                {
                	// todo : interpret the various effects that may be embedded here
					
				#ifdef JG_debug
                	printf("effect(s) not yet implemented: %d\n", volume_and_panning);
				#endif
				}
			}
			
			if (channel_mask[channel] & 8) // read command (byte value) and commandvalue
			{
				// Valid ranges from 0->31 (0=no effect, 1=A, 2=B, 3=C, etc.)
				
				ni->command = jgmod_getc(f);
				ni->extcommand = jgmod_getc(f);
				
				if (ni->command != 0)
					convert_it_command(&ni->command, &ni->extcommand, channel_special[channel]);
				
				channel_command[channel] = ni->command;
				channel_command_param[channel] = ni->extcommand;
			}
			
			if (channel_mask[channel] & 16) // note = lastnote for channel
			{
				ni->note = channel_note[channel];
			}
			
			if (channel_mask[channel] & 32) // instrument = lastinstrument for channel
			{
				ni->sample = channel_instrument[channel];
			}
			
			if (channel_mask[channel] & 64) // volume/pan = lastvolume/pan for channel
			{
				ni->volume = channel_volume[channel];
			}
			
			if (channel_mask[channel] & 128)
			{
                ni->command = channel_command[channel];
                ni->extcommand = channel_command_param[channel];
			}
		}
	}

    jgmod_fclose (f);
	
#if 0
	jgmod_seterror ("IT support is not completed yet. Wait a few more versions");
    jgmod_destroy (j);
    j = nullptr;
#endif

    return j;
}

}
