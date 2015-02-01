/***************************************************************************
 *   Copyright (C) 2007 by Raul Fernandes and Karl Grill                   *
 *   rgbr@yahoo.com.br                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

#ifndef BABYLON_H
#define BABYLON_H

#include <stdlib.h>
#include <zlib.h>

#include <string>
#include <vector>
#include "libcommon.h"

const std::string bgl_language[] = {
	"English", 
	"French",
	"Italian",
	"Spanish",
	"Dutch",
	"Portuguese",
	"German",
	"Russian",
	"Japanese",
	"Traditional Chinese",
	"Simplified Chinese",
	"Greek",
	"Korean",
	"Turkish",
	"Hebrew",
	"Arabic",
	"Thai",
	"Other",
	"Other Simplified Chinese dialects",
	"Other Traditional Chinese dialects",
	"Other Eastern-European languages",
	"Other Western-European languages",
	"Other Russian languages",
	"Other Japanese languages",
	"Other Baltic languages",
	"Other Greek languages",
	"Other Korean dialects",
	"Other Turkish dialects",
	"Other Thai dialects",
	"Polish",
	"Hungarian",
	"Czech",
	"Lithuanian",
	"Latvian",
	"Catalan",
	"Croatian",
	"Serbian",
	"Slovak",
	"Albanian",
	"Urdu",
	"Slovenian",
	"Estonian",
	"Bulgarian",
	"Danish",
	"Finnish",
	"Icelandic",
	"Norwegian",
	"Romanian",
	"Swedish",
	"Ukrainian",
	"Belarusian",
	"Farsi",
	"Basque",
	"Macedonian",
	"Afrikaans",
	"Faeroese",
	"Latin",
	"Esperanto",
	"Tamazight",
	"Armenian"
};


const std::string bgl_charsetname[] = {
	"Default" ,
	"Latin",
	"Eastern European",
	"Cyrillic",
	"Japanese",
	"Traditional Chinese",
	"Simplified Chinese",
	"Baltic",
	"Greek",
	"Korean",
	"Turkish",
	"Hebrew",
	"Arabic",
	"Thai"
};

const std::string bgl_charset[] = {
	"cp1252",       // Default                0x41
	"cp1252",       // Latin                    42
	"cp1250",       // Eastern European         43
	"cp1251",       // Cyrillic                 44
	"cp932",        // Japanese                 45
	"cp950",        // Traditional Chinese      46
	"cp936",        // Simplified Chinese       47
	"cp1257",       // Baltic                   48
	"cp1253",       // Greek                    49
	"cp949",        // Korean                   4A
	"cp1254",       // Turkish                  4B
	"cp1255",       // Hebrew                   4C
	"cp1256",       // Arabic                   4D
	"cp874"         // Thai                     4E
};

const std::string partOfSpeech[] = {
	"n.",
	"adj.",
	"v.",
	"adv.",
	"interj.",
	"pron.",
	"prep.",
	"conj.",
	"suff.",
	"pref.",
	"art."
};

struct bgl_block {
	unsigned type;
	std::vector<char> data;
};

struct bgl_entry {
	std::string headword;
	std::string definition;
	std::vector<std::string> alternates;
};

class Babylon
{
public:
	Babylon( const std::string& infilename, const std::string& outfilename );
	~Babylon();

	bool open();
	void close();
	bool readBlock( bgl_block& );
	bool read(const std::string &source_charset, const std::string &target_charset);
	bgl_entry readEntry(void);

	inline std::string title() const { return m_title; };
	inline std::string author() const { return m_author; };
	inline std::string email() const { return m_email; };
	inline std::string description() const { return m_description; };
	inline std::string copyright() const { return m_copyright; };
	inline std::string sourceLang() const { return m_sourceLang; };
	inline std::string targetLang() const { return m_targetLang; };
	inline unsigned int numEntries() const { return m_numEntries; };
	inline std::string charset() const { return m_defaultCharset; };

	inline std::string filename() const { return m_filename; };

private:
	/* Do not use DEFAULT_CHARSET identifier, there is a macro named DEFAULT_CHARSET
	in WinGDI.h on Windows. */
	enum BABYLON_CHARSET { BABYLON_DEFAULT_CHARSET, BABYLON_SOURCE_CHARSET, BABYLON_TARGET_CHARSET };

	bool bgl_readnum( int bytes, unsigned int& val );
	void convertToUtf8( std::string& s, BABYLON_CHARSET type);

	std::string m_filename;
	std::string m_resdirname;
	gzFile file;

	std::string m_title;
	std::string m_author;
	std::string m_email;
	std::string m_description;
	std::string m_copyright;
	std::string m_sourceLang;
	std::string m_targetLang;
	unsigned int m_numEntries;
	std::string m_defaultCharset;
	std::string m_sourceCharset;
	std::string m_targetCharset;
	
#ifdef _WIN32
	TempFile m_gz_temp_file;
#endif
};

#endif // BABYLON_H
