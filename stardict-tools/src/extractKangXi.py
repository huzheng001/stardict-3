#!/usr/bin/python
# -*- coding: utf-8 -*-
import sys, os, string, re, glob
import libxml2dom
fencoding = 'utf-8'
whattoextract = u'康熙字典'

#def TextInNode(node):
#	result = u''
#	for child in node.childNodes:
#		if child.nodeType == child.TEXT_NODE:
#			result += child.nodeValue
#		else:
#			result += TextInNode(child)
#	return result

filelist = glob.glob('*.htm')
filenum = len(filelist)
num = 0
errorfiles = []
for filename in filelist:
	num += 1
	print >> sys.stderr, filename, num, 'of', filenum
	try:
		fp = open(filename, 'r')
		doc = libxml2dom.parseString(fp.read(), html=1)
		fp.close()
		style = doc.getElementsByTagName("style")[0].textContent
		style = re.search(r'(?s)\s*\.(\S+)\s*{\s*display:\s*none', style)
		displaynone = style.group(1)
		tabpages = doc.getElementsByTagName("div")
		tabpages = filter(lambda s: s.getAttribute("class") == "tab-page", tabpages)
		for tabpage in tabpages:
			found = False
			for node in tabpage.childNodes:
				if node.nodeType == node.ELEMENT_NODE and node.name == 'h2':
					if node.textContent == whattoextract:
						found = True
					break
			if found:
				spans = tabpage.getElementsByTagName("span")
				for span in spans:
					if span.getAttribute("class") == "kszi":
						character = span.textContent
				paragraphs = tabpage.getElementsByTagName("p")
				thisitem = character + u'\t'
				for paragraph in paragraphs:
					if paragraph.getAttribute("class") <> displaynone:
						#print TextInNode(paragraph).encode(fencoding)
						text = paragraph.textContent
						#text = filter(lambda s: not s in u' \t\r\n', text)
						text = re.sub(r'\s+', r' ', text)
						thisitem += text + u'\\n'
				print thisitem.encode(fencoding)
	except:
		print >> sys.stderr, 'error occured'
		errorfiles += [filename]
		continue
if errorfiles:
	print >> sys.stderr, 'Error files:', '\n'.join(errorfiles)
