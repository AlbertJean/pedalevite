/*****************************************************************************

        Page.h
        Author: Laurent de Soras, 2016

--- Legal stuff ---

This program is free software. It comes without any warranty, to
the extent permitted by applicable law. You can redistribute it
and/or modify it under the terms of the Do What The Fuck You Want
To Public License, Version 2, as published by Sam Hocevar. See
http://sam.zoy.org/wtfpl/COPYING for more details.

*Tab=3***********************************************************************/



#pragma once
#if ! defined (mfx_uitk_Page_HEADER_INCLUDED)
#define mfx_uitk_Page_HEADER_INCLUDED

#if defined (_MSC_VER)
	#pragma warning (4 : 4250)
#endif



/*\\\ INCLUDE FILES \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

#include "mfx/ui/UserInputInterface.h"
#include "mfx/uitk/PageInterface.h"
#include "mfx/uitk/PageMgrInterface.h"
#include "mfx/uitk/NWindow.h"
#include "mfx/uitk/Rect.h"

#include <list>
#include <set>



namespace mfx
{

class Model;
class View;

namespace ui
{
	class DisplayInterface;
	class Font;
}

namespace uitk
{



class Page
:	public PageMgrInterface
{

/*\\\ PUBLIC \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

public:

	static const int  _disp_node_id = 666999;

	explicit       Page (Model &model, View &view, ui::DisplayInterface &display, ui::UserInputInterface::MsgQueue &queue_input_to_gui, ui::UserInputInterface &input_device, const ui::Font &fnt_t, const ui::Font &fnt_s, const ui::Font &fnt_m, const ui::Font &fnt_l);
	virtual        ~Page ();

	void           set_page_content (PageInterface &content, void *usr_ptr);
	const PageInterface *
	               get_page_content () const;
	int            get_cursor_node () const;
	void           clear (bool evt_flag = true);

	void           process_messages ();



/*\\\ PROTECTED \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

protected:

	// mfx::uitk::PageMgrInterface
	virtual void   do_set_nav_layout (const NavLocList &nav_list);
	virtual void   do_jump_to (int node_id);
	virtual void   do_set_timer (int node_id, bool enable_flag);
	virtual void   do_reset_display ();

	// mfx::uitk::ParentInterface via mfx::uitk::PageMgrInterface
	virtual Vec2d  do_get_coord_abs () const;
	virtual void   do_invalidate (const Rect &zone);

	// mfx::uitk::ContainerInterface via mfx::uitk::PageMgrInterface
	virtual void   do_push_back (NodeSPtr node_sptr);
	virtual void   do_set_node (int pos, NodeSPtr node_sptr);
	virtual void   do_insert (int pos, NodeSPtr node_sptr);
	virtual void   do_erase (int pos);
	virtual int    do_get_nbr_nodes () const;
	virtual NodeSPtr
	               do_use_node (int pos);



/*\\\ PRIVATE \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	class SetPageContent
	{
	public:
		PageInterface* _content_ptr;
		void *         _usr_ptr;
	};

	void           process_input ();
	void           handle_redraw ();
	void           send_button (int node_id, Button but);
	void           send_event (NodeEvt &evt);
	int            find_nav_node (int node_id) const;
	bool           check_curs (bool send_msg_flag);
	bool           process_nav (Button but);

	Model &        _model;
	View &         _view;
	ui::DisplayInterface &
	               _display;
	ui::UserInputInterface::MsgQueue &
	               _queue_input_to_gui;
	ui::UserInputInterface &            // To return message cells
	               _input_device;
	const PageInterface::FontSet
	               _fnt_set;
	const Vec2d    _disp_size;
	NWindow        _screen;
	Rect           _zone_inval;
	PageInterface* _content_ptr;        // 0 = not set

	NavLocList     _nav_list;
	int            _curs_pos;           // -1 = not shown
	int            _curs_id;            // -1 = not shown
	std::set <int> _timer_set;

	Button         _but_hold;
	std::chrono::microseconds           // Microseconds, beginning of the hold position
	               _but_hold_date;
	int            _but_hold_count;     // Number of elapsed repetitions

	std::list <SetPageContent>
	               _rec_spc;
	bool           _recursive_flag;

	std::chrono::microseconds
	               _first_refresh_date;





/*\\\ FORBIDDEN MEMBER FUNCTIONS \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/

private:

	               Page ()                               = delete;
	               Page (const Page &other)              = delete;
	Page &         operator = (const Page &other)        = delete;
	bool           operator == (const Page &other) const = delete;
	bool           operator != (const Page &other) const = delete;

}; // class Page



}  // namespace uitk
}  // namespace mfx



//#include "mfx/uitk/Page.hpp"



#endif   // mfx_uitk_Page_HEADER_INCLUDED



/*\\\ EOF \\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\\*/
