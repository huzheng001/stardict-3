#ifndef __SD_READWORD_H__
#define __SD_READWORD_H__

#include <glib.h>
#include <list>
#include <string>

class ReadWord {
public:
	ReadWord();
	void loadpath(const gchar *path);
	bool canRead(const gchar *word);
	void read(const gchar *word);
private:
	//! \todo why this is here, we should allow change preferences online
	std::list<std::string> ttspath;
	bool use_tts;
	bool use_tss_if_not_found;
	std::string tts_program_cmdline;	
};

#endif
