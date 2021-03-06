/*****************************************************************************

        TransSZBilin.cpp
        Author: Laurent de Soras, 2016

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if defined (_MSC_VER)
	#pragma warning (1 : 4130 4223 4705 4706)
	#pragma warning (4 : 4355 4786 4800)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "fstb/def.h"
#include "mfx/dsp/iir/TransSZBilin.h"

#include <cassert>
#include <cmath>



namespace mfx
{
namespace dsp
{
namespace iir
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*
==============================================================================
Name: prewarp_freq_rel_1
Description:
	Warp a single frequency, given a reference frequency.
	The output is a frequency (multiplier) relative to fref, hence suitable to
	be directly integrated as coefficient of an s equation centered on 1.
Input parameters:
	- f0: Frequency to be warped, in ]0 ; fs/2[
	- fref: Reference frequency, in ]0 ; fs/2[
	- fs: Sampling frequency, > 0.
Returns: the warped frequency, relative to 1, in ]0 ; +oo[
Throws: Nothing
==============================================================================
*/

double	TransSZBilin::prewarp_freq_rel_1 (double f0, double fref, double fs)
{
	assert (f0 > 0);
	assert (f0 < fs * 0.5);
	assert (fref > 0);
	assert (fref < fs * 0.5);
	assert (fs > 0);

	const double   mult   = fstb::PI / fs;
	const double   fref_w = tan (fref * mult);
	const double   f0_w   = tan (f0   * mult);

	const double   f_w_rel = f0_w / fref_w;
	assert (f_w_rel > 0);

	return f_w_rel;
}



/*
==============================================================================
Name: prewarp_freq_rel
Description:
	Warp a single frequency, given a reference frequency.
Input parameters:
	- f0: Frequency to be warped, in ]0 ; fs/2[
	- fref: Reference frequency, in ]0 ; fs/2[
	- fs: Sampling frequency, > 0.
Returns: the warped frequency, in ]0 ; +oo[
Throws: Nothing
==============================================================================
*/

double	TransSZBilin::prewarp_freq_rel (double f0, double fref, double fs)
{
	return prewarp_freq_rel_1 (f0, fref, fs) * fref;
}



/*
==============================================================================
Name: prewarp_freq_rel_mul
Description:
	Returns the multiplier to apply on the frequency to be warped.
Input parameters:
	- f0: Frequency to be warped, in ]0 ; fs/2[
	- fref: Reference frequency, in ]0 ; fs/2[
	- fs: Sampling frequency, > 0.
Returns: the multiplier, in ]0 ; +oo[
Throws: Nothing
==============================================================================
*/

double	TransSZBilin::prewarp_freq_rel_mul (double f0, double fref, double fs)
{
	assert (fs > 0);
	assert (f0 > 0);
	assert (f0 < fs * 0.5);
	assert (fref > 0);
	assert (fref < fs * 0.5);

	const double   mult   = fstb::PI / fs;
	const double   fref_w = tan (fref * mult);
	const double   f0_w   = tan (f0   * mult);

	const double   f_w_rel_mul = (f0_w * fref) / (f0 * fref_w);
	assert (f_w_rel_mul > 0);

	return f_w_rel_mul;
}



/*
==============================================================================
Name: prewarp_biquad
Description:
	Changes the coefficient of a biquad in order to make a certain frequency
	correctly warped.
Input parameters:
	- fref: Reference frequency for the biquad, in ]0 ; fs/2[
	- fs: Sampling frequency, > 0.
Input/output parameters:
	- b: numerator of the transfer function in the S-plane.
	- a: denominator of the transfer function in the S-plane.
Throws: Nothing
==============================================================================
*/

void	TransSZBilin::prewarp_biquad (double b [3], double a [3], double fref, double fs)
{
	assert (b != 0);
	assert (a != 0);
	assert (a [0] != 0);
	assert (a [2] != 0);

	const double   freq = sqrt (a [2] / a [0]) * fref;
	const double   mul  = prewarp_freq_rel_mul (freq, fref, fs);
	const double   mul2 = mul * mul;
	a [1] *= mul;
	b [1] *= mul;
	a [2] *= mul2;
	b [2] *= mul2;
}



void	TransSZBilin::map_s_to_z_approx (float z_eq_b [3], float z_eq_a [3], const float s_eq_b [3], const float s_eq_a [3], float k)
{
	const auto     kv   = fstb::ToolsSimd::set1_f32 (k);
	const auto     kk   = fstb::ToolsSimd::set1_f32 (k * k);

	const auto     x0s  = fstb::ToolsSimd::set_2f32 (s_eq_a [0], s_eq_b [0]);
	const auto     x1s  = fstb::ToolsSimd::set_2f32 (s_eq_a [1], s_eq_b [1]);
	const auto     x2s  = fstb::ToolsSimd::set_2f32 (s_eq_a [2], s_eq_b [2]);

	const auto     x1k  = x1s * kv;
	const auto     x2kk = x2s * kk;
	const auto     x2kk_plus_x0   = x2kk + x0s;
	auto           x0z  = x2kk_plus_x0 + x1k;
	auto           x2z  = x2kk_plus_x0 - x1k;
	const auto     x0s_minus_x2kk = x0s - x2kk;
	auto           x1z  = x0s_minus_x2kk + x0s_minus_x2kk;

	// On a0z only. Requires accuracy
	const auto     mult = fstb::ToolsSimd::Shift <0>::spread (
		fstb::ToolsSimd::rcp_approx2 (x0z)
	);

	x0z *= mult;
	x1z *= mult;
	x2z *= mult;

	z_eq_b [0] = fstb::ToolsSimd::Shift <1>::extract (x0z);
	z_eq_b [1] = fstb::ToolsSimd::Shift <1>::extract (x1z);
	z_eq_b [2] = fstb::ToolsSimd::Shift <1>::extract (x2z);

	z_eq_a [0] = 1;
	z_eq_a [1] = fstb::ToolsSimd::Shift <0>::extract (x1z);
	z_eq_a [2] = fstb::ToolsSimd::Shift <0>::extract (x2z);
}



void	TransSZBilin::map_s_to_z_approx (fstb::ToolsSimd::VectF32 z_eq_b [3], fstb::ToolsSimd::VectF32 z_eq_a [3], const fstb::ToolsSimd::VectF32 s_eq_b [3], const fstb::ToolsSimd::VectF32 s_eq_a [3], fstb::ToolsSimd::VectF32 k)
{
	const auto     kk   = k * k;

	const auto     b0s  = fstb::ToolsSimd::load_f32 (&s_eq_b [0]);
	const auto     b1s  = fstb::ToolsSimd::load_f32 (&s_eq_b [1]);
	const auto     b2s  = fstb::ToolsSimd::load_f32 (&s_eq_b [2]);

	const auto     b1k  = b1s * k;
	const auto     b2kk = b2s * kk;
	const auto     b2kk_plus_b0   = b2kk + b0s;
	const auto     b0z  = b2kk_plus_b0 + b1k;
	const auto     b2z  = b2kk_plus_b0 - b1k;
	const auto     b0s_minus_b2kk = b0s - b2kk;
	const auto     b1z  = b0s_minus_b2kk + b0s_minus_b2kk;

	const auto     a0s  = fstb::ToolsSimd::load_f32 (&s_eq_a [0]);
	const auto     a1s  = fstb::ToolsSimd::load_f32 (&s_eq_a [1]);
	const auto     a2s  = fstb::ToolsSimd::load_f32 (&s_eq_a [2]);

	const auto     a1k  = a1s * k;
	const auto     a2kk = a2s * kk;
	const auto     a2kk_plus_a0   = a2kk + a0s;
	const auto     a0z  = a2kk_plus_a0 + a1k;
	const auto     a2z  = a2kk_plus_a0 - a1k;
	const auto     a0s_minus_a2kk = a0s - a2kk;
	const auto     a1z  = a0s_minus_a2kk + a0s_minus_a2kk;

	const auto     mult = fstb::ToolsSimd::rcp_approx2 (a0z);   // Requires accuracy

	fstb::ToolsSimd::store_f32 (&z_eq_b [0], b0z * mult);
	fstb::ToolsSimd::store_f32 (&z_eq_b [1], b1z * mult);
	fstb::ToolsSimd::store_f32 (&z_eq_b [2], b2z * mult);

	fstb::ToolsSimd::store_f32 (&z_eq_a [0], fstb::ToolsSimd::set1_f32 (1));
	fstb::ToolsSimd::store_f32 (&z_eq_a [1], a1z * mult);
	fstb::ToolsSimd::store_f32 (&z_eq_a [2], a2z * mult);
}



void	TransSZBilin::map_s_to_z_one_pole_approx (float z_eq_b [2], float z_eq_a [2], const float s_eq_b [2], const float s_eq_a [2], float k)
{
	assert (z_eq_b != 0);
	assert (z_eq_a != 0);
	assert (s_eq_b != 0);
	assert (s_eq_a != 0);

	// s to z bilinear transform
	const auto     kv   = fstb::ToolsSimd::set1_f32 (k);

	const auto     x0s  = fstb::ToolsSimd::set_2f32 (s_eq_a [0], s_eq_b [0]);
	const auto     x1s  = fstb::ToolsSimd::set_2f32 (s_eq_a [1], s_eq_b [1]);

	const auto     x1k = x1s * kv;
	auto           x1z = x0s - x1k;
	auto           x0z = x0s + x1k;

	// On a0z only. Requires accuracy
	const auto     mult = fstb::ToolsSimd::Shift <0>::spread (
		fstb::ToolsSimd::rcp_approx2 (x0z)
	);

	x0z *= mult;
	x1z *= mult;

	z_eq_b [0] = fstb::ToolsSimd::Shift <1>::extract (x0z);
	z_eq_b [1] = fstb::ToolsSimd::Shift <1>::extract (x1z);

	z_eq_a [0] = 1;
	z_eq_a [1] = fstb::ToolsSimd::Shift <0>::extract (x1z);
}



void	TransSZBilin::map_s_to_z_one_pole_approx (fstb::ToolsSimd::VectF32 z_eq_b [2], fstb::ToolsSimd::VectF32 z_eq_a [2], const fstb::ToolsSimd::VectF32 s_eq_b [2], const fstb::ToolsSimd::VectF32 s_eq_a [2], fstb::ToolsSimd::VectF32 k)
{
	const auto     b0s  = fstb::ToolsSimd::load_f32 (&s_eq_b [0]);
	const auto     b1s  = fstb::ToolsSimd::load_f32 (&s_eq_b [1]);

	const auto     b1k = b1s * k;
	const auto     b1z = b0s - b1k;
	const auto     b0z = b0s + b1k;

	const auto     a0s  = fstb::ToolsSimd::load_f32 (&s_eq_a [0]);
	const auto     a1s  = fstb::ToolsSimd::load_f32 (&s_eq_a [1]);

	const auto     a1k = a1s * k;
	const auto     a1z = a0s - a1k;
	const auto     a0z = a0s + a1k;

	const auto     mult = fstb::ToolsSimd::rcp_approx2 (a0z);

	fstb::ToolsSimd::store_f32 (&z_eq_b [0], b0z * mult);
	fstb::ToolsSimd::store_f32 (&z_eq_b [1], b1z * mult);

	fstb::ToolsSimd::store_f32 (&z_eq_a [0], fstb::ToolsSimd::set1_f32 (1));
	fstb::ToolsSimd::store_f32 (&z_eq_a [1], a1z * mult);
}



void	TransSZBilin::map_s_to_z_ap1_approx (float z_eq_b [2], float k)
{
	const float    a1z = 1 - k;
	const float    a0z = 1 + k;

	// IIR coefficients
	assert (! fstb::is_null (a0z));
	const auto     mult =
		fstb::ToolsSimd::rcp_approx2 (fstb::ToolsSimd::set1_f32 (a0z));
	const float    m1 = fstb::ToolsSimd::Shift <0>::extract (mult);
	z_eq_b [0] = float (a1z * m1);
	z_eq_b [1] = 1;
}



void	TransSZBilin::map_s_to_z_ap2_approx (float z_eq_b [3], float s_eq_b1, float k)
{
	const float    kk = k*k;

	const float    a1k = s_eq_b1 * k;
	const float    a2kk_plus_a0 = kk + 1;
	const float    a0z = a2kk_plus_a0 + a1k;
	const float    a2z = a2kk_plus_a0 - a1k;
	const float    a1z = 2 * (1 - kk);

	// IIR coefficients
	assert (! fstb::is_null (a0z));
	const auto     mult =
		fstb::ToolsSimd::rcp_approx2 (fstb::ToolsSimd::set1_f32 (a0z));
	const auto     axz  = fstb::ToolsSimd::set_2f32 (a2z, a1z);
	const auto     z_eq = axz * mult;
	z_eq_b [0] = fstb::ToolsSimd::Shift <0>::extract (z_eq);
	z_eq_b [1] = fstb::ToolsSimd::Shift <1>::extract (z_eq);;
	z_eq_b [2] = 1;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}  // namespace iir
}  // namespace dsp
}  // namespace mfx



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
