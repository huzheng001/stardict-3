import sys, string
base = {}
for line in sys.stdin.readlines():
	words = string.split(line[:-1], '\t')
	if len(words) != 2:
		print("Error!")
		exit
	if words[0] in base:
		base[words[0]] += [words[1]]
	else:
		base[words[0]] = [words[1]]
keys = list(base.keys())
keys.sort()
for key in keys:
	print(key,'\t', end=' ')
	for val in base[key]:
		print(val,',', end=' ')
	print()
