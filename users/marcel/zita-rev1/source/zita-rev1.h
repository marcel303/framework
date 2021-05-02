// -----------------------------------------------------------------------
//
//  Copyright (C) 2003-2011 Fons Adriaensen <fons@linuxaudio.org>
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

#pragma once

#include "zita-rev1-pareq.h"

namespace ZitaRev1
{
	struct Diff1
	{
		~Diff1();

		void init(const int size, const float c);
		void shut();

		float process(float x)
		{
		// todo : avoid denormals ?
			const float z = _line[_i];
			
			x -= _c * z;
			
			_line[_i++] = x;
			
			if (_i == _size)
				_i = 0;
				
			return z + _c * x;
		}

		int                _i = 0;
		float              _c = 0.f;
		int                _size = 0;
		float * __restrict _line = nullptr;
	};

	//

	struct Filt1
	{
		void set_params(
			const float del,
			const float tmf,
			const float tlo,
			const float wlo,
			const float thi,
			const float chi);

		float process(float x)
		{
			_slo += _wlo * (x - _slo) + 1e-10f;
			
			x += _glo * _slo;
			
			_shi += _whi * (x - _shi);
			
			return _gmf * _shi;
		}
		
		float _gmf = 0.f;
		float _glo = 0.f;
		float _wlo = 0.f;
		float _whi = 0.f;
		
		float _slo = 0.f;
		float _shi = 0.f;
	};

	//

	struct Delay
	{
		~Delay();

		void init(const int size);
		void shut();

		float read() const
		{
			return _line[_i];
		}

		void write(const float x)
		{
			_line[_i++] = x;
			
			if (_i == _size)
				_i = 0;
		}

		int                _i = 0;
		int                _size = 0;
		float * __restrict _line = nullptr;
	};

	//

	struct Vdelay
	{
		~Vdelay();

		void init(const int size);
		void shut();
		
		void set_delay(const int del);

		float read()
		{
			const float x = _line[_ir++];
			
			if (_ir == _size)
				_ir = 0;
				
			return x;
		}

		void write(const float x)
		{
			_line[_iw++] = x;
			
			if (_iw == _size)
				_iw = 0;
		}

		int                _ir = 0;
		int                _iw = 0;
		int                _size = 0;
		float * __restrict _line = nullptr;
	};

	//

	struct Reverb
	{
		Reverb();
		~Reverb();

		void init(const float fsamp, const bool ambis);
		void shut();

		void prepare(
			const int numFrames); // note : numFrames may be bigger than numFrames during process..
		void process(
			const int numFrames,
			float * src[],
			float * dst[]);

		void set_delay(const float v) { _ipdel = v; _cntA1++; }
		void set_xover(const float v) { _xover = v; _cntB1++; }
		void set_rtlow(const float v) { _rtlow = v; _cntB1++; }
		void set_rtmid(const float v) { _rtmid = v; _cntB1++; _cntC1++; }
		void set_fdamp(const float v) { _fdamp = v; _cntB1++; }
		void set_opmix(const float v) { _opmix = v; _cntC1++; }
		void set_rgxyz(const float v) { _rgxyz = v; _cntC1++; }
		
		void set_eq1(const float f, const float g) { _pareq1.set_param(f, g); }
		void set_eq2(const float f, const float g) { _pareq2.set_param(f, g); }

	private:

		// -- audio spec
		float   _fsamp;
		bool    _ambis;

		// -- fdn
		Vdelay  _vdelay0;
		Vdelay  _vdelay1;
		Diff1   _diff1[8];
		Filt1   _filt1[8];
		Delay   _delay[8];
		
		// -- dirty tracking
		volatile int _cntA1; // fixme : not thread safe
		volatile int _cntB1;
		volatile int _cntC1;
		int     _cntA2;
		int     _cntB2;
		int     _cntC2;

		// -- params
		float   _ipdel;
		float   _xover;
		float   _rtlow;
		float   _rtmid;
		float   _fdamp;
		float   _opmix;
		float   _rgxyz;

		float   _g0, _d0; // dry-wet value and gradient
		float   _g1, _d1;

		// -- eq
		Pareq   _pareq1;
		Pareq   _pareq2;
	};
}
