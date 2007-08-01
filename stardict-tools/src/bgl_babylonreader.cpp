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

#include "bgl_babylonreader.h"
#include "bgl_babylon.h"
#include "bgl_dictbuilder.h"

#include <stdio.h>
#include <glib/gstdio.h>

BabylonReader::BabylonReader( std::string filename, DictBuilder *builder )
{
  m_babylon = new Babylon( filename );
  m_builder = builder;
}


bool BabylonReader::convert(std::string &source_charset, std::string &target_charset)
{
  if( !m_babylon->open() )
  {
    printf( "Error openning %s\n", m_babylon->filename().c_str() );
    return false;
  }

  if( !m_babylon->read(source_charset, target_charset) )
  {
    printf( "Error reading %s\n", m_babylon->filename().c_str() );
    return false;
  }

  m_builder->setTitle( m_babylon->title() );
  m_builder->setAuthor( m_babylon->author() );
  m_builder->setEmail( m_babylon->email() );
  m_builder->setLicense( m_babylon->copyright() );
  m_builder->setOrigLang( m_babylon->sourceLang() );
  m_builder->setDestLang( m_babylon->targetLang() );
  m_builder->setDescription( m_babylon->description() );

#ifdef _WIN32
	g_mkdir("res", S_IRWXU);
#else
  system("rm -rf res;mkdir res");
#endif
  bgl_entry entry;
  entry = m_babylon->readEntry();

  int n = 0;
  while( entry.headword != "" )
  {
    m_builder->addHeadword( entry.headword.c_str(), entry.definition.c_str(), entry.alternates );
    entry = m_babylon->readEntry();
    n++;
    if (n%100 == 1) {
      printf( "." );
      fflush(stdout);
    }
  }
  printf( "\n" );

  m_babylon->close();

  return true;
}
