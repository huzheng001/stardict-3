import sys, string
base = {}
for line in sys.stdin.readlines():
	words = string.split(line[:-1], '\t')
	if len(words) != 2:
		print "Error!"
		exit
	if base.has_key(words[0]):
		base[words[0]] += [words[1]]
	else:
		base[words[0]] = [words[1]]
keys = base.keys()
keys.sort()
for key in keys:
	print key,'\t',
	for val in base[key]:
		print val,',',
	print
