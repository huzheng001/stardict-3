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

#ifndef _HOTKEYEDITOR_H
#define _HOTKEYEDITOR_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


#include <gtk/gtk.h>

#define STARDICT_TYPE_HOTKEY_EDITOR            (stardict_hotkey_editor_get_type ())
#define STARDICT_HOTKEY_EDITOR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), STARDICT_TYPE_HOTKEY_EDITOR, StardictHotkeyEditor))
#define STARDICT_HOTKEY_EDITOR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), STARDICT_TYPE_HOTKEY_EDITOR, StardictHotkeyEditorClass))
#define STARDICT_IS_HOTKEY_EDITOR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE (obj, STARDICT_TYPE_HOTKEY_EDITOR))
#define STARDICT_IS_HOTKEY_EDITOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), STARDICT_TYPE_HOTKEY_EDITOR))
#define STARDICT_HOTKEY_EDITOR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), STARDICT_TYPE_HOTKEY_EDITOR, StardictHotkeyEditorClass))

typedef struct _StardictHotkeyEditorClass StardictHotkeyEditorClass;
typedef struct _StardictHotkeyEditor StardictHotkeyEditor;

struct _StardictHotkeyEditor {
	GtkEntry parent_instance;
	guint current_mods;
};

struct _StardictHotkeyEditorClass {
	GtkEntryClass parent_class;
	void (*hotkey_changed) (StardictHotkeyEditor *hkeditor,
	  guint key, guint modifiers);
};

GType stardict_hotkey_editor_get_type(void) G_GNUC_CONST;
extern StardictHotkeyEditor *stardict_hotkey_editor_new();

#ifdef __cplusplus
}
#endif                          /* __cplusplus */

#endif
