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

#ifndef TEXT_VIEW_PANGO_H
#define TEXT_VIEW_PANGO_H

#include <gtk/gtk.h>

extern void
gtk_text_buffer_insert_markup (GtkTextBuffer *buffer,
                               GtkTextIter   *iter,
                               const gchar   *markup);

template<class T>
T* gtk_text_iter_min(T *first, T *second)
{
	return gtk_text_iter_compare(first, second) < 0 ? first : second;
}

template<class T>
T* gtk_text_iter_max(T *first, T *second)
{
	return gtk_text_iter_compare(first, second) > 0 ? first : second;
}

#endif//tvpango.h
