/*****************************************************************************

        FlanchoDesc.cpp
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

#include "mfx/pi/flancho/FlanchoDesc.h"
#include "mfx/pi/flancho/Param.h"
#include "mfx/pi/flancho/Cst.h"
#include "mfx/pi/param/MapPiecewiseLinLog.h"
#include "mfx/pi/param/MapS.h"
#include "mfx/pi/param/TplEnum.h"
#include "mfx/pi/param/TplInt.h"
#include "mfx/pi/param/TplLin.h"
#include "mfx/pi/param/TplLog.h"
#include "mfx/pi/param/TplMapped.h"
#include "mfx/pi/ParamMapFdbkBipolar.h"

#include <cassert>



namespace mfx
{
namespace pi
{
namespace flancho
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



FlanchoDesc::FlanchoDesc ()
:	_desc_set (Param_NBR_ELT, 0)
{
	typedef param::TplMapped <ParamMapFdbkBipolar> TplFdbk;
	typedef param::TplMapped <param::MapPiecewiseLinLog> TplPll;
	typedef param::TplMapped <param::MapS <false> > TplMaps;

	// Speed
	TplPll *   pll_ptr = new TplPll (
		0.01, 1000,
		"Speed",
		"Hz",
		param::HelperDispNum::Preset_FLOAT_STD,
		0,
		"%7.3f"
	);
	pll_ptr->use_mapper ().set_first_value (     0.01);
	pll_ptr->use_mapper ().add_segment (0.75,   10, true);
	pll_ptr->use_mapper ().add_segment (1   , 1000, true);
	pll_ptr->set_categ (piapi::ParamDescInterface::Categ_FREQ_HZ);
	_desc_set.add_glob (Param_SPEED, pll_ptr);

	// Depth
	param::TplLin *   lin_ptr = new param::TplLin (
		0, 1,
		"D\nDpt\nDepth",
		"%",
		0,
		"%5.2f"
	);
	lin_ptr->use_disp_num ().set_preset (param::HelperDispNum::Preset_FLOAT_PERCENT);
	_desc_set.add_glob (Param_DEPTH, lin_ptr);

	// Delay
	param::TplLog *   log_ptr = new param::TplLog (
		Cst::_delay_min / 1e6, Cst::_delay_max / 1e6,
		"D\nDly\nDelay",
		"ms",
		param::HelperDispNum::Preset_FLOAT_MILLI,
		0,
		"%5.2f"
	);
	log_ptr->set_categ (piapi::ParamDescInterface::Categ_TIME_S);
	_desc_set.add_glob (Param_DELAY, log_ptr);

	// Feedback
	TplFdbk *      fbi_ptr = new TplFdbk (
		TplFdbk::Mapper::get_nat_min (),
		TplFdbk::Mapper::get_nat_max (),
		"F\nFdbk\nFeedback",
		"%",
		param::HelperDispNum::Preset_FLOAT_PERCENT,
		0,
		"%+6.1f"
	);
	_desc_set.add_glob (Param_FDBK, fbi_ptr);

	// Waveform Type
	param::TplEnum *  enu_ptr = new param::TplEnum (
		"Sine\nTriangle\nParabola\nInv.para\nRamp up\nRamp down\nRandom",
		"T\nWf.T\nWaveform\nWaveform type",
		"",
		0,
		"%s"
	);
	_desc_set.add_glob (Param_WF_TYPE, enu_ptr);

	// Waveform Shape
	lin_ptr = new param::TplLin (
		-1, 1,
		"S\nWf.S\nShape\nWaveform shape",
		"%",
		0,
		"%+6.1f"
	);
	lin_ptr->use_disp_num ().set_preset (param::HelperDispNum::Preset_FLOAT_PERCENT);
	_desc_set.add_glob (Param_WF_SHAPE, lin_ptr);

	// Number of voices
	param::TplInt *   int_ptr = new param::TplInt (
		1, Cst::_max_nbr_voices,
		"#\nVc#\nNumber of voices",
		"",
		0,
		"%1.0f"
	);
	_desc_set.add_glob (Param_NBR_VOICES, int_ptr);

	// Mix
	TplMaps *      maps_ptr = new TplMaps (
		0, 1,
		"M\nMix",
		"%",
		param::HelperDispNum::Preset_FLOAT_PERCENT,
		0,
		"%5.1f"
	);
	maps_ptr->use_mapper ().config (
		maps_ptr->get_nat_min (),
		maps_ptr->get_nat_max ()
	);
	_desc_set.add_glob (Param_MIX, maps_ptr);

	// Phase set
	lin_ptr = new param::TplLin (
		0, 1,
		"P\nPh.Set\nPhase set",
		"deg",
		0,
		"%3.0f"
	);
	lin_ptr->use_disp_num ().set_preset (param::HelperDispNum::Preset_FLOAT_STD);
	lin_ptr->use_disp_num ().set_scale (360);
	_desc_set.add_glob (Param_PHASE_SET, lin_ptr);

	// Negative
	enu_ptr = new param::TplEnum (
		"Add\nSub",
		"M\nMix\nMix Mode",
		"",
		0,
		"%s"
	);
	_desc_set.add_glob (Param_NEGATIVE, enu_ptr);

	// Oversampling
	enu_ptr = new param::TplEnum (
		"\xC3\x97" "1\n" "\xC3\x97" "4", // U+00D7 multiplication sign (UTF-8 C3 97)
		"O\nOvr\nOvrspl\nOversamp\nOversampling\nOversampling rate",
		"",
		0,
		"%s"
	);
	_desc_set.add_glob (Param_OVRSPL, enu_ptr);
}



ParamDescSet &	FlanchoDesc::use_desc_set ()
{
	return _desc_set;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



std::string	FlanchoDesc::do_get_unique_id () const
{
	return "flancho";
}



std::string	FlanchoDesc::do_get_name () const
{
	return "FlanCho";
}



void	FlanchoDesc::do_get_nbr_io (int &nbr_i, int &nbr_o, int &nbr_s) const
{
	nbr_i = 1;
	nbr_o = 1;
	nbr_s = 0;
}



bool	FlanchoDesc::do_prefer_stereo () const
{
	return true;
}



int	FlanchoDesc::do_get_nbr_param (piapi::ParamCateg categ) const
{
	return _desc_set.get_nbr_param (categ);
}



const piapi::ParamDescInterface &	FlanchoDesc::do_get_param_info (piapi::ParamCateg categ, int index) const
{
	return _desc_set.use_param (categ, index);
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



}  // namespace flancho
}  // namespace pi
}  // namespace mfx



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
