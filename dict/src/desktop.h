#ifndef _DESKTOP_HPP_
#define _DESKTOP_HPP_

#include <glib.h>
#include <string>
#include <gtk/gtk.h>

extern void play_sound_file(const std::string& filename);
extern void play_video_file(const std::string& filename);
extern void show_help(const gchar *section);
/* url in utf-8 encoding */
extern void show_url(const char *url);
extern void play_sound_on_event(const gchar *eventname);
extern bool confirm_enable_network_dicts(GtkWidget *parent_window = NULL);

#endif
