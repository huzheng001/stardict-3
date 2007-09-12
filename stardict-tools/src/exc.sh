#!/bin/bash
gcc exc2i2e.c -o exc2i2e
rename /usr/share/stardict/dic/i2e /usr/share/stardict/dic/temp /usr/share/stardict/dic/i2e.*  2>/dev/null
rename /usr/share/stardict/dic/e2i /usr/share/stardict/dic/temp2 /usr/share/stardict/dic/e2i.*  2>/dev/null
./exc2i2e >t
/usr/lib/stardict/i2e2dict t
rm t
rename /usr/share/stardict/dic/i2e /usr/share/stardict/dic/eng_irregular /usr/share/stardict/dic/i2e.*
rm /usr/share/stardict/dic/e2i.*
perl -p -i.old -e "s/I2E English-Spanish/English irregular forms/" /usr/share/stardict/dic/eng_irregular.ifo
rm /usr/share/stardict/dic/eng_irregular.ifo.old
rename /usr/share/stardict/dic/temp /usr/share/stardict/dic/i2e /usr/share/stardict/dic/temp.* 2>/dev/null
rename /usr/share/stardict/dic/temp2 /usr/share/stardict/dic/e2i /usr/share/stardict/dic/temp2.* 2>/dev/null
echo "Restart StarDict now!"
