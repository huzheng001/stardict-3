#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <cstring>
#include <string>

#include <glib/gi18n.h>

#include "conf.h"
#include "desktop.hpp"
#include "utils.h"
#include "stardict.h"

#include "readword.h"

ReadWord::ReadWord()
{
	const std::string &path = conf->get_string_at("dictionary/tts_path");
	LoadRealTtsPath(path.c_str());
	use_command_tts = conf->get_bool("/apps/stardict/preferences/dictionary/use_tts_program");
	tts_program_cmdline = conf->get_string("/apps/stardict/preferences/dictionary/tts_program_cmdline");
}

ReadWordType ReadWord::canRead(const gchar *word)
{
	if (RealTts_canRead(word))
		return READWORD_REALTTS;
	if (gpAppFrame->oStarDictPlugins->TtsPlugins.nplugins() > 0)
		return READWORD_TTS;
	if (use_command_tts && !tts_program_cmdline.empty())
		return READWORD_COMMAND;
	return READWORD_CANNOT;
}

void ReadWord::read(const gchar *word, ReadWordType type)
{
	if (type == READWORD_REALTTS) {
		RealTts_read(word);
	} else if (type == READWORD_TTS) {
		gpAppFrame->oStarDictPlugins->TtsPlugins.saytext(0, word);
	} else if (type == READWORD_COMMAND) {
		Command_read(word);
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
}

bool ReadWord::RealTts_canRead(const gchar *word)
{	
	bool return_val = false;
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

void ReadWord::Command_read(const gchar *word)
{
	gchar *eword = g_shell_quote(word);
	gchar *command = g_strdup_printf(tts_program_cmdline.c_str(), eword);
	g_free(eword);
	system(command);
	g_free(command);
}

void ReadWord::RealTts_read(const gchar *word)
{
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
				break;
			}
		}

	}
}

const int Engine_RealTTS = 0;
const int Engine_Command = 1;
const int Engine_VirtualTTS_Base = 2;

std::list<std::pair<std::string, int> > ReadWord::GetEngineList()
{
	std::list<std::pair<std::string, int> > engine_list;
	if (!ttspath.empty()) {
		engine_list.push_back(std::pair<std::string, int>(_("Real People TTS"), Engine_RealTTS));
	}
	size_t n = gpAppFrame->oStarDictPlugins->TtsPlugins.nplugins();
	for (size_t i = 0; i < n; i++) {
		engine_list.push_back(std::pair<std::string, int>(gpAppFrame->oStarDictPlugins->TtsPlugins.tts_name(i), Engine_VirtualTTS_Base + i));
	}
	if (use_command_tts && !tts_program_cmdline.empty()) {
		engine_list.push_back(std::pair<std::string, int>(_("Command TTS"), Engine_Command));
	}
	return engine_list;
}

void ReadWord::ReadByEngine(const gchar *word, int engine_index)
{
	if (engine_index == Engine_RealTTS) {
		RealTts_read(word);
	} else if (engine_index == Engine_Command) {
		Command_read(word);
	} else {
		gpAppFrame->oStarDictPlugins->TtsPlugins.saytext(engine_index - Engine_VirtualTTS_Base, word);
	}
}
