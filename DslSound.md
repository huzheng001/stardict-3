# How to add sound to .dsl dictionaries #

This article explains how to connect Lingvo sound files to StarDict dictionaries converted from .dsl dictionaries.

Some .dsl dictionaries come with sound files. Sound files contain recorded samples of dictionary word performed by real people. Sound files for Lingvo 12 are named like SoundD.dat, SoundE.dat, SoundF.dat. Sound files for Lingvo x3 have .lsa extensions.

To convert sound files we'll use lingvosound2resdb utility from StarDict-tools package.
  1. Install StarDict-tools package.
  1. Check that lingvosound2resdb utility is in your path.

```bash

$ lingvosound2resdb --help
Usage:
lingvosound2resdb [OPTION...]

Convert ABBYY Lingvo sound file into !StarDict resource database.

Note: This program requires sox utility to split sound file into
samples. You may get sox from http://sox.sourceforge.net/.

See extra comments in the source file.


Help Options:
-h, --help                            Show help options

Application Options:
-v, --verbose                         print additional debugging information
-t, --no-split                        skip samples extraction phase
-x, --no-sound-extract                skip sound extraction phase
-l, --lingvo-sound-file=file_name     Lingvo sound file to read
-s, --temp-sound-file=file_name       temp sound file extracted from Lingvo file
-d, --samples-dir=dir_name            directory to extract sound samples
-i, --res-info-file=file_name         Stardict resource file with .rifo extension
```

3. Install sox (http://sox.sourceforge.net)
```bash

$ sox --version
sox: SoX v14.3.1
```

4. Create a new directory, cd there, copy sound file there

5. Convert Lingvo sound file with the command
```bash

$ lingvosound2resdb --lingvo-sound-file SoundD.dat --temp-sound-file SoundD.ogg --samples-dir Deutsch.soundfiles -i res.rifo
```

Here

  * --lingvo-sound-file SoundD.dat - the sound file to convert
  * --temp-sound-file SoundD.ogg - a temporary file where to extract sound file. You may play it with ogg-aware player.
  * --samples-dir Deutsch.soundfiles - a directory where sound samples will be created. After conversion you get thousands files here.
  * -i res.rifo - name of the resource database to create

> If you are curious how the utility works, you may read comment in lingvosound2resdb.cpp source file.

Lingvo sound file SoundD.dat contains ogg stream that is concatenation of all samples used in the dictionary. lingvosound2resdb extracts ogg stream in a temporary file SoundD.ogg. In the next step lingvosound2resdb split the ogg stream into samples. The output is saved in ogg format again. We may use other output format, wav for example. Note, however, that wav file of the same bit-rate is considerably larger compared to ogg file.

SoundD.ogg - 36.8MiB, the corresponding wav file SoundD.wav - 1008.5MiB. We may get smaller wav file sacrificing bit-rate. SoundD-16.wav with rate 16kHz - 336.2Mib, SoundD-8.wav with rate 8kHz - 168.1MiB. See [converting SoundD.ogg](#Appendix_1._Converting_SoundD.ogg.md) for details.

I use SoundD.ogg file as example here to give you approximation of size of the result file. The same conversion is applied to each sample when lingvosound2resdb runs. Note that total size of all sample after conversions will be larger than the corresponding SoundD.wav file. This is due to file headers. SoundD.wav - 1 headers, all samples - number of samples headers.

Samples in ogg format will be smaller compared to samples in wav unless you select very low rate. The quality will be degraded. Then why to bother? Why not always use the ogg format? The problem is ogg is not that widespread like wav is. You need special utility to play ogg files in StarDict. On Windows you need to specify custom program to play ogg files while wav are natively supported.

Output file format is not limited to just ogg and wav, you may use any supported sox output format. Make sure that you'll be able to play it in StarDict. It must be either natively supported or you need to provide command-line utility to play it.

To generate samples in wav format with 8kHz sample rate, for example, use this command

```bash

$ lingvosound2resdb --lingvo-sound-file SoundD.dat --temp-sound-file SoundD.ogg --samples-dir Deutsch.soundfiles -i res.rifo --output-ext wav --sox-effects "rate 8k"
```

Here two parameters were added.
  * --output-ext wav - extension of output sample files without dot. It specifies output format.
  * --sox-effects "rate 8k" - sox effects. This string will be appended to each sox command creating a sample. For complete list of effect see sox documentation.

6. After conversion you'll found the following files in the current directory.
```
$ ls -1
res.rdic
res.ridx
res.rifo
SoundD.dat
SoundD.ogg
```

res.rdic res.ridx res.rifo - StarDict resource database, copy them into the directory of StarDict dictionary.

7. Rebuild StarDict dictionary
```bash

$ makedict -i dsl -o stardict --generator-option 'stardict_ver=3.0.0' --work-dir /output/path --parser-option 'sound_ext=ogg' --parser-option 'sound_name_convert=lower' /path/to/dsl/file.dsl
```

You need to add two new parameters to makedict, two parser options.
  * --parser-option 'sound\_ext=ogg' - specify that sound resources will be in ogg format. This must be the same extension you pass to lingvosound2resdb in --output-ext parameter.
  * --parser-option 'sound\_name\_convert=lower' - convert resource names to lower case. Lingvo uses case-insensitive match to find resource name, while in StarDict resource database file names are case-sensitive.

## Appendix 1. Converting SoundD.ogg ##
You may convert ogg to wav with the command.
```bash

$ sox SoundD.ogg SoundD.wav
$ sox SoundD.ogg SoundD-16.wav rate 16k
sox WARN rate: rate clipped 284 samples; decrease volume?
sox WARN dither: dither clipped 248 samples; decrease volume?
$ sox SoundD.ogg SoundD-8.wav rate 8k
sox WARN rate: rate clipped 244 samples; decrease volume?
sox WARN dither: dither clipped 222 samples; decrease volume?
```

Files characteristics
```bash

$ soxi SoundD.ogg

Input File     : 'SoundD.ogg'
Channels       : 1
Sample Rate    : 48000
Precision      : 16-bit
Duration       : 03:03:35.57 = 528747550 samples ~ 826168 CDDA sectors
File Size      : 38.6M
Bit Rate       : 28.0k
Sample Encoding: Vorbis

$ soxi SoundD.wav

Input File     : 'SoundD.wav'
Channels       : 1
Sample Rate    : 48000
Precision      : 16-bit
Duration       : 03:03:35.57 = 528747550 samples ~ 826168 CDDA sectors
File Size      : 1.06G
Bit Rate       : 768k
Sample Encoding: 16-bit Signed Integer PCM

$ soxi SoundD-16.wav

Input File     : 'SoundD-16.wav'
Channels       : 1
Sample Rate    : 16000
Precision      : 16-bit
Duration       : 03:03:35.57 = 176249183 samples ~ 826168 CDDA sectors
File Size      : 352M
Bit Rate       : 256k
Sample Encoding: 16-bit Signed Integer PCM

$ soxi SoundD-8.wav

Input File     : 'SoundD-8.wav'
Channels       : 1
Sample Rate    : 8000
Precision      : 16-bit
Duration       : 03:03:35.57 = 88124592 samples ~ 826168 CDDA sectors
File Size      : 176M
Bit Rate       : 128k
Sample Encoding: 16-bit Signed Integer PCM
```