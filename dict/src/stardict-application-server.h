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


#ifndef __STARDICT_APPLICATION_SERVER_H
#define __STARDICT_APPLICATION_SERVER_H

#include "GNOME_Stardict.h"
#include <bonobo/bonobo-object.h>

#if GTK_MAJOR_VERSION >= 3
#include <gtk/gtkx.h>
#else
#include <gdk/gdk.h>
#endif

G_BEGIN_DECLS

#define STARDICT_APPLICATION_SERVER_TYPE         (stardict_application_server_get_type ())
#define STARDICT_APPLICATION_SERVER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), STARDICT_APPLICATION_SERVER_TYPE, StardictApplicationServer))
#define STARDICT_APPLICATION_SERVER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), STARDICT_APPLICATION_SERVER_TYPE, StardictApplicationServerClass))
#define STARDICT_APPLICATION_SERVER_IS_OBJECT(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), STARDICT_APPLICATION_SERVER_TYPE))
#define STARDICT_APPLICATION_SERVER_IS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), STARDICT_APPLICATION_SERVER_TYPE))
#define STARDICT_APPLICATION_SERVER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), STARDICT_APPLICATION_SERVER_TYPE, StardictApplicationServerClass))

typedef struct
{
        BonoboObject parent;
} StardictApplicationServer;

typedef struct
{
        BonoboObjectClass parent_class;

        POA_GNOME_Stardict_Application__epv epv;
} StardictApplicationServerClass;

GType          stardict_application_server_get_type (void);

BonoboObject  *stardict_application_server_new      (GdkScreen *screen);

G_END_DECLS

#endif /* __STARDICT_APPLICATION_SERVER_H */
