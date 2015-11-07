# Repair broken StarDict dictionaries #

Many dictionary you may find on Internet for StarDict do not strictly adhere to StarDict dictionary format guidelines. Some of them violate format rules to the degree they may crash StarDict. StarDict 3.0.3 introduce new feature - check all dictionary at program start time. Broken dictionary cannot crash StarDict any more, the application simply refuse to load them. Many dictionaries accepted by StarDict may need some changes to be used effectively.

All these problems may be solved with a new tool introduced in StarDict 3.0.3, it is named stardict-repair. As the name implies it convert a StarDict dictionary into a valid one. Of cause, not every dictionary may be repaired, for example, if you are missing index file, stardict-repair cannot help. In most other cases stardict-repair will extract everything it can from your broken dictionary and create a new one, that may be loaded by StarDict.

## How a dictionary may be broken ##

There are many flaws that make a dictionary invalid (not necessary broken). For instance, a key word should not start with a space character. Why? For example, we have a key word " welcome" in index, but you normally will search for "welcome" in the search field, right? StarDict will not find the word " welcome" in that case. That is minor issue, but it worth fixing. Other key words may end with a space, that is again not dangerous but worth fixing.

StarDict put a limit on maximum key word length, long keys are not accepted. That is a serious problem for StarDict, dictionaries with long keys are not accepted. stardict-repair truncates long keys.

In many dictionaries index terms are not sorted correctly. That is violation of StarDict dictionary format. Having incorrectly sorted index, StarDict may not find some words in the dictionary despite they are really present there.

There are many ways in which dictionary may be invalid, watch for utility output if you are interested.

## Verifying a dictionary ##

To verify a dictionary, you may use stardict-verify tool (part of StarDict tools package). Invoke it as follows:

```
stardict-verify DICTIONARY.ifo
```

For example, let's verify dictd\_www.freedict.de\_deu-por dictionary:

```
$ stardict-verify dictd_www.freedict.de_deu-por.ifo
[message] Verifying dictionary 'dictd_www.freedict.de_deu-por.ifo'...
[message] Loading index file: 'dictd_www.freedict.de_deu-por.idx'...
[message] Loading dictionary file: 'dictd_www.freedict.de_deu-por.dict'...
[message] Dictionary 'dictd_www.freedict.de_deu-por.ifo'. Verification result: OK.

```

You must inspect the last line, the verification result. This dictionary is correct, the utility has not found any problem worth to mention.

Let's check another dictionary ldaf:
```
[message] Verifying dictionary 'ldaf.ifo'...
[message] Loading index file: 'ldaf.idx'...
[message] Loading dictionary file: 'ldaf.dict.dz'...
[message] Index item '-Müllcontainer'. Type id 'm'. Invalid field content: '''
Con·tai·ner [kn'te:n] der; -s, -; ein großer Behälter für Abfall oder zum Transport
|| K-: Containerbahnhof, Containerhafen, Containerschiff, Containerterminal || -K: Altpapiercontainer, Glascontainer, -Müllcontainer
'''
The text contains either invalid Unicode characters or Unicode characters not suitable for textual data (mainly control characters). The following characters are prohibited: 6, 16.
...
[message] Dictionary 'ldaf.ifo'. Verification result: Non-critical problems were found. The dictionary is safe to use.

```

This time stardict-verify produces a lot of messages, they were skipped for clarity. What is really important in the last line. It states: "Non-critical problems were found. The dictionary is safe to use." So this dictionary may be loaded by StarDict as is, but there is a room for fixes.

At last, lets take a look at a broken dictionary. It is quick\_english-korean dictionary:

```
[message] Verifying dictionary 'quick_english-korean.ifo'...
[message] Loading index file: 'quick_english-korean.idx'...
[warning] Wrong key order, first key = 'babbitt', second key = 'Babbitt'.
[message] Index item 'backwardly '. Key ends with a space character.
[warning] Wrong key order, first key = 'bacon', second key = 'Bacon'.
[warning] Wrong key order, first key = 'badge', second key = 'BADGE'.
[warning] Wrong key order, first key = 'bag(h)dad', second key = 'Bag(h)dad'.
[warning] Wrong key order, first key = 'balboa', second key = 'Balboa'.
[warning] Wrong key order, first key = 'bar', second key = 'BAR'.
...
[message] Dictionary 'quick_english-korean.ifo'. Verification result: The dictionary is broken. Do not use it.

```

As you can see the verification status is: "The dictionary is broken. Do not use it." StarDict will not load this dictionary. The problem with this dictionary is that the index is not properly sorted. That may be fixed with stardict-repair tool.

If you only want to know, whether the dictionary is correct or not and you do not care about details, you may pass -q option to stardict-verify. That option suppress all output except the verification result message.

## Repairing a dictionary ##

To repair a dictionary use stardict-repair tool, it is included in StarDict tools package.

Typical usage:
```
stardict-repair PATH/TO/DICTIONARY.ifo
```

or
```
stardict-repair --out-dir DIR PATH/TO/DICTIONARY.ifo
```

--out-dir option specifies the output directory where repaired dictionary will be created.

Note that the output directory must be different from the dictionary directory, otherwise the source dictionary will be overwritten. By default, the utility uses the current directory as output directory.

stardict-repair prints many messages, many of them are identical to those produced by stardict-verify. The most important message is the last one - repair result. It may be other success or failure.

Example of repairing 'quick\_english-korean'
```
[message] Loading dictionary: 'quick_english-korean.ifo'...
[message] Loading index file: 'quick_english-korean.idx'...
[warning] Wrong key order, first key = 'babbitt', second key = 'Babbitt'.
[message] The problem was fixed. Key will be reordered.
[message] Index item 'backwardly '. Key ends with a space character.
[message] The problem was fixed. Leading and trailing spaces trimmed.
[warning] Wrong key order, first key = 'bacon', second key = 'Bacon'.
[message] The problem was fixed. Key will be reordered.
...
[message] Loading dictionary file: 'quick_english-korean.dict.dz'...
[message] Dictionary loaded.
[message] Checking and repairing the dictionary...
[message] Dictionary repaired.
[message] Saving dictionary in 'a/quick_english-korean.ifo'...
[message] Using same type sequence 'g'.
[message] Dictionary saved.
Dictionary 'quick_english-korean.ifo'. Repair result: success
```

As with stardict-verify tool you may suppress most of the output with the -q option.

Another option worth to mention is the --lot-of-memory option. If you have enough memory, you may tell the tool to store intermediate results in memory. But default stardict-repair uses temporary files. Specifying this option should slightly improve performance.

To familiarize yourself with other available options, invoke stardict-repair with -h option.