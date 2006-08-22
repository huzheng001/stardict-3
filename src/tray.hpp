#ifndef _TRAY_HPP_
#define _TRAY_HPP_

#include <gtk/gtk.h>

/**
 * Base class for platform specific implementation of tray
 */
class TrayBase {
public:
	/**
         * A constructor
         * @param mainwin - window widget which we should control
         */

	TrayBase(GtkWidget *mainwin) : mainwin_(mainwin) {}

	virtual ~TrayBase() {}

	//! Minimize controlled widget to tray
        virtual void minimize_to_tray()
        {
                gtk_widget_hide(mainwin_);
        }

protected:
	GtkWidget *mainwin_;//!< Window widget which we should control
};

#endif//!_TRAY_HPP_
