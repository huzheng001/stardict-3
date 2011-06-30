#!/usr/bin/python
# WinVNKey Hannom Database to Stardict dictionary source Conversion Tool
# coded by wesnoth@ustc on 070804
# http://winvnkey.sourceforge.net
import sys, os, string, types, pprint
infileencoding = 'utf-16-le'
outfileencoding = 'utf-8'

def showhelp():
	print "Usage: %s filename" % sys.argv[0]

def ishantu(str):
	if len(str) > 0 and ord(str[0]) > 0x2e80:
		return True
	else:
		return False

def mysplit(line):
	status = 0 # 0: normal, 1: quote
	i = 0
	line = line.lstrip()
	linelen = len(line)
	while i < linelen:
		if status == 0 and line[i].isspace():
			break
		if line[i] == u'"':
			status = 1 - status
		i += 1
	#print 'mysplit: i=%d, line=%s' % (i, `line`)
	if i == 0:
		return []
	else: 
		line = [line[:i], line[i:].strip()]
		if line[1] == u'':
			return [line[0]]
		else:
			return line

if __name__ == '__main__':
	if len(sys.argv) <> 2:
		showhelp()
	else:
		fp = open(sys.argv[1], 'r')
		print 'Reading file...'
		lines = unicode(fp.read(), infileencoding).split(u'\n')
		lineno = 0
		hugedict = {}
		print 'Generating Han-Viet dict...'
		for line in lines:
			lineno += 1
			if line.endswith(u'\r'):
				line = line[:-1]
			if line.startswith(u'\ufeff'):
				line = line[1:]
			ind = line.find(u'#')
			if ind >= 0:
				line = line[:ind]
			line = mysplit(line)
			if len(line) == 0:
				continue
			elif len(line) == 1:
				continue # ignore this incomplete line
			if line[0].startswith(u'"') and line[0].endswith(u'"'):
				line[0] = line[0][1:-1]
			if line[0].startswith(u'U+') or line[0].startswith(u'u+'):
				line[0] = unichr(int(line[0][2:], 16))
			if not ishantu(line[0]):
				continue # invalid Han character
				#print 'error occurred on line %d: %s' % (lineno, `line`)
			if line[1].startswith(u'"') and line[1].endswith(u'"'):
				line[1] = line[1][1:-1]
			line[1] = filter(None, map(string.strip, line[1].split(u',')))
			#hugedict[line[0]] = hugedict.get(line[0], []) + line[1]
			for item in line[1]:
				if not hugedict.has_key(line[0]):
					hugedict[line[0]] = [item]
				elif not item in hugedict[line[0]]:
					hugedict[line[0]] +=  [item]
			#print lineno, `line`
		#for hantu, quocngu in hugedict.iteritems():
		#	print hantu.encode('utf-8'), ':', 
		#	for viettu in quocngu:
		#		print viettu.encode('utf-8'), ',', 
		#	print
		fp.close()
		print 'Generating Viet-Han dict...'
		dicthuge = {}
		for hantu, quocngu in hugedict.iteritems():
			for viettu in quocngu:
				if not dicthuge.has_key(viettu):
					dicthuge[viettu] = [hantu]
				elif not hantu in dicthuge[viettu]:
					dicthuge[viettu] +=  [hantu]
		print 'Writing Han-Viet dict...'
		gp = open('hanviet.txt', 'w')
		for hantu, quocngu in hugedict.iteritems():
			gp.write(hantu.encode('utf-8'))
			gp.write('\t')
			gp.write((u', '.join(quocngu)).encode('utf-8'))
			gp.write('\n')
		gp.close()
		print 'Writing Viet-Han dict...'
		gp = open('viethan.txt', 'w')
		for quocngu,hantu in dicthuge.iteritems():
			gp.write(quocngu.encode('utf-8'))
			gp.write('\t')
			gp.write((u' '.join(hantu)).encode('utf-8'))
			gp.write('\n')
		gp.close()
