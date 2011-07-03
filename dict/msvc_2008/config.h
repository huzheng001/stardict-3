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

#ifndef CONFIG_H
#define CONFIG_H

#if defined(_MSC_VER)
#  include <glib.h>
#  include <glib/gstdio.h>

#  define S_IRWXU 0
#  define S_IRWXG 0
#  define S_IRWXO 0
#  define __attribute__(x) /**/
#  define g_fopen stardict_g_fopen

// defined in glib_wrapper.c
G_BEGIN_DECLS
FILE *
stardict_g_fopen (const gchar *filename,
						const gchar *mode);
G_END_DECLS

#endif

#include <../../lib/config-custom.h>

#endif/*!CONFIG_H*/
