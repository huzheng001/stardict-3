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

#include "stardict_sapi_tts.h"
#include <glib/gi18n.h>
#include <windows.h>

#define _ATL_APARTMENT_THREADED

#include <atlbase.h>
//You may derive a class from CComModule and use it if you want to override something, 
//but do not change the name of _Module
extern CComModule _Module;
#include <atlcom.h>
#include <sapi.h>
#include <sphelper.h>


static const StarDictPluginSystemInfo *plugin_info = NULL;
static ISpVoice * pVoice = NULL;
static IAppDirs* gpAppDirs = NULL;

/* concatenate path1 and path2 inserting a path separator in between if needed. */
static std::string build_path(const std::string& path1, const std::string& path2)
{
	std::string res;
	res.reserve(path1.length() + 1 + path2.length());
	res = path1;
	if(!res.empty() && res[res.length()-1] != G_DIR_SEPARATOR)
		res += G_DIR_SEPARATOR_S;
	if(!path2.empty() && path2[0] == G_DIR_SEPARATOR)
		res.append(path2, 1, std::string::npos);
	else
		res.append(path2);
	return res;
}

static std::string get_cfg_filename()
{
	return build_path(gpAppDirs->get_user_config_dir(), "sapi_tts.cfg");
}

static void saytext(const char *text)
{
	DWORD dwNum = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
	wchar_t *pwText;
	pwText = new wchar_t[dwNum];
	MultiByteToWideChar(CP_UTF8, 0, text, -1, pwText, dwNum);
	pVoice->Speak(pwText, SPF_ASYNC | SPF_PURGEBEFORESPEAK | SPF_IS_NOT_XML, NULL);
	delete []pwText;
}

static bool configure_changed;

static void on_voice_combobox_changed(GtkComboBox *combobox, WCHAR** ppszTokenIds)
{
	configure_changed = true;
	gint index = gtk_combo_box_get_active(combobox);
	if (ppszTokenIds && ppszTokenIds[index]) {
		ISpObjectToken *pToken;
		if (SpGetTokenFromId(ppszTokenIds[index] , &pToken) == S_OK) {
			pVoice->SetVoice(pToken);
			pToken->Release();
		}
	}
}

static void on_volume_hscale_value_changed(GtkWidget *widget, gpointer user_data)
{
	configure_changed = true;
	USHORT volume = (USHORT)gtk_range_get_value(GTK_RANGE(widget));
	pVoice->SetVolume(volume);
}

static void on_rate_hscale_value_changed(GtkWidget *widget, gpointer user_data)
{
	configure_changed = true;
	long rate = (long)gtk_range_get_value(GTK_RANGE(widget));
	pVoice->SetRate(rate);
}

static void on_test_tts_button_clicked(GtkWidget *widget, GtkEntry *entry)
{
	const char *text = gtk_entry_get_text(entry);
	saytext(text);
}

static void configure()
{
	configure_changed = false;
	GtkWidget *window = gtk_dialog_new_with_buttons(_("SAPI TTS configuration"), GTK_WINDOW(plugin_info->pluginwin), GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
	GtkWidget *vbox = gtk_vbox_new(false, 5);
	gtk_container_set_border_width(GTK_CONTAINER(vbox),5);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(window)->vbox), vbox);
	GtkWidget *frame = gtk_frame_new(_("TTS voice engine"));
	gtk_box_pack_start(GTK_BOX(vbox), frame, false, false, 3);
	GtkWidget *table = gtk_table_new(2, 3, false);
	gtk_container_set_border_width(GTK_CONTAINER(table),8);
	gtk_container_add (GTK_CONTAINER (frame), table);
	GtkWidget *label = gtk_label_new(_("Voice :"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, .5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1, GTK_FILL, (GtkAttachOptions)0, 5, 0);
	GtkWidget *combobox = gtk_combo_box_new_text();
	gtk_table_attach(GTK_TABLE(table), combobox, 1, 2, 0, 1, GtkAttachOptions(GTK_FILL | GTK_EXPAND), (GtkAttachOptions)0, 5, 0);

	ULONG ulNumTokens;
	WCHAR** ppszTokenIds = NULL;
	CComPtr<IEnumSpObjectTokens> cpEnum;
	HRESULT hr = SpEnumTokens(SPCAT_VOICES, NULL, NULL, &cpEnum);
	if ( S_OK == hr ) {
		hr = cpEnum->GetCount( &ulNumTokens );
		if ( SUCCEEDED( hr ) && 0 != ulNumTokens ) {
			WCHAR *pszCurTokenId = NULL;
			ISpObjectToken *pToken;
			HRESULT hr = pVoice->GetVoice( &pToken );
			if ( SUCCEEDED( hr ) ) {
				pToken->GetId( &pszCurTokenId );
				pToken->Release();
			}
			if (pszCurTokenId) {
				ppszTokenIds = new WCHAR* [ulNumTokens];
				ZeroMemory( ppszTokenIds, ulNumTokens*sizeof( WCHAR* ) );
				LONG ulCurToken = -1;
				ULONG ulIndex = 0;
				while (cpEnum->Next(1, &pToken, NULL) == S_OK) {
					WCHAR *description;
					SpGetDescription( pToken, &description);
					DWORD dwNum = WideCharToMultiByte(CP_UTF8,NULL,description,-1,NULL,0,NULL,FALSE);
					char *text = new char[dwNum];
					WideCharToMultiByte (CP_UTF8,NULL,description,-1,text,dwNum,NULL,FALSE);
					gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), text);
					delete []text;
					CoTaskMemFree(description);
					pToken->GetId( &ppszTokenIds[ulIndex]);
					if (ulCurToken == -1 && _wcsicmp( pszCurTokenId, ppszTokenIds[ulIndex]) == 0) {
						ulCurToken = ulIndex;
					}
					ulIndex++;
					pToken->Release();
				}
				CoTaskMemFree( pszCurTokenId );
				if (ulCurToken != -1)
					gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), ulCurToken);
			}
		}
	}
	g_signal_connect (G_OBJECT (combobox), "changed", G_CALLBACK (on_voice_combobox_changed), ppszTokenIds);

	label = gtk_label_new(_("Volume :"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, .5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 1, 2, GTK_FILL, (GtkAttachOptions)0, 5, 0);
	GtkWidget *volume_hscale = gtk_hscale_new_with_range(0, 100, 1);
	gtk_table_attach(GTK_TABLE(table), volume_hscale, 1, 2, 1, 2, GtkAttachOptions(GTK_FILL | GTK_EXPAND), (GtkAttachOptions)0, 5, 0);
	USHORT volume;
	pVoice->GetVolume(&volume);
	gtk_range_set_value(GTK_RANGE(volume_hscale), volume);
	g_signal_connect(G_OBJECT(volume_hscale),"value-changed", G_CALLBACK(on_volume_hscale_value_changed), NULL);
	label = gtk_label_new(_("Rate :"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, .5);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3, GTK_FILL, (GtkAttachOptions)0, 5, 0);
	GtkWidget *rate_hscale = gtk_hscale_new_with_range(-10, 10, 1);
	gtk_table_attach(GTK_TABLE(table), rate_hscale, 1, 2, 2, 3, GtkAttachOptions(GTK_FILL | GTK_EXPAND), (GtkAttachOptions)0, 5, 0);
	long rate;
	pVoice->GetRate(&rate);
	gtk_range_set_value(GTK_RANGE(rate_hscale), rate);
	g_signal_connect(G_OBJECT(rate_hscale),"value-changed", G_CALLBACK(on_rate_hscale_value_changed), NULL);

	GtkWidget *vbox1 = gtk_vbox_new(false, 5);
	gtk_box_pack_start(GTK_BOX(vbox), vbox1, false, false, 10);
	label = gtk_label_new(_("Input the test text:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, .5);
	gtk_box_pack_start(GTK_BOX(vbox1), label, false, false, 0);
	GtkWidget *hbox = gtk_hbox_new(false, 5);
	gtk_box_pack_start(GTK_BOX(vbox1), hbox, false, false, 0);
	GtkWidget *entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(entry), "This is the test text");
	gtk_box_pack_start(GTK_BOX(hbox), entry, true, true, 0);
	GtkWidget *button = gtk_button_new_with_label(_("Test"));
	gtk_box_pack_start(GTK_BOX(hbox), button, false, false, 0);
	g_signal_connect(G_OBJECT(button),"clicked", G_CALLBACK(on_test_tts_button_clicked), GTK_ENTRY(entry));

	gtk_widget_show_all(vbox);
	gtk_dialog_run(GTK_DIALOG(window));
	if (configure_changed) {
		char *voice = NULL;
		ISpObjectToken *pToken;
		HRESULT hr = pVoice->GetVoice( &pToken );
		if ( SUCCEEDED( hr ) ) {
			WCHAR *id;
			pToken->GetId( &id );
			DWORD dwNum = WideCharToMultiByte(CP_UTF8,NULL,id,-1,NULL,0,NULL,FALSE);
			voice = new char[dwNum];
			WideCharToMultiByte (CP_UTF8,NULL,id,-1,voice,dwNum,NULL,FALSE);
			CoTaskMemFree(id);
			pToken->Release();
		}
		if (voice) {
			gint volume = (gint)gtk_range_get_value(GTK_RANGE(volume_hscale));
			gint rate = (gint)gtk_range_get_value(GTK_RANGE(rate_hscale));
			gchar *data = g_strdup_printf("[sapi_tts]\nvoice=%s\nvolume=%d\nrate=%d\n", voice, volume, rate);
			std::string res = get_cfg_filename();
			g_file_set_contents(res.c_str(), data, -1, NULL);
			g_free(data);
			delete []voice;
		}
	}
	ULONG ulIndex;
	if ( ppszTokenIds ) {
		for ( ulIndex = 0; ulIndex < ulNumTokens; ulIndex++ ) {
			if ( NULL != ppszTokenIds[ulIndex] ) {
				CoTaskMemFree( ppszTokenIds[ulIndex] );
			}
		}
		delete [] ppszTokenIds;
	}
	gtk_widget_destroy (window);
}

DLLIMPORT bool stardict_plugin_init(StarDictPlugInObject *obj, IAppDirs* appDirs)
{
	g_debug(_("Loading SAPI tts plug-in..."));
	if (strcmp(obj->version_str, PLUGIN_SYSTEM_VERSION)!=0) {
		g_print(_("Error: SAPI tts plugin version doesn't match!\n"));
		return true;
	}
	obj->type = StarDictPlugInType_TTS;
	obj->info_xml = g_strdup_printf("<plugin_info><name>%s</name><version>1.0</version><short_desc>%s</short_desc><long_desc>%s</long_desc><author>Hu Zheng &lt;huzheng001@gmail.com&gt;</author><website>http://stardict-4.sourceforge.net</website></plugin_info>", _("SAPI TTS"), _("SAPI TTS."), _("Pronounce words by SAPI TTS engine."));
	obj->configure_func = configure;
	plugin_info = obj->plugin_info;
	gpAppDirs = appDirs;
	return false;
}

DLLIMPORT void stardict_plugin_exit(void)
{
	pVoice->Release();
	::CoUninitialize();
	gpAppDirs = NULL;
}

DLLIMPORT bool stardict_tts_plugin_init(StarDictTtsPlugInObject *obj)
{
	if (FAILED(::CoInitialize(NULL)))
		return true;
	HRESULT hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void **)&pVoice);
	if(!SUCCEEDED(hr)) {
		::CoUninitialize();
		return true;
	}
	std::string res = get_cfg_filename();
	if (!g_file_test(res.c_str(), G_FILE_TEST_EXISTS)) {
		g_file_set_contents(res.c_str(), "[sapi_tts]\nvoice=\nvolume=100\nrate=0\n", -1, NULL);
	}
	GKeyFile *keyfile = g_key_file_new();
	g_key_file_load_from_file(keyfile, res.c_str(), G_KEY_FILE_NONE, NULL);
	gchar *text = g_key_file_get_string(keyfile, "sapi_tts", "voice", NULL);
	if (text) {
		if (text[0] != '\0') {
			DWORD dwNum = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
			wchar_t *pwText;
			pwText = new wchar_t[dwNum];
			MultiByteToWideChar(CP_UTF8, 0, text, -1, pwText, dwNum);
			ISpObjectToken *pToken;
			if (SpGetTokenFromId(pwText , &pToken) == S_OK) {
				pVoice->SetVoice(pToken);
				pToken->Release();
			}
			delete []pwText;
		}
		g_free(text);
	}
	GError *err;
	err = NULL;
	USHORT volume = g_key_file_get_integer(keyfile, "sapi_tts", "volume", &err);
	if (err) {
		g_error_free (err);
		volume = 100;
	}
	err = NULL;
	long rate = g_key_file_get_integer(keyfile, "sapi_tts", "rate", &err);
	if (err) {
		g_error_free (err);
		rate = 0;
	}
	g_key_file_free(keyfile);
	pVoice->SetVolume(volume);
	pVoice->SetRate(rate);
	obj->saytext_func = saytext;
	obj->tts_name = _("SAPI TTS");
	g_print(_("SAPI tts plug-in loaded.\n"));
	return false;
}

#ifdef _WIN32
BOOL APIENTRY DllMain (HINSTANCE hInst     /* Library instance handle. */ ,
                       DWORD reason        /* Reason this function is being called. */ ,
                       LPVOID reserved     /* Not used. */ )
{
    switch (reason)
    {
      case DLL_PROCESS_ATTACH:
        break;

      case DLL_PROCESS_DETACH:
        break;

      case DLL_THREAD_ATTACH:
        break;

      case DLL_THREAD_DETACH:
        break;
    }

    /* Returns TRUE on success, FALSE on failure */
    return TRUE;
}
#endif
