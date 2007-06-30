/* 
 * This file part of StarDict - A international dictionary for GNOME.
 * http://stardict.sourceforge.net
 * Copyright (C) 2006 Evgeniy <dushistov@mail.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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

#include "tray.hpp"

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
