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
#ifndef DICTBUILDER_H
#define DICTBUILDER_H

#include <string>
#include <vector>


class DictBuilder
{

public:

  virtual bool addHeadword( const std::string& word, const std::string& def,
    const std::vector<std::string>& alternates = std::vector<std::string>() ){ return true; };

  virtual bool finish(){ return true; };

  virtual std::string filename(){ return std::string(); };
  virtual void setTitle( const std::string& /*title*/ ){};
  virtual std::string title(){ return std::string(); };
  virtual void setAuthor( const std::string& /*author*/ ){};
  virtual std::string author(){ return std::string(); };
  virtual void setLicense( const std::string& /*license*/ ){};
  virtual std::string license(){ return std::string(); };
  virtual void setOrigLang( const std::string& /*origLang*/ ){};
  virtual std::string origLang(){ return std::string(); };
  virtual void setDestLang( const std::string& /*destLang*/ ){};
  virtual std::string destLang(){ return std::string(); };
  virtual void setDescription( const std::string& /*description*/ ){};
  virtual std::string description(){ return std::string(); };
  virtual void setComments( const std::string& /*comments*/ ){};
  virtual std::string comments(){ return std::string(); };
  virtual void setEmail( const std::string& /*email*/ ){};
  virtual std::string email(){ return std::string(); };
  virtual void setWebsite( const std::string& /*website*/ ){};
  virtual std::string website(){ return std::string(); };
  virtual void setVersion( const std::string& /*version*/ ){};
  virtual std::string version(){ return std::string(); };
  virtual void setCreationDate( const std::string& /*creationDate*/ ){};
  virtual std::string creationDate(){ return std::string(); };
  virtual void setLastUpdate( const std::string& /*lastUpdate*/ ){};
  virtual std::string lastUpdate(){ return std::string(); };
  virtual void setMisc( const std::string& /*misc*/ ){};
  virtual std::string misc(){ return std::string(); };
  virtual unsigned int headwords(){ return 0; };
  virtual unsigned int words(){ return 0; };

protected:
  DictBuilder(){};
  virtual ~DictBuilder(){};

};

#endif // DICTBUILDER_H
