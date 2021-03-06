/*****************************************************************************

        CtrlUnit.h
        Author: Laurent de Soras, 2016

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (mfx_CtrlUnit_HEADER_INCLUDED)
#define mfx_CtrlUnit_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "mfx/ControlCurve.h"
#include "mfx/ControlSource.h"
#include "mfx/Cst.h"

#include <set>



namespace mfx
{

namespace doc
{
	class CtrlLink;
}


class CtrlUnit
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	               CtrlUnit ()                        = default;
	               CtrlUnit (const CtrlUnit &other)   = default;
	               CtrlUnit (const doc::CtrlLink &other, bool abs_flag);
	virtual        ~CtrlUnit ()                       = default;
	CtrlUnit &     operator = (const CtrlUnit &other) = default;

	void           update_abs_val (float raw_val);
	void           update_internal_val (float val_nrm);
	float          evaluate (float param_val) const;

	ControlSource  _source;
	float          _step     = float (Cst::_step_param);  // For relative modes (incremental encoders). > 0
	float          _val      = 0;       // Absolute controller value (-1...1). Values from relative encoders are always unipolar (0...1).

	ControlCurve   _curve    = ControlCurve_LINEAR;
	bool           _u2b_flag = false;   // Unipolar to bipolar conversion (0...1 -> -1...1)
	bool           _abs_flag = false;   // true: controls the final value (only one per ControlledParam), false: modulator (additive)
	float          _base     = 0;       // Normalized value, for absolute mode
	float          _amp      = 1;       // Normalized scale, for all modes. Can be negative.

	std::set <float>                    // Normalized values. Applies on the final value
	               _notch_list;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	static float   apply_curve (float val, ControlCurve curve, bool invert_flag);
	static double  invert_s1 (double val);



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

}; // class CtrlUnit



}  // namespace mfx



//#include "mfx/CtrlUnit.hpp"



#endif   // mfx_CtrlUnit_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
