#!/bin/bash
# dictd2dic conversion script
#
# Usage: dictd2dic.sh mueller7
#
cd /usr/share/dictd

if test -e $1.dict.dz; then dictunzip $1.dict.dz; fi
if test -e $1.dict; then
echo "Please wait..."
touch $1.idxhead
echo "StarDict's dict ifo file" >dictd_www.freedict.de_$1.ifo
echo version=2.4.2 >>dictd_www.freedict.de_$1.ifo

echo wordcount=`/usr/lib/stardict/dictd2dic $1 |grep 'wordcount:' |cut -b 12-` >>dictd_www.freedict.de_$1.ifo
gunzip dictd_www.freedict.de_$1.idx.gz
echo idxfilesize=`stat -c %s dictd_www.freedict.de_$1.idx` >>dictd_www.freedict.de_$1.ifo
echo bookname=$1 >>dictd_www.freedict.de_$1.ifo
echo date=2007.01.01 >>dictd_www.freedict.de_$1.ifo
echo sametypesequence=m >>dictd_www.freedict.de_$1.ifo
rm $1.idxhead
cp dictd_www.freedict.de_$1* /usr/share/stardict/dic
echo "dictd_www.freedict.de_$1 created. The files are in /usr/share/stardict/dic"
echo "Restart StarDict now!"
else
echo "Usage: dictd2dic.sh mueller7"
fi