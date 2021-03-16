// -----------------------------------------------------------------------
//
//  Copyright (C) 2003-2010 Fons Adriaensen <fons@linuxaudio.org>
//    
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
//
// -----------------------------------------------------------------------

#include "reverb.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

namespace ZitaRev1
{
	Diff1::~Diff1()
	{
		shut();
	}

	void Diff1::init(const int size, const float c)
	{
		_size = size;
		_line = new float[size];
		
		memset(_line, 0, size * sizeof(float));
		
		_i = 0;
		_c = c;
	}

	void Diff1::shut()
	{
		delete [] _line;
		_line = nullptr;
		_size = 0;
	}

	//

	Delay::~Delay()
	{
		shut();
	}

	void Delay::init(const int size)
	{
		_size = size;
		_line = new float[size];
		
		memset(_line, 0, size * sizeof(float));
		
		_i = 0;
	}

	void Delay::shut()
	{
		delete [] _line;
		_line = 0;
		_size = 0;
	}

	//

	Vdelay::~Vdelay()
	{
		shut();
	}

	void Vdelay::init(const int size)
	{
		_size = size;
		_line = new float[size];
		
		memset(_line, 0, size * sizeof(float));
		
		_ir = 0;
		_iw = 0;
	}

	void Vdelay::shut()
	{
		delete [] _line;
		_line = 0;
		_size = 0;
	}

	void Vdelay::set_delay(const int del)
	{
		_ir = _iw - del;
		
		if (_ir < 0)
			_ir += _size;
	}

	//

	void Filt1::set_params(
		const float del,
		const float tmf,
		const float tlo,
		const float wlo,
		const float thi,
		const float chi)
	{
		_gmf = powf(0.001f, del / tmf);
		_glo = powf(0.001f, del / tlo) / _gmf - 1.0f;
		_wlo = wlo;
		
		const float g = powf(0.001f, del / thi) / _gmf;
		const float t = (1 - g * g) / (2 * g * g * chi);
		
		_whi = (sqrtf(1 + 4 * t) - 1) / (2 * t);
	}

	//

	static const float _tdiff1[8] =
	{
		20346e-6f,
		24421e-6f,
		31604e-6f,
		27333e-6f,
		22904e-6f,
		29291e-6f,
		13458e-6f,
		19123e-6f
	};

	static const float _tdelay[8] =
	{
	   153129e-6f,
	   210389e-6f,
	   127837e-6f,
	   256891e-6f,
	   174713e-6f,
	   192303e-6f,
	   125000e-6f,
	   219991e-6f
	};

	Reverb::Reverb()
	{
	}

	Reverb::~Reverb()
	{
		shut();
	}

	void Reverb::init(const float fsamp, const bool ambis)
	{
		_fsamp = fsamp;
		_ambis = ambis;
		
		_cntA1 = 1;
		_cntA2 = 0;
		_cntB1 = 1;
		_cntB2 = 0;
		_cntC1 = 1;
		_cntC2 = 0;

		_ipdel = 0.04f;
		_xover = 200.0f;
		_rtlow = 3.0f;
		_rtmid = 2.0f;
		_fdamp = 3e3f;
		_opmix = 0.5f;
		_rgxyz = 0.0f;

		_g0 = _d0 = 0.0f;
		_g1 = _d1 = 0.0f;

		_vdelay0.init(int(0.1f * _fsamp));
		_vdelay1.init(int(0.1f * _fsamp));
		
		for (int i = 0; i < 8; ++i)
		{
			const int k1 = int(floorf(_tdiff1[i] * _fsamp + 0.5f));
			const int k2 = int(floorf(_tdelay[i] * _fsamp + 0.5f));
			
			_diff1[i].init(k1, (i & 1) ? -0.6f : 0.6f);
			_delay[i].init(k2 - k1);
		}

		_pareq1.set_fsamp(fsamp);
		_pareq2.set_fsamp(fsamp);
	}

	void Reverb::shut()
	{
		for (int i = 0; i < 8; i++)
			_delay[i].shut();
	}

	void Reverb::prepare(const int numFrames)
	{
	// fixme : this isn't thread safe
		const int a = _cntA1;
		const int b = _cntB1;
		const int c = _cntC1;
		
		_d0 = _d1 = 0.0f;

		if (a != _cntA2)
		{
			const int k = int(floorf((_ipdel - 0.020f) * _fsamp + 0.5f));
			_vdelay0.set_delay(k);
			_vdelay1.set_delay(k);
			_cntA2 = a;
		}

		if (b != _cntB2)
		{
			 const float wlo = 6.2832f * _xover / _fsamp;
			 const float chi =
				_fdamp > 0.49f * _fsamp
					? 2
					: 1 - cosf(6.2832f * _fdamp / _fsamp);
			 
			 for (int i = 0; i < 8; ++i)
			 {
				 _filt1[i].set_params(
					_tdelay[i],
					_rtmid,
					_rtlow,
					wlo,
					0.5f * _rtmid,
					chi);
			 }
			 
			 _cntB2 = b;
		}

		if (c != _cntC2)
		{
			float t0;
			float t1;
			
			if (_ambis)
			{
				t0 = 1.0f / sqrtf(_rtmid);
				t1 = t0 * powf(10.0f, 0.05f * _rgxyz);
			}
			else
			{
				t0 = (1 - _opmix) * (1 + _opmix);
				t1 = 0.7f * _opmix * (2 - _opmix) / sqrtf(_rtmid);
			}
			
			_d0 = (t0 - _g0) / numFrames;
			_d1 = (t1 - _g1) / numFrames;
			
			_cntC2 = c;
		}

		_pareq1.prepare(numFrames);
		_pareq2.prepare(numFrames);
	}

	void Reverb::process(
		const int numFrames,
		float * inp[],
		float * out[])
	{
		const float * __restrict p0 = inp[0];
		const float * __restrict p1 = inp[1];
		      float * __restrict q0 = out[0];
		      float * __restrict q1 = out[1];
		      float * __restrict q2 = out[2]; // fixme : array access when stereo..
		      float * __restrict q3 = out[3];

		const float g = sqrtf(0.125f);

		for (int i = 0; i < numFrames; ++i)
		{
			_vdelay0.write(p0[i]);
			_vdelay1.write(p1[i]);
			
			float t, x0, x1, x2, x3, x4, x5, x6, x7;

			 t = 0.3f * _vdelay0.read();
			x0 = _diff1[0].process(_delay[0].read() + t);
			x1 = _diff1[1].process(_delay[1].read() + t);
			x2 = _diff1[2].process(_delay[2].read() - t);
			x3 = _diff1[3].process(_delay[3].read() - t);
			
			 t = 0.3f * _vdelay1.read();
			x4 = _diff1[4].process(_delay[4].read() + t);
			x5 = _diff1[5].process(_delay[5].read() + t);
			x6 = _diff1[6].process(_delay[6].read() - t);
			x7 = _diff1[7].process(_delay[7].read() - t);

			t = x0 - x1; x0 += x1;  x1 = t;
			t = x2 - x3; x2 += x3;  x3 = t;
			t = x4 - x5; x4 += x5;  x5 = t;
			t = x6 - x7; x6 += x7;  x7 = t;
			t = x0 - x2; x0 += x2;  x2 = t;
			t = x1 - x3; x1 += x3;  x3 = t;
			t = x4 - x6; x4 += x6;  x6 = t;
			t = x5 - x7; x5 += x7;  x7 = t;
			t = x0 - x4; x0 += x4;  x4 = t;
			t = x1 - x5; x1 += x5;  x5 = t;
			t = x2 - x6; x2 += x6;  x6 = t;
			t = x3 - x7; x3 += x7;  x7 = t;

			if (_ambis)
			{
				_g0 += _d0;
				_g1 += _d1;
				q0[i] = _g0 * x0;
				q1[i] = _g1 * x1;
				q2[i] = _g1 * x4;
				q3[i] = _g1 * x2;
			}
			else
			{
				_g1 += _d1;
				q0[i] = _g1 * (x1 + x2);
				q1[i] = _g1 * (x1 - x2);
			}

			_delay[0].write(_filt1[0].process(g * x0));
			_delay[1].write(_filt1[1].process(g * x1));
			_delay[2].write(_filt1[2].process(g * x2));
			_delay[3].write(_filt1[3].process(g * x3));
			_delay[4].write(_filt1[4].process(g * x4));
			_delay[5].write(_filt1[5].process(g * x5));
			_delay[6].write(_filt1[6].process(g * x6));
			_delay[7].write(_filt1[7].process(g * x7));
		}

		const int numChannels =
			_ambis
				? 4
				: 2;
		
		_pareq1.process(numFrames, numChannels, out);
		_pareq2.process(numFrames, numChannels, out);
		
		if (!_ambis)
		{
			// todo : what does this do? dry-wet?
			for (int i = 0; i < numFrames; ++i)
			{
				_g0 += _d0;
				
				q0[i] += _g0 * p0[i];
				q1[i] += _g0 * p1[i];
			}
		}
	}
}
