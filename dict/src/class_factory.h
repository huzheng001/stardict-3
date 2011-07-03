/*
 * Copyright 2011 kubtek <kubtek@mail.com>
 *
 * This file is part of StarDict.
 *
 * StarDict is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * StarDict is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with StarDict.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _CLASS_FACTORY_HPP_
#define _CLASS_FACTORY_HPP_

#include <gtk/gtk.h>
#include <string>

class TrayBase;
class AppSkin;

class PlatformFactory {
public:
	static void *create_class_by_name(const std::string& name, void *param=NULL);
	static TrayBase *create_tray_icon(GtkWidget *, bool, const AppSkin&);
};

#endif//!_CLASS_FACTORY_HPP_
