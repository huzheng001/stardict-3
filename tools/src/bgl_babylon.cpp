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

Babylon::Babylon( const std::string& infilename, const std::string& outfilename )
{
	m_filename = infilename;
	file = NULL;
	std::string::size_type pos = outfilename.find_last_of(G_DIR_SEPARATOR);
	if(pos == std::string::npos)
		m_resdirname = "res"; // in the current directory
	else
		m_resdirname = outfilename.substr(0, pos+1) + "res";
	if(g_file_test(m_resdirname.c_str(), G_FILE_TEST_EXISTS))
		if(remove_recursive(m_resdirname))
			g_critical("unable to remove the resource directory: %s", m_resdirname.c_str());
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

#ifdef _WIN32
	/* Looks like Windows does not work with file descriptors correctly.
	gzread always returns -1, gzerror(file, &err) returns "<fd:5>: Bad file descriptor".
	Create a temporary file containing a copy of the .bgl file starting from the gz header.
	We copy all data til the end of the file, that is we copy not only the gz stream, 
	but the data after it. We do not know where the gz stream ends,
	the only way to detect that is uncompressing the stream. */
	{
		if(m_gz_temp_file.create_temp_file().empty())
			return false;
		clib::File tmpf;
		tmpf.reset(fopen(m_gz_temp_file.get_file_name().c_str(), "wb"));
		if(!tmpf)
			return false;
		const size_t buf_size = 1024;
		char buf[buf_size];
		size_t size;
		while((size = fread(buf, 1, buf_size, get_impl(f))))
			if(size != fwrite(buf, 1, size, get_impl(tmpf)))
				return false;
		tmpf.reset(NULL); // close the file
		file = gzopen(m_gz_temp_file.get_file_name().c_str(), "rb");
	}
#else
	file = gzdopen( DUP( FILENO( get_impl(f) ) ), "rb" );
#endif
	if( file == NULL )
		return false;

	return true;
}

void Babylon::close()
{
	gzclose( file );
#ifdef _WIN32
	m_gz_temp_file.clear();
#endif
}

// return value: true - OK, false - error
bool Babylon::readBlock( bgl_block &block )
{
	if( file == NULL || gzeof( file ))
		return false;

	unsigned int length;
	if(!bgl_readnum( 1, length ))
		return false;
	block.type = length & 0xf;
	length >>= 4;
	if(length < 4) {
		if(!bgl_readnum( length + 1, length ))
			return false;
	} else
		length -= 4;
	if( length )
	{
		block.data.resize(length);
		int res = gzread( file, &block.data[0], length );
		if(res == -1) {
			g_critical("gzread error\n");
			return false;
		}
		if(res != (int) length) {
			g_critical("gzread: unexpected end of file\n");
			return false;
		}
	} else
		block.data.resize(0);

	return true;
}

// return value: true - OK, false - error
bool Babylon::bgl_readnum( int bytes, unsigned int& val )
{
	unsigned char buf[4];
	val = 0;

	if ( bytes < 1 || bytes > 4 )
		return false;

	int res = gzread( file, buf, bytes );
	if(res == -1) {
		g_critical("gzread error\n");
		return false;
	}
	if(res != bytes) {
		g_critical("gzread: unexpected end of file\n");
		return false;
	}
	for(int i=0;i<bytes;i++)
		val = (val << 8) | buf[i];
	return true;
}

bool Babylon::read(const std::string &source_charset, const std::string &target_charset)
{
	if( file == NULL )
		return false;

	bgl_block block;
	unsigned int pos;
	unsigned int type;
	std::string headword;
	std::string definition;

	m_sourceCharset = source_charset;
	m_targetCharset = target_charset;
	m_numEntries = 0;
	while(true)
	{
		if(!readBlock( block ))
			return false;
		if( block.type == 4 )
			break; // end of file marker
		if(block.data.size() == 0)
			continue;
		headword.clear();
		definition.clear();
		switch( block.type )
		{
		case 0:
			if(block.data.size() < 1)
				break;
			switch( block.data[0] )
			{
			case 8:
				if(block.data.size() < 2)
					break;
				type = (unsigned char)block.data[1];
				if( type > 64 )
					type -= 65;
				if(sizeof(bgl_charset)/sizeof(bgl_charset[0]) <= type)
					break;
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
			if(block.data.size() < 2)
				break;
			pos = 2;
			switch( (unsigned char) block.data[1] )
			{
			case 0x01:
				if(block.data.size() <= pos)
					break;
				m_title.assign(&block.data[pos], block.data.size()-pos);
				break;
			case 0x02:
				if(block.data.size() <= pos)
					break;
				m_author.assign(&block.data[pos], block.data.size()-pos);
				break;
			case 0x03:
				if(block.data.size() <= pos)
					break;
				m_email.assign(&block.data[pos], block.data.size()-pos);
				break;
			case 0x04:
				if(block.data.size() <= pos)
					break;
				m_copyright.assign(&block.data[pos], block.data.size()-pos);
				break;
			case 0x07:
				if(block.data.size() < 6)
					break;
				{
					unsigned int code = (unsigned char)(block.data[5]);
					if(sizeof(bgl_language)/sizeof(bgl_language[0]) <= code)
						break;
					m_sourceLang = bgl_language[code];
				}
				break;
			case 0x08:
				if(block.data.size() < 6)
					break;
				{
					unsigned int code = (unsigned char)(block.data[5]);
					if(sizeof(bgl_language)/sizeof(bgl_language[0]) <= code)
						break;
					m_targetLang = bgl_language[code];
				}
				break;
			case 0x09:
				headword.reserve( block.data.size() - pos );
				for(unsigned int a=pos;a<block.data.size();a++) {
					if (block.data[a] == '\r') {
					} else if (block.data[a] == '\n') {
						headword += "<br>";
					} else {
						headword += block.data[a];
					}
				}
				m_description = headword;
				break;
			case 0x1a:
				if(block.data.size() <= pos)
					break;
				type = (unsigned char)block.data[pos];
				if( type > 64 )
					type -= 65;
				if(sizeof(bgl_charset)/sizeof(bgl_charset[0]) <= type)
					break;
				if (m_sourceCharset.empty())
					m_sourceCharset = bgl_charset[type];
				break;
			case 0x1b:
				if(block.data.size() <= pos)
					break;
				type = (unsigned char)block.data[pos];
				if( type > 64 )
					type -= 65;
				if(sizeof(bgl_charset)/sizeof(bgl_charset[0]) <= type)
					break;
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
	}
	gzseek( file, 0, SEEK_SET );

	convertToUtf8( m_title, BABYLON_TARGET_CHARSET );
	convertToUtf8( m_author, BABYLON_TARGET_CHARSET );
	convertToUtf8( m_email, BABYLON_TARGET_CHARSET );
	convertToUtf8( m_copyright, BABYLON_TARGET_CHARSET );
	convertToUtf8( m_description, BABYLON_TARGET_CHARSET );
	g_message("Default charset: %s\nSource Charset: %s\nTargetCharset: %s\n",
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
	std::vector<std::string> alternates;
	std::string alternate;

	while( true )
	{
		if(!readBlock( block ))
			break;
		if( block.type == 4 )
			break; // end of file marker
		switch( block.type )
		{
		case 2:
			{
				if(block.data.size() < 1)
					break;
				pos = 0;
				len = (unsigned char)block.data[pos];
				++pos;
				if(block.data.size() < pos + len)
					break;
				std::string filename(&block.data[pos], len);
				pos += len;
				if (filename != "8EAF66FD.bmp" && filename != "C2EEF3F6.html") {
					filename = build_path(m_resdirname, filename);
					int size = block.data.size() - pos;
					if(size <= 0)
						break;
					FILE *ifile = g_fopen(filename.c_str(), "wb");
					fwrite(&block.data[pos], 1, size, ifile);
					fclose(ifile);
				}
				break;
			}
		case 1:
		case 10:
			alternate.clear();
			headword.clear();
			definition.clear();
			pos = 0;

			// Headword
			if(block.data.size() < 1)
				break;
			len = (unsigned char)block.data[pos];
			++pos;
			if(block.data.size() < pos+len)
				break;

			headword.assign(&block.data[pos], len);
			pos += len;
			convertToUtf8( headword, BABYLON_SOURCE_CHARSET );

			// Definition
			if(block.data.size() < pos + 2)
				break;
			len = 0;
			len = (unsigned char)block.data[pos] << 8;
			len |= (unsigned char)block.data[pos+1];
			pos += 2;
			if(block.data.size() < pos + len)
				break;
			definition.reserve( len );
			for(unsigned int a=pos; a<pos+len; a++)
			{
				unsigned char x = (unsigned char)block.data[a];
				if( x < 0x20 ) {
					if ( x == 0x14 ) {
						if( a <= pos+len - 3 && (unsigned char)block.data[a+1] == 0x02 ) {
							unsigned int index = (unsigned char)block.data[a+2] - 0x30;
							if (index < sizeof(partOfSpeech)/sizeof(partOfSpeech[0])) {
								definition = "<font color=\"blue\">" + partOfSpeech[index] + "</font> " + definition;
							}
						}
						break;
					} else if(x == 0x09) { // '\t'
						definition += block.data[a];
					} else if( x == 0x0a ) { // '\n'
						definition += "<br>";
					} // else ignore the control char
				} else 
					definition += block.data[a];
			}
			pos += len;
			convertToUtf8( definition, BABYLON_TARGET_CHARSET );

			// Alternate forms
			while( pos < block.data.size() )
			{
				len = (unsigned char)block.data[pos++];
				if(block.data.size() < pos + len)
					break;
				alternate.assign(&block.data[pos], len);
				pos += len;
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
	}
	entry.headword = "";
	return entry;
}

void Babylon::convertToUtf8( std::string &s, BABYLON_CHARSET type )
{
	if( s.empty() )
		return;
	if( type > 2 )
		return;

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
		g_critical("Error opening iconv library\n");
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
			//g_critical("\n%s\n", inbuf);
			g_critical("Error in iconv conversion\n");
			inbuf++;
			inbufbytes--;
		}
	}
	s = std::string( defbuf );

	free( defbuf );
	iconv_close( cd );
}
