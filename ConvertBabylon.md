# Convert .bgl dictionary for use in StarDict #

This article explain how to convert .bgl (Babylon) dictionaries to StarDict format.

  * Get Babylon dictionary

First, download a dictionary you want to convert. I will use [Babylon English](http://www.babylon.com/free-dictionaries/languages/Babylon-English/901.html) for example. The downloaded file is named `Babylon_English.bgl`. A Babylon dictionary is normally a file with .bgl extension, but some dictionaries may be packed into self-extracting archives with .exe extension. For example, you may download `THE_Eng_Cro_dictionary.exe` file. For information about unpacking exe file see [below](#Unpack_exe_archive.md).

  * Install [Python 2.x.](http://www.python.org/)

Note that you need Python 2, not Python 3. The conversion tool is not compatible with Python 3 yet.

To check that Python is installed, issue this command in console:
```
$ python --version
```

If you see output similar to the following, python is installed correctly.
```
Python 2.6.6
```

  * Install gtk2 and glade2 python packages

For example, on Debian it's enough to install python-glade2 package:
```
$ sudo aptitude install python-glade2
```

  * Download PyGlossary converter

PyGlossary project is hosted on [SourceForge](https://sourceforge.net/projects/pyglossary/). Unfortunately, by the time of writing this article, the required version of the converter is not release and is not available for download in [project files](https://sourceforge.net/projects/pyglossary/files/pyglossary/). That is why we'll use git repository. On the [PyGlossary page](https://github.com/ilius/pyglossary) click download button, you'll get the last version of sources code. In case you have problems with the latest version, you may try acquire PyGlossary snapshot used in this article. Click History link on [PyGlossary page](https://github.com/ilius/pyglossary), find and click commit titled "fix missing return" at 2011-02-26, click Download.


You get a file like ilius-pyglossary-535d0da.zip. Unpack it. Add the new directory containing pyglossary.sh file to path, or use full path to pyglossary.py in the following steps.

  * Create a new directory and copy your .bgl file there
  * Open console in the this folder

```
$ ls
Babylon_English.bgl
$
```

  * Start PyGlossary issuing the command
```
$ pyglossary.sh
```

PyGlossary window appears.

  * Navigate to Preferences tab. In the Convert group uncheck "Remove tags after loading".
  * Close the window
  * In console execute the command
```
$ pyglossary.sh Babylon_English.bgl Babylon_English.ifo
```

After a while the dictionary will be converted. Complete output should look like this:
```
$ pyglossary.sh Babylon_English.bgl Babylon_English.ifo
Reading file "Babylon_English.bgl"
numEntries = 162046
defaultCharset = cp1252
sourceCharset = cp1252
targetCharset = cp1252

sourceLang = English
targetLang = English

creationTime = 2000/05/29, 18:13
middleUpdated = 2002/10/31, 00:00
lastUpdated = 2010/05/04, 13:55

title = Babylon English
author = Babylon Ltd.
email = linguistic-support@babylon.com
copyright = 
description = This comprehensive English dictionary contains words, phrases, abbreviations & acronyms. It includes terms from a vast variety of subjects, such as Medicine, Electronics, Zoology, Business, Computers, Religion, etc., and features both the American and British forms of spelling.
Loading: |████████████████████████████████████████████████|100.0% Time: 00:00:21

Writing to file "Babylon_English.ifo"
filename=Babylon_English.ifo
done
$ 
```

In the current directory you'll find following files:
```
$ ls -1
Babylon_English.bgl
Babylon_English.dict
Babylon_English.idx
Babylon_English.ifo
Babylon_English.syn
```

Files ending on .dict, .idx, .ifo, .syn constitute StarDict dictionary. .syn file may be absent, .dict file may be replaced with .dict.dz, .idx file may be replaced with .idx.gz. You may also encounter res subdirectory in case the dictionary contains resources. All these files and directories must be copied into StarDict dictionary directory. Create a new subdirectory for this dictionary.

## Example of converting a dictionary with resources ##
```
$ ls -1
Theological and Philosophical Biography and Dictionary.BGL
$ pyglossary.sh "Theological and Philosophical Biography and Dictionary.BGL" "Theological and Philosophical Biography and Dictionary.ifo" 
Reading file "Theological and Philosophical Biography and Dictionary.BGL"
numEntries = 2212
defaultCharset = 
sourceCharset = cp1252
targetCharset = cp1252

sourceLang = English     
targetLang = English

creationTime = 2000/08/22, 11:00
middleUpdated = 2000/08/28, 12:13
lastUpdated = 

title = Theological and Philosophical Biography and Dictionary
author = John Barach
email = admin@motorera.com
copyright = <a href= http://www.freeyellow.com/members6/theology/tdica.htm  ></a>
description = A dictionary of words and expressions relating to Theology and Philosophy
Loading: |████████████████████████████████████████████████|100.0% Time: 00:00:00

Writing to file "Theological and Philosophical Biography and Dictionary.ifo"
filename=Theological and Philosophical Biography and Dictionary.ifo
done
$ ls -1
res
Theological and Philosophical Biography and Dictionary.BGL
Theological and Philosophical Biography and Dictionary.BGL_files
Theological and Philosophical Biography and Dictionary.dict
Theological and Philosophical Biography and Dictionary.idx
Theological and Philosophical Biography and Dictionary.ifo
Theological and Philosophical Biography and Dictionary.syn
$ 
```

## Notes ##

  * In my example both source and new dictionaries are in the same directory. That is not required. You may specify relative or absolute path to both files.

  * The StarDict dictionary may have any name. It's not necessary to use the name of the .bgl file.

  * For a dictionary containing resources, a `*_files` subdirectory will be created in the directory of the .bgl file.

  * Babylon dictionary file format in not documented. The converter was build on results of reverse engineering the .bgl file format. It's likely that PyGlossary will fail to convert some dictionaries. PyGlossary may print a lot of warnings while converting a Babylon dictionary, that is OK.

  * Python does not properly implement cp932 and cp950 encodings. It means that dictionaries using Japanese and Chinese scripts may be decoded incorrectly. I am not sure, maybe encoding problems were specific to my version of Python only.

## HTML in keys ##

You may notice that some dictionaries converted from .bgl files contain HTML tags and character references in keys. For example, you may encounter the following keys: "**`die Meinige c=t&gt;2003;</charset>`**", "**`hoch und Hoch</font>und Niedrig`**", "**`u.<font face'Lucida Sans Unicode'>&lt; M`**", "**`Deb&#xfc;tant`**". Babylon application does not process HTML codes in keys, it will show you the same garbage you see here. Such keys undoubtedly are dictionary errors.

By default, PyGlossary leaves all keys as is. However, you may tell PyGlossary to process HTML codes in keys. Processing means stripping HTML tags and decoding character references. For example, "**`Deb&#xfc;tant`**" will be converted to "**`Debütant`**", and "**`um das Unglück voll</font>zu<font face='Lucida Sans Unicode'>`**" to "**`um das Unglück voll zu`**".

`processHtmlInKey` reader option controls processing HTML codes in keys. By default, it's set to `False`. To turn HTML processing on, you should invoke PyGlossary like this:

```
$ pyglossary.sh --read-options=processHtmlInKey=True Babylon_English.bgl Babylon_English.ifo
```

If you need to pass more reader options, separate them with semicolon:

```
$ pyglossary.sh --read-options=resPath=path/to/dir;verbose=4;processHtmlInKey=True Babylon_English.bgl Babylon_English.ifo
```

Should you pass the processHtmlInKey option or not, depends on the dictionary. The recommended way of operation is to convert the dictionary without the processHtmlInKey option, then inspect the dictionary index. I can inspect dictionary index with stardict-index command (part of StarDict tool package):

```
$ stardict-index -k Dictionary.idx | grep '<>&'
```

The first command of the pipeline prints all dictionary keys, the second filters only those containing "<", ">", "&" characters. If you see that output looks like HTML text, reconvert the dictionary with processHtmlInKey option turned on.

If your dictionary has a synonyms file (.syn), you should inspect it as well. This time invoke the command:

```
$ stardict-index -k Dictionary.syn | grep '<>&'
```

processHtmlInKey option effects both files: the main index (.idx) and the synonyms file (.syn).

## Unpack exe archive ##

To extract a dictionary file from an exe archive you may use [7zip](http://www.7-zip.org/). exe archive contains a number of files, you need to extract only .bgl file(s).

On Linux you may issue this command:
```
$ 7z x THE_Eng_Cro_dictionary.exe
```

After that in the current directory you'll find these files:
```
Babylon.dat
Babylon.ico
BabyServices.dll
BException.dll
EULA.rtf
MyBabylonFF.exe
MyBabylonIE.exe
Setup32.exe
Strings.dat
THE_Eng_Cro_dictionary.BGL
```

You need only `THE_Eng_Cro_dictionary.BGL`.

To extract only .bgl files, issue this command:
```
$ 7z x THE_Eng_Cro_dictionary.exe '*.bgl' '*.BGL'
```