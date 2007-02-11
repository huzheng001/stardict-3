#include <string.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "libtabfile.h"
#include "libbabylonfile.h"
#include "libstardict2txt.h"
#include "libstardictverify.h"

static GtkWidget *main_window;
static GtkTextBuffer *compile_page_text_view_buffer;
static GtkWidget *compile_page_option_menu;
static GtkTextBuffer *decompile_page_text_view_buffer;
static GtkTextBuffer *edit_page_text_view_buffer;

void on_browse_button_clicked(GtkButton *button, gpointer data)
{
	GtkEntry *entry = GTK_ENTRY(data);
	GtkWidget *dialog;
	dialog = gtk_file_chooser_dialog_new ("Open file...",
			GTK_WINDOW(main_window),
			GTK_FILE_CHOOSER_ACTION_OPEN,
			GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
			GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
			NULL);
	gtk_file_chooser_set_filename(GTK_FILE_CHOOSER (dialog), gtk_entry_get_text(entry));
	if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
		gchar *filename;
		filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		gtk_entry_set_text(entry, filename);
		g_free (filename);
	}
	gtk_widget_destroy (dialog);
}

void compile_page_print_info(const char *info)
{
	gtk_text_buffer_insert_at_cursor(compile_page_text_view_buffer, info, -1);
}

void on_compile_page_build_button_clicked(GtkButton *button, gpointer data)
{
	GtkEntry *entry = GTK_ENTRY(data);	
	gtk_text_buffer_set_text(compile_page_text_view_buffer, "Building...\n", -1);
	gint key = gtk_option_menu_get_history(GTK_OPTION_MENU(compile_page_option_menu));
	if (key == 0)
		convert_tabfile(gtk_entry_get_text(entry), compile_page_print_info);
	else
		convert_babylonfile(gtk_entry_get_text(entry), compile_page_print_info);
	gtk_text_buffer_insert_at_cursor(compile_page_text_view_buffer, "Done!\n", -1);
}

void create_compile_page(GtkWidget *notebook)
{
	GtkWidget *vbox = gtk_vbox_new(false, 6);
	GtkWidget *label = gtk_label_new("Compile");
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);
	GtkWidget *hbox = gtk_hbox_new(false, 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, false, false, 0);
	label = gtk_label_new("File name:");
	gtk_box_pack_start(GTK_BOX(hbox), label, false, false, 0);
	GtkWidget *entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), entry, false, false, 0);
	GtkWidget *button = gtk_button_new_with_mnemonic("Bro_wse...");
	gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(on_browse_button_clicked), entry);
	label = gtk_label_new("This file should be encoded in UTF-8!");
	gtk_box_pack_start(GTK_BOX(vbox), label, false, false, 0);
	GtkWidget *text_view = gtk_text_view_new();
	gtk_widget_set_size_request(text_view, -1, 150);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD_CHAR);
	compile_page_text_view_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	gtk_text_buffer_set_text(compile_page_text_view_buffer,
		"Here is a example dict.tab file:\n"
		"============\n"
		"a\t1\\n2\\n3\n"
		"b\t4\\\\5\\n6\n"
		"c\t789\n"
		"============\n"
		"It means: write the search word first, then a Tab character, and the definition. If the definition contains new line, just write \\n, if contains \\ character, just write \\\\.\n"
		"\n\n"
		"Another format that StarDict recommends is babylon source file format, it is just like this:\n"
		"======\n"
		"apple|apples\n"
		"the meaning of apple\n"
		"\n"
		"2dimensional|2dimensionale|2dimensionaler|2dimensionales|2dimensionalem|2dimensionalen\n"
		"two dimensional's meaning<br>the sencond line.\n"
		"\n"
		"======\n"
		, -1);
	GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);
	gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, true, true, 0);
	hbox = gtk_hbox_new(false, 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, false, false, 0);
	compile_page_option_menu = gtk_option_menu_new();
	GtkWidget *menu = gtk_menu_new();
	GtkWidget *menuitem;
	menuitem=gtk_menu_item_new_with_mnemonic("Tab file");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	menuitem=gtk_menu_item_new_with_mnemonic("Babylon file");
	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	gtk_option_menu_set_menu(GTK_OPTION_MENU(compile_page_option_menu), menu);
	gtk_box_pack_start(GTK_BOX(hbox), compile_page_option_menu, true, false, 0);
	button = gtk_button_new_with_mnemonic("_Build");
	gtk_box_pack_start(GTK_BOX(hbox), button, true, false, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(on_compile_page_build_button_clicked), entry);
}

void decompile_page_print_info(const char *info)
{
	gtk_text_buffer_insert_at_cursor(decompile_page_text_view_buffer, info, -1);
}

void on_decompile_page_build_button_clicked(GtkButton *button, gpointer data)
{
	GtkEntry *entry = GTK_ENTRY(data);	
	gtk_text_buffer_set_text(decompile_page_text_view_buffer, "Building...\n", -1);
	convert_stardict2txt(gtk_entry_get_text(entry), decompile_page_print_info);
	gtk_text_buffer_insert_at_cursor(decompile_page_text_view_buffer, "Done!\n", -1);
}

void on_decompile_page_verify_button_clicked(GtkButton *button, gpointer data)
{
	GtkEntry *entry = GTK_ENTRY(data);	
	gtk_text_buffer_set_text(decompile_page_text_view_buffer, "Verifing dictionary files...\n", -1);
	stardict_verify(gtk_entry_get_text(entry), decompile_page_print_info);
	gtk_text_buffer_insert_at_cursor(decompile_page_text_view_buffer, "Done!\n", -1);
}

void create_decompile_page(GtkWidget *notebook)
{
	GtkWidget *vbox = gtk_vbox_new(false, 6);
	GtkWidget *label = gtk_label_new("DeCompile");
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);
	GtkWidget *hbox = gtk_hbox_new(false, 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, false, false, 0);
	label = gtk_label_new("File name:");
	gtk_box_pack_start(GTK_BOX(hbox), label, false, false, 0);
	GtkWidget *entry = gtk_entry_new();
	gtk_box_pack_start(GTK_BOX(hbox), entry, false, false, 0);
	GtkWidget *button = gtk_button_new_with_mnemonic("Bro_wse...");
	gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(on_browse_button_clicked), entry);
	label = gtk_label_new("Please choose the somedict.ifo file.");
	gtk_box_pack_start(GTK_BOX(vbox), label, false, false, 0);
	GtkWidget *text_view = gtk_text_view_new();
	gtk_widget_set_size_request(text_view, -1, 150);
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD_CHAR);
	decompile_page_text_view_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);
	gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, true, true, 0);
	hbox = gtk_hbox_new(false, 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, false, false, 0);
	button = gtk_button_new_with_mnemonic("_Decompile");
	gtk_box_pack_start(GTK_BOX(hbox), button, true, false, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(on_decompile_page_build_button_clicked), entry);
	button = gtk_button_new_with_mnemonic("_Verify");
	gtk_box_pack_start(GTK_BOX(hbox), button, true, false, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(on_decompile_page_verify_button_clicked), entry);
}

void on_edit_page_open_button_clicked(GtkButton *button, gpointer data)
{
        GtkWidget *dialog;
        dialog = gtk_file_chooser_dialog_new ("Open file...",
                        GTK_WINDOW(main_window),
                        GTK_FILE_CHOOSER_ACTION_OPEN,
                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
                        NULL);
        if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
                gchar *filename;
                filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		gchar *buffer;
		if (g_file_get_contents(filename, &buffer, NULL, NULL)) {
			gtk_text_buffer_set_text(edit_page_text_view_buffer, buffer, -1);
			g_free(buffer);
		}
                g_free (filename);
        }
        gtk_widget_destroy (dialog);

}

void on_edit_page_saveas_button_clicked(GtkButton *button, gpointer data)
{
	GtkWidget *dialog;
        dialog = gtk_file_chooser_dialog_new ("Save file...",
                        GTK_WINDOW(main_window),
                        GTK_FILE_CHOOSER_ACTION_SAVE,
                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                        NULL);
#ifndef _WIN32
		gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
#endif
        if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
                gchar *filename;
                filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
		GtkTextIter start, end;
		gtk_text_buffer_get_bounds(edit_page_text_view_buffer, &start, &end);
		gchar *buffer = gtk_text_buffer_get_text(edit_page_text_view_buffer, &start, &end, FALSE);
		FILE *file = g_fopen(filename, "wb");
		fwrite(buffer, 1, strlen(buffer), file);
		fclose(file);
		g_free(buffer);
                g_free (filename);
        }
        gtk_widget_destroy (dialog);
}

void create_edit_page(GtkWidget *notebook)
{
	GtkWidget *vbox = gtk_vbox_new(false, 6);
	GtkWidget *label = gtk_label_new("Edit");
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);
	GtkWidget *hbox = gtk_hbox_new(false, 6);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, false, false, 0);
	GtkWidget *button = gtk_button_new_with_mnemonic("_Open");
	gtk_box_pack_start(GTK_BOX(hbox), button, true, false, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(on_edit_page_open_button_clicked), NULL);
	button = gtk_button_new_with_mnemonic("_Save as");
	gtk_box_pack_start(GTK_BOX(hbox), button, true, false, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(on_edit_page_saveas_button_clicked), NULL);
	GtkWidget *text_view = gtk_text_view_new();
        gtk_widget_set_size_request(text_view, -1, 150);
        gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD_CHAR);
        edit_page_text_view_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	gtk_text_buffer_set_text(edit_page_text_view_buffer,
                "This is a simple UTF-8 text file editor.\n"
                , -1);
        GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);
        gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, true, true, 0);
}

gboolean on_delete_event(GtkWidget * window, GdkEvent *event , gpointer data)
{
	gtk_main_quit();
	return FALSE;
}

void create_window()
{
	main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position (GTK_WINDOW (main_window), GTK_WIN_POS_CENTER);
	gtk_window_set_title(GTK_WINDOW (main_window), "StarDict-Editor");
	gtk_container_set_border_width (GTK_CONTAINER (main_window), 5);
	g_signal_connect (G_OBJECT (main_window), "delete_event", G_CALLBACK (on_delete_event), NULL);
	GtkWidget *notebook = gtk_notebook_new();
	gtk_container_add(GTK_CONTAINER(main_window), notebook);
	create_compile_page(notebook);
	create_decompile_page(notebook);
	create_edit_page(notebook);
	gtk_widget_show_all(main_window);
}

int main(int argc,char **argv)
{
	gtk_set_locale();
	gtk_init(&argc, &argv);
	create_window();
	gtk_main();
	return 0;
}
