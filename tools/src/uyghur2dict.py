#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# uyghur2dict
# By Abdisalam (anatilim@gmail.com), inspired by Michael Robinson's hanzim2dict converter.
#  
# Original version, hanzim2dict, written by Michael Robinson (robinson@netrinsics.com)
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

file = open("ChineseUyghurStarDict.txt", "r")
lines = [x[:-1].split('\t\t') for x in file.readlines()]

for line in lines:
    code = line[0]
    definition = line[1]
    if code in wordmap:
        wordmap[code].add(definition)
    else:
        wordmap[code] = Word(code, definition)

dict = open("Anatilim_Chinese_Uyghur.dict", "wb")
idx = open("Anatilim_Chinese_Uyghur.idx", "wb")
ifo = open("Anatilim_Chinese_Uyghur.ifo", "wb")
offset = 0
count = 0
keylen = 0

keys = list(wordmap.keys())
keys.sort()

for key in keys:
    word = wordmap[key]
    deftext = ""
    for d in word.definition:
        deftext=d
	 
    deftext += '\0'
    dict.write(deftext)

    idx.write(key+'\0')
    idx.write(pack("!I", offset))
    idx.write(pack("!I", len(deftext)))
    offset += len(deftext)
    count += 1
    keylen += len(key)

dict.close()
idx.close()

ifo.write("StarDict's dict ifo file\n")
ifo.write("version=2.4.2\n")
ifo.write("bookname=Anatilim 《汉维词典》-- Anatilim Chinese Uyghur Dictionary\n")
ifo.write("wordcount="+str(count)+"\n")
ifo.write("idxfilesize="+str(keylen+(count*9))+"\n")
ifo.write("author=Abdisalam\n")
ifo.write("email=anatilim@gmail.com\n")
ifo.write("description=感谢新疆维吾尔自治区语委会、新疆青少年出版社为我们提供《汉维词典》的词库\n")
ifo.write("sametypesequence=m\n")
ifo.close()

