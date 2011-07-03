/*
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

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <list>
#include <string>
#include <vector>

#include <libgen.h>
#include <glib.h>
#include <cstring>

#include <ctype.h>
#include <arpa/inet.h>

#include "dictbuilder-tree.h"

struct sectionEntry
{
    char   sign;
};

struct indexEntry
{
    std::string word;
    size_t      offset;
    size_t      size;
};

typedef std::list<indexEntry> entrylist_type;
typedef tree<indexEntry*>     entrytree_type;

struct _core
{
    std::string input;

    std::string ifofile;
    std::string tfofile;
    std::string idxfile;
    std::string tdxfile;
    std::string dicfile;
    std::string tmpfile;

    char endl;
} core;

std::ofstream dictfs;
std::ofstream tmpfs;

sectionEntry section;
indexEntry  *entry;

entrylist_type entrylist;
entrytree_type entrytree;

void initsection();
void initentry();

bool entrycmp(const indexEntry& left,
              const indexEntry& right);

void help();

bool vaildSection();
bool vaildEntry();

void writeSection();
void writeEntry();

void writeTreeindex(entrytree_type::iterator it, std::ostream& os);

void action(std::istream& is);

int main(int argc, char* argv[])
{
    core.endl = '\r';

    for (int index = 1; index != argc ; index++)
    {
        if (strcasecmp(argv[index], "-o") == 0 ||
            strcasecmp(argv[index], "--output") == 0)
        {
            if (++index != argc)
            {
                std::string output = argv[index];
                core.ifofile = output + ".ifo";
                core.tfofile = output + ".tfo";
                core.idxfile = output + ".idx";
                core.tdxfile = output + ".tdx";
                core.dicfile = output + ".dict";
                core.dicfile = output + ".tmp";
            }
            continue;
        }

        if (strcasecmp(argv[index], "-n") == 0)
        {
            core.endl = '\n';
            continue;
        }

        if (strcasecmp(argv[index], "-h") == 0 ||
            strcasecmp(argv[index], "--help") == 0)
        {
            help();
            return 1;
        }

        core.input = argv[index];
    }

    if (core.input.size() == 0)
    {
        std::cerr << "must specify a input file." << std::endl;
        help();
        return 1;
    }

    if (core.dicfile.size() == 0)
    {
        std::string filename = basename((char*) core.input.c_str());
        std::string output;
        std::string::size_type pos = filename.find_last_of('.');
        if (pos != std::string::npos)
        {
            output = core.input.substr(0,
                core.input.size() - (filename.size() - pos));
            std::cout << output << std::endl;
        } else
        output = core.input;

        core.ifofile = output + ".ifo";
        core.tfofile = output + ".tfo";
        core.idxfile = output + ".idx";
        core.tdxfile = output + ".tdx";
        core.dicfile = output + ".dict";
        core.tmpfile = output + ".tmp";
    }

    std::ifstream ifs(core.input.c_str());
    if (!ifs.is_open())
    {
        std::cerr << "can't read file: " << core.input << std::endl;
        return 1;
    }

    dictfs.open(core.dicfile.c_str(),
        std::ios_base::out|std::ios_base::trunc|std::ios_base::binary);
    if (!dictfs.is_open())
    {
        std::cerr << "cant's create dict file: " << core.dicfile << std::endl;
        return 1;
    }

    *entrytree.root() = NULL;

    initentry();
    initsection();

    action(ifs);

    if (vaildSection())
        writeSection();
    if (vaildEntry())
        writeEntry();

    ifs.close();
    dictfs.close();

    size_t idx_size = 0, tdx_size = 0;

    {//for idx file
        std::ofstream ofs(core.idxfile.c_str(),
            std::ios_base::out|std::ios_base::trunc|std::ios_base::binary);
        if (!ofs.is_open())
        {
            std::cerr << "cant's create idx file: "
                      << core.idxfile << std::endl;
            return 1;
        }

        entrylist.sort(entrycmp);
        char zero = '\0';

        for(entrylist_type::const_iterator it = entrylist.begin();
            it != entrylist.end(); it++)
        {
            ofs.write(it->word.c_str(), it->word.size());
            ofs.write(&zero, 1);
            ofs.write((char*) &it->offset, sizeof(it->offset));
            ofs.write((char*) &it->size, sizeof(it->size));
        }

        idx_size = ofs.tellp();
        ofs.close();
    }

    if (entrytree.root().size() != 0)
    {//for tdx file;
        std::ofstream ofs(core.tdxfile.c_str(),
            std::ios_base::out|std::ios_base::trunc|std::ios_base::binary);
        if (!ofs.is_open())
        {
            std::cerr << "cant's create tdx file: "
                      << core.idxfile << std::endl;
            return 1;
        }

        writeTreeindex(entrytree.root(), ofs);

        tdx_size = ofs.tellp();
        ofs.close();
    }

    {//for ifo file;
        std::ofstream ofs(core.ifofile.c_str(),
            std::ios_base::out|std::ios_base::trunc);
        if (!ofs.is_open())
        {
            std::cerr << "cant's create ifo file: "
                      << core.ifofile << std::endl;
            return 1;
        }

        ofs << "StarDict's dict ifo file" << std::endl;
        ofs << "version=2.4.2" << std::endl;
        ofs << "bookname=" << core.input << std::endl;
        ofs << "wordcount=" << entrylist.size() << std::endl;
        ofs << "idxfilesize=" << idx_size << std::endl;
        ofs << "author=" << std::endl;
        ofs << "email=" << std::endl;
        ofs << "website=" << std::endl;
        ofs << "description=" << std::endl;
        ofs << "date=" << std::endl;

        ofs.close();
    }

    if (tdx_size != 0)
    {//for tfo file;
        std::ofstream ofs(core.tfofile.c_str(),
            std::ios_base::out|std::ios_base::trunc);
        if (!ofs.is_open())
        {
            std::cerr << "cant's create tfo file: "
                      << core.ifofile << std::endl;
            return 1;
        }

        ofs << "StarDict's treedict ifo file" << std::endl;
        ofs << "version=2.4.2" << std::endl;
        ofs << "bookname=" << core.input << std::endl;
        ofs << "wordcount=" << entrylist.size() << std::endl;
        ofs << "tdxfilesize=" << tdx_size << std::endl;
        ofs << "author=" << std::endl;
        ofs << "email=" << std::endl;
        ofs << "website=" << std::endl;
        ofs << "description=" << std::endl;
        ofs << "date=" << std::endl;

        ofs.close();
    }

    return 0;
}

gint stardict_strcmp(const gchar *s1, const gchar *s2)
{
    int ret = g_ascii_strcasecmp(s1, s2);
    if (ret == 0)
        return strcmp(s1, s2);
    else
        return ret;
}

bool entrycmp(const indexEntry& left,
              const indexEntry& right)
{
    return stardict_strcmp(left.word.c_str(), right.word.c_str()) < 0;
}

void initsection()
{
    section.sign = '\0';
}

void initentry()
{
    entry = NULL;
}

bool vaildSection()
{
    return section.sign != '\0';
}

bool vaildEntry()
{
    return entry != NULL;
}

void writeSection()
{
    if (tmpfs.is_open())
    {
        size_t size = htonl(tmpfs.tellp());
        tmpfs.close();
        tmpfs.clear();

        dictfs.write((char*) &size, sizeof(size));
        std::ifstream ifs(core.tmpfile.c_str(),
            std::ios_base::in|std::ios_base::binary);
        std::istreambuf_iterator<char> begin(ifs), end;
        std::copy(begin, end, std::ostreambuf_iterator<char>(dictfs));
        ifs.close();
    } else
    {
        char zero = '\0';
        dictfs.write(&zero, 1);
    }

    initsection();
}

void writeEntry()
{
    entry->size = size_t(dictfs.tellp()) - entry->offset;
    entry->offset = htonl(entry->offset);
    entry->size   = htonl(entry->size);

    initentry();
}

void action(std::istream& is)
{
    std::string line;

    while(std::getline(is, line))
    {
        switch(line[0])
        {
        case '%':
            {
                if (line[1] == '%')
                {
                    //一个词条的开始
                    if (vaildSection())
                        writeSection();
                    if (vaildEntry())
                        writeEntry();

                    entrylist.resize(entrylist.size() + 1);
                    entry = &entrylist.back();

                    entry->word   = line.substr(2);
                    entry->offset = dictfs.tellp();
                    entry->size   = 0;

                    if (entry->word.size() > 256)
                    {
                        std::cerr << "too long word which longer than 256, trunc to 255." << std::endl;
                        entry->word.resize(255);
                    }

                    std::cerr << "word: " << section.sign << core.endl;
                } else
                {//否则是一个段落的开始
                    if (vaildSection())
                        writeSection();
                    section.sign = line[1];
                    if (vaildSection())
                    {
                        if (isupper(section.sign))
                        {
                            tmpfs.open(core.tmpfile.c_str(),
                                std::ios_base::out|std::ios_base::trunc|
                                std::ios_base::binary);
                            if (!tmpfs.is_open())
                            {
                                std::cerr << "can't create tmp file: "
                                          << core.tmpfile << std::endl;
                                exit(1);
                            }
                        }
                        dictfs.write(&section.sign, 1);
                    }
                }
            }
            continue;

        case '+':
            {
                std::string path = line.substr(1), node;
                std::string::size_type pos, oldpos = 0;
                entrytree_type::iterator it = entrytree.root();

                typedef std::vector<std::string> path_type;
                path_type paths;

                do
                {
                    pos = path.find(':', oldpos);
                    node = path.substr(oldpos, pos);

                    paths.push_back(node);
                    oldpos = pos + 1;
                } while(pos != std::string::npos);

                if (paths.size() != 0)
                {
                    if (*it == NULL || (*it)->word != paths[0])
                    {
                        if (*it != NULL && (*it)->size == 0)
                            delete *it;

                        indexEntry* tmpentry = new indexEntry;
                        tmpentry->word   = paths[0];
                        tmpentry->offset = 0;
                        tmpentry->size   = 0;

                        *it = tmpentry;
                    }

                    for(path_type::size_type index = 1;
                        index < paths.size(); index++)
                    {
                        entrytree_type::iterator child;

                        for(child = it.begin(); child != it.end(); child++)
                        {
                            if ((*child)->word == paths[index])
                                break;
                        }

                        if (child == it.end())
                        {
                            indexEntry* tmpentry = new indexEntry;
                            tmpentry->word   = paths[index];
                            tmpentry->offset = 0;
                            tmpentry->size   = 0;

                            child = it.append(tmpentry);
                        }

                        it = child;
                    }

                    {
                        entrytree_type::iterator child;

                        for(child = it.begin(); child != it.end(); child++)
                        {
                            if ((*child)->word == entry->word)
                                break;
                        }

                        if (child == it.end())
                        {
                            it.append(entry);
                        } else
                        {
                        	if ((*child)->size == 0)
                        		delete *child;
							*child = entry;
                        }
                    }

                } else
                {
                    *it = entry;
                }

                continue;
            }
            continue;

        case '!':
            {
                std::string filename = line.substr(1);
                std::ifstream ifs(filename.c_str(),
                    std::ios_base::in|std::ios_base::binary);
                if (ifs.is_open())
                {
                    std::istreambuf_iterator<char> begin(ifs), end;
                    std::copy(begin, end,
                        std::ostreambuf_iterator<char>(
                            tmpfs.is_open()?tmpfs:dictfs));
                    ifs.close();
                } else
                std::cerr << "can't read file: " << filename << std::endl;
            }
            continue;

        case '^':
            {
                std::string filename = line.substr(1);
                std::ifstream ifs(filename.c_str());
                if (ifs.is_open())
                {
                    action(ifs);
                    ifs.close();
                } else
                std::cerr << "can't read file: " << filename << std::endl;
            }
            continue;

        case '#':
            continue;
        }

        if (!vaildSection())
            continue;

        std::ostream& ofs = tmpfs.is_open()?tmpfs:dictfs;
        if (line.size() > 0 &&
            line[line.size() - 1] == '\\')
            line.resize(line.size() - 1);
        else
            line.push_back('\n');

        ofs.write(line.c_str(), line.size());
    }
}

void writeTreeindex(entrytree_type::iterator it, std::ostream& os)
{
    indexEntry* entry = *it;
    char zero = '\0';
    size_t size = htonl(it.size());

    os.write(entry->word.c_str(), entry->word.size());
    os.write(&zero , 1);
    os.write((char*) &entry->offset, sizeof(entry->offset));
    os.write((char*) &entry->size, sizeof(entry->size));
    os.write((char*) &size, sizeof(size));

    for (entrytree_type::iterator child = it.begin();
         child != it.end(); child++)
        writeTreeindex(child, os);
}

void help()
{
    std::cout << "dictbuilder [-o output] dictfile" << std::endl;
}
