# This tool convert KangXiZiDian djvu files to tiff files.
# Download djvu files: http://bbs.dartmouth.edu/~fangq/KangXi/KangXi.tar
# Character page info: http://wenq.org/unihan/Unihan.txt as kIRGKangXi field.
# Character seek position in Unihan.txt http://wenq.org/unihan/unihandata.txt
# DjVuLibre package provides the ddjvu tool.
# The 410 page is bad, but it should be blank page in fact. so just remove 410.tif

import os

if __name__ == "__main__":
	os.system("mkdir tif")
	pages = list(range(1, 1683+1))
	for i in pages:
		page = str(i)
		print(page)
		os.system("ddjvu -format=tiff -page="+ page + " -scale=100 -quality=150 KangXiZiDian.djvu"+ " tif/" + page + ".tif")
