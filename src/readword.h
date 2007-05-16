#ifndef __SD_READWORD_H__
#define __SD_READWORD_H__

#include <glib.h>
#include <list>
#include <string>

enum ReadWordType {
	READWORD_CANNOT = 0,
	READWORD_REALTTS,
	READWORD_TTS,
	READWORD_COMMAND,
};

class ReadWord {
public:
	ReadWord();
	void LoadRealTtsPath(const gchar *path);
	ReadWordType canRead(const gchar *word);
	void read(const gchar *word, ReadWordType type);
private:
	bool RealTts_canRead(const gchar *word);
	void RealTts_read(const gchar *word);
	std::list<std::string> ttspath;
	bool use_tts;
	bool use_tss_if_not_found;
	std::string tts_program_cmdline;	
};

#endif
