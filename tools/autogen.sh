#!/bin/sh

echo "Boostrapping StarDict tools..."

libtoolize=libtoolize
command -v "$libtoolize" > /dev/null || libtoolize=glibtoolize

("$libtoolize" --version) < /dev/null > /dev/null 2>&1 || {
	echo;
	echo "You must have libtool installed to compile Stardict";
	echo;
	exit;
}

(automake --version) < /dev/null > /dev/null 2>&1 || {
	echo;
	echo "You must have automake installed to compile Stardict";
	echo;
	exit;
}

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
	echo;
	echo "You must have autoconf installed to compile Stardict";
	echo;
	exit;
}

echo "Generating configuration files for Stardict, please wait...."
echo;

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.
topdir=`pwd`

cd "$srcdir"
echo "Running $libtoolize, please ignore non-fatal messages...."
echo n | "$libtoolize" --copy --force || exit;

echo "Running aclocal...."
aclocal -I m4 || exit;
echo "Running autoheader...."
autoheader || exit;
echo "Running automake --add-missing --copy...."
automake --add-missing --copy;
echo "Running autoconf ...."
autoconf || exit;
echo "Running automake ...."
automake || exit;
cd "$topdir"
#"${srcdir}/configure" --prefix=/usr --sysconfdir=/etc --mandir=/usr/share/man "$@"
