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

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <glib/gstdio.h>
#include <iconv.h>
#include <cstring>

#ifdef _WIN32
#include <io.h>
#define DUP _dup
#define FILENO _fileno
#else
#define DUP dup
#define FILENO fileno
#endif

#include "bgl_babylon.h"
#include "resourcewrap.hpp"
#include "libcommon.h"


Babylon::Babylon( const std::string& infilename, const std::string& outfilename,
		print_info_t print_info)
{
	m_filename = infilename;
	file = NULL;
	this->print_info = print_info;
	std::string::size_type pos = outfilename.find_last_of(G_DIR_SEPARATOR);
	if(pos == std::string::npos)
		m_resdirname = "res"; // in current directory
	else
		m_resdirname = outfilename.substr(0, pos+1) + "res";
	if(g_file_test(m_resdirname.c_str(), G_FILE_TEST_EXISTS))
		if(remove_recursive(m_resdirname))
			print_info("unable to remove the resource directory: %s", m_resdirname.c_str());
	g_mkdir_with_parents(m_resdirname.c_str(), S_IRWXU);
}

Babylon::~Babylon()
{
}

bool Babylon::open()
{
	clib::File f;
	unsigned char buf[6];
	int i;

	f.reset(g_fopen( m_filename.c_str(), "rb" ));
	if(!f)
		return false;

	i = fread( buf, 1, 6, get_impl(f) );

	/* First four bytes: BGL signature 0x12340001 or 0x12340002 (big-endian) */
	if( i < 6 || memcmp( buf, "\x12\x34\x00", 3 ) || buf[3] == 0 || buf[3] > 2 )
		return false;

	/* Calculate position of gz header */

	i = buf[4] << 8 | buf[5];

	if( i < 6 )
		return false;

	if( fseek( get_impl(f), i, SEEK_SET ) ) /* can't seek - emulate */
		for(int j=0;j < i - 6;j++) fgetc( get_impl(f) );

	if( ferror( get_impl(f) ) || feof( get_impl(f) ) )
		return false;

	/* we need to flush the file because otherwise some nfs mounts don't seem
	* to properly update the file position for the following reopen */

	fflush( get_impl(f) );

	file = gzdopen( DUP( FILENO( get_impl(f) ) ), "rb" );
	if( file == NULL )
		return false;

	return true;
}

void Babylon::close()
{
	gzclose( file );
}

bool Babylon::readBlock( bgl_block &block )
{
	if( file == NULL || gzeof( file ))
		return false;

	block.length = bgl_readnum( 1 );
	block.type = block.length & 0xf;
	if( block.type == 4 ) return false; // end of file marker
	block.length >>= 4;
	block.length = block.length < 4 ? bgl_readnum( block.length + 1 ) : block.length - 4 ;
	if( block.length )
	{
		block.data = (char *)malloc( block.length );
		gzread( file, block.data, block.length );
	}

	return true;
}

unsigned int Babylon::bgl_readnum( int bytes )
{
	unsigned char buf[4];
	unsigned val = 0;

	if ( bytes < 1 || bytes > 4 ) return (0);

	gzread( file, buf, bytes );
	for(int i=0;i<bytes;i++) val= (val << 8) | buf[i];
	return val;
}

bool Babylon::read(const std::string &source_charset, const std::string &target_charset)
{
	if( file == NULL ) return false;

	bgl_block block;
	unsigned int pos;
	unsigned int type;
	std::string headword;
	std::string definition;

	m_sourceCharset = source_charset;
	m_targetCharset = target_charset;
	m_numEntries = 0;
	while( readBlock( block ) )
	{
		headword.clear();
		definition.clear();
		switch( block.type )
		{
		case 0:
			switch( block.data[0] )
			{
			case 8:
				type = (unsigned int)block.data[2];
				if( type > 64 ) type -= 65;
				m_defaultCharset = bgl_charset[type];
				break;
			default:
				break;
			}
			break;
		case 1:
		case 10:
			// Only count entries
			m_numEntries++;
			break;
		case 3:
			pos = 2;
			switch( block.data[1] )
			{
			case 1:
				headword.reserve( block.length - 2 );
				for(unsigned int a=0;a<block.length-2;a++) headword += block.data[pos++];
				m_title = headword;
				break;
			case 2:
				headword.reserve( block.length - 2 );
				for(unsigned int a=0;a<block.length-2;a++) headword += block.data[pos++];
				m_author = headword;
				break;
			case 3:
				headword.reserve( block.length - 2 );
				for(unsigned int a=0;a<block.length-2;a++) headword += block.data[pos++];
				m_email = headword;
				break;
			case 4:
				headword.reserve( block.length - 2 );
				for(unsigned int a=0;a<block.length-2;a++) headword += block.data[pos++];
				m_copyright = headword;
				break;
			case 7:
				headword = bgl_language[(unsigned char)(block.data[5])];
				m_sourceLang = headword;
				break;
			case 8:
				headword = bgl_language[(unsigned char)(block.data[5])];
				m_targetLang = headword;
				break;
			case 9:
				headword.reserve( block.length - 2 );
				for(unsigned int a=0;a<block.length-2;a++) {
					if (block.data[pos] == '\r') {
					} else if (block.data[pos] == '\n') {
						headword += "<br>";
					} else {
						headword += block.data[pos];
					}
					pos++;
				}
				m_description = headword;
				break;
			case 26:
				type = (unsigned int)block.data[2];
				if( type > 64 ) type -= 65;
				if (m_sourceCharset.empty())
					m_sourceCharset = bgl_charset[type];
				break;
			case 27:
				type = (unsigned int)block.data[2];
				if( type > 64 ) type -= 65;
				if (m_targetCharset.empty())
					m_targetCharset = bgl_charset[type];
				break;
			default:
				break;
			}
			break;
		default:
			;
		}
		if( block.length ) free( block.data );
	}
	gzseek( file, 0, SEEK_SET );

	convertToUtf8( m_title, BABYLON_TARGET_CHARSET );
	convertToUtf8( m_author, BABYLON_TARGET_CHARSET );
	convertToUtf8( m_email, BABYLON_TARGET_CHARSET );
	convertToUtf8( m_copyright, BABYLON_TARGET_CHARSET );
	convertToUtf8( m_description, BABYLON_TARGET_CHARSET );
	print_info("Default charset: %s\nSource Charset: %s\nTargetCharset: %s\n",
		m_defaultCharset.c_str(), m_sourceCharset.c_str(), m_targetCharset.c_str());
	return true;
}

bgl_entry Babylon::readEntry()
{
	bgl_entry entry;

	if( file == NULL )
	{
		entry.headword = "";
		return entry;
	}

	bgl_block block;
	unsigned int len, pos;
	std::string headword;
	std::string definition;
	std::string temp;
	std::vector<std::string> alternates;
	std::string alternate;

	while( readBlock( block ) )
	{
		switch( block.type )
		{
		case 2:
			{
				pos = 0;
				len = (unsigned char)block.data[pos++];
				std::string filename(block.data+pos, len);
				if (filename != "8EAF66FD.bmp" && filename != "C2EEF3F6.html") {
					filename = m_resdirname + G_DIR_SEPARATOR_S + filename;
					pos += len;
					FILE *ifile = g_fopen(filename.c_str(), "w");
					fwrite(block.data + pos, 1, block.length -pos, ifile);
					fclose(ifile);
				}
				break;
			}
		case 1:
		case 10:
			alternate.clear();
			headword.clear();
			definition.clear();
			temp.clear();
			pos = 0;

			// Headword
			len = 0;
			len = (unsigned char)block.data[pos++];

			headword.reserve( len );
			for(unsigned int a=0;a<len;a++) headword += block.data[pos++];
			convertToUtf8( headword, BABYLON_SOURCE_CHARSET );

			// Definition
			len = 0;
			len = (unsigned char)block.data[pos++] << 8;
			len |= (unsigned char)block.data[pos++];
			definition.reserve( len );
			for(unsigned int a=0;a<len;a++)
			{
				if( (unsigned char)block.data[pos] == 0x0a )
				{
					definition += "<br>";
					pos++;
				}else if( (unsigned char)block.data[pos] < 0x20 )
				{
					if( a <= len - 3 && block.data[pos] == 0x14 && block.data[pos+1] == 0x02 ) {
						int index = (unsigned char)block.data[pos+2] - 0x30;
						if (index <= 10) {
							definition = "<font color=\"blue\">" + partOfSpeech[index] + "</font> " + definition;
						}
						pos += len - a;
						break;
					} else if (block.data[pos] == 0x14) {
						pos++;
					} else {
						definition += block.data[pos++];
					}
				}else definition += block.data[pos++];
			}
			convertToUtf8( definition, BABYLON_TARGET_CHARSET );

			// Alternate forms
			while( pos != block.length )
			{
				len = (unsigned char)block.data[pos++];
				alternate.reserve( len );
				for(unsigned int a=0;a<len;a++) alternate += block.data[pos++];
				convertToUtf8( alternate, BABYLON_SOURCE_CHARSET );
				alternates.push_back( alternate );
				alternate.clear();
			}

			entry.headword = headword;
			entry.definition = definition;
			entry.alternates = alternates;
			return entry;

			break;
		default:
			;
		}
		if( block.length ) free( block.data );
	}
	entry.headword = "";
	return entry;
}

void Babylon::convertToUtf8( std::string &s, unsigned int type )
{
	if( s.size() < 1 ) return;
	if( type > 2 ) return;

	std::string charset;
	switch( type )
	{
	case BABYLON_DEFAULT_CHARSET:
		if(!m_defaultCharset.empty()) charset = m_defaultCharset;
		else charset = m_sourceCharset;
		break;
	case BABYLON_SOURCE_CHARSET:
		if(!m_sourceCharset.empty()) charset = m_sourceCharset;
		else charset = m_defaultCharset;
		break;
	case BABYLON_TARGET_CHARSET:
		if(!m_targetCharset.empty()) charset = m_targetCharset;
		else charset = m_defaultCharset;
		break;
	default:
		;
	}

	iconv_t cd = iconv_open( "UTF-8", charset.c_str() );
	if( cd == (iconv_t)(-1) )
	{
		print_info( "Error opening iconv library\n" );
		exit(1);
	}

	char *outbuf, *defbuf;
	size_t inbufbytes, outbufbytes;

	inbufbytes = s.size();
	outbufbytes = s.size() * 6;
#ifdef _WIN32
	const char *inbuf;
	inbuf = s.data();
#else
	ICONV_CONST char *inbuf;
	inbuf = (ICONV_CONST char *)s.data();
#endif
	outbuf = (char*)malloc( outbufbytes + 1 );
	memset( outbuf, '\0', outbufbytes + 1 );
	defbuf = outbuf;
	while (inbufbytes) {
		if (iconv(cd, &inbuf, &inbufbytes, &outbuf, &outbufbytes) == (size_t)-1) {
			print_info( "\n%s\n", inbuf );
			print_info( "Error in iconv conversion\n" );
			inbuf++;
			inbufbytes--;
		}
	}
	s = std::string( defbuf );

	free( defbuf );
	iconv_close( cd );
}
