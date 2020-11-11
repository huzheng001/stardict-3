#!/bin/sh

echo "Bootstrapping StarDict root..."

# (libtoolize --version) < /dev/null > /dev/null 2>&1 || {
# 	echo;
# 	echo "You must have libtool installed to compile StarDict";
# 	echo;
# 	exit;
# }

(automake --version) < /dev/null > /dev/null 2>&1 || {
	echo;
	echo "You must have automake installed to compile StarDict";
	echo;
	exit;
}

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
	echo;
	echo "You must have autoconf installed to compile StarDict";
	echo;
	exit;
}

echo "Generating configuration files for StarDict, please wait...."
echo;

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.
topdir=`pwd`

cd "$srcdir"
# echo "Running libtoolize, please ignore non-fatal messages...."
# echo n | libtoolize --copy --force || exit;

echo "Running aclocal...."
test -d m4 || mkdir m4
aclocal -I m4 || exit;
# echo "Running autoheader...."
# autoheader || exit;
echo "Running automake --add-missing --copy...."
automake --add-missing --copy;
echo "Running autoconf ...."
autoconf || exit;
echo "Running automake ...."
automake || exit;
cd "$topdir"
${srcdir}/lib/autogen.sh
${srcdir}/dict/autogen.sh
${srcdir}/tools/autogen.sh
#"${srcdir}/configure" --prefix=/usr --sysconfdir=/etc --mandir=/usr/share/man --disable-deprecations --disable-dictdotcn --disable-gnome-support --disable-scrollkeeper "$@"
