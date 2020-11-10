#!/usr/bin/env python
#
# hanzim2dict
# 
# Original version written by Michael Robinson (robinson@netrinsics.com)
# Version 0.0.2
# Copyright 2004
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# Usage: Run hanzim2dict in a directory containing the "zidianf.gb", 
# "cidianf.gb", and "sanzidianf.gb" files from the Hanzi Master distribution
# (available at http://zakros.ucsd.edu/~arobert/hanzim.html).  The output
# will be a StarDict dictionary in 2.4.2 format: hanzim.dict, hanzim.idx,
# and hanzim.ifo
#
# The dictionary and index files may be compressed as follows:
# $ gzip -9 hanzim.idx
# $ dictzip hanzim.dict
#

from struct import pack

class Word:
    def __init__(self, code, definition):
        self.code = code
        self.definition = [definition]
    def add(self, definition):
        self.definition.append(definition)

wordmap = {}

file = open("zidianf.gb", "r", encoding = "GBK")
lines = [x[:-1].split('\t') for x in file.readlines()]

for line in lines:
    code = line[0]
    pinyin = line[2]
    definition = '<'+pinyin+'> '+line[3]+' ['+line[1]+']'
    if code in wordmap:
        wordmap[code].add(definition)
    else:
        wordmap[code] = Word(code, definition)

for filename in ("cidianf.gb", "sanzicidianf.gb"):
    file = open(filename, "r", encoding = "GBK")
    lines = [x[:-1].split('\t') for x in file.readlines()]

    for line in lines:
        if len(line) < 2:
            print((len(line)))
            continue
        code = line[0][:-2]
        definition = line[1]+' ['+line[0][-1:]+']'
        if code in wordmap:
            wordmap[code].add(definition)
        else:
            wordmap[code] = Word(code, definition)

dict = open("hanzim.dict", "wb")
idx = open("hanzim.idx", "wb")
ifo = open("hanzim.ifo", "w")
offset = 0
count = 0
keylen = 0

keys = list(wordmap.keys())
keys.sort()

for key in keys:
    word = wordmap[key]
    deftext = ""
    multi = False
    for d in word.definition:
        if multi:
            deftext += '\n'
        deftext += d
        multi = True

    dict.write(deftext.encode())

    idx.write((key+'\0').encode())
    idx.write(pack("!I", offset))
    idx.write(pack("!I", len(deftext)))
    offset += len(deftext)
    count += 1
    keylen += len(key.encode())

dict.close()
idx.close()

ifo.write("StarDict's dict ifo file\n")
ifo.write("version=2.4.2\n")
ifo.write("bookname=Hanzi Master 1.3\n")
ifo.write("wordcount="+str(count)+"\n")
ifo.write("idxfilesize="+str(keylen+(count*9))+"\n")
ifo.write("author=Adrian Robert\n")
ifo.write("email=arobert@cogsci.ucsd.edu\n")
ifo.write("website=http://zakros.ucsd.edu/~arobert/hanzim.html\n")
ifo.write("sametypesequence=m\n")
ifo.close()


