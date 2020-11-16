/*
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
#  include "config.h"
#endif

#include <cstring>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <cstdlib>
#include <cstdio>
#include <sstream>

#include "libtabfile.h"
#include "libbabylonfile.h"
#include "libstardict2txt.h"
#include "lib_dict_verify.h"
#include "lib_stardict_bin2text.h"
#include "lib_stardict_text2bin.h"
#include "libcommon.h"
#include "libbgl2txt.h"
#include "connector.h"
#include "generator.h"
#include "parser.h"

static GtkWidget *main_window;
static GtkTextBuffer *compile_page_text_view_buffer;
static GtkWidget *compile_page_combo_box;
static GtkWidget *decompile_page_combo_box;
static GtkTextBuffer *decompile_page_text_view_buffer;
static GtkTextBuffer *edit_page_text_view_buffer;
static GtkWidget *decompile_page_entry_chunk_size;
static GtkWidget *decompile_page_textual_stardict_hbox;
static GtkWidget *compile_page_show_xinclude_check_box;
static GtkWidget *compile_page_use_sametypesequence_check_box;
static GtkWidget *compile_page_textual_stardict_hbox;

class HookMessages
{
public:
	explicit HookMessages(GtkTextBuffer * buffer)
	:
		buffer(buffer)
	{
		handler_id = g_log_set_handler(NULL, // const gchar *log_domain,
			static_cast<GLogLevelFlags>(G_LOG_LEVEL_MASK | G_LOG_FLAG_FATAL), // GLogLevelFlags log_levels,
			LogFunc, // GLogFunc log_func,
			(gpointer)this //gpointer user_data
		);
	}
	~HookMessages(void)
	{
		g_log_remove_handler(NULL, handler_id);
	}

private:
	static void LogFunc(const gchar *log_domain,
		GLogLevelFlags log_level,
		const gchar *message,
		gpointer user_data)
	{
		HookMessages * obj = (HookMessages*)user_data;
		std::stringstream buf;
		if(log_domain && log_domain[0])
			buf << "(" << log_domain << ") ";
		if(log_level & G_LOG_LEVEL_ERROR)
			buf << "[error] ";
		else if(log_level & G_LOG_LEVEL_CRITICAL)
			buf << "[critical] ";
		else if(log_level & G_LOG_LEVEL_WARNING)
			buf << "[warning] ";
		else if(log_level & G_LOG_LEVEL_MESSAGE)
			buf << "[message] ";
		else if(log_level & G_LOG_LEVEL_INFO)
			buf << "[info] ";
		else if(log_level & G_LOG_LEVEL_DEBUG)
			buf << "[debug] ";
		if(message)
			buf << message;
		buf << "\n";
		gtk_text_buffer_insert_at_cursor(obj->buffer, buf.str().c_str(), -1);
	}
	GtkTextBuffer * buffer;
	guint handler_id;
};

static std::string get_file_path_without_extension(const std::string& full_file_name)
{
	std::string::size_type pos = full_file_name.find_last_of('.');
	return (pos == std::string::npos ? full_file_name : full_file_name.substr(0, pos));
}

static void on_browse_button_clicked(GtkButton *button, gpointer data)
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

static void convert_dslfile(const std::string &infile)
{
	std::string basename(infile);
	std::string::size_type pos = basename.rfind(G_DIR_SEPARATOR);
	std::string cur_workdir;
	if (pos != std::string::npos) {
		cur_workdir.assign(basename, 0, pos);
	}
	else
		cur_workdir = ".";

	std::unique_ptr<ParserBase> parser(ParsersRepo::get_instance().create_codec("dsl"));
	g_assert(parser.get());
	std::unique_ptr<GeneratorBase> generator(GeneratorsRepo::get_instance().create_codec("stardict"));
	g_assert(generator.get());
	Connector connector(*generator, cur_workdir);
	
	parser->reset_ops(&connector);
	generator->reset_ops(&connector);
	
	StringList parser_options;

	parser->run(parser_options, infile);
}

static void on_compile_page_compile_button_clicked(GtkButton *button, gpointer data)
{
	GtkEntry *entry = GTK_ENTRY(data);
	std::string srcfilename(gtk_entry_get_text(entry));
	gtk_text_buffer_set_text(compile_page_text_view_buffer, "Building...\n", -1);
	HookMessages hookOutput(compile_page_text_view_buffer);
	gint output_format_ind = gtk_combo_box_get_active(GTK_COMBO_BOX(compile_page_combo_box));
	bool res = true;
	if (output_format_ind == 0) {
		res = convert_tabfile(srcfilename.c_str());
	} else if (output_format_ind == 1) {
		convert_babylonfile(srcfilename.c_str(), true);
	} else if (output_format_ind == 2) {
		convert_bglfile(srcfilename, get_file_path_without_extension(srcfilename) + ".babylon", "", "");
	} else if (output_format_ind == 3) {
		convert_dslfile(srcfilename);
	} else {
		std::string ifofilename = get_file_path_without_extension(srcfilename) + ".ifo";
		bool show_xincludes = gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(compile_page_show_xinclude_check_box));
		bool use_same_type_sequence = gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(compile_page_use_sametypesequence_check_box));
		res = (EXIT_SUCCESS == stardict_text2bin(srcfilename, ifofilename,
			show_xincludes, use_same_type_sequence));
	}
	gtk_text_buffer_insert_at_cursor(compile_page_text_view_buffer, 
		res ? "Done!\n" : "Failed!\n",
		-1);
}

static void set_compile_parameter_panel(void)
{
	gint output_format_ind = gtk_combo_box_get_active(GTK_COMBO_BOX(compile_page_combo_box));
	if(output_format_ind == 2)
		gtk_widget_show(GTK_WIDGET(compile_page_textual_stardict_hbox));
	else
		gtk_widget_hide(GTK_WIDGET(compile_page_textual_stardict_hbox));
}

static void on_compile_page_combo_box_changed(GtkComboBox *widget, gpointer user_data)
{
	set_compile_parameter_panel();
}

static void create_compile_page(GtkWidget *notebook)
{
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
	GtkWidget *vbox = gtk_vbox_new(FALSE, 6);
#endif
	GtkWidget *label = gtk_label_new("Compile");
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
#else
	GtkWidget *hbox = gtk_hbox_new(FALSE, 6);
#endif
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
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD_CHAR);
	compile_page_text_view_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	gtk_text_buffer_set_text(compile_page_text_view_buffer,
		"Compile any supported file format to StarDict dictionary.\n"
		"\n"
		"Tab file format\n"
		"---------------\n"
		"Here is a example dict.tab file:\n"
		"============\n"
		"a\t1\\n2\\n3\n"
		"b\t4\\\\5\\n6\n"
		"c\t789\n"
		"============\n"
		"Each line contains a word - definition pair. The word is split from definition with a tab character. "
		"You may use the following escapes: \\n - new line, \\\\ - \\, \\t - tab character.\n"
		"\n\n"
		"Babylon source file format\n"
		"--------------------------\n"
		"=====\n"
		"apple|apples\n"
		"the meaning of apple\n"
		"\n"
		"2dimensional|2dimensionale|2dimensionaler|2dimensionales|2dimensionalem|2dimensionalen\n"
		"two dimensional's meaning<br>the second line.\n"
		"\n"
		"=====\n"
		"Each article must be followed by an empty line. The file must end with two empty lines!\n"
		"\n"
		"You may specify field like bookname, author, description that will be used in the generated StarDict dictionary. "
		"You may specify options effecting processing of the babylon source file. "
		"See libbabylongfile.cpp source file for complete list of supported fields and options. "
		"To specify options and fields, leave the first line blank, than write options, one option per line. "
		"Precede each line with a hash sign.\n"
		"For example:\n"
		"=====\n"
		"\n"
		"#bookname=My dictionary\n"
		"#author=My name\n"
		"#description=...\n"
		"#other fields=\n"
		"\n"
		"articles...\n"
		"=====\n"
		"\n"
		"Textual StarDict file format\n"
		"----------------------------\n"
		"See doc\\TextualDictionaryFileFormat in source tarball for information about Textual StarDict dictionary.\n"
		"\n"
		"\nFor converting Babylon (.bgl) files to StarDict format use PyGlossary. See http://code.google.com/p/stardict-3/wiki/ConvertBabylon\n"
		"\nFor converting Lingvo (.dsl) files to StarDict format use makedict. See http://code.google.com/p/stardict-3/wiki/ConvertLingvo\n"
		, -1);
	GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(scrolled_window, -1, 250);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);
	gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, true, true, 0);
#if GTK_MAJOR_VERSION >= 3
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
#else
	hbox = gtk_hbox_new(FALSE, 6);
#endif
	gtk_box_pack_start(GTK_BOX(vbox), hbox, false, false, 0);
	
	compile_page_combo_box = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(compile_page_combo_box), "Tab file");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(compile_page_combo_box), "Babylon file");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(compile_page_combo_box), "BGL file");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(compile_page_combo_box), "DSL file");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(compile_page_combo_box), "Textual StarDict dictionary");
	gtk_combo_box_set_active(GTK_COMBO_BOX(compile_page_combo_box), 0);
	gtk_box_pack_start(GTK_BOX(hbox), compile_page_combo_box, true, false, 0);

	button = gtk_button_new_with_mnemonic("_Compile");
	gtk_box_pack_start(GTK_BOX(hbox), button, true, false, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(on_compile_page_compile_button_clicked), entry);

	// parameter panel
#if GTK_MAJOR_VERSION >= 3
	compile_page_textual_stardict_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
#else
	compile_page_textual_stardict_hbox = gtk_hbox_new(false, 6);
#endif
	gtk_box_pack_start(GTK_BOX(vbox), compile_page_textual_stardict_hbox, false, false, 3);
	
	compile_page_show_xinclude_check_box = gtk_check_button_new_with_label("show xincludes");
	gtk_box_pack_start(GTK_BOX(compile_page_textual_stardict_hbox), 
		compile_page_show_xinclude_check_box, false, false, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(compile_page_show_xinclude_check_box), TRUE);
	
	compile_page_use_sametypesequence_check_box = gtk_check_button_new_with_label("use same type sequence");
	gtk_box_pack_start(GTK_BOX(compile_page_textual_stardict_hbox), 
		compile_page_use_sametypesequence_check_box, false, false, 0);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(compile_page_use_sametypesequence_check_box), TRUE);

	// must be after the parameter panel is created
	g_signal_connect(G_OBJECT(compile_page_combo_box), "changed", G_CALLBACK(on_compile_page_combo_box_changed), NULL);
}

static void on_decompile_page_build_button_clicked(GtkButton *button, gpointer data)
{
	GtkEntry *entry = GTK_ENTRY(data);
	std::string ifofilename(gtk_entry_get_text(entry));
	gtk_text_buffer_set_text(decompile_page_text_view_buffer, "Building...\n", -1);
	HookMessages hookOutput(decompile_page_text_view_buffer);
	gint output_format_ind = gtk_combo_box_get_active(GTK_COMBO_BOX(decompile_page_combo_box));
	int res = EXIT_SUCCESS;
	if(output_format_ind == 0) {
		std::string txtfilename = get_file_path_without_extension(ifofilename) + ".txt";
		convert_stardict2txt(ifofilename.c_str(), txtfilename.c_str());
	} else {
		std::string xmlfilename = get_file_path_without_extension(ifofilename) + ".xml";
		const gchar* chunk_size_str = gtk_entry_get_text(GTK_ENTRY(decompile_page_entry_chunk_size));
		int chunk_size = atoi(chunk_size_str);
		if(chunk_size < 0)
			chunk_size = 0;
		glib::CharStr temp(g_strdup_printf("%d", chunk_size));
		// set the chunk size back to the entry for the user to see what value was actually used.
		gtk_entry_set_text(GTK_ENTRY(decompile_page_entry_chunk_size), get_impl(temp));
		res = stardict_bin2text(ifofilename, xmlfilename, chunk_size);
	}
	gtk_text_buffer_insert_at_cursor(decompile_page_text_view_buffer, 
		(res == EXIT_SUCCESS) ? "Done!\n" : "Failed!\n",
		-1);
}

static void on_decompile_page_verify_button_clicked(GtkButton *button, gpointer data)
{
	GtkEntry *entry = GTK_ENTRY(data);
	gtk_text_buffer_set_text(decompile_page_text_view_buffer, "Verifying dictionary files...\n", -1);
	HookMessages hookOutput(decompile_page_text_view_buffer);
	int res = stardict_verify(gtk_entry_get_text(entry));
	gtk_text_buffer_insert_at_cursor(decompile_page_text_view_buffer, 
		(res == EXIT_SUCCESS) ? "Done!\n" : "Failed!\n",
		-1);
}

static void set_decompile_parameter_panel(void)
{
	gint output_format_ind = gtk_combo_box_get_active(GTK_COMBO_BOX(decompile_page_combo_box));
	if(output_format_ind == 1)
		gtk_widget_show(GTK_WIDGET(decompile_page_textual_stardict_hbox));
	else
		gtk_widget_hide(GTK_WIDGET(decompile_page_textual_stardict_hbox));
}

static void on_decompile_page_combo_box_changed(GtkComboBox *widget, gpointer user_data)
{
	set_decompile_parameter_panel();
}

static void create_decompile_page(GtkWidget *notebook)
{
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
	GtkWidget *vbox = gtk_vbox_new(false, 6);
#endif
	GtkWidget *label = gtk_label_new("DeCompile/Verify");
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
#else
	GtkWidget *hbox = gtk_hbox_new(false, 6);
#endif
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
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD_CHAR);
	decompile_page_text_view_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(scrolled_window, -1, 250);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);
	gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, true, true, 0);
#if GTK_MAJOR_VERSION >= 3
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
#else
	hbox = gtk_hbox_new(false, 6);
#endif
	gtk_box_pack_start(GTK_BOX(vbox), hbox, false, false, 0);

	decompile_page_combo_box = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(decompile_page_combo_box), "Tab file");
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(decompile_page_combo_box), "Textual StarDict dictionary");
	gtk_combo_box_set_active(GTK_COMBO_BOX(decompile_page_combo_box), 0);
	gtk_box_pack_start(GTK_BOX(hbox), decompile_page_combo_box, true, false, 0);

	button = gtk_button_new_with_mnemonic("_Decompile");
	gtk_box_pack_start(GTK_BOX(hbox), button, true, false, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(on_decompile_page_build_button_clicked), entry);
	button = gtk_button_new_with_mnemonic("_Verify");
	gtk_box_pack_start(GTK_BOX(hbox), button, true, false, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(on_decompile_page_verify_button_clicked), entry);

	// parameter panel
#if GTK_MAJOR_VERSION >= 3
	decompile_page_textual_stardict_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
#else
	decompile_page_textual_stardict_hbox = gtk_hbox_new(false, 6);
#endif
	gtk_box_pack_start(GTK_BOX(vbox), decompile_page_textual_stardict_hbox, false, false, 3);
	label = gtk_label_new("Chunk size (in bytes, 0 - do not split):");
	gtk_box_pack_start(GTK_BOX(decompile_page_textual_stardict_hbox), label, false, false, 0);
	decompile_page_entry_chunk_size = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(decompile_page_entry_chunk_size), "0");
	gtk_box_pack_start(GTK_BOX(decompile_page_textual_stardict_hbox), decompile_page_entry_chunk_size, false, false, 0);

	// must be after the parameter panel is created
	g_signal_connect(G_OBJECT(decompile_page_combo_box), "changed", G_CALLBACK(on_decompile_page_combo_box_changed), NULL);
}

static void on_edit_page_open_button_clicked(GtkButton *button, gpointer data)
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

static void on_edit_page_saveas_button_clicked(GtkButton *button, gpointer data)
{
	GtkWidget *dialog;
	dialog = gtk_file_chooser_dialog_new ("Save file...",
		GTK_WINDOW(main_window),
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
		NULL);
	gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (dialog), TRUE);
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

static void create_edit_page(GtkWidget *notebook)
{
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
#else
	GtkWidget *vbox = gtk_vbox_new(false, 6);
#endif
	GtkWidget *label = gtk_label_new("Edit");
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), vbox, label);
#if GTK_MAJOR_VERSION >= 3
	GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
#else
	GtkWidget *hbox = gtk_hbox_new(false, 6);
#endif
	gtk_box_pack_start(GTK_BOX(vbox), hbox, false, false, 0);
	GtkWidget *button = gtk_button_new_with_mnemonic("_Open");
	gtk_box_pack_start(GTK_BOX(hbox), button, true, false, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(on_edit_page_open_button_clicked), NULL);
	button = gtk_button_new_with_mnemonic("_Save as");
	gtk_box_pack_start(GTK_BOX(hbox), button, true, false, 0);
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(on_edit_page_saveas_button_clicked), NULL);
	GtkWidget *text_view = gtk_text_view_new();
	gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD_CHAR);
	edit_page_text_view_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
	gtk_text_buffer_set_text(edit_page_text_view_buffer,
		"This is a simple UTF-8 text file editor.\n"
		, -1);
	GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(scrolled_window, -1, 250);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scrolled_window), text_view);
	gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, true, true, 0);
}

static gboolean on_delete_event(GtkWidget * window, GdkEvent *event , gpointer data)
{
	gtk_main_quit();
	return FALSE;
}

static void create_window()
{
	main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position (GTK_WINDOW (main_window), GTK_WIN_POS_CENTER);
	gtk_window_set_title(GTK_WINDOW (main_window), "StarDict-Editor");
	gtk_window_set_default_size(GTK_WINDOW (main_window), -1, 430);
	gtk_container_set_border_width (GTK_CONTAINER (main_window), 5);
	g_signal_connect (G_OBJECT (main_window), "delete_event", G_CALLBACK (on_delete_event), NULL);
	GtkWidget *notebook = gtk_notebook_new();
	gtk_container_add(GTK_CONTAINER(main_window), notebook);
	create_compile_page(notebook);
	create_decompile_page(notebook);
	create_edit_page(notebook);
	gtk_widget_show_all(main_window);
	set_decompile_parameter_panel();
	set_compile_parameter_panel();
}

#ifdef _WIN32
#if BUILDING_DLL
# define DLLIMPORT __declspec (dllexport)
#else /* Not BUILDING_DLL */
# define DLLIMPORT __declspec (dllimport)
#endif /* Not BUILDING_DLL */

extern "C" {
	DLLIMPORT extern int stardict_editor_main(HINSTANCE hInstance, int argc, char **argv);
}

DLLIMPORT int stardict_editor_main(HINSTANCE /*hInstance*/, int argc, char **argv)
#else
int main(int argc,char **argv)
#endif
{
	gtk_init(&argc, &argv);
	create_window();
	gtk_main();
	return 0;
}
