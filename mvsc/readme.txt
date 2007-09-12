You need to install developer packages into "gtk" directory, they can be found at http://www.gimp.org/~tml/gimp/win32/downloads.html

These packages are needed:
glib-dev-2.12.13.zip gtk+-dev-2.10.14.zip pango-dev-1.16.4.zip atk-dev-1.12.3.zip cairo-dev-1.4.8.zip gettext-dev-0.14.5.zip  Zlib 1.2.3

Just extract them into "mvsc\gtk\" is OK.

Then download libsigc++ and compile it. Copy sigc-2.0d.lib to "mvsc\gtk\lib\sigc\".
libsigc++ have static link problem on vs2005 presently.

You should can compile stardict successfully now. But that exe file is not well tested.
