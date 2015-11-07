# StarDict frequently asked questions #
**Q) In Windows, I have installed the somedict.tar.bz2 file at dic/, but StarDict didn't load it.**

> A) You need to use WinRAR or 7-Zip to uncompress this file.

**Q) StarDict Windows version is not stable.**
> A) Try StarDict-3.0.1, this is a stable and classical version!

**Q) StarDict is freezed when I am using it.**
> A) This seems to be esound's bug, try this command:"killall esd". Another case is pango's bug, try to drag the hpaned to resize the textview.

**Q) How to mouse over get word in Adobe Reader win32?**
> A) We have developed a plug-in for Adobe Reader, but only Acrobat Professional edition will load this plug-in, if we want to register the plug-in for Adobe Reader that make it load the StarDict plug-in, we need to pay $2500, which have not be done yet. So you can change to Acrobat Professional edition or just use scan clipboard feature in Adobe Reader.

**Q) I get some problem when install StarDict from the source code tarball.**
> A) Read Building#Linux

**Q) How to show the correct phonetics in some babylon dictionaries?**
> A) Download http://www.stardict.org/files/ksphonet.ttf and install it. In linux, you can put it into ~/.fonts/ directory.

**Q) How to compile StarDict on Windows?**
> A) read Building#Windows

**Q) How to compile StarDict on Mac OS X?**
> A) read Building#MacOSX

**Q) Why I need to select another words then select the original words again to pop-up the query dialog?**
> A) This only happen on some applications such as firefox or KDE programs, and the gnome terminal too. Because when you un-select the words, the application didn't clear the selection buffer, so StarDict can't sense your un-select, then you need to select another words and select the original words to make StarDict know that the selection buffer has changed. All the GTK software(standard widget) haven't this problem. OK, to summarize, this is Not the bug of StarDict, and it can only be fixed by the corresponding applications.

**Q) The Chinese character can't be shown correctly in Windows edition.**
> A) Setup the font to "tahoma" in the preferences dialog.

**Q) Mouse over get word don't work in Firefox 3, VS2008 on Windows platform.**
> A) Sorry, I have no ability and no time to develop a new and better algorithm recently.