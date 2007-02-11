#!/bin/bash
gcc exc2i2e.c -o exc2i2e
./exc2i2e >t
/usr/lib/stardict/i2e2dict t
rename /usr/share/stardict/dic/i2e /usr/share/stardict/dic/eng_irregular /usr/share/stardict/dic/i2e.*
rm /usr/share/stardict/dic/e2i.*