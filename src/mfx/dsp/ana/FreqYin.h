/*****************************************************************************

        FreqYin.h
        Author: Laurent de Soras, 2016

Based on:
Alain de Chevigne, Hideki Kawahara
YIN, a fundamental frequency estimator for speech and music,
Acoustical Society of America, 2002
Implemented up to step 5.

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (mfx_dsp_ana_FreqYin_HEADER_INCLUDED)
#define mfx_dsp_ana_FreqYin_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#define mfx_dsp_ana_USE_SIMD

#if defined (mfx_dsp_ana_USE_SIMD)
#include "fstb/AllocAlign.h"

#include <array>
#endif

#include <vector>



namespace mfx
{
namespace dsp
{
namespace ana
{



class FreqYin
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	               FreqYin ()  = default;
	virtual        ~FreqYin () = default;

	void           set_sample_freq (double sample_freq);
	void           clear_buffers ();
	float          process_block (const float spl_ptr [], int nbr_spl);
	float          process_sample (float x);



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	void           analyse_sample ();

	const double   _min_freq    = 30.0; // Hz
	const double   _max_freq    = 500.0;
	const float    _threshold   = 0.1f;
	const float    _smoothing   = 0.125f;  // Smoothing coefficient (LERP between the new and accumulated values)
	const float    _smooth_thr  = 0.02f;// Smoothing is done only on close values (threshold is relative)

#if defined (mfx_dsp_ana_USE_SIMD)
	typedef std::vector <float, fstb::AllocAlign <float, 16> > BufAlign;
	std::array <BufAlign, 4>
	               _buf_arr;
#else
	std::vector <float>
	               _buffer;
#endif
	int            _buf_pos     = 0;    // Beginning of the analysis buffer within the ring buffer
	int            _buf_mask    = 0;
	float          _sample_freq = 0;    // Hz. 0 = not set
	int            _win_len     = 0;    // Samples
	int            _min_delta   = 0;
	int            _step_size   = 0;    // Number of samples between two analysis

	int            _delta       = 1;    // We write at _win_len + _delta - 1 relative to _buf_pos.
	float          _freq        = 0;    // Hz. 0 = not found (yet)
	float          _freq_prev   = 0;    // Hz
	float          _dif_sum     = 0;
	std::vector <float>
	               _cmndf_arr;


/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               FreqYin (const FreqYin &other)           = delete;
	FreqYin &      operator = (const FreqYin &other)        = delete;
	bool           operator == (const FreqYin &other) const = delete;
	bool           operator != (const FreqYin &other) const = delete;

}; // class FreqYin



}  // namespace ana
}  // namespace dsp
}  // namespace mfx



//#include "mfx/dsp/ana/FreqYin.hpp"



#endif   // mfx_dsp_ana_FreqYin_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
