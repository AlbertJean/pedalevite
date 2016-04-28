/*****************************************************************************

        NotificationFlag.h
        Author: Laurent de Soras, 2016

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (fstb_util_NotificationFlag_HEADER_INCLUDED)
#define fstb_util_NotificationFlag_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include	"fstb/util/NotificationFlagInterface.h"



namespace fstb
{
namespace util
{



class NotificationFlag
:	public NotificationFlagInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	               NotificationFlag ()  = default;
	inline explicit
	               NotificationFlag (bool state_flag);
	virtual        ~NotificationFlag () = default;



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// ObserverInterface via NotificationFlagInterface
	virtual inline void
	               do_update (ObservableInterface &subject);

	// NotificationFlagInterface
	virtual inline bool
	               do_get_state () const;
	virtual inline void
	               do_reset ();
	virtual inline void
	               do_set ();



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	bool           _state_flag = false;



/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               NotificationFlag (const NotificationFlag &other)  = delete;
	NotificationFlag &
	               operator = (const NotificationFlag &other)        = delete;
	bool           operator == (const NotificationFlag &other) const = delete;
	bool           operator != (const NotificationFlag &other) const = delete;

}; // class NotificationFlag



}  // namespace util
}  // namespace fstb



#include "fstb/util/NotificationFlag.hpp"



#endif   // fstb_util_NotificationFlag_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
