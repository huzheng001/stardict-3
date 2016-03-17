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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdlib.h>

#include <cstring>
#include <cstdlib>
#include <string>

#include <glib/gi18n.h>

#include "conf.h"
#include "desktop.h"
#include "lib/utils.h"
#include "stardict.h"

#include "readword.h"

ReadWord::ReadWord()
{
	const std::list<std::string> &pathlist = conf->get_strlist_at("dictionary/tts_path");
#ifdef _WIN32
	std::list<std::string> paths;
	abs_path_to_data_dir(pathlist, paths);
	LoadRealTtsPath(paths);
#else
	LoadRealTtsPath(pathlist);
#endif
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

void ReadWord::LoadRealTtsPath(const std::list<std::string>& pathlist)
{
	ttspath.clear();
	for (std::list<std::string>::const_iterator it=pathlist.begin(); it!=pathlist.end(); ++it) {
		if(it->empty())
			continue;
		if (g_file_test(it->c_str(), G_FILE_TEST_EXISTS))
			ttspath.push_back(*it);
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
			filename = build_path(*it, std::string(1, lowerword[0]) + G_DIR_SEPARATOR_S + lowerword + ".wav");
			return_val = g_file_test(filename.c_str(), G_FILE_TEST_EXISTS);
			if (return_val)
				break;
		}
	}
	return return_val;
}

void ReadWord::Command_read(const gchar *word)
{
	const gchar *str, *p;
	str = tts_program_cmdline.c_str();
	p = strrchr(str, '&');
	if (p) {
	} else {
		g_print(_("Wrong tts_program_cmdline string! No \'&\' at the end!\n"));
		return;
	}
	p = strstr(str, "%s");
	if (p) {
		const gchar *p1;
		p1 = p +2;
		p = strchr(p1, '%');
		if (p) {
			g_print(_("Wrong tts_program_cmdline string! More than 1 \'%%\' in the string!\n"));
			return;
		}
	} else {
		g_print(_("Wrong tts_program_cmdline string! No \'%%s\' in the string!\n"));
		return;
	}

	gchar *eword = g_shell_quote(word);
	gchar *command = g_strdup_printf(str, eword);
	g_free(eword);
	int result;
	result = system(command);
	if (result == -1) {
		g_print(_("system() error!\n"));
	}
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
			filename = build_path(*it, std::string(1, lowerword[0]) + G_DIR_SEPARATOR_S + lowerword + ".wav");
			if (g_file_test(filename.c_str(), G_FILE_TEST_EXISTS)) {
				play_sound_file(filename);
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
