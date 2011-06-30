#!/usr/bin/perl -w
# by ChaosLawful@SMTH at 2006-3-23
use strict;
use Encode;
use FileHandle;
use Fcntl ':seek';	# for constants SEEK_*

my @ncce_dicts=("ec","ce");

for my $ncce_dict (@ncce_dicts) {
	my $buf;
	my $idxFH=new FileHandle("$ncce_dict.idx") or die;
	my $libFH=new FileHandle("$ncce_dict.lib") or die;
	my $outFH=new FileHandle(">ncce_$ncce_dict.tab") or die;
	binmode($idxFH);
	binmode($libFH);
	binmode($outFH);

	sysread($idxFH,$buf,4);
	my ($totalRecord)=unpack("L",$buf);	# got total record number

	for my $idxNo (1..$totalRecord) {
		seek($idxFH,$idxNo*4,SEEK_SET);	# find offset
		sysread($idxFH,$buf,4);

		my ($off)=unpack("L",$buf);	# seek into lib file
		seek($libFH,$off,SEEK_SET);
		{
			# and read corresponding entry record
			local $/=chr(1);
			chomp($buf=<$libFH>);
		}

		$buf=pack "C*",map $_+0x1e,unpack "C*",$buf;	# decrypt record

		$buf=~s/\\/\\\\/gs;
		my @fields=split(/\x1e/,$buf);	# split entry into word and explanation
		toTextDict($outFH,$fields[0],$fields[1]);	# output
	}
}

sub toTextDict
{
	my ($fh,$word,$explain)=@_;
	# Kingsoft custom dictionary's export format:
	# every line contains one entry, whose format is:
	# <word>|<explanation>
	# where <explanation>:=<literal>[\r\n<explanation>]

	# strip leading and trailing spaces, squeeze inner spaces
	$word=~s/^\s+//gs;
	$word=~y/ / /s;
	$word=~s/\s+$//gs;

	$explain=~s/^\s+//gs;
	$explain=~y/ / /s;
	$explain=~s/\s+$//gs;

	# split NCCE entry into multiple explanations, optional
	$explain=~s/;\s*/\\n/gs;
	# convert fullwidth comma between alphadigits into halfwidth comma, optional
	$word=~s/\xa3\xac(?=\w)/,/gs;
	$explain=~s/\xa3\xac(?=\w)/,/gs;
	# convert to utf-8
	$word=encode("utf-8",decode("cp936",$word));
	$explain=encode("utf-8",decode("cp936",$explain));
	print $fh "$word\t$explain\n";
}

