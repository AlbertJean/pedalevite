/*****************************************************************************

        DAsio.cpp
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

#include "asiosdk2/common/asiosys.h"
#include "fstb/fnc.h"
#include "mfx/adrv/CbInterface.h"
#include "mfx/adrv/DAsio.h"

#include <stdexcept>

#include <cassert>
#include <cstdint>
#include <cstring>



extern ::AsioDrivers *	asioDrivers; // From the SDK



namespace mfx
{
namespace adrv
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



DAsio::DAsio ()
:	_state (State_UNLOADED)
,	_cb_ptr (0)
,	_drivers_ptr (0)
,	_driver_index (0)
,	_block_size (0)
,	_sample_freq (0)
,	_asio_info ()
,	_chn_base_arr ({{ 0, 0 }})
,	_msg_err ()
,	_asio_callbacks ({
		process_asio,
		samplerate_did_change,
		asio_message,
		buffer_switch_time_info
	})
,	_buf_info_arr ()
,	_chn_info_arr ()
,	_buf_alig ()
{
	assert (_instance_ptr == 0);
	if (_instance_ptr != 0)
	{
		throw std::runtime_error ("mfx::adrv::DAsio already instantiated.");
	}
	_instance_ptr = this;
}



DAsio::~DAsio ()
{
	stop ();
	_instance_ptr = 0;
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



int	DAsio::do_init (double &sample_freq, int &max_block_size, CbInterface &callback, const char *driver_0, int chn_idx_in, int chn_idx_out)
{
	assert (_state == State_UNLOADED);
	int            ret_val = 0;
	::ASIOError    err;

	_msg_err.clear ();
	sample_freq    = 0;
	max_block_size = 0;
	_cb_ptr        = &callback;
	_chn_base_arr [Dir_IN ] = chn_idx_in;
	_chn_base_arr [Dir_OUT] = chn_idx_out;

	// -  -  -  -  - -  -  -  -  - -  -  -  -  - -  -  -  -  - -  -  -  -  -
	// Unloaded

	_drivers_ptr = new ::AsioDrivers;
	::asioDrivers = _drivers_ptr;

	std::vector <std::array <char, 32+1> > driver_name_content_list (32);
	std::vector <char *> driver_name_list;
	for (auto &x : driver_name_content_list)
	{
		driver_name_list.push_back (&x [0]);
	}
	const long     nbr_drivers = _drivers_ptr->getDriverNames (
		&driver_name_list [0],
		long (driver_name_list.size ())
	);
	if (driver_0 != 0)
	{
		bool           found_flag = false;
		for (long pos = 0; pos < nbr_drivers && ! found_flag; ++pos)
		{
			if (strcmp (driver_name_list [pos], driver_0) == 0)
			{
				_driver_index = int (pos);
				found_flag    = true;
			}
		}
	}
	if (_driver_index >= nbr_drivers)
	{
		_msg_err = "Driver not found";
		ret_val  = -1;
	}

	if (ret_val == 0)
	{
		const bool     ok_flag = _drivers_ptr->loadDriver (
			driver_name_list [_driver_index]
		);
		if (ok_flag)
		{
			_state = State_LOADED;
		}
		else
		{
			_msg_err = "Cannot load driver";
			ret_val  = -1;
		}
	}

	// -  -  -  -  - -  -  -  -  - -  -  -  -  - -  -  -  -  - -  -  -  -  -
	// Loaded

	// Driver init
	if (ret_val == 0)
	{
		memset (&_asio_info, 0, sizeof (_asio_info));
		_asio_info.asioVersion = 2;
		_asio_info.sysRef      =
			reinterpret_cast <void *> (::GetDesktopWindow ());
		err = ::ASIOInit (&_asio_info);
		if (err == ::ASE_OK)
		{
			_state = State_INITIALISED;
		}
		else
		{
			_msg_err = _asio_info.errorMessage;
			ret_val  = -1;
		}
	}

	// -  -  -  -  - -  -  -  -  - -  -  -  -  - -  -  -  -  - -  -  -  -  -
	// Initialized

	// Sampling rate
	::ASIOSampleRate  sample_rate = 44100.0;
	if (ret_val == 0)
	{
		err = ::ASIOCanSampleRate (sample_rate);
		if (err != ::ASE_OK)
		{
			ret_val  = -1;
			_msg_err = "ASIOCanSampleRate failed";
		}
	}
	if (ret_val == 0)
	{
		err = ::ASIOSetSampleRate (sample_rate);
		if (err == ::ASE_OK)
		{
			sample_freq = double (sample_rate);
		}
		else
		{
			ret_val  = -1;
			_msg_err = "ASIOSetSampleRate failed";
		}
	}

	// Buffer size
	long           buffer_size = 0;
	if (ret_val == 0)
	{
		long           buf_size_min;
		long           buf_size_max;
		long           buf_size_pref;
		long           granularity;
		err = ::ASIOGetBufferSize (
			&buf_size_min,
			&buf_size_max,
			&buf_size_pref,
			&granularity
		);
		if (err != ::ASE_OK)
		{
			_msg_err = "ASIOGetBufferSize failed";
			ret_val  = -1;
		}
		else
		{
			buffer_size = buf_size_pref;
		}
	}
	if (ret_val == 0)
	{
		std::array <long, Dir_NBR_ELT> nbr_chn_arr;
		err = ::ASIOGetChannels (&nbr_chn_arr [Dir_IN ], &nbr_chn_arr [Dir_OUT]);
		if (err != ::ASE_OK)
		{
			_msg_err = "ASIOGetChannels failed";
			ret_val  = -1;
		}
		for (int dir = 0; dir < int (nbr_chn_arr.size ()) && ret_val == 0; ++dir)
		{
			if (nbr_chn_arr [dir] < _chn_base_arr [dir] + int (_buf_info_arr [dir].size ()))
			{
				_msg_err = "ASIOGetChannels: insufficient number of channels";
				ret_val  = -1;
			}
		}
	}
	if (ret_val == 0)
	{
		const int      buf_alig_sz = int (buffer_size + 3) & -4;
		_buf_alig.resize (buf_alig_sz * _nbr_chn * Dir_NBR_ELT);
	}

	// Channel information
	if (ret_val == 0)
	{
		for (int dir = 0; dir < Dir_NBR_ELT && ret_val == 0; ++dir)
		{
			for (int chn = 0; chn < int (_buf_info_arr [dir].size ()) && ret_val == 0; ++chn)
			{
				::ASIOBufferInfo &   buf = _buf_info_arr [dir] [chn];
				buf.isInput      = (dir == Dir_IN) ? ::ASIOTrue : ::ASIOFalse;
				buf.channelNum   = chn + _chn_base_arr [dir];

				::ASIOChannelInfo &  chn_info = _chn_info_arr [dir] [chn];
				chn_info.channel = buf.channelNum;
				chn_info.isInput = buf.isInput;
				err              = ::ASIOGetChannelInfo (&chn_info);
				if (err != ::ASE_OK)
				{
					_msg_err = "ASIOGetChannelInfo failed";
					ret_val  = -1;
				}
				else if (chn_info.type != ::ASIOSTInt32LSB)
				{
					_msg_err = "ASIOGetChannelInfo: unsupported data type";
					ret_val  = -1;
				}
			}
		}
	}

	// Channel allocation
	if (ret_val == 0)
	{
		err = ::ASIOCreateBuffers (
			&_buf_info_arr [0] [0],
			long (
				  _buf_info_arr [Dir_IN ].size ()
				+ _buf_info_arr [Dir_OUT].size ()
			),
			buffer_size,
			const_cast <::ASIOCallbacks *> (&_asio_callbacks)
		);

		if (err == ::ASE_OK)
		{
			max_block_size  = buffer_size;
			_state          = State_PREPARED;
		}
		else
		{
			ret_val = -1;
			_msg_err = "ASIOCreateBuffers failed";
		}
	}

	if (ret_val == 0)
	{
		_sample_freq = sample_freq;
		_block_size	 = max_block_size;
	}

	return ret_val;
}



int	DAsio::do_start ()
{
	assert (_state == State_PREPARED);

	int            ret_val = 0;
	_msg_err.clear ();

	::ASIOError    err = ::ASIOStart ();
	if (err != ::ASE_OK)
	{
		_msg_err = "ASIOStart failed";
		ret_val  = -1;
	}

	return ret_val;
}



int	DAsio::do_stop ()
{
	_msg_err.clear ();

	if (_state == State_RUNNING)
	{
		::ASIOStop ();
		_state = State_PREPARED;
	}

	if (_state == State_PREPARED)
	{
		::ASIODisposeBuffers ();
		_state = State_INITIALISED;
	}

	if (_state == State_INITIALISED)
	{
		::ASIOExit ();
		_state = State_LOADED;
	}

	if (_state == State_LOADED)
	{
		if (_drivers_ptr != 0 && asioDrivers != 0)
		{
			_drivers_ptr->removeCurrentDriver ();
		}
		if (_drivers_ptr != 0 && asioDrivers != 0)
		{
			delete _drivers_ptr;
		}
		asioDrivers  = 0;
		_drivers_ptr = 0;
		_state       = State_UNLOADED;
	}

	return 0;
}



std::string	DAsio::do_get_last_error () const
{
	return _msg_err;
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	DAsio::process_block (long buf_index)
{
	const int      buf_alig_sz = (_block_size + 3) & -4;

	const std::array <float *, _nbr_chn> src_arr =
	{{
		&_buf_alig [buf_alig_sz * 0],
		&_buf_alig [buf_alig_sz * 1]
	}};
	const std::array <float *, _nbr_chn> dst_arr =
	{{
		&_buf_alig [buf_alig_sz * 2],
		&_buf_alig [buf_alig_sz * 3]
	}};
	assert (src_arr [_nbr_chn - 1] != 0);
	assert (dst_arr [_nbr_chn - 1] != 0);

	for (int chn = 0; chn < _nbr_chn; ++chn)
	{
		const ::ASIOBufferInfo &   buf_info = _buf_info_arr [Dir_IN ] [chn];
		const int32_t *   asio_src_ptr = reinterpret_cast <const int32_t *> (
			buf_info.buffers [buf_index]
		);

		for (int pos = 0; pos < _block_size; ++pos)
		{
			src_arr [chn] [pos] = asio_src_ptr [pos] * (1.0f / (32768.0f * 65536.0f));
		}
	}

	_cb_ptr->process_block (&dst_arr [0], &src_arr [0], _block_size);

	for (int chn = 0; chn < _nbr_chn; ++chn)
	{
		const ::ASIOBufferInfo &   buf_info = _buf_info_arr [Dir_OUT] [chn];
		int32_t *      asio_dst_ptr = reinterpret_cast <int32_t *> (
			buf_info.buffers [buf_index]
		);

		if (asio_dst_ptr != 0)
		{
			for (int pos = 0; pos < _block_size; ++pos)
			{
				float          val     = dst_arr [chn] [pos];
				int32_t        val_int = fstb::conv_int_fast (val * (1 << 23));
				val_int = fstb::limit (val_int, -(1 << 23), (1 << 23) - 1);
				asio_dst_ptr [pos] = val_int << 8;
			}
		}
	}
}



void	DAsio::process_asio (long doubleBufferIndex, ::ASIOBool directProcess)
{
	_instance_ptr->process_block (doubleBufferIndex);
}



void	DAsio::samplerate_did_change (::ASIOSampleRate sRate)
{
	if (sRate != _instance_ptr->_sample_freq)
	{
		_instance_ptr->_cb_ptr->request_exit ();
	}
}



long	DAsio::asio_message (long selector, long value, void* message, double* opt)
{
	long           ret_val = 0;

	switch (selector)
	{
	case ::kAsioSelectorSupported:
		switch (value)
		{
		case ::kAsioSelectorSupported:
		case ::kAsioResetRequest:
		case ::kAsioBufferSizeChange:
		case ::kAsioResyncRequest:
		case ::kAsioOverload:
			ret_val = 1;
			break;
		}
		break;

	case ::kAsioResetRequest:
		_instance_ptr->_cb_ptr->request_exit ();
		ret_val = 1;
		break;
	case ::kAsioBufferSizeChange:
	case ::kAsioResyncRequest:
		_instance_ptr->_cb_ptr->request_exit ();
		break;
	case ::kAsioOverload:
		_instance_ptr->_cb_ptr->notify_dropout ();
		break;
	}

	return ret_val;
}



::ASIOTime *	DAsio::buffer_switch_time_info (::ASIOTime* params, long doubleBufferIndex, ::ASIOBool directProcess)
{
	process_asio (doubleBufferIndex, directProcess);

	return params;
}



DAsio *	DAsio::_instance_ptr = 0;



}  // namespace adrv
}  // namespace mfx



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
