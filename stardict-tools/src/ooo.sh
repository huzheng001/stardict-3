#!/bin/bash
gcc `pkg-config --cflags --libs glib-2.0` ooo2dict.c -o ooo2dict
echo |./ooo2dict 2>/dev/null

recode latin1..utf-8 </usr/share/myspell/dicts/th_de_DE_v2.dat >th_de_DE_v2.dat.utf
echo "OpenThesaurus.de_German" |./ooo2dict th_de_DE_v2.dat.utf 2>/dev/null

recode latin1..utf-8 </usr/share/myspell/dicts/th_it_IT_v2.dat >th_it_IT_v2.dat.utf 2>/dev/null
echo "OpenOffice.org_Italian" |./ooo2dict th_it_IT_v2.dat.utf 2>/dev/null