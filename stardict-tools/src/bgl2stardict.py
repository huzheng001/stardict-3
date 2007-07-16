#!/usr/bin/env python

import sys
import os

def main():
	for arg in sys.argv[1:]:
		start = arg.rfind('/')
		if (start == -1):
			start = 0
		else:
			start+=1
		dictname = arg[start:arg.rfind('.')]
		os.system("./bgl2txt "+ arg);
		os.system("./babylon "+dictname+".babylon")
		dirname = "stardict-babylon-"+dictname+"-2.4.2"
		os.system("rm -rf " + dirname + ";mkdir "+dirname);
		os.system("mv "+dictname+".ifo "+dirname);
		os.system("mv "+dictname+".idx "+dirname);
		os.system("mv "+dictname+".dict.dz "+dirname);
		os.system("mv "+dictname+".syn "+dirname);
		os.system("rmdir res;mv res "+dirname);
		os.system("chown -R root.root "+dirname);
		os.system("chmod 644 "+dirname+"/*");
		os.system("chmod 755 "+dirname+"/res");
		os.system("chmod 644 "+dirname+"/res/*");
		os.system("tar -cjvf "+dirname+".tar.bz2 "+dirname);

if __name__ == "__main__":
	main()
