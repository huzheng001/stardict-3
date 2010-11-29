Textual file format for normal (index-based) StarDict dictionary

Why do we need a new file format?

StarDict uses a opaque binary file format. 

The advantages of the binary format are:
1. compact representation of data
2. fast access by index, when we need to find a particular word in the dictionary
3. the whole dictionary can be scanned through at moderate cost 
	(we should iterate over all index items, extract item-specific data)

The disadvantages of the binary file format are:
1. dictionary is opaque, we cannot see what it contains, the source code. 
	We can only see one entry at time in preformatted state.
2. we cannot change a dictionary (edit word definition, add new word to the dictionary) 
	without special tools. Existing tools do not allow to carry all desirable operations.
2.1. How to create a new dictionary from scratch?

Textual file format may be used to:
1. examine dictionary content
2. make changes to a dictionary
3. create a new dictionary from scratch

We may use any text editor to perform these tasks, no special tools are needed.

A textual dictionary is not supposed to be used by StarDict directly. 
StarDict will continue to use binary dictionaries.
To convert between binary and textual file formats use special converters:
stardict-bin2text (StarDict normal dictionary binary to text), 
stardict-text2bin (StarDict normal dictionary text to binary).

The textual format must reflect the binary format as close as possible, 
not sacrificing benefits of textual format, of course.

The textual format should not be used for the purpose of redistributing
dictionaries, long-term storage of dictionaries. The textual file format may
be changed in the future breaking backward compatibility.


Textual file format is xml-based.

Textual file format should be used to store only textual data, binary data like
images, sounds, attachments should be stored in independent files (maybe
resource storage). A textual file may contain references to binary data, for
example, refer to an image file by name. 

The order of items in textual dictionary does not matter. Conversion tool takes
care that articles are stored in a binary dictionary in the right order. 


Problems
========

viewing and editing xml files
-----------------------------

A dictionary in textual format is a large file. It may take 10-50 Mb or more on
the disk. A textual dictionary has size of uncompressed binary dictionary plus
extra space for tags. You should add roughly 100 bytes per each index word. 

XML tags add to file size, however a dictionary file has to be large, simple
because a typical dictionary is large, it contains much data.

How to view and edit a large textual file?

Not every text editor copes well with files of these sizes, but there are
editors that can do, Kate for example.

If you simply open a large (several Mb) xml file in Kate, it take quite a while
before you can edit it, or you can easily be out of memory. The solution here is
to change the file extension. Use .txml, for example. Then Kate open the file as
a plan text file, that is fast. XML tags are not highlighted, of course. You can
view the file without noticeable delays. Search is adequately fast for a file of
such size. Editing is still a problem. Editing is an inherent problem of large
files. The whole is rewritten each time you save changed.

The solution here is to split a large xml file into smaller parts. XML
Inclusions (XInclude) are used to combine xml chunks.

whitespace normalization
------------------------

Attribute whitespace normalization is normal behavior in XML 1.0. All XML parsers 
must normalize an attribute's value before reporting it to other applications.
This may cause problems with some values. The key attribute of the resource element
contains a file name, whitespace normalization here is unwanted. Maybe we should
turn the key attribute into a child element.

textual file format
===================

Unless stated otherwise, the order of elements does not matter. There may be
a preferred order for readability purpose.

root element: stardict

stardict contains: info & contents* & article*
info element is mandatory, contents and article are optional
'stardict' must contain one 'info' element
'stardict' may contain any number of 'contents' and 'article' elements
A dictionary must contain at least one article.
Normally the info section should come first.

info contains: version & bookname & author? & email? & website? & description? & date? & dicttype?
An info section contains general information about a dictionary, it corresponds
to .ifo file of a binary dictionary. See doc/StarDictFileFormat for the meaning
of info section subelements.
All elements may contain only text, may appear 0 or 1 times.
Mandatory elements: version, bookname.

contents contains: article* & contents*
'contents' may be empty,
'contents' may contain other 'contents' elements and 'article' elements
'contents' should be used as a root element of an included xml file. Included
files may include other files them selves. Thus we may get a hierarchy of
'contents' elements. We may put any number of 'contents' elements in one
document at the same level. We have to allows this because of includes, an xml
document must contain only one root element. 'contents' elements may be useful
for specifying default attributes, for example, content type of a definition.
So 'contents' element is an implementation artifact.

'contents' element may have optional 'type' attribute. See 'type' attribute of
the 'definition' element for details.

article contains: key & synonym* & (definition | definition-r)+
must be 1 'key'
must be 0 or more 'synonym'
must be 1 or more 'definition' or 'definition-r'
'article' element may have optional 'type' attribute. See 'type' attribute of
the 'definition' element for details.

'definition' element may contain only plain text, no markup.

Rationale

Textual file format was designed to reflect structure of a dictionary. Structure
of an article is another question. To keep file format simple, let's do not mix
two different things.

'definition' element may have an optional 'type' attribute. The 'type' attribute
specifies content type of the definition. See "Type identifies" in
doc/StarDictFileFormat for possible values. Content type 'r' is disallowed, use
'definition-r' element for this content type.

Content type of definition may be specified on different levels (in the order
of increasing precedence): contents, article, definition. If we specify content
type on a 'contents' element, then all definition inside the 'contents' element
by default have that content type. You may overwrite content type of a specific
definition. 

Often all definitions in a dictionary have the same content type, you may save
a lot of typing if you put all articles in a 'contents' group and specify
content type on the group only.

Each 'definition' must have content type specified. It may be specified on that
'definition' explicitly or inherited from one of its parent elements.

definition-r: resource+
definition-r is a special type of definition for content type 'r'.
It contains one or more resource elements.

resource - empty element
contains attributes: type, key
both attributes are mandatory
the type attribute may be ( img | snd | vdo | att )

Leading and trailing blanks in element values are considered insignificant. They
are removed by textual-to-binary converter. This applies to all items of the
info block, values of 'definition' elements. 

tools
=====

stardict-text2bin and stardict-bin2text

stardict-bin2text must do its best to recover from errors. We should skip items
we cannot read correctly. The reason for that is that binary dictionary cannot
not be fixed. We must extract as much information as possible from it.
stardict-bin2text should do possible corrections to data, for example, remove
invalid chars, like control chars. 

stardict-bin2text on the other hand is allowed to stop on the first error. The
source data are in xml format is cannot be fixed by the user. We may apply
simple fixes to data, but we should not encourage the user to ignore textual
dictionary standards. 

Validating textual dictionary
=============================

stardict-textual-dict.rnc and stardict-textual-dict.rng files contain RELAX NG
schema for textual format of StarDict dictionary. .rnc - compact syntax, .rng -
XML syntax. Both definitions are equivalent. You can translate the XML syntax to
and from the compact syntax using existing translators.

For example with trang (http://code.google.com/p/jing-trang/)  you can do that
this way:
# .rnc to .rng
java -jar trang.jar stardict-textual-dict.rnc stardict-textual-dict.rng
or
trang stardict-textual-dict.rnc stardict-textual-dict.rng

# .rng to .rnc
java -jar trang.jar stardict-textual-dict.rng stardict-textual-dict.rnc
or
trang stardict-textual-dict.rng stardict-textual-dict.rnc

To validate a textual dictionary with xmllint (http://xmlsoft.org/xmllint.html)
use this command:
xmllint --noout --xinclude --relaxng stardict-textual-dict.rng dict.xml

xmllint understands only xml syntax.

RELAX NG schema open questions
------------------------------

We use the default token datatype for content type and resource type attributes.
This implies that attribute value undergoes whitespace normalization before
validation.
Maybe we should change datatype to string to match values as is.
