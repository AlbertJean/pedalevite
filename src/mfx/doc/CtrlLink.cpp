/*****************************************************************************

        CtrlLink.cpp
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

#include "fstb/fnc.h"
#include "mfx/doc/CtrlLink.h"
#include "mfx/doc/SerRInterface.h"
#include "mfx/doc/SerWInterface.h"

#include <cassert>



namespace mfx
{
namespace doc
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	CtrlLink::ser_write (SerWInterface &ser) const
{
	ser.begin_list ();

	_source.ser_write (ser);
	ser.write (_step);
	ser.write (_curve);
	ser.write (_base);
	ser.write (_amp);
	ser.write (_u2b_flag);

	ser.begin_list ();
	for (const auto &n : _notch_list)
	{
		ser.write (n);
	}
	ser.end_list ();

	ser.end_list ();
}



void	CtrlLink::ser_read (SerRInterface &ser)
{
	ser.begin_list ();

	_source.ser_read (ser);
	ser.read (_step);
	ser.read (_curve);
	ser.read (_base);
	ser.read (_amp);
	ser.read (_u2b_flag);

	int            nbr_elt;
	ser.begin_list (nbr_elt);
	_notch_list.clear ();
	for (int k = 0; k < nbr_elt; ++k)
	{
		float          n;
		ser.read (n);
		_notch_list.insert (n);
	}
	ser.end_list ();

	ser.end_list ();
}



bool	CtrlLink::is_similar (const CtrlLink &other) const
{
	const float    tol = 1e-5f;

	bool           same_flag = (_source == other._source);
	same_flag &= (_curve    == other._curve   );
	same_flag &= (_u2b_flag == other._u2b_flag);
	same_flag &= fstb::is_eq (_step, other._step, tol);
	same_flag &= fstb::is_eq (_base, other._base, tol);
	same_flag &= fstb::is_eq (_amp , other._amp , tol);

	const size_t   nbr_n = _notch_list.size ();
	same_flag &= (nbr_n == other._notch_list.size ());
	if (same_flag)
	{
		auto           it_1 = _notch_list.begin ();
		auto           it_2 = other._notch_list.begin ();
		while (it_1 != _notch_list.end () && same_flag)
		{
			same_flag = fstb::is_eq (*it_1, *it_2, tol);
			++ it_1;
			++ it_2;
		}
	}

	return same_flag;
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}  // namespace doc
}  // namespace mfx



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
