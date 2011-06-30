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
