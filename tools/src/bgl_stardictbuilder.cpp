/***************************************************************************
 *   Copyright (C) 2007 by Raul Fernandes                                  *
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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <iostream>
#include <glib.h>
#include <cstring>
#include "bgl_stardictbuilder.h"

StarDictBuilder::StarDictBuilder( const std::string& outfilename )
{
  m_babylonfilename = outfilename;
  file.open( m_babylonfilename.c_str() );
  m_entriescount = 0;
}

StarDictBuilder::~StarDictBuilder()
{
}

bool StarDictBuilder::addHeadword( const std::string& word, const std::string& def,
		const std::vector<std::string>& alternates )
{
  if (m_entriescount == 0) {
    file.write("\n", 1);
    std::string line;
    line = "#stripmethod=keep\n#sametypesequence=h\n";
    file.write(line.data(), line.length());
    if (!m_title.empty()) {
      line = "#bookname=";
      line += m_title;
      line += '\n';
      file.write(line.data(), line.length());
    }
    if (!m_author.empty()) {
      line = "#author=";
      line += m_author;
      line += '\n';
      file.write(line.data(), line.length());
    }
    if (!m_email.empty()) {
      line = "#email=";
      line += m_email;
      line += '\n';
      file.write(line.data(), line.length());
    }
    if (!m_website.empty()) {
      line = "#website=";
      line += m_website;
      line += '\n';
      file.write(line.data(), line.length());
    }
    if (!m_description.empty()) {
      line = "#description=";
      line += m_description;
      line += '\n';
      file.write(line.data(), line.length());
    }
    file.write("\n", 1);
  }
  m_entriescount++;
  std::string headword;
  int len = word.length();
  if (word[len-1]=='$') {
    const char *p = word.c_str();
    const char *p1 = strchr(p, '$');
    if (p1) {
      headword.assign(p, p1-p);
    } else {
      headword = word;
    }
  } else {
    headword = word;
  }
  std::string lines;
  lines = headword;
  for (std::vector<std::string>::const_iterator i = alternates.begin(); i != alternates.end(); ++i) {
    lines += '|';
    lines += *i;
  }
  lines += '\n';
  lines += def;
  lines += "\n\n";
  file.write(lines.data(), lines.length());

  return true;
}

bool StarDictBuilder::finish()
{
  file.close();
  g_message("Write file: %s\n\nBookname: %s\nWord count: %d\nAuthor: %s\nEmail: %s\nWebsite: %s\nDescription: %s\n",
    m_babylonfilename.c_str(), m_title.c_str(), m_entriescount, m_author.c_str(),
    m_email.c_str(), m_website.c_str(), m_description.c_str());

  return true;
}
