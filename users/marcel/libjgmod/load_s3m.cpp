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
 *  S3M and Unreal S3M loader. */

#include "jgmod.h"
#include "jshare.h"
#include "file_io.h"

#include "allegro2-voiceApi.h"

#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

//#define JG_debug

extern const int noteperiod[];

namespace jgmod
{
	void S3M_get_num_chn(FILE * f, uint8_t chn_set[32], char remap[32]);
	void S3M_load_pat(FILE * f, JGMOD * j, NOTE_INFO * n, int no_chn, char remap[32]);
	void convert_s3m_command(int * command, int * extcommand);
	void convert_s3m_pitch(int * pitch);
	int get_mod_no_pat(const int * table, const int max_trk);

	int get_s3m_info(const char * filename, const int start_offset, JGMOD_INFO * ji)
	{
		FILE * f = jgmod_fopen(filename, "rb");
		
		if (f == nullptr)
		{
			jgmod_seterror("Unable to open %s", filename);
			return -1;
		}

		if (start_offset == 0)
		{
			ji->type = JGMOD_TYPE_S3M;
			sprintf(ji->type_name, "S3M");
		}
		else
		{
			ji->type = JGMOD_TYPE_UNREAL_S3M;
			sprintf(ji->type_name, "Unreal S3M (UMX)");
			jgmod_skip(f, start_offset);
		}

		jgmod_fread(ji->name, 28, f);
		jgmod_fclose(f);
		return 1;
	}

	// to detect unreal s3m files
	int detect_unreal_s3m(const char * filename)
	{
		const unsigned char umx_id[4] = { 0xc2, 0xa1, 0xc3, 0x89 };

		FILE * f = jgmod_fopen(filename, "rb");
		if (f == nullptr)
			return 0;
		
		// detect a umx file
		
		char id[4];
		jgmod_fread(id, 4, f);
		
		if (memcmp(id, umx_id, 4) != 0)
		{
			jgmod_fclose(f);
			return -1;
		}

		id[0] = jgmod_getc(f);
		id[1] = jgmod_getc(f);
		id[2] = jgmod_getc(f);
		id[3] = jgmod_getc(f);
		
		int start_offset = 8;

		for (int index = 0; index < 500; ++index)
		{
			// detect a S3M file
			if (memcmp(id, "SCRM", 4) == 0)
				return start_offset - 48;

			id[0] = id[1];
			id[1] = id[2];
			id[2] = id[3];
			id[3] = jgmod_getc(f);
			
			start_offset++;
		}

		return -1;
	}

	// to detect s3m files
	int detect_s3m(const char * filename)
	{
		FILE * f = jgmod_fopen(filename, "rb");
		if (f == nullptr)
			return 0;
		
		char id[4];
		
		jgmod_skip(f, 0x2c);
		jgmod_fread(id, 4, f);
		jgmod_fclose(f);
		
		if (memcmp(id, "SCRM", 4) == 0)
		{
			// detect successful
			return 1;
		}

		// not a s3m
		return -1;
	}

	// to get the number of channels actually used.
	// must seek to the pattern first
	void S3M_get_num_chn(FILE * f, uint8_t chn_set[32], char remap[32])
	{
		int row = 0;
		
		while (row < 64)
		{
			const int flag = jgmod_getc(f);

			if (flag)
			{
				const int ch = flag & 31;
				
				if (chn_set[ch] < 32)
					remap[ch] = 0;

				if (flag & 32)
					jgmod_skip(f, 2);

				if (flag & 64)
					jgmod_getc(f);

				if (flag & 128)
					jgmod_skip(f, 2);
			}
			else
			{
				row++;
			}
		}
	}

	// similar to s3m_get_num_chn but load the notes into the jgmod structure
	void S3M_load_pat(FILE * f, JGMOD * j, NOTE_INFO * n, const int no_chn, char remap[32])
	{
		NOTE_INFO dummy;

		int row = 0;
		
		while (row < 64)
		{
			const int flag = jgmod_getc(f);

			if (flag)
			{
				const int ch = remap[flag & 31];

				NOTE_INFO * ni;
				
				if (ch != -1)
					ni = &n[row * no_chn + ch];
				else
					ni = &dummy;

				if (flag & 32)
				{
					ni->note = jgmod_getc(f);
					ni->sample = jgmod_getc(f);
					convert_s3m_pitch(&ni->note);

					if (ni->sample > j->no_sample)
						ni->sample = 0;
				}
				
				if (flag & 64)
				{
					const int temp = jgmod_getc(f);
					ni->volume = temp + 0x10;
				}

				if (flag & 128)
				{
					ni->command = jgmod_getc(f);
					ni->extcommand = jgmod_getc(f);
					convert_s3m_command (&ni->command, &ni->extcommand);
				}
			}
			else
			{
				row++;
			}
		}
	}

	// convert the s3m note table into hz
	void convert_s3m_pitch(int * pitch)
	{
		int octave;

		if (*pitch == 254) // note cut
		{
			*pitch = -3; // note : check if this behavior is correct !
			return;
		}

		if ((*pitch % 16) > 11)
		{
			*pitch = 0;
			return;
		}

		octave = *pitch >> 4; // pitch / 16
		*pitch = noteperiod[*pitch % 16] >> octave;

		if (*pitch != 0)
		{
			*pitch = JGMOD_NTSC / *pitch;
		}
	}

	// load a s3m file
	JGMOD * load_s3m(const char * filename, const int start_offset, const bool fast_loading)
	{
		int sf; // sample format (signed or unsigned)
		int dp; // default pan positions
		int pan[32];
		uint8_t chn_set[32];
		char remap[32];

		FILE * f = jgmod_fopen(filename, "rb");
		
		if (f == nullptr)
		{
			jgmod_seterror("Unable to open %s", filename);
			return nullptr;
		}

		JGMOD * j = (JGMOD*)jgmod_calloc(sizeof(JGMOD));
		if (j == nullptr)
		{
			jgmod_fclose(f);
			jgmod_seterror("Unable to allocate enough memory for JGMOD structure");
			return nullptr;
		}
		
		jgmod_skip(f, start_offset);
		jgmod_fread(j->name, 28, f);
		jgmod_skip(f, 4);
		j->no_trk = jgmod_igetw(f);
		j->no_sample = jgmod_igetw(f);
		j->no_pat = jgmod_igetw(f);

		j->si = (SAMPLE_INFO*)jgmod_calloc(sizeof(SAMPLE_INFO) * j->no_sample);
		j->s  = (SAMPLE*)jgmod_calloc(sizeof(SAMPLE) * j->no_sample);
		
		if (j->si == nullptr || j->s == nullptr)
		{
			jgmod_destroy(j);
			jgmod_fclose(f);
			jgmod_seterror("Unable to allocate enough memory for SAMPLE or SAMPLE_INFO");
			return nullptr;
		}

		// skip the flag and tracker info
		jgmod_igetw(f);
		jgmod_igetw(f);
		sf = jgmod_igetw(f);
		jgmod_skip(f, 4); // skip SCRM
		j->global_volume = jgmod_getc(f);
		j->mixing_volume = 64;
		j->tempo = jgmod_getc(f);
		j->bpm = jgmod_getc(f);
		jgmod_getc(f);
		jgmod_getc(f);
		dp = jgmod_getc(f);

		jgmod_skip(f, 10);
		jgmod_fread(chn_set, 32, f);

		if (j->tempo == 0)
			j->tempo = 6;

		if (j->bpm == 0)
			j->bpm = 125;

		for (int index = 0; index < 64; ++index)
			j->channel_volume[index] = 64;
		
		// read the order number
		for (int index = 0; index < j->no_trk; ++index)
			j->pat_table[index] = jgmod_getc(f);

		// to store instruments and patterns parapointers
		int * parapointer = (int*)jgmod_calloc((j->no_sample + j->no_pat) * sizeof(int));
		if (parapointer == nullptr)
		{
			jgmod_destroy(j);
			jgmod_fclose(f);
			jgmod_seterror("Unable to allocate enough memory for parapointer");
			return nullptr;
		}

		for (int index = 0; index < j->no_sample + j->no_pat; ++index)
			parapointer[index] = jgmod_igetw(f) * 16;

		// load panning table
		if (dp == 252)
		{
			for (int index = 0; index < 32; ++index)
				pan[index] = jgmod_getc(f);
		}

		// load those samples
		//------------------------------------------------------------------
		for (int index = 0; index < j->no_sample; ++index)
		{
			char id[4];
			int memseg;
			int type;

			SAMPLE_INFO * si = j->si + index;
			SAMPLE      * s =  j->s  + index;

		// todo : check result of seek operations
			jgmod_fseek(&f, filename, parapointer[index] + start_offset);
			
			if (jgmod_getc(f) != 1) // is not a sample structure
			{
				s->data = jgmod_calloc(0);
				continue;
			}

			// skip the filename
			jgmod_skip(f, 12);
			
			memseg = (int)(jgmod_getc(f) << 16) + (int)jgmod_getc(f) + (int)(jgmod_getc(f) << 8);
			memseg = memseg * 16;

			si->lenght = jgmod_igetl(f);
			si->repoff = jgmod_igetl(f);
			si->replen = jgmod_igetl(f);
			si->volume = jgmod_getc(f);
			si->global_volume = 64;
			si->transpose = 0;
			jgmod_getc(f);
			jgmod_getc(f); // skip packing type
			type = jgmod_getc(f);
			si->c2spd = jgmod_igetl (f) & 0xFFFF;
			jgmod_skip(f, 12);
		// todo : store the sample name
			jgmod_skip(f, 28); // skip sample name

			jgmod_fread(id, 4, f);

			// now load the samples
			jgmod_fseek(&f, filename, memseg + start_offset);

			s->freq = si->c2spd;
			s->len = si->lenght;
			s->priority = JGMOD_PRIORITY;
			s->loop_start = si->repoff;
			s->loop_end = si->replen;
			s->param = -1;

			// don't load the samples
			if (memcmp (id, "SCRS", 4) != 0)
			{
				s->data = jgmod_calloc(0);
				continue;
			}

			if (type & 4)
				s->data = jgmod_calloc(s->len*2);
			else
				s->data = jgmod_calloc(s->len);

			if (type & 4) // sample is 16 bit
			{
				short * data = (short *)s->data;
				
				s->bits = 16;

				for (int counter = 0; counter < s->len; ++counter)
					data[counter] = jgmod_igetw (f);

				if (sf == 1)
				{
					for (int counter = 0; counter < s->len; ++counter)
						data[counter] ^= 0x8000;
				}
			}
			else // otherwise 8 bit
			{
				char * data = (char *)s->data;
				
				s->bits = 8;

				jgmod_fread(s->data, s->len, f);
				
				if (sf == 1)
				{
					for (int counter = 0; counter < s->len; ++counter)
						data[counter] ^= 0x80;
				}
			}

			if (type & 1)
				si->loop = JGMOD_LOOP_ON;
			else
				si->loop = JGMOD_LOOP_OFF;
		}

		// detect the no of channels used
		//-------------------------------------------------------------------
		j->no_chn = 0;
		memset(remap, -1, 32 * sizeof(char));

		if (fast_loading == true) // fast detection but less accurate
		{
			for (int index = 0; index < 32; ++index)
			{
				if (chn_set[index] < 32)
					remap[index] = 0;
			}
		}
		else // slow detection but accurate
		{
			for (int index = 0; index < j->no_pat; ++index)
			{
				jgmod_fseek(&f, filename, parapointer[j->no_sample + index] + 2 + start_offset);
				S3M_get_num_chn(f, chn_set, remap);
			}
		}
		
		for (int index = 0; index < 32; ++index)
		{
			if (remap[index] == 0)
			{
				remap[index] = j->no_chn;
				j->no_chn++;
			}
		}

		// get the pannings ------------------------------------------------------
		for (int index = 0; index < 32; ++index)
		{
			if (chn_set[index] < 16 && remap[index] != -1)
			{
				if (chn_set[index] < 8)
					j->panning[(int)remap[index]] = 64;
				else
					j->panning[(int)remap[index]] = 192;
			}
		}
		
		if (dp == 252)
		{
			for (int index = 0; index < 32; ++index)
			{
				if ((pan[index] & 0x20) != 0 && chn_set[index] < 16 && remap[index] != -1)
					j->panning[(int)remap[index]] = (pan[index] & 0xf) * 17;
			}
		}

		// rearrange the pattern order
		int temp = 0;
		for (int index = 0; index < j->no_trk; ++index)
		{
			j->pat_table[temp] = j->pat_table[index];
			
			if (j->pat_table[index] < 254)
				temp++;
		}
		j->no_trk = temp;

		// store the number of actual patterns
		const int actual_pat = get_mod_no_pat(j->pat_table, j->no_trk);

		// -- this section initialize and load all the patterns -----------------
		// allocate patterns
		j->pi = (PATTERN_INFO*)jgmod_calloc(sizeof(PATTERN_INFO) * actual_pat);

		for (int index = 0; index < actual_pat; ++index)
		{
			PATTERN_INFO * pi = j->pi + index;
			pi->no_pos = 64;

			pi->ni = (NOTE_INFO*)jgmod_calloc(sizeof(NOTE_INFO) * 64 * j->no_chn);
		}

		// now load all those patterns
		for (int index = 0; index < actual_pat; ++index)
		{
			if (index >= j->no_pat)
				continue;

			PATTERN_INFO * pi = j->pi + index;
			jgmod_fseek(&f, filename, parapointer[j->no_sample + index] + 2 + start_offset);
			S3M_load_pat(f, j, pi->ni, j->no_chn, remap);
		}
		j->no_pat = actual_pat;

	#ifdef JG_debug
		for (int index = 0; index < j->no_sample; ++index)
		{
			printf ("instrument %2d = %d bit\n", index, j->s[index].bits);
		}

		printf("\n\nActual pattern : %d", actual_pat);

		for (int index = 0; index < j->no_pat; ++index)
		{
			NOTE_INFO *ni;

			pi = j->pi + index;
			ni = pi->ni;
			
			printf ("\n\nPattern %d\n", index);
			
			for (int temp = 0; temp < 64 * j->no_chn; ++temp)
			{
				if ((temp % j->no_chn) == 0)
					printf ("\n");
					
				printf ("%02d %03d    ", ni->command, ni->extcommand);
				
				ni++;
			}
		}
	#endif

		free(parapointer);
		jgmod_fclose(f);
		
		return j;
	}

	//convert s3m commands to protracker like commands
	void convert_s3m_command(int * command, int * extcommand)
	{
		const int no = (*extcommand & 0xF0 ) >> 4;

		if (*command == 1)                      // s3m set tempo
			*command = S3EFFECT_A;
		else if (*command == 2)                 // pattern jump
			*command = PTEFFECT_B;
		else if (*command == 3)                 // pattern break
			*command = PTEFFECT_D;
		else if (*command == 4)                 // S3M volume slide
			*command = S3EFFECT_D;
		else if (*command == 5)                 // S3M portamento down
			*command = S3EFFECT_E;
		else if (*command == 6)                 // S3M portamento up
			*command = S3EFFECT_F;
		else if (*command == 7)                 // slide to note
			*command = PTEFFECT_3;
		else if (*command == 8)                 // vibrato
			*command = PTEFFECT_4;
		else if (*command == 9)                 // s3m tremor
			*command = S3EFFECT_I;
		else if (*command == 10)                // s3m arpeggio
			*command = S3EFFECT_J;
		else if (*command == 11)                // vibrato+volume slide
			*command = S3EFFECT_K;
		else if (*command == 12)                // porta to note
			*command = S3EFFECT_L;
		else if (*command == 15)                // set sample offset
			*command = PTEFFECT_9;
		else if (*command == 17)                //
			*command = S3EFFECT_Q;
		else if (*command == 18)                // s3m tremolo
			*command = S3EFFECT_R;
		else if (*command == 20)                // set bpm
			*command = S3EFFECT_T;
		else if (*command == 21)                // fine vibrato
			*command = S3EFFECT_U;
		else if (*command == 22)                // set global volume
			*command = S3EFFECT_V;
		else if (*command == 24)                // set panning
			*command = S3EFFECT_X;
		else if(*command == 19 && no == 1)
		{
			*command = PTEFFECT_E;
			*extcommand = (*extcommand & 0xF) | 0x30;
		}
		else if(*command == 19 && no == 2)      // set finetune
		{
			*command = PTEFFECT_E;
			*extcommand = (*extcommand & 0xF) | 0x50;
		}
		else if (*command == 19 && no == 3)     // set vibrato waveform
		{
			*command = PTEFFECT_E;
			*extcommand = (*extcommand & 0xF) | 0x40;
		}
		else if (*command == 19 && no == 4)     // set tremolo waveform
		{
			*command = PTEFFECT_E;
			*extcommand = (*extcommand & 0xF) | 0x70;
		}
		else if (*command == 19 && no == 8)     // set 16 pan position
			*command = PTEFFECT_E;
		else if (*command == 19 && no == 0xA)   // stereo control
		{
			*command = PTEFFECT_E;
			*extcommand = (*extcommand & 0xF) | 0x100;
		}
		else if (*command == 19 && no == 0xB)   // pattern loop
		{
			*command = PTEFFECT_E;
			*extcommand = (*extcommand & 0xF) | 0x60;
		}
		else if (*command == 19 && no == 0xC)   // note cut
			*command = PTEFFECT_E;
		else if (*command == 19 && no == 0xD)   // note delay
			*command = PTEFFECT_E;
		else if (*command == 19 && no == 0xE)   // pattern delay
			*command = PTEFFECT_E;
		else
		{
			*command = 0;
			*extcommand = 0;
		}
	}
}
