#include "stardict_qqwry.h"
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

static const StarDictPluginSystemInfo *plugin_info = NULL;


static char *build_dictdata(char type, const char *definition)
{
	size_t len = strlen(definition);
	guint32 size;
	size = sizeof(char) + len + 1;
	char *data = (char *)g_malloc(sizeof(guint32) + size);
	char *p = data;
	*((guint32 *)p)= size;
	p += sizeof(guint32);
	*p = type;
	p++;
	memcpy(p, definition, len+1);
	return data;
}

#define REDIRECT_MODE_1 0x01
#define REDIRECT_MODE_2 0x02

static unsigned long getValue(FILE *fp, unsigned long start, int length)
{
	unsigned long variable=0;
	long *val = (long *)malloc(sizeof(long) *length);
	long i;
	fseek(fp,start,SEEK_SET);
	for(i=0;i<length;i++) {
		val[i]=fgetc(fp)&0x000000FF;
	}
	for(i=length-1;i>=0;i--) {
		variable=variable*0x100+val[i];
	}
	free(val);
	return variable;
}

static int getString(FILE *fp, unsigned long start, std::string &string)
{
	unsigned long i=0;
	char val;
	fseek(fp,start,SEEK_SET);
	do {
		val=fgetc(fp);
		string += val;
		i++;
	} while(val!=0x00);
	return i;
}

static void getAddress(FILE *fp, unsigned long start, std::string &country, std::string &location)
{
	unsigned long redirect_address,counrty_address,location_address;
	char val;
	start+=4;
	fseek(fp,start,SEEK_SET);
	val=(fgetc(fp)&0x000000FF);
	if(val==REDIRECT_MODE_1) {
		redirect_address=getValue(fp,start+1,3);
		fseek(fp,redirect_address,SEEK_SET);
		if((fgetc(fp)&0x000000FF)==REDIRECT_MODE_2) {
			counrty_address=getValue(fp,redirect_address+1,3);
			location_address=redirect_address+4;
			getString(fp,counrty_address,country);
		} else {
			counrty_address=redirect_address;
			location_address=redirect_address+getString(fp,counrty_address,country);
		}
	} else if (val==REDIRECT_MODE_2) {
		counrty_address=getValue(fp,start+1,3);
		location_address=start+4;
		getString(fp,counrty_address,country);
	} else {
		counrty_address=start;
		location_address=counrty_address+getString(fp,counrty_address,country);
	}
	fseek(fp,location_address,SEEK_SET);
	if((fgetc(fp)&0x000000FF)==REDIRECT_MODE_2||(fgetc(fp)&0x000000FF)==REDIRECT_MODE_1) {
		location_address=getValue(fp,location_address+1,3);
	}
	getString(fp,location_address,location);
}

static void getHead(FILE *fp,unsigned long *start,unsigned long *end)
{
	*start=getValue(fp,0L,4);
	*end=getValue(fp,4L,4);
}

static int beNumber(char c)
{
	if(c>='0'&&c<='9')
		return 0;
	else
		return 1;
}

static unsigned long getIP(const char *ip_addr)
{
	unsigned long ip=0;
	size_t i;
	int j=0;
	for(i=0;i<strlen(ip_addr);i++) {
		if (*(ip_addr+i)=='.') {
			ip=ip*0x100+j;
			j=0;
		} else {
			if(beNumber(*(ip_addr+i))==0) {
				j=j*10+*(ip_addr+i)-'0';
			} else {
				return 0;
			}
		}
	}
	ip=ip*0x100+j;
	return ip;
}

static unsigned long searchIP(FILE *fp, unsigned long index_start, unsigned long index_end, unsigned long ip)
{
	unsigned long index_current,index_top,index_bottom;
	unsigned long record;
	index_bottom=index_start;
	index_top=index_end;
	index_current=((index_top-index_bottom)/7/2)*7+index_bottom;
	do {
		record=getValue(fp,index_current,4);
		if(record>ip) {
			index_top=index_current;
		} else {
			index_bottom=index_current;
		}
		index_current=((index_top-index_bottom)/14)*7+index_bottom;
	} while(index_bottom<index_current);
	return index_current;
}

static void get_address_from_ip(const char *text, std::string &ipstr, std::string &address)
{
	GMatchInfo *match_info;
	GRegex *regex = g_regex_new ("(((\\d{1,2})|(1\\d{2})|(2[0-4]\\d)|(25[0-5]))\\.){3}((\\d{1,2})|(1\\d{2})|(2[0-4]\\d)|(25[0-5]))", (GRegexCompileFlags)0, (GRegexMatchFlags)0, NULL);
	g_regex_match (regex, text, (GRegexMatchFlags)0, &match_info);
	if (g_match_info_matches(match_info)) {
		gchar *word = g_match_info_fetch (match_info, 0);
		ipstr = word;
		g_free (word);
	}
	g_match_info_free (match_info);
	g_regex_unref (regex);
	if (ipstr.empty())
		return;
	std::string datafilename = plugin_info->datadir;
	datafilename += G_DIR_SEPARATOR_S "data" G_DIR_SEPARATOR_S "QQWry.Dat";
	FILE *fp = g_fopen(datafilename.c_str(), "rb");
	if (!fp) {
		gchar *msg = g_strdup_printf(_("Error: Open file %s failed!"), datafilename.c_str());
		address = msg;
		g_free(msg);
		return;
	}
	unsigned long index_start,index_end;
	getHead(fp,&index_start,&index_end);
	unsigned long ip = getIP(ipstr.c_str());
	unsigned long current=searchIP(fp,index_start,index_end,ip);
	std::string country,location;
	getAddress(fp,getValue(fp,current+4,3),country,location);
	gchar *c = g_convert(country.c_str(), -1, "UTF-8", "GB18030", NULL, NULL, NULL);
	if (c) {
		address += c;
		address += ' ';
		g_free(c);
	}
	gchar *l = g_convert(location.c_str(), -1, "UTF-8", "GB18030", NULL, NULL, NULL);
	if (l) {
		address += l;
		g_free(l);
	}
	fclose(fp);
}

static void lookup(const char *text, char ***pppWord, char ****ppppWordData)
{
	std::string ipstr, address;
	get_address_from_ip(text, ipstr, address);
	if (address.empty()) {
		*pppWord = NULL;
	} else {
		*pppWord = (gchar **)g_malloc(sizeof(gchar *)*2);
		(*pppWord)[0] = g_strdup(ipstr.c_str());
		(*pppWord)[1] = NULL;
		*ppppWordData = (gchar ***)g_malloc(sizeof(gchar **)*(1));
		(*ppppWordData)[0] = (gchar **)g_malloc(sizeof(gchar *)*2);
		(*ppppWordData)[0][0] =  build_dictdata('m', address.c_str());
		(*ppppWordData)[0][1] = NULL;
	}
}

static void configure()
{
	GtkWidget *window = gtk_dialog_new_with_buttons(_("QQWry configuration"), GTK_WINDOW(plugin_info->pluginwin), GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL);
	GtkWidget *vbox = gtk_vbox_new(false, 5);
	std::string msg;
	std::string datafilename = plugin_info->datadir;
	datafilename += G_DIR_SEPARATOR_S "data" G_DIR_SEPARATOR_S "QQWry.Dat";
	if (g_file_test(datafilename.c_str(), G_FILE_TEST_EXISTS)) {
		msg = _("You can update the QQWry.Dat file from this website:\nhttp://www.cz88.net");
	} else {
		gchar *str = g_strdup_printf(_("Error: File %s not found!\nYou can download it from this website:\nhttp://www.cz88.net"), datafilename.c_str());
		msg = str;
		g_free(str);
	}
	GtkWidget *label = gtk_label_new(msg.c_str());
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
	gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), label, true, true, 0);
	gtk_widget_show_all(vbox);
	gtk_container_add (GTK_CONTAINER (GTK_DIALOG(window)->vbox), vbox);
	gtk_dialog_run(GTK_DIALOG(window));
	gtk_widget_destroy (window);
}

DLLIMPORT bool stardict_plugin_init(StarDictPlugInObject *obj)
{
	if (strcmp(obj->version_str, PLUGIN_SYSTEM_VERSION)!=0) {
		g_print("Error: QQWry plugin version doesn't match!\n");
		return true;
	}
	obj->type = StarDictPlugInType_VIRTUALDICT;
	obj->info_xml = g_strdup_printf("<plugin_info><name>%s</name><version>1.0</version><short_desc>%s</short_desc><long_desc>%s</long_desc><author>Hu Zheng &lt;huzheng_001@163.com&gt;</author><website>http://stardict.sourceforge.net</website></plugin_info>", _("QQWry"), _("Show QQWry IP information."), _("Show the address information by the IP."));
	obj->configure_func = configure;
	plugin_info = obj->plugin_info;

	return false;
}

DLLIMPORT void stardict_plugin_exit(void)
{
}

DLLIMPORT bool stardict_virtualdict_plugin_init(StarDictVirtualDictPlugInObject *obj)
{
	obj->lookup_func = lookup;
	obj->dict_name = _("QQWry");
	g_print(_("QQWry plug-in loaded.\n"));
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
