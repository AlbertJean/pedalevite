/*****************************************************************************

        Squeezer.cpp
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
#include "hiir/PolyphaseIir2Designer.h"
#include "mfx/dsp/mix/Align.h"
#include "mfx/pi/lpfs/Param.h"
#include "mfx/pi/lpfs/Squeezer.h"
#include "mfx/piapi/EventParam.h"
#include "mfx/piapi/EventTs.h"
#include "mfx/piapi/EventType.h"

#include <algorithm>

#include <cassert>



namespace mfx
{
namespace pi
{
namespace lpfs
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



Squeezer::Squeezer ()
:	_state (State_CREATED)
,	_desc ()
,	_state_set ()
,	_sample_freq (0)
,	_param_change_flag ()
,	_param_change_flag_freq_reso ()
,	_param_change_flag_color ()
,	_param_change_flag_drive ()
,	_drive_gain (1)
,	_drive_inv (1)
,	_drive_gain_old (1)
,	_drive_inv_old (1)
,	_type (0)
,	_buf ()
,	_buf_ovrspl ()
,	_chn_arr (_max_nbr_chn)
{
	dsp::mix::Align::setup ();

	const ParamDescSet & desc_set = _desc.use_desc_set ();
	_state_set.init (piapi::ParamCateg_GLOBAL, desc_set);

	_state_set.set_val_nat (desc_set, Param_FREQ , 5120);
	_state_set.set_val_nat (desc_set, Param_RESO ,    0.25f);
	_state_set.set_val_nat (desc_set, Param_COLOR,    0.75f);
	_state_set.set_val_nat (desc_set, Param_DRIVE,    4);
	_state_set.set_val_nat (desc_set, Param_TYPE,     0);

	_state_set.add_observer (Param_FREQ , _param_change_flag_freq_reso);
	_state_set.add_observer (Param_RESO , _param_change_flag_freq_reso);
	_state_set.add_observer (Param_COLOR, _param_change_flag_color    );
	_state_set.add_observer (Param_DRIVE, _param_change_flag_drive    );
	_state_set.add_observer (Param_TYPE , _param_change_flag_type     );

	_param_change_flag_freq_reso.add_observer (_param_change_flag);
	_param_change_flag_color    .add_observer (_param_change_flag);
	_param_change_flag_drive    .add_observer (_param_change_flag);
	_param_change_flag_type     .add_observer (_param_change_flag);

	_state_set.set_ramp_time (Param_FREQ , 0.010);
	_state_set.set_ramp_time (Param_DRIVE, 0.010);
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



piapi::PluginInterface::State	Squeezer::do_get_state () const
{
	return _state;
}



double	Squeezer::do_get_param_val (piapi::ParamCateg categ, int index, int note_id) const
{
	assert (categ == piapi::ParamCateg_GLOBAL);

	return _state_set.use_state (index).get_val_tgt ();
}



int	Squeezer::do_reset (double sample_freq, int max_buf_len, int &latency)
{
	latency = 0;

	_sample_freq = float (sample_freq);
	_state_set.set_sample_freq (sample_freq);
	_state_set.clear_buffers ();

	double         coef_42 [_nbr_coef_42];
	double         coef_21 [_nbr_coef_21];
	hiir::PolyphaseIir2Designer::compute_coefs_spec_order_tbw (
		coef_42, _nbr_coef_42, 1.0 / 5
	);
	hiir::PolyphaseIir2Designer::compute_coefs_spec_order_tbw (
		coef_21, _nbr_coef_21, 1.0 / 100
	);
	for (auto &c : _chn_arr)
	{
		c._ds.set_coefs (coef_42, coef_21);
		c._us.set_coefs (coef_42, coef_21);
		c._lpf1.set_sample_freq (_sample_freq * _ovrspl);
		c._lpf2.set_sample_freq (_sample_freq * _ovrspl);
		c._lpf3.set_sample_freq (_sample_freq * _ovrspl);
	}
	const int      buf_len4 = (max_buf_len + 3) & -4;
	_buf.resize (buf_len4);
	_buf_ovrspl.resize (buf_len4 * _ovrspl);

	update_param (true);
	clear_buffers ();

	_state = State_ACTIVE;

	return Err_OK;
}



void	Squeezer::do_clean_quick ()
{
	clear_buffers ();
}



void	Squeezer::do_process_block (ProcInfo &proc)
{
	const int      nbr_chn_src = proc._nbr_chn_arr [Dir_IN ];
	const int      nbr_chn_dst = proc._nbr_chn_arr [Dir_OUT];
	assert (nbr_chn_src <= nbr_chn_dst);

	// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
	// Events
	const int      nbr_evt = proc._nbr_evt;
	for (int index = 0; index < nbr_evt; ++index)
	{
		const piapi::EventTs &  evt = *(proc._evt_arr [index]);
		if (evt._type == piapi::EventType_PARAM)
		{
			const piapi::EventParam &  evtp = evt._evt._param;
			assert (evtp._categ == piapi::ParamCateg_GLOBAL);
			_state_set.set_val (evtp._index, evtp._val);
		}
	}

	int            pos = 0;
	do
	{
		// We need this intermediate varaible because for some reason GCC
		// fails to link when _update_resol is directly used in std::min.
		const int      max_len  = _update_resol;
		const int      work_len = std::min (proc._nbr_spl - pos, max_len);

		// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
		// Parameters
		_state_set.process_block (work_len);
		update_param ();

		// -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -  -
		// Signal processing

		for (int chn_cnt = 0; chn_cnt < nbr_chn_src; ++chn_cnt)
		{
			Channel &      chn = _chn_arr [chn_cnt];

			// Drive
			dsp::mix::Align::copy_1_1_vlrauto (
				&_buf [0],
				proc._src_arr [chn_cnt],
				work_len,
				_drive_gain_old,
				_drive_gain
			);

			// Hard-clips the input to avoid blowing the filter off
			// at high cutoff frequencies (not enough oversampling...)
			const auto     ma = fstb::ToolsSimd::set1_f32 ( 2);
			const auto     mi = fstb::ToolsSimd::set1_f32 (-2);
			for (int p = 0; p < work_len; p += 4)
			{
				auto           x = fstb::ToolsSimd::load_f32 (&_buf [p]);
				x = fstb::ToolsSimd::min_f32 (x, ma);
				x = fstb::ToolsSimd::max_f32 (x, mi);
				fstb::ToolsSimd::store_f32 (&_buf [p], x);
			}

			// Upsampling
			chn._us.process_block (&_buf_ovrspl [0], &_buf [0], work_len);

			// Filtering
			switch (_type)
			{
			case 0:
				chn._lpf1.process_block (&_buf_ovrspl [0], work_len * _ovrspl);
				break;
			case 1:
				chn._lpf2.process_block (&_buf_ovrspl [0], work_len * _ovrspl);
				break;
			default:
				chn._lpf3.process_block (&_buf_ovrspl [0], work_len * _ovrspl);
				break;
			}

			// Downsampling
			chn._ds.process_block (&_buf [0], &_buf_ovrspl [0], work_len);

			// Inverse drive
			dsp::mix::Align::copy_1_1_vlrauto (
				proc._dst_arr [chn_cnt],
				&_buf [0],
				work_len,
				_drive_inv_old,
				_drive_inv
			);
		}

		_drive_gain_old = _drive_gain;
		_drive_inv_old	 = _drive_inv;

		pos += work_len;
	}
	while (pos < proc._nbr_spl);

	for (int chn_cnt = nbr_chn_src; chn_cnt < nbr_chn_dst; ++chn_cnt)
	{
		dsp::mix::Align::copy_1_1 (
			proc._dst_arr [chn_cnt],
			proc._dst_arr [0],
			proc._nbr_spl
		);
	}
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	Squeezer::clear_buffers ()
{
	for (auto &c : _chn_arr)
	{
		c._ds.clear_buffers ();
		c._us.clear_buffers ();
		c._lpf1.clear_buffers ();
		c._lpf2.clear_buffers ();
		c._lpf3.clear_buffers ();
	}

	_drive_gain_old = _drive_gain;
	_drive_inv_old	 = _drive_inv;
}



void	Squeezer::update_param (bool force_flag)
{
	if (_param_change_flag (true) || force_flag)
	{
		if (_param_change_flag_type (true) || force_flag)
		{
			_type = fstb::round_int (_state_set.get_val_tgt_nat (Param_TYPE));

			// Forces update of the filter parameters
			_param_change_flag_freq_reso.set ();
			_param_change_flag_color.set ();
		}

		if (_param_change_flag_freq_reso (true) || force_flag)
		{
			const float    freq = float (_state_set.get_val_end_nat (Param_FREQ));
			const float    reso = float (_state_set.get_val_end_nat (Param_RESO));
			for (auto &c : _chn_arr)
			{
				switch (_type)
				{
				case 0:
					c._lpf1.set_freq (freq);
					c._lpf1.set_reso (reso);
					c._lpf1.update_eq ();
					break;
				case 1:
					c._lpf2.set_freq (freq);
					c._lpf2.set_reso (reso);
					c._lpf2.update_eq ();
					break;
				default:
					c._lpf3.set_freq (freq);
					c._lpf3.set_reso (reso);
					c._lpf3.update_eq ();
					break;
				}
			}
		}

		if (_param_change_flag_color (true) || force_flag)
		{
			const float    col = float (_state_set.get_val_end_nat (Param_COLOR));
			for (auto &c : _chn_arr)
			{
				switch (_type)
				{
				case 0:
					c._lpf1.set_p1 (col);
					break;
				case 1:
					c._lpf2.set_p1 (col);
					break;
				default:
					c._lpf3.set_p1 (col);
					break;
				}
			}
		}

		if (_param_change_flag_drive (true) || force_flag)
		{
			_drive_gain = float (_state_set.get_val_end_nat (Param_DRIVE));
			_drive_inv  = fstb::limit (1.0f / _drive_gain, 0.25f, 1.0f);
		}
	}
}



}  // namespace lpfs
}  // namespace pi
}  // namespace mfx



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
