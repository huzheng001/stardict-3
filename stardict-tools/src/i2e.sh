#!/bin/bash
recode latin1..utf-8 </usr/share/i2e/i2e.dict >i2e.dict.utf
gcc -g `pkg-config --cflags --libs glib-2.0` i2e2dict.c -o i2e2dict
./i2e2dict
rm i2e.dict.utf
echo "Restart StarDict now!"
