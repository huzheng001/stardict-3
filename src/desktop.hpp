#ifndef _DESKTOP_HPP_
#define _DESKTOP_HPP_

#include <glib.h>
#include <string>

extern void play_wav_file(const std::string& filename);
extern void show_help(const gchar *section);
extern void show_url(const char *url);
extern void play_sound_on_event(const gchar *eventname);

#endif
