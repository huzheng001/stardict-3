#!/usr/bin/perl
use strict;
use warnings;
use File::Find;
use Getopt::Long;
use Pod::Usage;
my $dicts_dir='.';
my $help=0;
GetOptions('dicts-dir|d=s' => \$dicts_dir, 'help' => \$help) or pod2usage(2);
pod2usage(1) if $help;

=head1 NAME

dicts-dump - list StarDict dictionaries info

=head1 SYNOPSIS

dicts-dump [options]

  Options:
    --help                brief help message
    --dicts-dir|-d=FILE   dictionary directory, '.' - default

=head1 OPTIONS

=over 8

=item B<--help>

Print a brief help message and exit.

=item B<-d DIR, --dicts-dir=DIR>

Specifies the base directory of dictionaries. Current directory is used by default.

=back

=cut

&PrintHtmlBegin();
find( { wanted => \&wanted, preprocess => \&preprocess }, $dicts_dir);
&PrintHtmlEnd();

sub wanted
{
	my $item = $_;
	return if $item eq "." or $item eq "..";
	
	my $rel_name = substr($File::Find::name, length($dicts_dir));
	$rel_name =~ s!^[\\/]*!!;
	print "<p><a href='$rel_name'>$rel_name</a></p>\n";
	if(-d $item) {
	} else {
		return if /\.dat\.bz2$/i;
		if(not /\.tar\.bz2$/) {
			print STDERR "warning: unknown file format: $item\n";
			return;
		}
		my $contents = 
		#`bzip2 --decompress --stdout "$item" | tar --extract --file - --to-stdout "*.ifo"`;
		`bzip2 --decompress --stdout "$item" | tar -x -f - -O "*.ifo"`;
		$contents =~ s/^StarDict's dict ifo file$/type=index dictionary/mg;
		$contents =~ s/^StarDict's treedict ifo file$/type=tree dictionary/mg;
		&PrintTable($contents);
	}
}

sub preprocess
{
	my (@dirs, @files);
	foreach (@_) {
		next if $_ eq "." or $_ eq "..";
		if(-d $_) {
			push @dirs, $_;
		} else {
			push @files, $_;
		}
	}
	return (sort(@files), sort(@dirs));
}

sub PrintHtmlBegin
{
print <<'EOL';
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml">
<head>
<meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
<title>Directory contents</title>
<style TYPE="text/css">
td {
vertical-align: top
}
</style>
</head>
<body>
EOL
}

sub PrintHtmlEnd
{
print <<'EOL';
</body>
</html>
EOL
}

sub PrintTable
{
	my ($contents) = @_;
	print <<'EOL';
<table>
EOL

	$contents =~ s/&/&amp;/g;
	$contents =~ s/</&lt;/g;
	$contents =~ s/>/&gt;/g;
	$contents =~ s/"/&quot;/g;
	$contents =~ s/^(?=.)/<tr><td>/gm;
	$contents =~ s!^(.*?)=!$1</td><td>!gm;
	$contents =~ s!(?<=.)$!</td></tr>!gm;
	print $contents;

	print <<'EOL';
</table>
EOL
}
