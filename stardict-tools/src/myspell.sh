#!/bin/bash
gcc `pkg-config --cflags --libs glib-2.0` myspell2dic.c -o myspell2dic
echo "Please wait..."

recode latin1..utf-8 </usr/share/myspell/dicts/es_ES.dic >es_ES.dic.utf
recode latin1..utf-8 </usr/share/myspell/dicts/es_ES.aff >es_ES.aff.utf
echo |./myspell2dic es_ES.dic.utf es_ES.aff.utf 2>/dev/null

recode koi8-r..utf-8 </usr/share/myspell/dicts/ru_RU.dic >ru_RU.dic.utf
recode koi8-r..utf-8 </usr/share/myspell/dicts/ru_RU.aff >ru_RU.aff.utf
echo "Russian" |./myspell2dic ru_RU.dic.utf ru_RU.aff.utf 2>/dev/null

recode ISO8859-2..utf-8 </usr/share/myspell/dicts/pl_PL.dic >pl_PL.dic.utf
recode ISO8859-2..utf-8 </usr/share/myspell/dicts/pl_PL.aff >pl_PL.aff.utf
echo "Polish" |./myspell2dic pl_PL.dic.utf pl_PL.aff.utf 2>/dev/null

recode KOI8-RU..UTF-8 </usr/share/myspell/dicts/uk_UA.dic >uk_UA.dic.utf
recode KOI8-RU..UTF-8 </usr/share/myspell/dicts/uk_UA.aff >uk_UA.aff.utf
echo "Ukrainian" |./myspell2dic uk_UA.dic.utf uk_UA.aff.utf 2>/dev/null

echo "Restart StarDict now!"
