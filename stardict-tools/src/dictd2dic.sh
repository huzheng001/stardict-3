#!/bin/bash
# dictd2dic conversion script
#
# Usage: dictd2dic.sh foldoc
#
if test -e $1.dict.dz; then dictunzip $1.dict.dz; fi
if test -e $1.dict; then
touch $1.idxhead
echo "StarDict's dict ifo file" >dictd_www.freedict.de_$1.ifo
echo version=2.4.2 >>dictd_www.freedict.de_$1.ifo

echo wordcount=`/usr/lib/stardict/dictd2dic $1 |grep 'wordcount:' |cut -b 12-` >>dictd_www.freedict.de_$1.ifo
gunzip dictd_www.freedict.de_$1.idx.gz
echo idxfilesize=`stat -c %s dictd_www.freedict.de_$1.idx` >>dictd_www.freedict.de_$1.ifo
echo bookname=$1 >>dictd_www.freedict.de_$1.ifo
echo date=2007.01.01 >>dictd_www.freedict.de_$1.ifo
echo sametypesequence=m >>dictd_www.freedict.de_$1.ifo
echo "dictd_www.freedict.de_$1 created. Put the files to /usr/share/stardict/dic"
rm $1.idxhead
else
echo "Usage: dictd2dic.sh foldoc"
fi