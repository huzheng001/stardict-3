#!/usr/bin/env python
# -*- coding: utf-8 -*-
#
# Script for decoding Lingea Dictionary (.trd) file
# Result is <header>\t<definition> file, convertable easily
# by stardict-editor from package stardict-tools into native
# Stardict dictionary (stardict.sf.net and www.stardict.org)
# 
# Copyright (C) 2007 - Klokan Petr Přidal (www.klokan.cz)
#
# Based on script CobuildConv.rb by Nomad
# http://hp.vector.co.jp/authors/VA005784/cobuild/cobuildconv.html
#
# Version history:
# 0.4 (30.10.2007) Patch by Petr Dlouhy, optional HTML generation
# 0.3 (28.10.2007) Patch by Petr Dlouhy, cleanup, bugfix. More dictionaries.
# 0.2 (19.7.2007) Changes, documentation, first 100% dictionary
# 0.1 (20.5.2006) Initial version based on Nomad specs
#
# Supported dictionaries:
# - Lingea Německý Kapesní slovník
# - Lingea Anglický Kapesní slovník
# - Lingea 2002 series (theoretically)
#
# Modified by:
# - Petr Dlouhy (petr.dlouhy | email.cz)
# Generalization of data block rules, sampleFlag 0x04, sound out fix, data phrase prefix with comment (0x04)
# HTML output, debugging patch, options on command line
#
# <write your name here>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Library General Public
# License as published by the Free Software Foundation; either
# version 2 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Library General Public License for more details.
#
# You should have received a copy of the GNU Library General Public
# License along with this library; if not, write to the
# Free Software Foundation, Inc., 59 Temple Place - Suite 330,
# Boston, MA 02111-1307, USA.

# VERSION
VERSION = "0.4"

import getopt, sys
def usage():
   print("Lingea Dictionary Decoder")
   print("-------------------------")
   print("Version: %s" % VERSION)
   print("Copyright (C) 2007 - Klokan Petr Pridal, Petr Dlouhy")
   print()
   print("Usage: python lingea-trd-decoder.py DICTIONARY.trd > DICTIONARY.tab")
   print("Result conversion by stardict-tools: /usr/lib/stardict-tools/tabfile")
   print()
   print("    -o <num>      --out-style        : Output style")
   print("                                          0   no tags")
   print("                                          1   \\n tags")
   print("                                          2   html tags")
   print("    -h            --help             : Print this message")
   print("    -d            --debug            : Degub")
   print("    -r            --debug-header     : Degub - print headers")
   print("    -a            --debug-all        : Degub - print all records")
   print("    -l            --debug-limit      : Degub limit")
   print()
   print("For HTML support in StarDict dictionary .ifo has to contain:")
   print("sametypesequence=g")
   print("!!! Change the .ifo file after generation by tabfile !!!")
   print()

try:
   opts, args = getopt.getopt(sys.argv[1:], "hdo:ral:", ["help", "debug", "out-style=", "debug-header", "debug-all", "debug-limit="])
except getopt.GetoptError:
   usage()
   print("ERROR: Bad option")
   sys.exit(2)
   
import locale
DEBUG = False
OUTSTYLE = 2
DEBUGHEADER = False
DEBUGALL = False
DEBUGLIMIT = 1
for o, a in opts:
   if o in ("-d", "-debug"):
      # DEBUGING !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      DEBUG = True
   if o in ("-o", "--out-style"):
      # output style
      OUTSTYLE = locale.atoi(a)
      if OUTSTYLE > 2:
         usage()
         print("ERROR: Output style not specified")
   if o in ("-r", "--debug-header"):
      # If DEBUG and DEBUGHEADER, then print just all header records
      DEBUGHEADER = True
   if o in ("-a", "--debug-all"):
      # If DEBUG and DEBUGALL then print debug info for all records
      DEBUGALL = True
   if o in ("-h", "--help"):
      usage()
      sys.exit(0)
   if o in ("-l", "--debug-limit"):
      # Number of wrong records for printing to stop during debugging 
      DEBUGLIMIT = locale.atoi(a)
# FILENAME is a first parameter on the commandline now

if len(args) == 1:
    FILENAME = args[0]
else:
   usage()
   print("ERROR: You have to specify .trd file to decode")
   sys.exit(2)

from struct import *
import re

alpha = ['\x00', 'a','b','c','d','e','f','g','h','i',
    'j','k','l','m','n','o','p','q','r','s',
    't','u','v','w','x','y','z','#AL27#','#AL28#','#AL29#',
    '#AL30#','#AL31#', ' ', '.', '<', '>', ',', ';', '-', '#AL39#',
        '#GRAVE#', '#ACUTE#', '#CIRC#', '#TILDE#', '#UML#', '#AL45#', '#AL46#', '#CARON#', '#AL48#', '#CEDIL#',
        '#AL50#', '#AL51#', '#GREEK#', '#AL53#', '#AL54#', '#AL55#', '#AL56#', '#AL57#', '#AL58#', '#SYMBOL#',
        '#AL60#', '#UPCASE#', '#SPECIAL#', '#UNICODE#'] # 4 bytes after unicode

upcase = ['#UP0#','#UP1#','#UP2#','#UP3#','#UP4#','#UP5#','#UP6#','#UP7#','#UP8#','#UP9#',
    '#UP10#','#UP11#','#UP12#','#UP13#','#UP14#','#UP15#','#UP16#','#UP17#','#UP18#','#UP19#',
    '#UP20#','#UP21#','#UP22#','#UP23#','#UP24#','#UP25#','#UP26#','#UP27#','#UP28#','#UP29#',
    '#UP30#','#UP31#','A','B','C','D','E','F','G','H',
    'I','J','K','L','M','N','O','P','Q','R',
    'S','T','U','V','W','X','Y','Z','#UP58#','#UP59#',
    '#UP60#','#UP61#','#UP62#','#UP63#']

upcase_pron = ['#pr0#', '#pr1#','#pr2#','#pr3#','#pr4#','#pr5#','#pr6#','#pr7#','#pr8#','#pr9#',
    '#pr10#', '#pr11#','#pr12#','#pr13#','#pr14#','#pr15#','#pr16#','#pr17#','#pr18#','#pr19#',
    '#pr20#', '#pr21#','#pr22#','#pr23#','#pr24#','#pr25#','#pr26#','#pr27#','#pr28#','#pr29#',
    '#pr30#', '#pr31#','ɑ','#pr33#','ʧ','ð','ə','ɜ','#pr38#','æ',
    'ɪ', 'ɭ','#pr42#','ŋ','#pr44#','ɳ','ɔ','#pr47#','ɒ','ɽ',
    'ʃ', 'θ','ʊ','ʌ','#pr54#','#pr55#','#pr56#','ʒ','#pr58#','#pr59#',
    '#pr60#', '#pr61#','#pr62#','#pr63#']

symbol = ['#SY0#', '#SY1#','#SY2#','#SY3#','§','#SY5#','#SY6#','#SY7#','#SY8#','#SY9#',
    '#SY10#', '#SY11#','#SY12#','#SY13#','#SY14#','™','#SY16#','#SY17#','¢','£',
    '#SY20#', '#SY21#','#SY22#','#SY23#','©','#SY25#','#SY26#','#SY27#','®','°',
    '#SY30#', '²','³','#SY33#','#SY34#','#SY35#','¹','#SY37#','#SY38#','#SY39#',
    '½', '#SY41#','#SY42#','×','÷','#SY45#','#SY46#','#SY47#','#SY48#','#SY49#',
    '#SY50#', '#SY51#','#SY52#','#SY53#','#SY54#','#SY55#','#SY56#','#SY57#','#SY58#','#SY59#',
    '#SY60#', '#SY61#','#SY62#','#SY63#']

special = ['#SP0#', '!','"','#','$','%','&','\'','(',')',
    '*', '+','#SP12#','#SP13#','#SP14#','/','0','1','2','3',
    '4', '5','6','7','8','9',':',';','<','=',
    '>', '?','@','[','\\',']','^','_','`','{',
    '|', '}','~','#SP43#','#SP44#','#SP45#','#SP46#','#SP47#','#SP48#','#SP49#',
    '#SP50#', '#SP51#','#SP52#','#SP53#','#SP54#','#SP55#','#SP56#','#SP57#','#SP58#','#SP59#',
    '#SP60#', '#SP61#','#SP62#','#SP63#']

wordclass = ('#0#','n:','adj:','pron:','#4#','v:','adv:','prep:','#8#','#9#',
    'intr:','phr:','#12#','#13#','#14#','#15#','#16#','#17#','#18#','#19#',
    '#20#','#21#','#22#','#23#','#24#','#25#','#26#','#27#','#28#','#29#',
    '#30#','#31#')

if OUTSTYLE == 0:
    tag = {
           'db':(''   ,''),    #Data begining
           'rn':(''   ,'\t'),  #Record name
           'va':(''   ,' '),   #Header variant
           'wc':('('  ,')'),   #WordClass
           'pa':(''   ,' '),   #Header parts
           'fo':('('  ,') '),  #Header forms
           'on':('('  ,')' ),  #Header origin note
           'pr':('['  ,']'),   #Header pronunciation
           'dv':('{'  ,'} '),  #Header dataVariant
           'sa':('`'  ,'`' ),  #Data sample
           'sw':(''   ,''),    #Data sample wordclass; is no printed by Lingea
           'do':('`'  ,'`' ),  #Data origin note
           'df':(''   ,' '),   #Data definition
           'ps':('"'  ,'" '),  #Data phrase short form
           'pg':('"'  ,' = '), #Data phrase green
           'pc':('`'  ,'`'),   #Data phrase comment; this comment is not printed by Lingea), but it seems useful
           'p1':('"'  ,' = '), #Data phrase 1
           'p2':(''   ,'" ' ), #Data phrase 2
           'sp':('"'  ,' = ' ),#Data simple phrase
           'b1':('"'  ,' = '), #Data phrase (block) 1
           'b2':('" ' ,''),    #Data phrase (block) 2
           }
if OUTSTYLE == 1:
    tag = {
           'db':('•'       ,''),      #Data begining
           'rn':(''        ,'\t'),    #Record name
           'va':(''        ,' '),     #Header variant
           'wc':(''        ,'\\n'),   #WordClass
           'pa':(''        ,':\\n'),  #Header parts
           'fo':('('       ,') '),    #Header forms
           'on':('('       ,')\\n' ), #Header origin note
           'pr':('['       ,']\\n'),  #Header pronunciation
           'dv':('{'       ,'} '),    #Header dataVariant
           'sa':('    '    ,'\\n' ),  #Data sample
           'sw':(''        ,''),      #Data sample wordclass; is not printed by Lingea
           'do':('    '    ,' ' ),    #Data origin note
           'df':('    '    ,'\\n'),   #Data definition
           'ps':('    '    ,'\\n'),   #Data phrase short form
           'pg':('    '    ,' '),     #Data phrase green
           'pc':('    '    ,' '),     #Data phrase comment; this comment is not printed by Lingea), but it seems useful
           'p1':('    '    ,' '),     #Data phrase 1
           'p2':('      '  ,'\\n' ),  #Data phrase 2
           'sp':(''        ,'\\n' ),  #Data simple phrase
           'b1':('"'       ,' = '),   #Data phrase (block) 1
           'b2':('" '      ,''),      #Data phrase (block) 2
          }
if OUTSTYLE == 2:
    tag = {
           'db':('•'                                                 ,''),              #Data begining
           'rn':(''                                                  ,'\t'),            #Record name
           'va':(''                                                  ,' '),             #Header variant
           'wc':('<span size="larger" color="darkred" weight="bold">','</span>\\n'),    #WordClass
           'pa':('<span size="larger" color="darkred" weight="bold">',':</span>\\n'),   #Header parts
           'fo':('('                                                 ,') '),            #Header forms
           'on':('<span color="blue">('                              ,')</span>\\n' ),  #Header origin note
           'pr':('['                                                 ,']\\n'),          #Header pronunciation
           'dv':('{'                                                 ,'} '),            #Header dataVariant
           'sa':('    <span color="darkred" weight="bold">'          ,'</span>\\n' ),   #Data sample
           'sw':(''                                                  ,''),              #Data sample wordclass; is not printed by Lingea
           'do':('    <span color="darkred" weight="bold">'          ,'</span> ' ),     #Data origin note
           'df':('    <span weight="bold">'                          ,'</span>\\n'),    #Data definition
           'ps':('    <span color="dimgray" weight="bold">'          ,'</span>\\n'),    #Data phrase short form
           'pg':('    <span color="darkgreen" style="italic">'       ,'</span> '),      #Data phrase green
           'pc':('    <span color="darkgreen" style="italic">'       ,'</span> '),      #Data phrase comment; this comment is not printed by Lingea), but it seems useful
           'p1':('    <span color="dimgray" style="italic">'         ,'</span> '),      #Data phrase 1
           'p2':('      '                                            ,'\\n' ),          #Data phrase 2
           'sp':('<span color="cyan">'                               ,'</span>\\n' ),   #Data simple phrase
           'b1':('"'                                                 ,' = '),           #Data phrase (block) 1
           'b2':('" '                                                ,''),              #Data phrase (block) 2
          }



# Print color debug functions
purple = lambda c: '\x1b[1;35m'+c+'\x1b[0m'
blue = lambda c: '\x1b[1;34m'+c+'\x1b[0m'
cyan = lambda c: '\x1b[36m'+c+'\x1b[0m'
gray = lambda c: '\x1b[1m'+c+'\x1b[0m'

def getRec(n):
    """Get data stream for record of given number"""
    if n >= 0 and n < entryCount:
        f.seek(index[n])
        return f.read(index[n+1] - index[n])
    else:
        return ''

def decode_alpha( stream, nullstop=True):
    """Decode 6-bit encoding data stream from the begining untit first NULL"""
    offset = 0
    triple = 0
    result = []
    while triple < len( stream ):
        if offset % 4 == 0:
            c = stream[triple] >> 2
            triple += 1
        if offset % 4 == 1:
            c = (stream[triple-1] & 3) << 4 | stream[triple] >> 4
            triple += 1
        if offset % 4 == 2:
            c = (stream[triple-1] & 15) << 2 | (stream[triple] & 192) >> 6
            triple += 1
        if offset % 4 == 3:
            c = stream[triple-1] & 63
        if c == 0 and nullstop:
            break
        offset += 1
        # TODO: ENCODE UNICODE 4 BYTE STREAM!!! and but it after #UNICODE# as unichr()
        result.append(c)
    return decode_alpha_postprocessing(result), triple - 1


def decode_alpha_postprocessing( input ):
    """Lowlevel alphabet decoding postprocessing, combines tuples into one character"""
    result = ""
    input.extend([0x00]*5)

    # UPCASE, UPCASE_PRON, SYMBOL, SPECIAL
    skip = False
    for i in range(0,len(input)-1):
        if skip:
            skip = False
            continue

        bc = input[i]
        c = alpha[bc]
        bc1 = input[i+1]
        c1 = alpha[bc1]

        if bc < 40:
            result += c
        else:
            if c == "#GRAVE#":
                if   c1 == 'a': result += 'à'
                else: result += '#GRAVE%s#' % c1
            elif c == "#UML#":
                if   c1 == 'o': result += 'ö'
                elif c1 == 'u': result += 'ü'
                elif c1 == 'a': result += 'ä'
                elif c1 == ' ': result += 'Ä'
                elif c1 == '#AL46#': result += 'Ö'
                elif c1 == '#GREEK#': result += 'Ü'
                else: result += '#UML%s#' % c1
            elif c == "#ACUTE#":
                if   c1 == 'a': result += 'á'
                elif c1 == 'e': result += 'é'
                elif c1 == 'i': result += 'í'
                elif c1 == 'o': result += 'ó'
                elif c1 == 'u': result += 'ú'
                elif c1 == 'y': result += 'ý'
                elif c1 == ' ': result += 'Á'
                elif c1 == '#GRAVE#': result += 'Í'
                else: result += '#ACUTE%s#' % c1
            elif c == "#CARON#":
                if   c1 == 'r': result += 'ř'
                elif c1 == 'c': result += 'č'
                elif c1 == 's': result += 'š'
                elif c1 == 'z': result += 'ž'
                elif c1 == 'e': result += 'ě'
                elif c1 == 'd': result += 'ď'
                elif c1 == 't': result += 'ť'
                elif c1 == 'a': result += 'å'
                elif c1 == 'u': result += 'ů'
                elif c1 == 'n': result += 'ň'
                elif c1 == '<': result += 'Č'
                elif c1 == '#CEDIL#': result += 'Ř'
                elif c1 == '#AL50#': result += 'Š'
                elif c1 == '#AL57#': result += 'Ž'
                else: result += '#CARON%s#' % c1
            elif c == "#UPCASE#":
                result += upcase[bc1]
            elif c == "#SYMBOL#":
                result += symbol[bc1]
            elif c == "#AL51#":
                if c1 == 's': result += 'ß'
            elif c == "#AL48#":
                result += "#AL48#%s" % c1
            elif c == "#SPECIAL#":
                result += special[bc1]
            elif c == "#UNICODE#":
                result += '#UNICODE%s#' % bc1
            elif c == "#CIRC#":
                if   c1 == 'a': result += 'â'
                else: result += '#CARON%s#' % c1
            else:
                result += '%sX%s#' % (c[:-1], bc1)
            skip = True
    return result

def pronunciation_encode(s):
    """Encode pronunciation upcase symbols into IPA symbols"""
    for i in range(0, 64):
        s = s.replace(upcase[i], upcase_pron[i])
    return s

re_d = re.compile(r'<d(.*?)>')
re_w = re.compile(r'<w(.*?)>')
re_y = re.compile(r'<y(.*?)>')
re_c = re.compile(r'<c(.*?)>')

def decode_tag_postprocessing(input):
    """Decode and replace tags used in lingea dictionaries; decode internal tags"""
    s = input

    # General information in http://www.david-zbiral.cz/El-slovniky-plnaverze.htm#_Toc151656799
    # TODO: Better output handling

    if OUTSTYLE == 0:
        # ?? <d...> 
        s = re_d.sub(r'(\1)',s)
        # ?? <w...>
        s = re_w.sub(r'(\1)',s)
        # ?? <y...>
        s = re_y.sub(r'(\1)',s)
        # ?? <c...>
        s = re_c.sub(r'(\1)',s)
        # ...
    if OUTSTYLE == 1:
        # ?? <d...> 
        s = re_d.sub(r'(\1)',s)
        # ?? <w...>
        s = re_w.sub(r'(\1)',s)
        # ?? <y...>
        s = re_y.sub(r'(\1)',s)
        # ?? <c...>
        s = re_c.sub(r'(\1)',s)
        # ...
    if OUTSTYLE == 2:
        # ?? <d...> 
        s = re_d.sub(r'<span size="small" color="blue">(\1)</span>',s)
        # ?? <w...>
        s = re_w.sub(r'<span size="small" color="blue" style="italic">\1</span>',s)
        # ?? <y...>
        s = re_y.sub(r'<span size="small" color="blue" style="italic">\1</span>',s)
        # ?? <c...>
        s = re_c.sub(r'<span size="small" color="blue" style="italic">\1</span>',s)
        # ...

    return s

def toBin( b ):
    """Prettify debug output format: hex(bin)dec"""
    original = b
    r = 0;
    i = 1;
    while b > 0:
        if b & 0x01 != 0: r += i
        i *= 10
        b = b >> 1
    return "0x%02X(%08d)%03d" % (original, r, original)


def out( comment = "", skip = False):
    """Read next byte or string (with skip=True) and output DEBUG info"""
    global bs, pos
    s, triple  = decode_alpha(bs[pos:])
    s = s.split('\x00')[0] # give me string until first NULL
    if (comment.find('%') != -1):
        if skip:
            comment = comment % s
        else:
            comment = comment % bs[pos]
    if DEBUG: print("%03d %s %s | %s | %03d" % (pos, toBin(bs[pos]),comment, s, (triple + pos)))
    if skip:
        pos += triple + 1
        return s.replace('`','') # Remove '`' character from words
    else:
        pos += 1
        return bs[pos-1]

outInt = lambda c: out(c)
outStr = lambda c: out(c, True)

def decode(stream):
    """Decode byte stream of one record, return decoded string with formatting in utf"""
    result = ""
    global bs, pos
    # stream - data byte stream for one record
    bs = unpack("<%sB" % len(stream), stream)
    # bs - list of bytes from stream

    pos = 0
    itemCount = outInt("ItemCount: %s") # Number of blocks in the record
    mainFlag = outInt("MainFlag: %s")

    # HEADER BLOCK
    # ------------
    if mainFlag & 0x01:
        headerFlag = outInt("HeaderFlag: %s") # Blocks in header
        if headerFlag & 0x01:
            result += tag['rn'][0] + outStr("Header record name: %s").replace('_','') + tag['rn'][1]  # Remove character '_' from index
        if headerFlag & 0x02:
            result += tag['va'][0] + outStr("Header variant: %s") + tag['va'][1]
        if headerFlag & 0x04:
            s = outInt("Header wordclass: %s")
            if s < 32:
                result += tag['wc'][0] + wordclass[s] + tag['wc'][1]
            else:
                raise "Header wordclass out of range in: %s" % result
        if headerFlag & 0x08:
            result += tag['pa'][0] + outStr("Header parts: %s") + tag['pa'][1]
        if headerFlag & 0x10:
            result += tag['fo'][0] + outStr("Header forms: %s") + tag['fo'][1]
        if headerFlag & 0x20:
            result += tag['on'][0] + outStr("Header origin note: %s") +  tag['on'][1]
        if headerFlag & 0x80:
            result += tag['pr'][0] + pronunciation_encode(outStr("Header pronunciation: %s")) + tag['pr'][1]
    
    # Header data block
    if mainFlag & 0x02:
        headerFlag = outInt("Header dataFlag: %s") # Blocks in header
        if headerFlag & 0x02:
            result += tag['dv'][0] + outStr("Header dataVariant: %s")+ tag['dv'][1]

    # ??? Link elsewhere
    pass

    # SOUND DATA REFERENCE
    if mainFlag & 0x80:
       outInt("Sound reference byte #1: %s")
       outInt("Sound reference byte #2: %s")
       outInt("Sound reference byte #3: %s")
       outInt("Sound reference byte #4: %s")
       outInt("Sound reference byte #5: %s")
       #out("Sound data reference (5 bytes)", 6)

    # TODO: Test all mainFlags in header!!!!

    #result += ': '
    li = 0
 
    #print just every first word class identifier
    # TODO: this is not systematic (should be handled by output)
    global lastWordClass
    lastWordClass = 0

    # DATA BLOCK(S)
    # -------------
    for i in range(0, itemCount):
        item = tag['db'][0] + tag['db'][1]
        ol = False
        dataFlag = outInt("DataFlag: %s -----------------------------")
        if dataFlag & 0x01: # small index
            sampleFlag = outInt("Data sampleFlag: %s")
            if sampleFlag & 0x01:
                result += tag['sa'][0] + outStr("Data sample: %s") +  tag['sa'][1]
            if sampleFlag & 0x04:
               s = outInt("Data wordclass: %s")
               if s != lastWordClass: 
                  if s < 32:
                      result += tag['wc'][0] + wordclass[s] + tag['wc'][1]
                  else:
                      raise "Header wordclass out of range in: %s" % result
               lastWordClass = s
            if sampleFlag & 0x08:
                result += tag['sw'][0] + outStr("Data sample wordclass: %s") + tag['sw'][1]
            if sampleFlag & 0x10:
                outInt("Data sample Int: %s")
                outInt("Data sample Int: %s")
                outInt("Data sample Int: %s")
            if sampleFlag & 0x20:
                item += tag['do'][0] + outStr("Data origin note: %s") + tag['do'][1]
            if sampleFlag & 0x80:
                item += "    "
                result += tag['pr'][0] + pronunciation_encode(outStr("Data sample pronunciation: %s")) + tag['pr'][1]
        if dataFlag & 0x02:
            item += "    "
            subFlag = outInt("Data subFlag: %s")
            if subFlag == 0x80:
                outStr("Data sub prefix: %s")
                # It seams that data sub prefix content is ignored and there is a generated number for the whole block instead.
                li += 1
                ol = True
        if dataFlag & 0x04: # chart
            pass # ???
        if dataFlag & 0x08: # reference
            item += tag['df'][0] + outStr("Data definition: %s") + tag['df'][1]
        if dataFlag & 0x10:
            pass # ???
        if dataFlag & 0x20: # phrase
            phraseFlag1 = outInt("Data phraseFlag1: %s")
            if phraseFlag1 & 0x01:
                item += tag['ps'][0] + outStr("Data phrase short form: %s") + tag['ps'][1]
            if phraseFlag1 & 0x02:
                phraseCount = outInt("Data phraseCount: %s")
                for i in range(0, phraseCount):
                    phraseComment = outInt("Data phrase prefix")
                    if phraseComment & 0x04:
                       item += tag['pc'][0] + outStr("Data phrase comment: %s")  + tag['pc'][1]
                    item += tag['p1'][0] + outStr("Data phrase 1: %s") + tag['p1'][1]
                    item += tag['p2'][0] + outStr("Data phrase 2: %s") + tag['p2'][1]
            if phraseFlag1 & 0x04:
                phraseCount = outInt("Data phraseCount: %s")
                for i in range(0, phraseCount):
                    phraseComment = outInt("Data phrase prefix")
                    if phraseComment & 0x04:
                       item += tag['pc'][0] + outStr("Data phrase 1: %s")  + tag['pc'][1]
                    item += tag['pg'][0] + outStr("Data phrase comment: %s")  + tag['pg'][1]
                    item += tag['p2'][0] + outStr("Data phrase 2: %s") +  tag['p2'][1]
            if phraseFlag1 & 0x08:
                phraseCount = outInt("Data simple phraseCount: %s")
                for i in range(0, phraseCount):
                    item += "    "
                    item += tag['sp'][0] + outStr("Data simple phrase: %s") +  tag['sp'][1]
            if phraseFlag1 & 0x40:
                item += tag['ps'][0] + outStr("Data phrase short form: %s") + tag['ps'][1]


            # TODO: be careful in changing the rules, to have back compatibility! 
        if dataFlag & 0x40: # reference, related language
            #0x01 synonym ?
            #0x02 antonym ?
            pass
        if dataFlag & 0x80: # Phrase block
            flags = [
            out("Data phrase block: %s"),
            out("Data phrase block: %s"),
            out("Data phrase block: %s"),
            out("Data phrase block: %s"),
            out("Data phrase block: %s"),
            out("Data phrase block: %s"),
            out("Data phrase block: %s"),
            out("Data phrase block: %s")]
            if flags == [0x80,0x80,0xF9,0xDF,0x9D,0x00,0x0B,0x01]:
                result += "\\nphr: "
                li = 1
                ol = True
                item += tag['b1'][0]+outStr("Data phrase 1: %s") + tag['b1'][1]
                out("Data phrase block: %s")
                out("Data phrase block: %s")
                out("Data phrase block: %s")
                out("Data phrase block: %s")
                item += tag['ds'][0] + outStr("Data phrase 2: %s") + tag['ds'][1]
            if flags == [0x80,0x80,0xF9,0xDF,0x9D,0x00,0x23,0x01]:
                result += "\\nphr: "
                li = 1
                ol = True
                item += tag['b1'][0]+outStr("Data phrase 1: %s") + tag['b1'][1]
                out("Data phrase block: %s")
                out("Data phrase block: %s")
                out("Data phrase block: %s")
                out("Data phrase block: %s")
                out("Data phrase block: %s")
                item += tag['ds'][0] + outStr("Data phrase 2: %s") + tag['ds'][1]
        if ol:
            result += "\\n%d. %s" % (li, item)
        else:
            result += item

    ok = True
    while pos < len(stream):
        ok = (out() == 0x00) and ok

    if ok:
        result += '\n'

    return decode_tag_postprocessing(result)

################################################################
# MAIN
################################################################


f = open(FILENAME,'rb')

# DECODE HEADER OF FILE

copyright = unpack("<64s",f.read(64))[0]
a = unpack("<16L",f.read(64))

entryCount = a[4]
indexBaseCount = a[6]
indexOffsetCount = a[7]
pos1 = a[8]
indexPos = a[9]
bodyPos = a[10]
smallIndex = (a[3] == 2052)

# DECODE INDEX STRUCTURE OF FILE

index = []
f.seek(indexPos)
bases = unpack("<%sL" % indexBaseCount, f.read(indexBaseCount * 4))
if smallIndex: # In small dictionaries every base is used 4-times
    bases4 = []
    for i in bases:
        bases4.extend([i,i,i,i])
    bases = bases4
for b in bases:
    offsets = unpack("<64H", f.read(64*2))
    for o in offsets:
        if len(index) < indexOffsetCount:
            #print "Index %s: %s + %s + %s * 4 = %s" % (len(index), bodyPos, b, o, toBin(bodyPos + b + o * 4))
            index.append(bodyPos + b + o * 4)

# DECODE RECORDS

if DEBUG:
    # PRINTOUT DEBUG OF FIRST <DEBUGLIMIT> WRONG RECORDS:
    for i in range(1,entryCount):
        if not DEBUGALL:
            DEBUG = False
        s = decode(getRec(i))
        if DEBUGHEADER:
            # print s.split('\t')[0]
            print(s)
        if DEBUGLIMIT > 0 and not s.endswith('\n'):
            DEBUG = True
            print("-"*80)
            print("%s) at address %s" % (i, toBin(index[i])))
            print()
            s = decode(getRec(i))
            print(s)
            DEBUGLIMIT -= 1
    DEBUG = True
else:
    # DECODE EACH RECORD AND PRINT IT IN FORMAT FOR stardict-editor <term>\t<definition>
    for i in range(1,entryCount):
        s = decode(getRec(i))
        if s.endswith('\n'):
            print(s, end=' ')
        else:
            print(s)
            print("!!! RECORD STRUCTURE DECODING ERROR !!!")
            print("Please run this script in DEBUG mode and repair DATA BLOCK(S) section in function decode()")
            print("If you succeed with whole dictionary send report (name of the dictionary and source code of script) to slovniky@googlegroups.com")
            break
