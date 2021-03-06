/*****************************************************************************

        ListPresets.cpp
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

#include "mfx/uitk/pg/ListPresets.h"
#include "mfx/uitk/NodeEvt.h"
#include "mfx/uitk/PageMgrInterface.h"
#include "mfx/uitk/PageSwitcher.h"
#include "mfx/ui/Font.h"
#include "mfx/LocEdit.h"
#include "mfx/Model.h"
#include "mfx/View.h"

#include <cassert>



namespace mfx
{
namespace uitk
{
namespace pg
{



/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



ListPresets::ListPresets (PageSwitcher &page_switcher, LocEdit &loc_edit)
:	_page_switcher (page_switcher)
,	_loc_edit (loc_edit)
,	_model_ptr (0)
,	_view_ptr (0)
,	_page_ptr (0)
,	_page_size ()
,	_fnt_ptr (0)
,	_menu_sptr (new NWindow (Entry_WINDOW))
,	_preset_list ()
,	_preset_pos_map ()
,	_action (Action_INVALID)
,	_state (State_NORMAL)
,	_name_param ()
,	_state_set_idx (-1)
{
	// Nothing
}



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	ListPresets::do_connect (Model &model, const View &view, PageMgrInterface &page, Vec2d page_size, void *usr_ptr, const FontSet &fnt)
{
	assert (_loc_edit._slot_id >= 0);
	assert (usr_ptr != 0 || _state != State_NORMAL); // Actually we're not sure it cannot happen

	_model_ptr = &model;
	_view_ptr  = &view;
	_page_ptr  = &page;
	_page_size = page_size;
	_fnt_ptr   = &fnt._m;

	// Back from name prompt
	if (_state == State_EDIT_NAME)
	{
		_state = State_NORMAL;
		if (_name_param._ok_flag)
		{
			switch (_action)
			{
			case Action_STORE:
				store_2 ();
				_page_switcher.switch_to (pg::PageType_MENU_PRESETS, 0);
				return;
			case Action_RENAME:
				rename_2 ();
				break;
			default:
				assert (false);
				break;
			}
		}
	}

	// Direct
	else
	{
		Param &        param = *reinterpret_cast <Param *> (usr_ptr);
		assert (param._action >= 0);
		assert (param._action < Action_NBR_ELT);
		_action        = param._action;
		_state_set_idx = -1;
	}

	_menu_sptr->set_size (_page_size, Vec2d ());
	_menu_sptr->set_disp_pos (Vec2d ());

	_page_ptr->push_back (_menu_sptr);

	update_display ();

	// Jumps to the selected preset for the current plug-in type
	const doc::Preset &  preset = _view_ptr->use_preset_cur ();
	const doc::Slot &    slot   = preset.use_slot (_loc_edit._slot_id);
	auto                 it_pos = _preset_pos_map.find (slot._pi_model);
	if (it_pos != _preset_pos_map.end ())
	{
		assert (! _preset_list.empty ());
		const int      nbr_presets = int (_preset_list.size ());
		const int      set_idx     = std::min (it_pos->second, nbr_presets - 1);
		const int      node_id     = set_idx;
		_page_ptr->jump_to (node_id);
		if (_action == Action_BROWSE)
		{
			load (set_idx);
		}
	}
}



void	ListPresets::do_disconnect ()
{
	// Nothing
}



MsgHandlerInterface::EvtProp	ListPresets::do_handle_evt (const NodeEvt &evt)
{
	EvtProp        ret_val = EvtProp_PASS;

	const int      node_id = evt.get_target ();

	if (evt.is_cursor () && evt.get_cursor () == NodeEvt::Curs_ENTER)
	{
		if (_action == Action_BROWSE)
		{
			load (node_id);
		}
	}
	else if (evt.is_button_ex ())
	{
		const Button   but = evt.get_button_ex ();
		switch (but)
		{
		case Button_S:
			ret_val = EvtProp_CATCH;
			switch (_action)
			{
			case Action_LOAD:
				if (load (node_id))
				{
					update_display ();
				}  // And we stay on the list page
				break;
			case Action_BROWSE:
				_page_switcher.switch_to (pg::PageType_MENU_PRESETS, 0);
				break;
			case Action_STORE:
				store_1 (node_id);
				break;
			case Action_SWAP:
				swap (node_id);
				update_display ();
				break;
			case Action_RENAME:
				rename_1 (node_id);
				break;
			case Action_MORPH:
				/*** To do ***/
				break;
			case Action_DELETE:
				del (node_id);
				update_display ();
				break;
			default:
				assert (false);
				break;
			}
			break;
		case Button_E:
			_page_switcher.switch_to (pg::PageType_MENU_PRESETS, 0);
			ret_val = EvtProp_CATCH;
			break;
		default:
			// Nothing
			break;
		}
	}

	return ret_val;
}



void	ListPresets::do_activate_preset (int index)
{
	_page_switcher.switch_to (pg::PageType_EDIT_PROG, 0);
}



void	ListPresets::do_remove_slot (int slot_id)
{
	if (slot_id == _loc_edit._slot_id)
	{
		_page_switcher.switch_to (pg::PageType_EDIT_PROG, 0);
	}
}



void	ListPresets::do_set_plugin (int slot_id, const PluginInitData &pi_data)
{
	if (slot_id == _loc_edit._slot_id)
	{
		_page_switcher.switch_to (pg::PageType_EDIT_PROG, 0);
	}
}



void	ListPresets::do_remove_plugin (int slot_id)
{
	if (slot_id == _loc_edit._slot_id)
	{
		_page_switcher.switch_to (pg::PageType_EDIT_PROG, 0);
	}
}



void	ListPresets::do_add_settings (std::string model, int index, std::string name, const doc::PluginSettings &s_main, const doc::PluginSettings &s_mix)
{
	const doc::Preset &  preset = _view_ptr->use_preset_cur ();
	const doc::Slot &    slot   = preset.use_slot (_loc_edit._slot_id);
	if (model == slot._pi_model)
	{
		update_display ();
	}
}



void	ListPresets::do_remove_settings (std::string model, int index)
{
	const doc::Preset &  preset = _view_ptr->use_preset_cur ();
	const doc::Slot &    slot   = preset.use_slot (_loc_edit._slot_id);
	if (model == slot._pi_model)
	{
		update_display ();
	}
}



void	ListPresets::do_clear_all_settings ()
{
	update_display ();
}



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/



void	ListPresets::update_display ()
{
	assert (_fnt_ptr != 0);

	_preset_list.clear ();
	_menu_sptr->clear_all_nodes ();
	PageMgrInterface::NavLocList  nav_list;

	const doc::Setup &   setup   = _view_ptr->use_setup ();
	const doc::Preset &  preset  = _view_ptr->use_preset_cur ();
	const doc::Slot &    slot    = preset.use_slot (_loc_edit._slot_id);
	const auto           it_mps  =
		setup._map_plugin_settings.find (slot._pi_model);
	bool                 last_empty_flag = false;
	static const char *  empty_0 = "<Empty>";
	if (it_mps != setup._map_plugin_settings.end ())
	{
		const doc::CatalogPluginSettings &  cat = it_mps->second;
		const int      nbr_settings = int (cat._cell_arr.size ());
		_preset_list.resize (nbr_settings);
		for (int set_idx = 0; set_idx < nbr_settings; ++set_idx)
		{
			std::string    name;
			bool           same_flag = false;

			if (cat.is_preset_existing (set_idx))
			{
				const doc::CatalogPluginSettings::Cell & cell =
					*(cat._cell_arr [set_idx]);

				name = cell._name;
				same_flag = (
					   cell._main.is_similar (slot.use_settings (PiType_MAIN))
					&& cell._mixer.is_similar (slot.use_settings (PiType_MIX))
				);
			}

			else
			{
				name            = empty_0;
				last_empty_flag = (set_idx == nbr_settings - 1);
			}

			add_entry (set_idx, name, nav_list, same_flag);
		}
	}

	if (! last_empty_flag)
	{
		const int      nbr_settings = int (_preset_list.size ());
		_preset_list.resize (nbr_settings + 1);
		add_entry (nbr_settings, empty_0, nav_list, false);
	}

	_page_ptr->set_nav_layout (nav_list);

	_menu_sptr->invalidate_all ();
}



void	ListPresets::add_entry (int set_idx, std::string name, PageMgrInterface::NavLocList &nav_list, bool same_flag)
{
	const int      scr_w   = _page_size [0];
	const int      h_m     = _fnt_ptr->get_char_h ();
	const int      node_id = set_idx;
	TxtSPtr        preset_sptr (new NText (node_id));
	char           txt_0 [255+1];
	fstb::snprintf4all (
		txt_0, sizeof (txt_0),
		"%02d %s", set_idx, name.c_str ()
	);
	preset_sptr->set_font (*_fnt_ptr);
	preset_sptr->set_coord (Vec2d (0, set_idx * h_m));
	preset_sptr->set_frame (Vec2d (scr_w, 0), Vec2d ());
	preset_sptr->set_text (txt_0);
	preset_sptr->set_bold (same_flag, false);
	_preset_list [set_idx] = preset_sptr;
	_menu_sptr->push_back (preset_sptr);
	PageMgrInterface::add_nav (nav_list, node_id);
}



bool	ListPresets::load (int set_idx)
{
	bool                 set_flag = false;

	const doc::Setup &   setup    = _view_ptr->use_setup ();
	const doc::Preset &  preset   = _view_ptr->use_preset_cur ();
	const doc::Slot &    slot     = preset.use_slot (_loc_edit._slot_id);
	const auto           it_mps   =
		setup._map_plugin_settings.find (slot._pi_model);
	if (it_mps != setup._map_plugin_settings.end ())
	{
		const doc::CatalogPluginSettings &  cat = it_mps->second;
		if (cat.is_preset_existing (set_idx))
		{
			const doc::CatalogPluginSettings::Cell &   cell =
				*(cat._cell_arr [set_idx]);
			_model_ptr->load_plugin_settings (
				_loc_edit._slot_id,
				cell._main,
				cell._mixer
			);
			set_flag = true;
		}
	}

	return set_flag;
}



void	ListPresets::store_1 (int set_idx)
{
	// Gets the name of the previous preset
	std::string          name;
	const doc::Setup &   setup    = _view_ptr->use_setup ();
	const doc::Preset &  preset   = _view_ptr->use_preset_cur ();
	const doc::Slot &    slot     = preset.use_slot (_loc_edit._slot_id);
	const auto           it_mps   =
		setup._map_plugin_settings.find (slot._pi_model);
	if (it_mps != setup._map_plugin_settings.end ())
	{
		const doc::CatalogPluginSettings &  cat = it_mps->second;
		if (cat.is_preset_existing (set_idx))
		{
			const doc::CatalogPluginSettings::Cell &   cell =
				*(cat._cell_arr [set_idx]);
			name = cell._name;
		}
	}

	// Prompts user for the name
	char           txt_0 [255+1];
	fstb::snprintf4all (
		txt_0,
		sizeof (txt_0),
		"Save to %02d %s:",
		set_idx,
		name.c_str ()
	);
	_name_param._title = txt_0;
	_name_param._text  = name;
	_state             = State_EDIT_NAME;
	_state_set_idx     = set_idx;
	_page_switcher.call_page (PageType_EDIT_TEXT, &_name_param, set_idx);
}



void	ListPresets::store_2 ()
{
	assert (_state_set_idx >= 0);

	const doc::Preset &  preset = _view_ptr->use_preset_cur ();
	const doc::Slot &    slot   = preset.use_slot (_loc_edit._slot_id);
	_model_ptr->add_settings (
		slot._pi_model,
		_state_set_idx,
		_name_param._text,
		slot.use_settings (PiType_MAIN),
		slot.use_settings (PiType_MIX)
	);
	_state_set_idx = -1;

	const int      ret_val = _model_ptr->save_to_disk ();
	if (ret_val == 0)
	{
		_page_switcher.switch_to (pg::PageType_MENU_SLOT, 0);
	}
	else
	{
		/*** To do ***/
		assert (false);
	}
}



void	ListPresets::swap (int set_idx)
{
	const doc::Setup &   setup    = _view_ptr->use_setup ();
	const doc::Preset &  preset   = _view_ptr->use_preset_cur ();
	const doc::Slot &    slot     = preset.use_slot (_loc_edit._slot_id);
	doc::CatalogPluginSettings::Cell cur;
	cur._name  = "Untitled";
	cur._main  = slot.use_settings (PiType_MAIN);
	cur._mixer = slot.use_settings (PiType_MIX );

	doc::CatalogPluginSettings::Cell tmp (cur);
	const auto     it_mps    =
		setup._map_plugin_settings.find (slot._pi_model);
	bool           full_flag = false;
	if (it_mps != setup._map_plugin_settings.end ())
	{
		const doc::CatalogPluginSettings &  cat = it_mps->second;
		if (cat.is_preset_existing (set_idx))
		{
			full_flag = true;
			tmp       = *(cat._cell_arr [set_idx]);
			cur._name = tmp._name;
		}
	}

	if (full_flag)
	{
		_model_ptr->load_plugin_settings (
			_loc_edit._slot_id,
			tmp._main,
			tmp._mixer
		);
	}
	_model_ptr->add_settings (
		slot._pi_model,
		set_idx,
		cur._name,
		cur._main,
		cur._mixer
	);

	const int      ret_val = _model_ptr->save_to_disk ();
	if (ret_val != 0)
	{
		/*** To do ***/
		assert (false);
	}
}



void	ListPresets::rename_1 (int set_idx)
{
	// Gets the name of the preset
	const doc::Setup &   setup    = _view_ptr->use_setup ();
	const doc::Preset &  preset   = _view_ptr->use_preset_cur ();
	const doc::Slot &    slot     = preset.use_slot (_loc_edit._slot_id);
	const auto           it_mps   =
		setup._map_plugin_settings.find (slot._pi_model);
	if (it_mps != setup._map_plugin_settings.end ())
	{
		const doc::CatalogPluginSettings &  cat = it_mps->second;
		if (cat.is_preset_existing (set_idx))
		{
			const doc::CatalogPluginSettings::Cell &   cell =
				*(cat._cell_arr [set_idx]);

			// Prompts user for the new name
			char           txt_0 [255+1];
			fstb::snprintf4all (
				txt_0,
				sizeof (txt_0),
				"Rename %02d %s:",
				set_idx,
				cell._name.c_str ()
			);
			_name_param._title = txt_0;
			_name_param._text  = cell._name;
			_state             = State_EDIT_NAME;
			_state_set_idx     = set_idx;
			_page_switcher.call_page (PageType_EDIT_TEXT, &_name_param, set_idx);
		}
	}
}



void	ListPresets::rename_2 ()
{
	assert (_state_set_idx >= 0);

	bool           done_flag    = false;
	const doc::Setup &   setup  = _view_ptr->use_setup ();
	const doc::Preset &  preset = _view_ptr->use_preset_cur ();
	const doc::Slot &    slot   = preset.use_slot (_loc_edit._slot_id);
	const auto           it_mps =
		setup._map_plugin_settings.find (slot._pi_model);
	if (it_mps != setup._map_plugin_settings.end ())
	{
		const doc::CatalogPluginSettings &  cat = it_mps->second;
		if (cat.is_preset_existing (_state_set_idx))
		{
			const doc::CatalogPluginSettings::Cell &   cell =
				*(cat._cell_arr [_state_set_idx]);

			_model_ptr->add_settings (
				slot._pi_model,
				_state_set_idx,
				_name_param._text,
				cell._main,
				cell._mixer
			);
			done_flag = true;
		}
	}

	_state_set_idx = -1;

	if (done_flag)
	{
		const int      ret_val = _model_ptr->save_to_disk ();
		if (ret_val != 0)
		{
			/*** To do ***/
			assert (false);
		}
	}
}



void	ListPresets::del (int set_idx)
{
	const doc::Preset &  preset   = _view_ptr->use_preset_cur ();
	const doc::Slot &    slot     = preset.use_slot (_loc_edit._slot_id);
	_model_ptr->remove_settings (slot._pi_model, set_idx);

	const int      ret_val = _model_ptr->save_to_disk ();
	if (ret_val != 0)
	{
		/*** To do ***/
		assert (false);
	}
}



}  // namespace pg
}  // namespace uitk
}  // namespace mfx



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
