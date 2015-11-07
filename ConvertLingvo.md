# Convert .dsl dictionaries for use in StarDict #

This article describes how to convert an .dsl dictionary to StarDict format.

An .dsl dictionary consists of the following files.
  * Main dictionary content Dictionary.dsl and Dictionary.ann.
  * Abbreviation files, the first found pair is used. Optional.
    * abbrev.dsl and abbrev.ann
    * Dictionary\_abrv.dsl and Dictionary\_abrv.ann
    * Dictionary\_abbrev.dsl and Dictionary\_abbrev.dsl
  * Dictionary icon Dictionary.bmp. Optional.

Here `Dictionary` is a dictionary name. All files must be in the same directory.

.dsl is a source dictionary, you may view any file comprising a .dsl dictionary (except icon) in a text editor. This article explains how to convert a .dsl dictionary.

Let us assume you have a dictionary `Dictionary.dsl` and other files you want to convert.

We will use makedict, a part of [XDXF project](https://sourceforge.net/projects/xdxf/), to perform conversion.

1. Get makedict source code
> Get the latest source of makedict with the command
```bash

svn co https://xdxf.svn.sourceforge.net/svnroot/xdxf/trunk makedict
```

> You'll get makedict subdirectory with makedict source code.

2. Build makedict

In makedict directory:
```bash

$ mkdir build
$ cd build
$ cmake ..
...
$ make
...
$ sudo make install
```

> Now you should have makedict in your path.

3. Test that makedict is available in your path

```bash

$ makedict --version
Utility for creating dictionaries, 0.3.1-beta1
```

4. Convert .dsl dictionary

```bash

$ makedict -i dsl -o stardict --generator-option 'stardict_ver=3.0.0' --work-dir /output/path /path/to/dsl/file.dsl
```

> Here

  * -i dsl - input format - dsl
  * -o stardict - output format - stardict
  * --work-dir - working directory, a directory were new StarDict dictionary will be created. If omitted, the current directory is used.
  * --generator-option 'stardict\_ver=3.0.0' pass stardict\_ver parameter to generator. When passed, the result dictionary will adhere to StarDict 3.0.0 file format. By default the format is 2.4.2.

5. Result files

After conversion you should find stardict-Dictionary-3.0.0 directory in your working directory, it contains the new dictionary. It's ready to be used in StarDict.

# Languages #

Occasionally you may notice that makedict fails with a warning like that:
```
Unknown target language UnknownLanguage. Ignoring.

For the list of supported languages use --list-supported-languages makedict option, you need English language name column.

CONTENTS_LANGUAGE not defined.

To specify CONTENTS_LANGUAGE on command line, use lang_to option.
For example, --parser-option lang_to=ENG.
For the list of supported languages use --list-supported-languages makedict option, you need alpha-3 code column.
```

makedict have to know source and target language of the dictionary it converts. Normally a dsl dictionary specifies source and target language at the beginning of the file:

```
#INDEX_LANGUAGE "SourceLanguage"
#CONTENTS_LANGUAGE      "TargetLanguage"
```

Occasionally this information is missed or nonstandard identifies are used. In this case you have to specify the language manually. There are a number of options.

1. Override the source (or target) language with command line option. For example:
```bash

$ makedict -i dsl -o stardict --parser-option lang_from=LNG /path/to/dsl/file.dsl
```

> Here LNG is the alpha-3 (bibliographic) code of the language according to ISO 639-2 Standard. All standard language codes are build into makedict. You may list them invoking makedict with --list-supported-languages option.

> lang\_from - name of the source language, lang\_to - name of the target language.

2. Create a language file describing your specific languages.

> This approach comes in handy when you have a number of dictionaries using nonstandard language names.
> Invoke makedict like this:
```bash

$ makedict -i dsl -o stardict ---language-file language-file.txt /path/to/dsl/file.dsl
```

> Languages listed in your language-file.txt will be added to the list of standard languages makedict already aware of.

> Format of the language-file.txt is as follows:
```
spa|es|SpanishCustomName
eng|en|EnglishCustomName
apa||Apache languages
```

  * file encoding - utf-8
  * Each line describes one language (not more).
  * Empty line are ignored.
  * A language may be described in a number of lines thus introducing alternative language names.
  * All lines describing one language should have identical first and second fields, only the the third field (English language name) may differ.
  * Each line contains the following fields:
    * alpha-3 (bibliographic) code, alpha-2 code, English language name
    * alpha-2 code may be blank.
  * Fields are separated with '|'.

> Here SpanishCustomName and EnglishCustomName are custom names of Spanish and English languages as they appear in .dsl files.

# Transcription #

Dictionaries for ABBYY Lingvo 12 and earlier versions of the program used custom chars code for transcription characters. You can easily check that yourself, open a .dsl file in a text editor, move to `[t] - [/t]` tags. What you see hardly reminds a transcription. makedict takes care of that, it convert transcription chars codes to normal Unicode characters.

Since x3 version, Lingvo starts to use normal Unicode characters in transcription, the conversion is no longer needed. Unfortunately, makedict cannot detect automatically if the conversion in required or no. The conversion is turn on by default, to disable it, pass --parser-option no\_ipa\_translation=yes options. In fact, you may use any non-blank string instead of "yes", makedict only checks if the option is assigned to a non-blank value or not.