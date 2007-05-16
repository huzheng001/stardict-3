#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstring>
#include <string>

#include "conf.h"
#include "desktop.hpp"
#include "utils.h"
#include "stardict.h"

#include "readword.h"

ReadWord::ReadWord()
{
	const std::string &path = conf->get_string_at("dictionary/tts_path");
	LoadRealTtsPath(path.c_str());
}

ReadWordType ReadWord::canRead(const gchar *word)
{
	if (RealTts_canRead(word))
		return READWORD_REALTTS;
	if (gpAppFrame->oStarDictPlugins->TtsPlugins.nplugins() > 0)
		return READWORD_TTS;
	//TODO READWORD_COMMAND
	return READWORD_CANNOT;
}

void ReadWord::read(const gchar *word, ReadWordType type)
{
	if (type == READWORD_REALTTS) {
		RealTts_read(word);
	} else if (type == READWORD_TTS) {
		gpAppFrame->oStarDictPlugins->TtsPlugins.saytext(0, word);
	} else if (type == READWORD_COMMAND) {
		//TODO
	}
}

void ReadWord::LoadRealTtsPath(const gchar *path)
{
	std::list<std::string> paths;
	std::string str;
	const gchar *p, *p1;
	p = path;
	do {
		p1 = strchr(p, '\n');
		if (p1) {
			str.assign(p, p1-p);
			if (!str.empty())
				paths.push_back(str);
			p = p1+1;
		}
	} while (p1);
	str = p;
	if (!str.empty())
		paths.push_back(str);

	ttspath.clear();
	std::list<std::string>::const_iterator it;
	for (it=paths.begin(); it!=paths.end(); ++it) {
#ifdef _WIN32
		if (it->length()>1 && (*it)[1]==':') {
			if (g_file_test(it->c_str(), G_FILE_TEST_EXISTS))
				ttspath.push_back(*it);
		} else {
			str = gStarDictDataDir + G_DIR_SEPARATOR_S + *it;
			if (g_file_test(str.c_str(), G_FILE_TEST_EXISTS))
				ttspath.push_back(str);
		}
#else
		if (g_file_test(it->c_str(), G_FILE_TEST_EXISTS))
			ttspath.push_back(*it);
#endif
	}
	
	use_tts = conf->get_bool("/apps/stardict/preferences/dictionary/use_tts_program");
	use_tss_if_not_found = conf->get_bool("/apps/stardict/preferences/dictionary/use_tts_program_if_not_found");
	const std::string &cmdline = conf->get_string("/apps/stardict/preferences/dictionary/tts_program_cmdline");
	tts_program_cmdline = cmdline;
}

bool ReadWord::RealTts_canRead(const gchar *word)
{	
	bool return_val = false;
	
	if (use_tts && !tts_program_cmdline.empty())
		return true;
	
	if (!ttspath.empty() && word && g_ascii_isalpha(word[0])) {
		std::string lowerword;
		const gchar *p = word;
		while (*p) {
			if (*p!=' ')
				lowerword+=g_ascii_tolower(*p);
			p++;
		}
		std::string filename;
		std::list<std::string>::const_iterator it;
		for (it=ttspath.begin(); it!=ttspath.end(); ++it) {
			filename = *it + G_DIR_SEPARATOR_S + lowerword[0] + G_DIR_SEPARATOR_S + lowerword + ".wav";
			return_val = g_file_test(filename.c_str(), G_FILE_TEST_EXISTS);
			if (return_val)
				break;
		}
	}
	return return_val;
}

void ReadWord::RealTts_read(const gchar *word)
{
	bool tts_sound_file_exist = false;
	
	if (use_tts && !use_tss_if_not_found)
	{
		if (!tts_program_cmdline.empty())
		{
			std::string cmdline = tts_program_cmdline;
			std::string placeholder("{WORD}");
			std::string queryword(word);
			if (cmdline.find(placeholder) != std::string::npos)
				cmdline.replace(cmdline.find(placeholder), placeholder.size(), queryword);
				
			g_spawn_command_line_async (cmdline.c_str(), NULL);
		}
		
		return ;
	}

	if (!ttspath.empty() && word && g_ascii_isalpha(word[0])) {
		std::string lowerword;
		const gchar *p = word;
		while (*p) {
			if (*p!=' ')
				lowerword+=g_ascii_tolower(*p);
			p++;
		}
		std::string filename;
		std::list<std::string>::const_iterator it;
		for (it=ttspath.begin(); it!=ttspath.end(); ++it) {
			filename = *it + G_DIR_SEPARATOR_S + lowerword[0] + G_DIR_SEPARATOR_S + lowerword + ".wav";
			if (g_file_test(filename.c_str(), G_FILE_TEST_EXISTS)) {
				play_wav_file(filename);
				tts_sound_file_exist = true;
				break;
			}
		}

	}
	
	if (!tts_sound_file_exist && use_tts)
	{
		if (!tts_program_cmdline.empty())
		{
			std::string cmdline = tts_program_cmdline;
			std::string placeholder("{WORD}");
			std::string queryword(word);
			if (cmdline.find(placeholder) != std::string::npos)
				cmdline.replace(cmdline.find(placeholder), placeholder.size(), queryword);
				
			g_spawn_command_line_async (cmdline.c_str(), NULL);
		}
	}	
}
