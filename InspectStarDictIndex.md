# Inspect StarDict index file #

This article describes how to inspect StarDict index (.idx) files, synonyms (.syn) files, and resource database index (.ridx) files.

To inspect an index file use stardict-index utility from `StarDict-tools` package. Invoke the utility as follows:

```
stardict-index DICTIONARY.idx
```

Here DICTIONARY.idx is an index file to inspect.

Note. The utility does not support compressed indexes, that is files with extensions .idx.gz, .ridx.gz. If you have such files, you need to decompress them first with gzip tool.

```
$ ls
Babylon_Greek_English.idx.gz
$ gzip -d Babylon_Greek_English.idx.gz 
$ ls
Babylon_Greek_English.idx
$ 
```

stardict-index prints content of the index in human readable form.

```
$ stardict-index Babylon_Greek_English.idx
    OFFSET       SIZE KEY
         0         33 100 τετραγωνικά μέτρα
        33         37 1000 ηλεκτρικές μονάδες ενέργειας
        70         38 31 Οκτώβρη
       108         39 aftershave
       147         32 ale
       179         16 American Express
       195         40 Art Nouveau
       235         36 bourbon
       271         33 Brie
...
   2386101         36 ώσμωση
   2386137         42 ώσπου
   2386179         43 ώσπου
   2386222         38 ώστε
   2386260         40 ώστε
   2386300         41 ώχρα
number of entries: 53012
```

The first line of output is a header. It reminds the meaning of the columns. In case of .idx files: first column - offset in the dictionary file, second column - size of the definition in the index file, third column - the key word. The last line of output indicate the total number of index entries.

You suppress auxiliary information (the first and the last lines) passing the -q option.

```
$ stardict-index -q Babylon_Greek_English.idx
         0         33 100 τετραγωνικά μέτρα
        33         37 1000 ηλεκτρικές μονάδες ενέργειας
        70         38 31 Οκτώβρη
       108         39 aftershave
       147         32 ale
       179         16 American Express
       195         40 Art Nouveau
       235         36 bourbon
       271         33 Brie
...
   2386101         36 ώσμωση
   2386137         42 ώσπου
   2386179         43 ώσπου
   2386222         38 ώστε
   2386260         40 ώστε
   2386300         41 ώχρα
```

This time only index entries are printed.

Most of the time, however, you are interested in key words only. To display only key words, pass the -k option.

```
$ stardict-index -k Babylon_Greek_English.idx
100 τετραγωνικά μέτρα
1000 ηλεκτρικές μονάδες ενέργειας
31 Οκτώβρη
aftershave
ale
American Express
Art Nouveau
bourbon
Brie
bungee jumping
...
ώσαν βελούδο
ώσαν τενεκές
ώσμωση
ώσπου
ώσπου
ώστε
ώστε
ώχρα
```