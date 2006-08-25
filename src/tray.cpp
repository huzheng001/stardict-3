/* 
 * This file part of StarDict - A international dictionary for GNOME.
 * http://stardict.sourceforge.net
 * Copyright (C) 2006 Evgeniy <dushistov@mail.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include "conf.h"
#include "stardict.h"

#include "tray.hpp"

void TrayBase::on_change_scan(bool val)
{
	conf->set_bool_at("dictionary/scan_selection", val);
}

void TrayBase::on_quit()
{
	gpAppFrame->Quit();
}

void TrayBase::on_maximize()
{       
	if (gpAppFrame->oTopWin.get_text()[0]) {
//so user can input word directly.
		gtk_widget_grab_focus(gpAppFrame->oMidWin.oTextWin.view->Widget()); 
	} else {
		//this won't change selection text.
		gpAppFrame->oTopWin.grab_focus();
	}
}

void TrayBase::on_middle_button_click()
{
	if (conf->get_bool_at("notification_area_icon/query_in_floatwin")) {
		gpAppFrame->oSelection.LastClipWord.clear();
		gtk_selection_convert(gpAppFrame->oSelection.selection_widget,
				      GDK_SELECTION_PRIMARY,
				      gpAppFrame->oSelection.UTF8_STRING_Atom, GDK_CURRENT_TIME);
	} else {
		maximize_from_tray();
		gtk_selection_convert(gpAppFrame->oMidWin.oTextWin.view->Widget(),
				      GDK_SELECTION_PRIMARY,
				      gpAppFrame->oSelection.UTF8_STRING_Atom, GDK_CURRENT_TIME);
	}
}

void TrayBase::set_scan_mode(bool is_on)
{
        if (!hide_state_ && is_on == is_scan_on_)
                return;

        hide_state_ = false;

        if (is_on)
                scan_on();
        else
                scan_off();

        is_scan_on_ = is_on;
}

void TrayBase::hide_state()
{
        if (hide_state_)
                return;
        show_normal_icon();
        hide_state_ = true;
}
