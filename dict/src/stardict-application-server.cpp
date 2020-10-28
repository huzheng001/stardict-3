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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <bonobo/bonobo-generic-factory.h>
#include <bonobo/bonobo-main.h>
#include <bonobo/bonobo-context.h>
#include <bonobo-activation/bonobo-activation-register.h>


#include "GNOME_Stardict.h"
#include "stardict.h"

#include "stardict-application-server.h"


static void stardict_application_server_class_init (StardictApplicationServerClass *klass);
static void stardict_application_server_init (StardictApplicationServer *a);
static void stardict_application_server_object_finalize (GObject *object);
static GObjectClass *stardict_application_server_parent_class;

static BonoboObject *
stardict_application_server_factory (BonoboGenericFactory *this_factory,
			   const char *iid,
			   gpointer user_data)
{
        StardictApplicationServer *a;
        
        a  = (StardictApplicationServer *)g_object_new (STARDICT_APPLICATION_SERVER_TYPE, NULL);

        return BONOBO_OBJECT (a);
}

BonoboObject *
stardict_application_server_new (GdkScreen *screen_null)
{
	BonoboGenericFactory *factory;
	GdkDisplay *display;
	const char           *display_name;
	char                 *registration_id;

	/* We must ensure an instance of stardict per screen
	 * as stardict has no multiscreen support 
	 */
	display = gdk_display_get_default();
	display_name = gdk_display_get_name(display);
	registration_id = bonobo_activation_make_registration_id (
					"OAFIID:GNOME_Stardict_Factory",
					display_name);

	factory = bonobo_generic_factory_new (registration_id,
					      stardict_application_server_factory,
					      NULL);

	g_free (registration_id);

	return BONOBO_OBJECT (factory);
}

static void
impl_stardict_application_server_queryWord (PortableServer_Servant _servant,
				   const CORBA_char *str,
				   CORBA_Environment * ev)
{
	gpAppFrame->Query((const gchar *)str);
}

static void
impl_stardict_application_server_grabFocus (PortableServer_Servant _servant,
					 CORBA_Environment * ev)
{
	gtk_window_present (GTK_WINDOW (gpAppFrame->window));
}

static void
impl_stardict_application_server_hide (PortableServer_Servant _servant,
					 CORBA_Environment * ev)
{
	gtk_widget_hide(gpAppFrame->window);
}

static void
impl_stardict_application_server_quit (PortableServer_Servant _servant,
                                CORBA_Environment * ev)
{
	gpAppFrame->Quit();
}

static void
stardict_application_server_class_init (StardictApplicationServerClass *klass)
{
        GObjectClass *object_class = (GObjectClass *) klass;
        POA_GNOME_Stardict_Application__epv *epv = &klass->epv;

        stardict_application_server_parent_class = (GObjectClass *)g_type_class_peek_parent (klass);

        object_class->finalize = stardict_application_server_object_finalize;

        /* connect implementation callbacks */
	epv->queryWord              = impl_stardict_application_server_queryWord;
	epv->grabFocus              = impl_stardict_application_server_grabFocus;
	epv->hide              = impl_stardict_application_server_hide;	
	epv->quit              = impl_stardict_application_server_quit;
}

static void
stardict_application_server_init (StardictApplicationServer *c) 
{
}

static void
stardict_application_server_object_finalize (GObject *object)
{
        StardictApplicationServer *a = STARDICT_APPLICATION_SERVER (object);

        stardict_application_server_parent_class->finalize (G_OBJECT (a));
}

BONOBO_TYPE_FUNC_FULL (
        StardictApplicationServer,                    
        GNOME_Stardict_Application, 
        BONOBO_TYPE_OBJECT,           
        stardict_application_server);
