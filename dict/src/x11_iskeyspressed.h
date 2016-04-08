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

#ifndef _X11_ISKEYSPRESSED_HPP_
#define _X11_ISKEYSPRESSED_HPP_

#include <gdk/gdkx.h>
#include <X11/keysym.h>
#include <gtk/gtk.h>
#include <memory>
#include "iskeyspressed.h"

typedef enum { SYM, CODE, BUTTON } KeyType_t;
typedef enum { PRESS, RELEASE } EventType_t;

struct Keys_t {

  KeyType_t type;

  EventType_t event_type;

  union {
    KeySym sym;
    KeyCode code;
    unsigned int button;
  } key;

  unsigned int modifier;

  Keys_t(KeyType_t t=SYM, EventType_t et=PRESS, KeySym s=0, unsigned int m=0) :
    type(t), event_type(et), modifier(m) 
  {
    key.sym=s;
  }
};

class x11_hotkeys : public hotkeys {
public:
  x11_hotkeys(GtkWindow *win_);
	~x11_hotkeys();
	const std::list<std::string>& possible_combs();
  void set_comb(const std::string& comb);
  bool is_pressed();
private:
  std::string comb;
  GtkWindow *win;
  bool pressed;
  Keys_t grab_key;
  std::list<std::string> possb_combs;
  std::unique_ptr<hotkeys> def_hot_keys;/*I don't want to duplicate work, 
																				so hold here pointer to default 
																				object of this class*/
  Display *display;
  static unsigned int numlock_mask, scrolllock_mask, capslock_mask;

  static GdkFilterReturn key_filter(GdkXEvent *gdk_xevent, 
																		GdkEvent *event, x11_hotkeys *th);
  void ungrabkeys(void);
  static void get_offending_modifiers(Display * dpy);
  static void my_grab_key(Display * dpy, KeyCode keycode, 
													unsigned int modifier, Window win);
  static void my_grab_button(Display * dpy, unsigned int button, 
														 unsigned int modifier, Window win);
  static bool grab_keys(Display *dpy, Keys_t keys[], int nb_keys);
};

#endif//!_X11_ISKEYSPRESSED_HPP_
