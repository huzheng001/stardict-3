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
	void LoadRealTtsPath(const std::list<std::string>& pathlist);
	ReadWordType canRead(const gchar *word);
	void read(const gchar *word, ReadWordType type);
	void ReadByEngine(const gchar *word, int engine_index);
	bool use_command_tts;
	std::string tts_program_cmdline;
	std::list<std::pair<std::string, int> > GetEngineList();
private:
	bool RealTts_canRead(const gchar *word);
	void RealTts_read(const gchar *word);
	void Command_read(const gchar *word);
	std::list<std::string> ttspath;
};

#endif
