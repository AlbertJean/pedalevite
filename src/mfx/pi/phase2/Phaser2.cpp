/*****************************************************************************

        Phaser2.cpp
        Author: Laurent de Soras, 2017

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

#include "fstb/Approx.h"
#include "fstb/fnc.h"
#include "fstb/ToolsSimd.h"
#include "mfx/dsp/iir/TransSZBilin.h"
#include "mfx/dsp/mix/Align.h"
#include "mfx/pi/phase2/LfoType.h"
#include "mfx/pi/phase2/Param.h"
#include "mfx/pi/phase2/Phaser2.h"
#include "mfx/piapi/EventParam.h"
#include "mfx/piapi/EventTs.h"
#include "mfx/piapi/EventType.h"

#include <cassert>
#include <cmath>



namespace mfx
{
namespace pi
{
namespace phase2
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



Phaser2::Phaser2 ()
:	_state (State_CREATED)
,	_desc ()
,	_state_set ()
,	_sample_freq (0)
,	_inv_fs (0)
,	_param_change_flag ()
,	_param_change_flag_freq ()
,	_param_change_flag_misc ()
,	_param_change_flag_lfo ()
,	_param_change_flag_lfo_base ()
,	_param_change_flag_lfo_phase ()
,	_param_change_flag_lfo_phset ()
,	_param_change_flag_lfo_shape ()
,	_param_change_flag_lfo_wf ()
,	_lfo ()
,	_nbr_stages (4)
,	_fdbk_pos (4)
,	_freq_min_l2 (log2 ( 350.f))
,	_freq_max_l2 (log2 (2000.f))
,	_feedback (0)
,	_mix (0.5f)
,	_chn_arr ()
,	_buf ()
{
	mfx::dsp::mix::Align::setup ();

	const ParamDescSet & desc_set = _desc.use_desc_set ();
	_state_set.init (piapi::ParamCateg_GLOBAL, desc_set);

	_state_set.set_val_nat (desc_set, Param_SPEED      , 1);
	_state_set.set_val_nat (desc_set, Param_MIX        , 0.5);
	_state_set.set_val_nat (desc_set, Param_FEEDBACK   , 0);
	_state_set.set_val_nat (desc_set, Param_NBR_STAGES , 2);
	_state_set.set_val_nat (desc_set, Param_FREQ_MIN   , 320.f);
	_state_set.set_val_nat (desc_set, Param_FREQ_MAX   , 2560.f);
	_state_set.set_val_nat (desc_set, Param_FDBK_POS   , 4);
	_state_set.set_val_nat (desc_set, Param_WAVEFORM   , LfoType_TRIANGLE);
	_state_set.set_val_nat (desc_set, Param_SNH        , 0);
	_state_set.set_val_nat (desc_set, Param_SMOOTH     , 0);
	_state_set.set_val_nat (desc_set, Param_CHAOS      , 0);
	_state_set.set_val_nat (desc_set, Param_PH_DIST_AMT, 0.5);
	_state_set.set_val_nat (desc_set, Param_PH_DIST_OFS, 0);
	_state_set.set_val_nat (desc_set, Param_SIGN       , 0);
	_state_set.set_val_nat (desc_set, Param_POLARITY   , 0);
	_state_set.set_val_nat (desc_set, Param_VAR1       , 0);
	_state_set.set_val_nat (desc_set, Param_VAR2       , 0);
	_state_set.set_val_nat (desc_set, Param_PHASE_SET  , 0);

	_state_set.add_observer (Param_MIX        , _param_change_flag_misc);
	_state_set.add_observer (Param_FEEDBACK   , _param_change_flag_misc);
	_state_set.add_observer (Param_NBR_STAGES , _param_change_flag_misc);
	_state_set.add_observer (Param_FDBK_POS   , _param_change_flag_misc);

	_state_set.add_observer (Param_FREQ_MIN   , _param_change_flag_freq);
	_state_set.add_observer (Param_FREQ_MAX   , _param_change_flag_freq);

	_state_set.add_observer (Param_SPEED      , _param_change_flag_lfo_base);
	_state_set.add_observer (Param_WAVEFORM   , _param_change_flag_lfo_wf);
	_state_set.add_observer (Param_SNH        , _param_change_flag_lfo_shape);
	_state_set.add_observer (Param_SMOOTH     , _param_change_flag_lfo_shape);
	_state_set.add_observer (Param_CHAOS      , _param_change_flag_lfo_phase);
	_state_set.add_observer (Param_PH_DIST_AMT, _param_change_flag_lfo_phase);
	_state_set.add_observer (Param_PH_DIST_OFS, _param_change_flag_lfo_phase);
	_state_set.add_observer (Param_SIGN       , _param_change_flag_lfo_shape);
	_state_set.add_observer (Param_POLARITY   , _param_change_flag_lfo_shape);
	_state_set.add_observer (Param_VAR1       , _param_change_flag_lfo_wf);
	_state_set.add_observer (Param_VAR2       , _param_change_flag_lfo_wf);
	_state_set.add_observer (Param_PHASE_SET  , _param_change_flag_lfo_phset);

	_param_change_flag_lfo_base .add_observer (_param_change_flag_lfo);
	_param_change_flag_lfo_phase.add_observer (_param_change_flag_lfo);
	_param_change_flag_lfo_shape.add_observer (_param_change_flag_lfo);
	_param_change_flag_lfo_wf   .add_observer (_param_change_flag_lfo);
	_param_change_flag_lfo_phset.add_observer (_param_change_flag_lfo);

	_param_change_flag_misc.add_observer (_param_change_flag);
	_param_change_flag_freq.add_observer (_param_change_flag);
	_param_change_flag_lfo .add_observer (_param_change_flag);

	_state_set.set_ramp_time (Param_MIX   , 0.010);

	for (auto &chn : _chn_arr)
	{
		chn._apf.reserve (32);
	}
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



piapi::PluginInterface::State	Phaser2::do_get_state () const
{
	return _state;
}



double	Phaser2::do_get_param_val (piapi::ParamCateg categ, int index, int note_id) const
{
	assert (categ == piapi::ParamCateg_GLOBAL);

	return _state_set.use_state (index).get_val_tgt ();
}



int	Phaser2::do_reset (double sample_freq, int max_buf_len, int &latency)
{
	latency = 0;

	_sample_freq = float (    sample_freq);
	_inv_fs      = float (1 / sample_freq);

	_state_set.set_sample_freq (sample_freq);
	_state_set.clear_buffers ();

	_lfo.set_sample_freq (sample_freq);

	const int      mbs_align = (max_buf_len + 3) & -4;
	_buf.resize (mbs_align);

	_param_change_flag_freq     .set ();
	_param_change_flag_misc     .set ();
	_param_change_flag_lfo_base .set ();
	_param_change_flag_lfo_phase.set ();
	_param_change_flag_lfo_phset.set ();
	_param_change_flag_lfo_shape.set ();
	_param_change_flag_lfo_wf   .set ();

	update_param (true);
	clear_buffers ();

	_state = State_ACTIVE;

	return Err_OK;
}



void	Phaser2::do_clean_quick ()
{
	clear_buffers ();
}



void	Phaser2::clear_buffers ()
{
	_lfo.clear_buffers ();
	for (auto &chn : _chn_arr)
	{
		chn._apf.clear_buffers ();
		chn._fdbk = 0;
	}
}



void	Phaser2::do_process_block (ProcInfo &proc)
{
	// Channels
	const int      nbr_chn_src =
		proc._nbr_chn_arr [piapi::PluginInterface::Dir_IN ];
	const int      nbr_chn_dst =
		proc._nbr_chn_arr [piapi::PluginInterface::Dir_OUT];
	assert (nbr_chn_src <= nbr_chn_dst);
	const int      nbr_chn_proc = std::min (nbr_chn_src, nbr_chn_dst);

	// Events
	for (int evt_cnt = 0; evt_cnt < proc._nbr_evt; ++evt_cnt)
	{
		const piapi::EventTs &  evt = *(proc._evt_arr [evt_cnt]);
		if (evt._type == piapi::EventType_PARAM)
		{
			const piapi::EventParam &  evtp = evt._evt._param;
			assert (evtp._categ == piapi::ParamCateg_GLOBAL);
			_state_set.set_val (evtp._index, evtp._val);
		}
	}

	const int      nbr_spl  = proc._nbr_spl;

	// Parameters
	const float    mix_beg   = _mix;
	_state_set.process_block (nbr_spl);
	update_param (false);
	_lfo.tick (nbr_spl);
	const float    mix_end   = _mix;
	const float    lfo_end   = (1 + float (_lfo.get_val ())) * 0.5f;

	// Signal processing
	const float    f0        = fstb::Approx::exp2 (
		fstb::lerp (_freq_min_l2, _freq_max_l2, lfo_end)
	);
	update_filter (f0);
	assert (_fdbk_pos <= _nbr_stages);
	for (int chn_index = 0; chn_index < nbr_chn_proc; ++chn_index)
	{
		Channel &      chn      = _chn_arr [chn_index];
		const float *  src_ptr  = proc._src_arr [chn_index];
		float *        dst_ptr  = &_buf [0];
		const float    fdbk_g   = _feedback;
		float          fdbk_val = chn._fdbk;
		for (int pos = 0; pos < nbr_spl; ++pos)
		{
			float          x = src_ptr [pos];
			x += fdbk_val * fdbk_g;
			float          y = chn._apf.process_sample (x);
			y = saturate (y);
			dst_ptr [pos] = y;
			fdbk_val      = chn._apf.get_state (_fdbk_pos);
		}
		chn._fdbk = fdbk_val;

		dsp::mix::Align::copy_xfade_2_1_vlrauto (
			proc._dst_arr [chn_index],
			src_ptr,
			dst_ptr,
			nbr_spl,
			mix_beg,
			mix_end
		);
	}

	// Duplicates the remaining output channels
	for (int chn_index = 0; chn_index < nbr_chn_dst; ++chn_index)
	{
		dsp::mix::Align::copy_1_1 (
			proc._dst_arr [chn_index],
			proc._dst_arr [0],
			proc._nbr_spl
		);
	}
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	Phaser2::update_param (bool force_flag)
{
	if (_param_change_flag (true) || force_flag)
	{
		if (_param_change_flag_misc (true) || force_flag)
		{
			_mix      = float (_state_set.get_val_end_nat (Param_MIX       ));
			_feedback = float (_state_set.get_val_end_nat (Param_FEEDBACK  ));
			const int   nbr_pairs = fstb::round_int (
				_state_set.get_val_tgt_nat (Param_NBR_STAGES)
			);
			_nbr_stages = nbr_pairs * 2;
			if (_nbr_stages != _chn_arr [0]._apf.get_length ())
			{
				for (auto &chn : _chn_arr)
				{
					chn._apf.set_length (_nbr_stages);
				}
			}
			const int      fdbk_pos = fstb::round_int (
				_state_set.get_val_tgt_nat (Param_FDBK_POS)
			);
			set_fdbk_pos (fdbk_pos); // After having set the number of stages
		}

		if (_param_change_flag_freq (true) || force_flag)
		{
			_freq_min_l2 = fstb::Approx::log2 (
				float (_state_set.get_val_end_nat (Param_FREQ_MIN))
			);
			_freq_max_l2 = fstb::Approx::log2 (
				float (_state_set.get_val_end_nat (Param_FREQ_MAX))
			);
		}

		if (_param_change_flag_lfo (true) || force_flag)
		{
			if (_param_change_flag_lfo_base (true) || force_flag)
			{
				const double  spd = _state_set.get_val_tgt_nat (Param_SPEED);
				_lfo.set_period (1.0 / spd);
			}
			if (_param_change_flag_lfo_phase (true) || force_flag)
			{
				const float    pch = float (_state_set.get_val_tgt_nat (Param_CHAOS));
				const float    pda = float (_state_set.get_val_tgt_nat (Param_PH_DIST_AMT));
				const float    pdo = float (_state_set.get_val_tgt_nat (Param_PH_DIST_OFS));

				_lfo.set_chaos (pch);
				_lfo.set_phase_dist (pda);
				_lfo.set_phase_dist_offset (pdo);
			}
			if (_param_change_flag_lfo_phset (true) || force_flag)
			{
				float          p = float (_state_set.get_val_tgt_nat (Param_PHASE_SET));
				if (p >= 1)
				{
					p = 0;
				}
				_lfo.set_phase (p);
			}
			if (_param_change_flag_lfo_shape (true) || force_flag)
			{
				const float    snh = float (_state_set.get_val_tgt_nat (Param_SNH));
				const float    smt = float (_state_set.get_val_tgt_nat (Param_SMOOTH));
				const bool     inv_flag =
					(_state_set.get_val_tgt_nat (Param_SIGN) >= 0.5f);
				const bool     uni_flag =
					(_state_set.get_val_tgt_nat (Param_POLARITY) >= 0.5f);
				_lfo.set_snh (snh);
				_lfo.set_smooth (smt);
				_lfo.set_sign (inv_flag);
				_lfo.set_polarity (uni_flag);
			}
			if (_param_change_flag_lfo_wf (true) || force_flag)
			{
				const LfoType  wf = static_cast <LfoType> (fstb::round_int (
					_state_set.get_val_tgt_nat (Param_WAVEFORM)
				));
				dsp::ctrl::lfo::LfoModule::Type  wf2 =
					dsp::ctrl::lfo::LfoModule::Type_SINE;
				switch (wf)
				{
				case LfoType_SINE:      wf2 = dsp::ctrl::lfo::LfoModule::Type_SINE      ; break;
				case LfoType_TRIANGLE:  wf2 = dsp::ctrl::lfo::LfoModule::Type_TRIANGLE  ; break;
				case LfoType_SQUARE:    wf2 = dsp::ctrl::lfo::LfoModule::Type_SQUARE    ; break;
				case LfoType_SAW:       wf2 = dsp::ctrl::lfo::LfoModule::Type_SAW       ; break;
				case LfoType_PARABOLA:  wf2 = dsp::ctrl::lfo::LfoModule::Type_PARABOLA  ; break;
				case LfoType_BIPHASE:   wf2 = dsp::ctrl::lfo::LfoModule::Type_BIPHASE   ; break;
				case LfoType_N_PHASE:   wf2 = dsp::ctrl::lfo::LfoModule::Type_N_PHASE   ; break;
				case LfoType_VARISLOPE: wf2 = dsp::ctrl::lfo::LfoModule::Type_VARISLOPE ; break;
				case LfoType_NOISE_FLT: wf2 = dsp::ctrl::lfo::LfoModule::Type_NOISE_FLT2; break;
				default:
					assert (false);
					break;
				}
				_lfo.set_type (wf2);
				const float    v1 = float (_state_set.get_val_tgt_nat (Param_VAR1));
				const float    v2 = float (_state_set.get_val_tgt_nat (Param_VAR2));
				_lfo.set_variation (0, v1);
				_lfo.set_variation (1, v2);
			}
		}
	}
}



void	Phaser2::update_filter (float f0)
{
	assert (_inv_fs > 0);
	assert (f0 > 0);

	static const float   b_s [2] = { 1, -1 };
	static const float   a_s [2] = { 1,  1 };
	float                b_z [2];
	float                a_z [2];
	const float          k       =
		dsp::iir::TransSZBilin::compute_k_approx (f0 * _inv_fs);
	dsp::iir::TransSZBilin::map_s_to_z_one_pole_approx (
		b_z, a_z, b_s, a_s, k
	);
	for (auto &chn : _chn_arr)
	{
		chn._apf.set_coef_all (b_z [0]);
	}
}



void	Phaser2::set_fdbk_pos (int pos)
{
	assert (pos > 0);

	_fdbk_pos = std::min (pos, _nbr_stages);
}



float   Phaser2::saturate (float x)
{
	const float    bias = 0.1f;

	x += bias;
	x = fstb::limit (x, -1.0f, 1.0f);
	const float    x2 = x  * x;
	const float    x4 = x2 * x2;
	x  =    x * (1 -                        x4 * 0.2f);
	x -= bias * (1 - bias * bias * bias * bias * 0.2f);

	return x;
}



}  // namespace phase2
}  // namespace pi
}  // namespace mfx



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
