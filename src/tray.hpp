#ifndef _TRAY_HPP_
#define _TRAY_HPP_

#include <gtk/gtk.h>
#include "sigc++/sigc++.h"

/**
 * Base class for platform specific implementation of tray
 */
class TrayBase {
public:
	sigc::signal<void> on_quit_;//!< On quit menu choise
	sigc::signal<void, bool> on_change_scan_;//!< Emitted when user turn on/off scan mode
	sigc::signal<void> on_maximize_;
	sigc::signal<void> on_middle_btn_click_;

	/**
         * A constructor
         * @param mainwin - window widget which we should control
         */

	TrayBase(GtkWidget *mainwin, bool is_scan_on) :
		mainwin_(mainwin),
		is_scan_on_(is_scan_on),
		hide_state_(false) 
		{
		}

	virtual ~TrayBase() {}

	//! Minimize controlled widget to tray
        virtual void minimize_to_tray()
        {
                gtk_widget_hide(mainwin_);
        }
	 //! Maximize controlled widget from tray
	virtual void maximize_from_tray()
	{
		gtk_window_present(GTK_WINDOW(mainwin_));
	}
	/**
         * Change state of object
         * @param is_on - turn on or turn off scan mode
         */
        virtual void set_scan_mode(bool is_on);
	/**
         * Noramlly it is called when you do not want show current state
         */
        void hide_state();

protected:
	GtkWidget *mainwin_;//!< Window widget which we should control

	virtual void scan_on() = 0;
        virtual void scan_off() = 0;
        virtual void show_normal_icon() = 0;

	bool is_scan_on() const { return is_scan_on_; }
	bool is_hide_state() const { return hide_state_; }
private:
	bool is_scan_on_;
        bool hide_state_;
};

#endif//!_TRAY_HPP_
