/*****************************************************************************

        LineBreakAction.h
        Author: Laurent de Soras, 2008

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#if ! defined (fstb_txt_unicode_LineBreakAction_HEADER_INCLUDED)
#define	fstb_txt_unicode_LineBreakAction_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma once
	#pragma warning (4 : 4250) // "Inherits via dominance."
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



namespace fstb
{
namespace txt
{
namespace unicode
{



enum LineBreakAction
{

	LineBreakAction_DIRECT = 0,
	LineBreakAction_INDIRECT,
	LineBreakAction_COMBINING_INDIRECT,
	LineBreakAction_COMBINING_PROHIBITED,
	LineBreakAction_PROHIBITED,
	LineBreakAction_EXPLICIT,

	LineBreakAction_NBR_ELT

};	// enum LineBreakAction



}	// namespace unicode
}	// namespace txt
}	// namespace fstb



//#include	"fstb/txt/unicode/LineBreakAction.hpp"



#endif	// fstb_txt_unicode_LineBreakAction_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
