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

#include "bgl_babylonreader.h"
#include "bgl_babylon.h"
#include "bgl_dictbuilder.h"

#include <stdio.h>
#include <glib/gstdio.h>

BabylonReader::BabylonReader( const std::string& infilename, const std::string& outfilename,
  DictBuilder *builder )
{
	m_babylon = new Babylon( infilename, outfilename );
	m_builder = builder;
}

BabylonReader::~BabylonReader()
{
	delete m_babylon;
}

bool BabylonReader::convert(const std::string &source_charset, const std::string &target_charset)
{
	if( !m_babylon->open() )
	{
		g_critical("Error opening %s\n", m_babylon->filename().c_str());
		return false;
	}

	if( !m_babylon->read(source_charset, target_charset) )
	{
		g_critical("Error reading %s\n", m_babylon->filename().c_str());
		return false;
	}

	m_builder->setTitle( m_babylon->title() );
	m_builder->setAuthor( m_babylon->author() );
	m_builder->setEmail( m_babylon->email() );
	m_builder->setLicense( m_babylon->copyright() );
	m_builder->setOrigLang( m_babylon->sourceLang() );
	m_builder->setDestLang( m_babylon->targetLang() );
	m_builder->setDescription( m_babylon->description() );

	bgl_entry entry;
	entry = m_babylon->readEntry();

	int n = 0;
	while( entry.headword != "" )
	{
		m_builder->addHeadword( entry.headword.c_str(), entry.definition.c_str(), entry.alternates );
		entry = m_babylon->readEntry();
		n++;
	}

	m_babylon->close();

	return true;
}
