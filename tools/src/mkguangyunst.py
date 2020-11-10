import sys, string
for line in sys.stdin.readlines():
	words = string.split(line[:-1], '\t')
	muci  = words[1]
	sheng = words[2]
	deng  = words[3]
	hu    = words[4]
	yunbu = words[5]
	diao  = words[6]
	fanqie= words[7]
	she   = words[8]
	chars = words[9]
	romazi= words[10]
	beizhu= words[12]
	pinyin= words[13]
	psyun = words[22]
	if beizhu == '':
		print("%s\t%s %s%s%s%s%s%s %sQIE PINYIN%s PSYUN%s\\n%s" % (romazi, muci, sheng, yunbu, she, hu, deng, diao, fanqie, pinyin, psyun, chars))
	else:
		print("%s\t%s %s%s%s%s%s%s %sQIE PINYIN%s PSYUN%s\\n%s\\n%s" % (romazi, muci, sheng, yunbu, she, hu, deng, diao, fanqie, pinyin, psyun, chars, beizhu))
