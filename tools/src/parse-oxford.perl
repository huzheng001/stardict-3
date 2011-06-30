#!/usr/bin/perl -w

# My attempt to fix StarDict Oxford (GB) dictionary, v0.3
# by Anthony Fok (2006-06-01 Thursday)
#
# Previous version: v0.2 (2005-10-14 Friday)
#
# License: GNU General Public License v2 or later.
#
# Todo: Make "use strict";
#       Fix more bugs.
#       Contact Hu Zheng


use utf8;					# 因为此脚本含有 UTF-8 编码的字符
use encoding 'utf8';				# 读进来的数据文件的字符串以 UTF-8 编码

open(INDEX, "<oxford-gb.idx") or die;		# 打开原来的辞典索引文件进行读入
binmode(INDEX);					# (MS-DOS 兼容) 二进制文件格式
open(DICT, "<oxford-gb.dict") or die;		# 打开原来的辞典文件进行读入
binmode(DICT);

open INDEX_OUT, ">oxford-gb-new.idx" or die;	# 打开修正的辞典文件进行写出
binmode(INDEX_OUT);
open DICT_OUT, ">:utf8", "oxford-gb-new.dict" or die;

$dups = 0;
$wordcount = 0;
$offset_out = 0;

$/ = "\0";					# 把输入文件的“行尾”定义为“空”(NULL)


while ($word = <INDEX>) {
	chomp($word);
	if ($word eq "-ability") {
		$word = $word.", -ibility";
	} elsif ($word eq "-able") {
		$word .= ", -ible";
	} elsif ($word eq "-ably") {
		$word .= ", -ibly";
	} elsif ($word eq "-ance") {
		$word .= ", -ence";
	} elsif ($word eq "-ancy") {
		$word .= ", -ency";
	} elsif ($word eq "-ant") {
		$word .= ", -ent";
	}

	read(INDEX, $buffer, 8) or die;
	($offset, $length) = unpack("NN", $buffer);

	print "$word: $offset, $length\n";


	my $definition = "";

	seek(DICT, $offset, 0);
	$act_read = read(DICT, $definition, $length);
	die if ($act_read != $length);

	@lines = split "\n", $definition;

	print 'lines: ' . $#lines . "\n";
	if ($#lines >= 2 and $lines[0] eq $lines[2] and $lines[1] eq "") {
		print STDERR "Duplicate line(s) found in '$word'!\n";
		$definition = $lines[0];
		$dups++;
	}

	# 手工看出来的；$lines[0] 跟 $lines[2] 有一丁点排版上的区别，但其实内容一模一样
	if ($word eq "a priori" or
	    $word eq "a posteriori" or
	    $word eq "A level" or
	    $word eq "ad infinitum" or
	    $word eq "ad lib") {
		print STDERR "Duplicate line(s) found in '$word'!\n";
		$definition = join("\n", @lines[ 0 .. $#lines/2 ]);
		$dups++;
	}

	if ($word eq "-ability, -ibility" or $word eq "-ancy, -ency"
	    and $#lines == 2) {
		$definition = $lines[2];
	}

	$definition =~ s/、 /、/g;
	$definition =~ s/(?<=[这那什怎多要好])麽/么/g;
	$definition =~ s/甚(?:麽|么)/什么/g;
	$definition =~ s/麽(?=\?)/么/g;
	$definition =~ s/著/着/g;
	$definition =~ s/(?<=[土原新名])着/著/g;
	$definition =~ s/着(?=作)/著/g;
	$definition =~ s/於/于/g;

	$definition =~ s/certainc/certain c/;
	$definition =~ s/我一定在屋  等着/我一定在屋里等着/;
	$definition =~ s/应该看报以便  解时事/应该看报以便了解时事/;


	@lines = split "\n", $definition;

	foreach $line (@lines) {
		if ($line =~ /^\/(.*?;.*?)\//g) {
			$pronunciation = $1;
			$pronunciation =~ s/^ //;
#			$pronunciation =~ tr/59:\\[^ABCEFgIJLNQRTUVWZ/ˈˌːɜɝgæɑɒəʃɡɪᴜɚŋʌɔðʊʒθɛ/;
			# 参考 Ksphonet.TTF，不过没有全跟
			$pronunciation =~ tr/156789:<=ABCDEFGHIJKLMNOPQRTUVWXZ[\\]^g/…ˈ!ˌ?ˌːüêæɑɒãəʃɣɥɪᴜʏɚɲŋœɵʌɔðʊʒθøɛɝɜçˇgɡ/;
			$pronunciation =~ s/4/<i>:<\/i>/g;
			# Combining Unicode
			$pronunciation =~ s/S/ɔ̃/g;
			$pronunciation =~ s/Y/ɛ̃/g;
			$pronunciation =~ s/\?\@/<i>US<\/i>/g;
			# 把修正后的 IPA 读音放回去
			substr($line, 0, pos($line), '/' . $pronunciation . '/');
		}
	}

	$definition = join("\n", @lines);
	print $definition . "\n\n";
	$wordcount++;

	$length_out = do { use bytes; length($definition) };
	print INDEX_OUT pack("Z*NN", $word, $offset_out, $length_out);
	$offset_out += $length_out;
	print DICT_OUT $definition or die;

}

print "wordcount=$wordcount\n";
print "Number of entries with duplicate lines: $dups\n";
