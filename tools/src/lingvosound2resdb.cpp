/*
 * This file is part of StarDict.
 *
 * StarDict is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * StarDict is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with StarDict.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <string.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <vector>
#include <sstream>
#include <algorithm>

#include <glib.h>
#include <glib/gstdio.h>

// See description in ParseCommandLine function

struct TSoundSample {
	std::string OrigFileName;
	guint32 Start;
	guint32 Duration;
	// file name where sound sample will be extracted temporary
	std::string TmpFileName;
	// file name that will be used in the resource database
	std::string DbFileName;

	TSoundSample(const std::string& f, guint32 s, guint32 d)
	: OrigFileName(f), Start(s), Duration(d)
	{
	}
};

typedef std::vector<TSoundSample> TSoundSampleVec;

struct TLingvoSoundFileData {
	TSoundSampleVec Samples;
	// number of sound samples packed in the Lingvo sound file
	guint32 NumSamples;
	// offset of the sound data in Lingvo .dat file
	long SoundDataOffset;
	// dat file
	std::string LingvoSoundFile;
	// sound data extracted from Lingvo sound file, in .ogg format
	std::string SoundDataFile;
	// directory where sound samples will extracted
	std::string SoundSamplesDir;
	/* resource database file base (without extension)
	 * Produced files will be: 
	 * ResDbBase + ".rifo", 
	 * ResDbBase + ".ridx",
	 * ResDbBase + ".rdic" */
	std::string ResDbBase;
	// output format, extension of sample files without dot
	std::string OutputExt;
	// sox effects applied to each produced sample
	// This string is appended to each sox command.
	std::string SoxEffects;
	
	// options
	// print additional debugging information
	gboolean op_verbose;
	gboolean op_no_split;
	gboolean op_no_sound_extract;

	TLingvoSoundFileData(void)
	:
	NumSamples(0), 
	SoundDataOffset(0),
	OutputExt("ogg"),
	op_verbose(FALSE),
	op_no_split(FALSE),
	op_no_sound_extract(FALSE)
	{
		
	}
};

// this function is used to sort sound samples
bool SoundSamplesSort(const TSoundSample& v1, const TSoundSample& v2)
{
	// Stardict requires this function be used to sort index words
	return strcmp(v1.DbFileName.c_str(), v2.DbFileName.c_str()) < 0;
}

void ReadTilStopByte(FILE *fh, int StopByte, std::vector<char>& res)
{
	res.clear();
	int c;
	while(true) {
		c = fgetc(fh);
		if(c == EOF) {
			std::cerr << "Unexpected end of file" << std::endl;
			exit(1);
		}
		if(c == StopByte)
			break;
		res.push_back((char)c);
	}
}

guint32 Readguint32(FILE *fh)
{
	guint32 ui;
	if(1 != fread(&ui, sizeof(ui), 1, fh)) {
		std::cerr << "Unexpected end of file" << std::endl;
		exit(1);
	}
	return ui;
}

std::string BufUTF16ToUTF8(const std::vector<char>& buf)
{ 
	gchar* gstr = g_utf16_to_utf8((const gunichar2*)&buf[0], buf.size()/2, NULL, 
		NULL, NULL);
	if(!gstr) {
		std::cerr << "Unable to convert string" << std::endl;
		exit(1);
	}
	std::string str(gstr);
	g_free(gstr);
	return str;
}

void ReadLingvoSoundFile(TLingvoSoundFileData& LingvoData)
{
	FILE *fh = NULL;
	std::vector<char> buf;

	fh = fopen(LingvoData.LingvoSoundFile.c_str(), "rb");
	if(!fh) {
		std::cerr << "Unable to open file: " << LingvoData.LingvoSoundFile 
			<< std::endl;
		exit(1);
	}
	// skip header
	ReadTilStopByte(fh, 0xff, buf);
	LingvoData.NumSamples = Readguint32(fh);
	LingvoData.Samples.clear();
	LingvoData.Samples.reserve(LingvoData.NumSamples);
	
	std::string SampleFileName;
	guint32 SampleStart, SampleDuration;
	
	// file name
	ReadTilStopByte(fh, 0x0D, buf);
	SampleFileName = BufUTF16ToUTF8(buf);
	ReadTilStopByte(fh, 0xff, buf);
	SampleStart = 0;
	SampleDuration = Readguint32(fh);
	LingvoData.Samples.push_back(
		TSoundSample(SampleFileName, SampleStart, SampleDuration)
	);
	for(guint32 SampleNum = 1; SampleNum < LingvoData.NumSamples; ++SampleNum) {
		ReadTilStopByte(fh, 0x0D, buf);
		SampleFileName = BufUTF16ToUTF8(buf);
		ReadTilStopByte(fh, 0xff, buf);
		SampleStart = Readguint32(fh);
		fgetc(fh); // skip one byte
		SampleDuration = Readguint32(fh);
		LingvoData.Samples.push_back(
			TSoundSample(SampleFileName, SampleStart, SampleDuration)
		);
	}
	LingvoData.SoundDataOffset = ftell(fh);
	fclose(fh);
}

void ExtractSoundData(TLingvoSoundFileData& LingvoData)
{
	FILE *ifh, *ofh;
	ifh = fopen(LingvoData.LingvoSoundFile.c_str(), "rb");
	if(!ifh) {
		std::cerr << "Unable to open file: " << LingvoData.LingvoSoundFile << std::endl;
		exit(1);
	}
	ofh = fopen(LingvoData.SoundDataFile.c_str(), "wb");
	if(!ofh) {
		std::cerr << "Unable to open file: " << LingvoData.SoundDataFile << std::endl;
		exit(1);
	}
	size_t chunksize = 1024*1024;
	std::vector<char> buf(chunksize);
	if(fseek(ifh, LingvoData.SoundDataOffset, SEEK_SET)) {
		std::cerr << "Unable to fseek in file: " << LingvoData.LingvoSoundFile 
			<< std::endl;
		exit(1);
	}
	size_t bytes_read;
	while(true) {
		bytes_read = fread(&buf[0], 1, chunksize, ifh);
		if(!bytes_read)
			break;
		if(bytes_read != fwrite(&buf[0], 1, bytes_read, ofh)) {
			std::cerr << "Error while writing to file: " << LingvoData.SoundDataFile 
				<< std::endl;
			exit(1);
		}
	}
	fclose(ifh);
	fclose(ofh);
}

// create a directory if it does not exist
void PrepareDir(const std::string& dir)
{
	if (!g_file_test(dir.c_str(), 
		GFileTest(G_FILE_TEST_IS_DIR))) {
		if (g_mkdir(dir.c_str(), 0700)==-1) {
			std::cerr << "Unable to create directory: " << dir << std::endl;
			exit(1);
		}
	}
}

/* Generate file name that will be used to store sound samples */
void GenerateTmpFileNames(TLingvoSoundFileData& LingvoData)
{
	for(guint32 i=0; i<LingvoData.NumSamples; ++i) {
		std::ostringstream out;
		out << LingvoData.SoundSamplesDir << G_DIR_SEPARATOR << i << "." << LingvoData.OutputExt;
		LingvoData.Samples[i].TmpFileName = out.str();
	}
}

void SplitSoundFile(TLingvoSoundFileData& LingvoData)
{
	for(guint32 i=0; i<LingvoData.NumSamples; ++i) {
		std::cout << "splitting " << i << " of " << LingvoData.NumSamples 
			<< std::endl;
		std::ostringstream cmd;
		cmd << "sox \"" << LingvoData.SoundDataFile + "\" \""
			<< LingvoData.Samples[i].TmpFileName << "\" trim "
			<< LingvoData.Samples[i].Start << "s "
			<< LingvoData.Samples[i].Duration << "s "
			<< LingvoData.SoxEffects;
		if(!g_spawn_command_line_sync(cmd.str().c_str(), NULL, NULL, NULL, NULL)) {
			std::cerr << "Error splitting sound file" << std::endl;
			exit(1);
		}
	}
}

// generate file names that will be used in database
void GenerateDatabaseFileNames(TLingvoSoundFileData& LingvoData)
{
	for(guint32 i=0; i<LingvoData.NumSamples; ++i) {
		const std::string& OrigFileName = LingvoData.Samples[i].OrigFileName;
		size_t pos = OrigFileName.find_last_of('.');
		if(pos == std::string::npos) {
			std::cerr << "File name without extension not supported: "
				<< OrigFileName << std::endl;
			exit(1);
		}
		LingvoData.Samples[i].DbFileName = OrigFileName.substr(0, pos+1) + LingvoData.OutputExt;
	}
}

// for debug purpose
void PrintLingvoData(TLingvoSoundFileData& LingvoData)
{
	std::cout << "NumSamples: " << LingvoData.NumSamples << "\n"
		<< "SoundDataOffset: " << LingvoData.SoundDataOffset << "\n"
		<< "LingvoSoundFile: " << LingvoData.LingvoSoundFile << "\n"
		<< "SoundDataFile: " << LingvoData.SoundDataFile << "\n"
		<< "SoundSamplesDir: " << LingvoData.SoundSamplesDir << "\n"
		<< "ResDbBase: " << LingvoData.ResDbBase << "\n";
	for(guint32 i=0; i<LingvoData.NumSamples; ++i) {
		TSoundSample& sample = LingvoData.Samples[i];
		std::cout << "sample " << i << "\n"
			<< "\tOrigFileName: " << sample.OrigFileName << "\n"
			<< "\tStart: " << sample.Start << "\n"
			<< "\tDuration: " << sample.Duration << "\n"
			<< "\tTmpFileName: " << sample.TmpFileName << "\n"
			<< "\tDbFileName: " << sample.DbFileName << "\n";
	}
	std::cout.flush();
}

void SortSamples(TLingvoSoundFileData& LingvoData)
{
	std::sort(LingvoData.Samples.begin(), LingvoData.Samples.end(), 
		SoundSamplesSort);
}

void GenerateResourceDatabase(TLingvoSoundFileData& LingvoData)
{
	std::string InfoFileName = LingvoData.ResDbBase + ".rifo";
	std::string IndexFileName = LingvoData.ResDbBase + ".ridx";
	std::string DictFileName = LingvoData.ResDbBase + ".rdic";

	// generating index and database files
	FILE *ifh = NULL;
	FILE *dfh = NULL;
	ifh = fopen(IndexFileName.c_str(), "wb");
	if(!ifh) {
		std::cerr << "Unable to open file for writing: " << IndexFileName 
			<< std::endl;
		exit(1);
	}
	dfh = fopen(DictFileName.c_str(), "wb");
	if(!dfh) {
		std::cerr << "Unable to open file for writing: " << DictFileName 
			<< std::endl;
		exit(1);
	}
	guint32 offset = 0;
	guint32 t;
	gchar* TmpFileContents = NULL;
	gsize TmpFileLength = 0;
	for(guint32 i=0; i<LingvoData.NumSamples; ++i) {
		const TSoundSample& sample = LingvoData.Samples[i];
		if(g_file_get_contents(sample.TmpFileName.c_str(), &TmpFileContents, 
			&TmpFileLength, NULL)) {
			fwrite(TmpFileContents, TmpFileLength, 1, dfh);
			g_free(TmpFileContents);
		} else {
			std::cerr << "Unable to read temporary file: " << sample.TmpFileName
				<< std::endl;
			exit(1);
		}
		// file name with terminating '\0' char
		fwrite(sample.DbFileName.c_str(), sample.DbFileName.length() + 1, 1, ifh);
		// offset
		t = g_htonl(offset);
		fwrite(&t, 4, 1, ifh);
		// size
		t = g_htonl(TmpFileLength);
		fwrite(&t, 4, 1, ifh);
		offset += TmpFileLength;
	}
	long IndexFileSize = ftell(ifh);
	fclose(ifh);
	fclose(dfh);

	// generating info file
	std::ostringstream str;
	str << "StarDict's storage ifo file\n"
		<< "version=3.0.0\n"
		<< "filecount=" << LingvoData.NumSamples << "\n"
		<< "ridxfilesize=" << IndexFileSize << "\n";
	FILE *fh = NULL;
	fh = fopen(InfoFileName.c_str(), "wb");
	if(!fh) {
		std::cerr << "Unable to open file for writing: " << InfoFileName 
			<< std::endl;
		exit(1);
	}
	fwrite(str.str().c_str(), str.str().length(), 1, fh);
	fclose(fh);
	
}

void ParseCommandLine(TLingvoSoundFileData& LingvoData, int argc,char * argv [])
{
	LingvoData.LingvoSoundFile = "SoundE.dat";
	LingvoData.SoundDataFile = "SoundE.ogg";
	LingvoData.SoundSamplesDir = "soundfiles";
	LingvoData.ResDbBase = "res";
	gchar *lingvo_file_name = NULL;
	gchar *temp_sound_file_name = NULL;
	gchar *samples_dir_name = NULL;
	gchar *rifo_file_name = NULL;
	gchar *output_ext = NULL;
	gchar *sox_effects = NULL;
	static GOptionEntry entries[] = {
		{ "verbose", 'v', 0, G_OPTION_ARG_NONE, &LingvoData.op_verbose,
			"print additional debugging information", NULL },
		{ "no-split", 't', 0, G_OPTION_ARG_NONE, &LingvoData.op_no_split,
			"skip samples extraction phase", NULL },
		{ "no-sound-extract", 'x', 0, G_OPTION_ARG_NONE, 
			&LingvoData.op_no_sound_extract,
			"skip sound extraction phase", NULL },
		{ "lingvo-sound-file", 'l', 0, G_OPTION_ARG_FILENAME, &lingvo_file_name,
			"Lingvo sound file to read", "file_name"},
		{ "temp-sound-file", 's', 0, G_OPTION_ARG_FILENAME, &temp_sound_file_name,
			"temp sound file extracted from Lingvo file", "file_name"},
		{ "samples-dir", 'd', 0, G_OPTION_ARG_FILENAME, &samples_dir_name,
			"directory to extract sound samples", 
			"dir_name"},
		{ "res-info-file", 'i', 0, G_OPTION_ARG_FILENAME, &rifo_file_name,
			"Stardict resource file with .rifo extension", 
			"file_name"},
		{ "output-ext", 0, 0, G_OPTION_ARG_STRING, &output_ext,
			"extension of sample files without dot",
			"ext"},
		{ "sox-effects", 0, 0, G_OPTION_ARG_STRING, &sox_effects,
			"sox effects applied to each sample",
			"effects"},
		{ NULL },
	};
	/* Here is a short explanation of how this utility works. That helps
	 * to understand the meaning of the options.
	 * 
	 * First, we need ABBYY Lingvo sound file. That is a file with .dat
	 * extension, it can be found in the Data directory of the Lingvo
	 * install directory. As of Lingvo 12, there are three such files:
	 * SoundD.dat, SoundE.dat, SoundF.dat for Deutsch, English and
	 * French respectively.
	 *
	 * Use --lingvo-sound-file option to specify the lingvo sound file.
	 * 
	 * In the first step ligvosound2resdb reads the lingvo sound
	 * file. That file contains names of sound sample, as well as sound
	 * itself.
	 * 
	 * In the second phase we extract the sound packed into the lingvo
	 * sound file into a separate file. That temporary file is specified
	 * with --temp-sound-file option.  We get an ordinary sound file in
	 * ogg format that can be played with any ogg-aware player. Note
	 * that that file must have .ogg extension.
	 * 
	 * In the third phase we split the ogg file into samples. They are
	 * saved into the samples directory that can be specified with the
	 * --samples-dir option. File are saved with names 1.ogg, 2.ogg, and
	 * so on. This phase requires sox utility. That is the longest phase,
	 * it takes 1-2 hours.
	 * 
	 * In the fourth phase we generate resource database file. That are
	 * three files with .rifo, .ridx, .rdic extension. You may specify
	 * rifo file with --res-info-file option. By default res.rifo file
	 * will be used and you get three files res.rifo, res.ridx, res.rdic
	 * in the current directory. These are the files you need to place
	 * into Stardict dictionary directory.
	 * 
	 * There are also some options that allow you to skip some
	 * phases. There are for the case when you run lingovsound2resdb the
	 * second and more time and you already extracted the ogg file and
	 * performed splitting. 
	 * 
	 * This program was tested with Lingvo 12.
	 */
	GOptionContext *popt_cnt = g_option_context_new("");
	g_option_context_add_main_entries(popt_cnt, entries, NULL);
	g_option_context_set_help_enabled(popt_cnt, TRUE);
	g_option_context_set_summary(popt_cnt, 
		"Convert ABBYY Lingvo sound file into Stardict resource database.\n"
		"\n"
		"Note: This program requires sox utility to split sound file into\n"
		"samples. You may get sox from http://sox.sourceforge.net/.\n"
		"\n"
		"See extra comments in the source file.\n"
		);
	GError *perr = NULL;
	if (!g_option_context_parse(popt_cnt, &argc, &argv, &perr)) {
		std::cerr << "Options parsing failed: " <<  perr->message << std::endl;
		exit(1);
	}

	if(lingvo_file_name)
		LingvoData.LingvoSoundFile = lingvo_file_name;
	if(temp_sound_file_name)
		LingvoData.SoundDataFile = temp_sound_file_name;
	if(samples_dir_name)
		LingvoData.SoundSamplesDir = samples_dir_name;
	if(rifo_file_name) {
		LingvoData.ResDbBase = rifo_file_name;
		size_t len = LingvoData.ResDbBase.length();
		if(LingvoData.ResDbBase.compare(len-5, 5, ".rifo") != 0) {
			std::cerr << "Resource database file must have .rifo extension" 
								<< std::endl; 
			exit(1);
		}
		// erase extension
		LingvoData.ResDbBase.erase(len-5);
	}
	if(output_ext)
		LingvoData.OutputExt = output_ext;
	if(sox_effects)
		LingvoData.SoxEffects = sox_effects;
	
	g_option_context_free(popt_cnt);
	if(perr)
		g_error_free(perr);
	g_free(lingvo_file_name);
	g_free(temp_sound_file_name);
	g_free(samples_dir_name);
	g_free(rifo_file_name);
	g_free(output_ext);
	g_free(sox_effects);
}

int
main(int argc,char * argv [])
{
	setlocale(LC_ALL, "");
	TLingvoSoundFileData LingvoData;
	
	ParseCommandLine(LingvoData, argc, argv);
	ReadLingvoSoundFile(LingvoData);
	if(!LingvoData.op_no_sound_extract)
		ExtractSoundData(LingvoData);
	PrepareDir(LingvoData.SoundSamplesDir);
	GenerateTmpFileNames(LingvoData);
	if(!LingvoData.op_no_split)
		SplitSoundFile(LingvoData);
	GenerateDatabaseFileNames(LingvoData);
	SortSamples(LingvoData);
	GenerateResourceDatabase(LingvoData);
	
	if(LingvoData.op_verbose)
		PrintLingvoData(LingvoData);
	std::cout << "Done!" << std::endl;

	return 0;
}
