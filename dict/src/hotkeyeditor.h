#ifndef _HOTKEYEDITOR_H
#define _HOTKEYEDITOR_H

#ifdef __cplusplus
extern "C"
{
#endif                          /* __cplusplus */


#include <gtk/gtkentry.h>

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
