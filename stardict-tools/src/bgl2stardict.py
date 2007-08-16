#!/usr/bin/env python

import sys
import os

def main():
	start = sys.argv[1].rfind('/')
	if (start == -1):
		start = 0
	else:
		start+=1
	dictname = sys.argv[1][start:sys.argv[1].rfind('.')]
	command = "./bgl2txt"
	for arg in sys.argv:
		command += " "
		command += arg
	os.system(command);
	os.system("./babylon "+dictname+".babylon")
	dirname = "stardict-babylon-"+dictname+"-2.4.2"
	os.system("rm -rf " + dirname + ";mkdir "+dirname);
	os.system("mv "+dictname+".ifo "+dirname);
	os.system("mv "+dictname+".idx "+dirname);
	os.system("mv "+dictname+".dict.dz "+dirname);
	os.system("mv "+dictname+".syn "+dirname);
	os.system("rmdir res;mv res "+dirname);
	os.system("chown -R root.root "+dirname);
	os.system("chmod 644 "+dirname+"/*.*");
	os.system("chmod -R 644 "+dirname+"/res");
	os.system("chmod 755 "+dirname+"/res");
	os.system("tar -cjf "+dirname+".tar.bz2 "+dirname);

if __name__ == "__main__":
	main()
