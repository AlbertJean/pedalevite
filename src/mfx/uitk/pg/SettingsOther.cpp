/*****************************************************************************

        SettingsOther.cpp
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
#include "mfx/uitk/pg/SettingsOther.h"
#include "mfx/uitk/NodeEvt.h"
#include "mfx/uitk/PageMgrInterface.h"
#include "mfx/uitk/PageSwitcher.h"
#include "mfx/ui/Font.h"
#include "mfx/ChnMode.h"
#include "mfx/MeterResultSet.h"
#include "mfx/Model.h"
#include "mfx/View.h"

#include <algorithm>

#include <cassert>



namespace mfx
{
namespace uitk
{
namespace pg
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



SettingsOther::SettingsOther (PageSwitcher &page_switcher)
:	_page_switcher (page_switcher)
,	_model_ptr (0)
,	_view_ptr (0)
,	_page_ptr (0)
,	_page_size ()
,	_tempo_sptr (TxtSPtr (new NText (Entry_TEMPO)))
,	_click_sptr (TxtSPtr (new NText (Entry_CLICK)))
{
	// Nothing
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	SettingsOther::do_connect (Model &model, const View &view, PageMgrInterface &page, Vec2d page_size, void *usr_ptr, const FontSet &fnt)
{
	_model_ptr = &model;
	_view_ptr  = &view;
	_page_ptr  = &page;
	_page_size = page_size;

	const int      h_m   = fnt._m.get_char_h ();

	_tempo_sptr->set_font (fnt._m);
	_click_sptr->set_font (fnt._m);

	_tempo_sptr->set_coord (Vec2d (0, 0 * h_m));
	_click_sptr->set_coord (Vec2d (0, 1 * h_m));

	_page_ptr->push_back (_tempo_sptr);
	_page_ptr->push_back (_click_sptr);

	PageMgrInterface::NavLocList  nav_list;
	PageMgrInterface::add_nav (nav_list, Entry_TEMPO);
	PageMgrInterface::add_nav (nav_list, Entry_CLICK);
	page.set_nav_layout (nav_list);

	refresh_display ();
}



void	SettingsOther::do_disconnect ()
{
	// Nothing
}



MsgHandlerInterface::EvtProp	SettingsOther::do_handle_evt (const NodeEvt &evt)
{
	EvtProp        ret_val = EvtProp_PASS;

	const int      node_id = evt.get_target ();

	if (evt.is_timer ())
	{
		refresh_display ();
		ret_val = EvtProp_CATCH;
	}

	else if (evt.is_button_ex ())
	{
		const Button   but = evt.get_button_ex ();
		switch (but)
		{
		case Button_S:
			ret_val = EvtProp_CATCH;
			switch (node_id)
			{
			case Entry_CLICK:
				{
					const bool     click_flag = _view_ptr->is_click_active ();
					_model_ptr->set_click (! click_flag);
				}
				break;
			default:
				ret_val = EvtProp_PASS;
				break;
			}
			break;
		case Button_E:
			_page_switcher.switch_to (pg::PageType_CUR_PROG, 0);
			ret_val = EvtProp_CATCH;
			break;
		case Button_L:
			if (node_id == Entry_TEMPO)
			{
				ret_val = change_tempo (-1);
			}
			break;
		case Button_R:
			if (node_id == Entry_TEMPO)
			{
				ret_val = change_tempo (+1);
			}
			break;
		default:
			// Nothing
			break;
		}
	}

	return ret_val;
}



MsgHandlerInterface::EvtProp	SettingsOther::change_tempo (double delta)
{
	double         bpm = _view_ptr->get_tempo ();
	bpm += delta;
	bpm = fstb::limit (bpm, double (Cst::_tempo_min), double (Cst::_tempo_max));

	_model_ptr->set_tempo (bpm);

	return EvtProp_CATCH;
}



void	SettingsOther::do_set_tempo (double /*bpm*/)
{
	refresh_display ();
}



void	SettingsOther::do_set_click (bool /*click_flag*/)
{
	refresh_display ();
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	SettingsOther::refresh_display ()
{
	const bool     click_flag = _view_ptr->is_click_active ();
	const double   tempo      = _view_ptr->get_tempo ();

	char           txt_0 [255+1];
	fstb::snprintf4all (txt_0, sizeof (txt_0), "Tempo: %7.3f BPM", tempo);
	_tempo_sptr->set_text (txt_0);

	std::string    click_txt = "Click: ";
	if (click_flag)
	{
		click_txt += "On";
	}
	else
	{
		click_txt += "Off";
	}
	_click_sptr->set_text (click_txt);
}



}  // namespace pg
}  // namespace uitk
}  // namespace mfx



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
