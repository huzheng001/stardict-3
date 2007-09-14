You need to install developer packages into "gtk" directory, they can be found at http://www.gimp.org/~tml/gimp/win32/downloads.html

These packages are needed:
glib-dev-2.12.13.zip gtk+-dev-2.10.14.zip pango-dev-1.16.4.zip atk-dev-1.12.3.zip cairo-dev-1.4.8.zip gettext-dev-0.14.5.zip  Zlib 1.2.3

Just extract them into "mvsc\gtk\" is OK.

Then download libsigc++ and compile it. Copy sigc-2.0d.lib to "mvsc\gtk\lib\".
libsigc++ have static link problem on vs2005 presently.

For stardict_powerword_parsedata.cpp, you need to add a UTF-8 BOM in its head to fix the compile problem. Just use the notepad to open it then save.

There are two crash bug with vs2005, which you need to notice.
1. Use my_g_fopen instead of g_fopen. See http://bugzilla.gnome.org/show_bug.cgi?id=476810
2. Use fprintf_s instead of fprintf, or it will crash. This is a little strange.

You should can compile and run stardict successfully now.
