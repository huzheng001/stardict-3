/* prepares directory structure for NSIS installer (stardict-installer.nsi file)
To run the script, execute the command in Windows Console:
cscript //nologo stardict-prepare-installer.js
this script must run in the root StarDict directory
You may need to tweak the options below.
*/

// options
var InstallDirMustNotExist = false;
var VSConfig = "release"; // release or debug
var UnixDistDir = "stardict-unix-dist";
var UnixBuildDir = "stardict-build";

var BaseDir = GetBaseDir();
var InstallDir = BaseDir + "win32-install-dir\\";
var MSVSDir = BaseDir + "msvc_2008\\";
var fso = WScript.CreateObject("Scripting.FileSystemObject");
var shell = WScript.CreateObject("WScript.Shell");

function usage()
{
	var txt;
	txt =  "Usage:\n";
	txt += "  cscript " + WScript.ScriptName + " <options>\n";
	txt += "  cscript " + WScript.ScriptName + " help\n\n";
	txt += "Options can be specified in the form <option>=<value>.\n\n";
	txt += "\nDefault value given in parentheses:\n\n";
	txt += "  vsconfig:     Visual Studio configuration [release|debug] (" + VSConfig + ") \n";
	txt += "  distdir:      StarDict source directory after executing `make dist` or\n";
	txt += "                a directory with unpacked distribution tarball for Unix.\n";
	txt += "                Used to get html help files.\n";
	txt += "                (" + UnixDistDir + ") \n";
	txt += "  builddir:     StarDict build directory or.\n";
	txt += "                Used to get compiled translation files.\n";
	txt += "                (" + UnixBuildDir + ") \n";
	WScript.Echo(txt);
}

/* This function was inspired by configure.js script of libxml2 package. */
function ParseCommandLine() {
	var error = 0;
	for (i = 0; (i < WScript.Arguments.length) && (error == 0); i++) {
		var arg, opt;
		arg = WScript.Arguments(i);
		opt = arg.substring(0, arg.indexOf("="));
		if (opt.length == 0)
			opt = arg.substring(0, arg.indexOf(":"));
		if (opt.length > 0) {
			if (opt == "vsconfig")
				VSConfig = arg.substring(opt.length + 1, arg.length);
			else if (opt == "distdir")
				UnixDistDir = arg.substring(opt.length + 1, arg.length);
			else if (opt == "builddir")
				UnixBuildDir = arg.substring(opt.length + 1, arg.length);
		} else if (i == 0) {
			if (arg == "help") {
				usage();
				WScript.Quit(0);
			}
		} else {
			error = 1;
		}
	}
	// If we fail here, it is because the user supplied an unrecognised argument.
	if (error != 0) {
		usage();
		WScript.Quit(error);
	}
}

function GetBaseDir() {
	var t = WScript.ScriptFullName.lastIndexOf("\\");
	return WScript.ScriptFullName.substr(0, t+1);
}

function CopyFile(src, target) {
	if(!fso.FileExists(src)) {
		WScript.Echo("File not found: " + src);
		WScript.Quit(1);
	}
	//WScript.Echo("src: " + src + ", target: " + target);
	fso.CopyFile(src, target);
}

/* copy content of the folder SrcDir into TargetDir. 
TargetDir must exist. */
function CopyFolderContent(SrcDir, TargetDir) {
	/* fix TargetDir to end with "\\" */
	if(!TargetDir.match(/.*\\$/)) {
		TargetDir += "\\";
	}
	var oFolder = fso.GetFolder(SrcDir);
	var oFolders = new Enumerator(oFolder.SubFolders);
	for(; !oFolders.atEnd(); oFolders.moveNext())
	{
		if(oFolders.item().name.toLowerCase() == ".svn") {
			continue;
		}
		//WScript.Echo("folder: " + oFolders.item().path);
		var NewDir = TargetDir + oFolders.item().name + "\\";
		CreateFolder(NewDir);
		CopyFolderContent(oFolders.item().path, NewDir);
	}
	var oFiles = new Enumerator(oFolder.Files);
	for(; !oFiles.atEnd(); oFiles.moveNext()) 
	{
		if(oFiles.item().name.toLowerCase() == "makefile.am"
		|| oFiles.item().name.toLowerCase() == "makefile.in") {
			continue;
		}
		//WScript.Echo("file: " + oFiles.item().path);
		fso.CopyFile(oFiles.item().path, TargetDir);
	}
}

function CreateFolder(path) {
	var start = 0;
	var dir;
	var i;
	do {
		i = path.indexOf("\\", start);
		if(i < 0)
			dir = path;
		else {
			dir = path.substr(0, i);
			start = i + 1;
		}
		if(!fso.FolderExists(dir))
			fso.CreateFolder(dir);
	} while(i >= 0);
}

function DeleteFolder(path) {
	if(fso.FolderExists(path)) {
		fso.DeleteFolder(path);
	}
}

/* path - directory name where to search for files,
name_regex - a regular expression that file name must match */
function FindFiles(path, name_regex) {
	var oFolder = fso.GetFolder(path);
	var oFiles = new Enumerator(oFolder.Files);
	var arr = new Array();
	for (; !oFiles.atEnd(); oFiles.moveNext())
	{
		var name = oFiles.item().name;
		if(name.match(name_regex)) {
			arr.push(name);
		}
	}
	return arr;
}

if (!fso.FileExists(BaseDir + "stardict-installer.nsi")) {
	WScript.Echo("This script must be started in the Stardict source directory.");
	usage();
	WScript.Quit(1);
}

ParseCommandLine();

var MSVSOutputDir = MSVSDir + VSConfig + "\\";

if(!fso.FolderExists(UnixDistDir)) {
	WScript.Echo("StarDict distribution directory does not exist.\n"
		+ "This directory must contain unpacked distribution tarball for Unix or\n"
		+ "it must be StarDict source directory after executing `make dist`.\n"
		+ "Folder: " + UnixDistDir);
	usage();
	WScript.Quit(1);
}

if(!fso.FolderExists(UnixBuildDir)) {
	WScript.Echo("StarDict build directory does not exist.\n"
		+ "Folder: " + UnixBuildDir);
	usage();
	WScript.Quit(1);
}

if(InstallDirMustNotExist) {
	if (fso.FolderExists(BaseDir + "win32-install-dir")) {
		WScript.Echo("Directory win32-install-dir already exists, "
			+ "remove it and start this script again.");
		WScript.Quit(1);
	} 
} else {
	DeleteFolder(BaseDir + "win32-install-dir");
}
CreateFolder(InstallDir);
CopyFile(MSVSOutputDir + "stardict-loader.exe", InstallDir + "stardict.exe");
CopyFile(MSVSOutputDir + "stardict.dll", InstallDir);
CopyFile(MSVSOutputDir + "TextOutSpy.dll", InstallDir);
CopyFile(MSVSOutputDir + "TextOutHook.dll", InstallDir);
CopyFile(MSVSOutputDir + "stardict-editor.exe", InstallDir);
{
	var oFolder = fso.GetFolder(fso.BuildPath(UnixBuildDir, "po"));
	var oFiles = new Enumerator(oFolder.Files);
	var cnt = 0;
	for (; !oFiles.atEnd(); oFiles.moveNext())
	{
		var oFile = oFiles.item();
		if(oFile.name.match(/\.gmo$/i)) {
			++cnt;
			var BaseName = oFile.name.replace(/(.*)\..*/, "$1");
			var LocaleDir = InstallDir + "locale\\" + BaseName + "\\LC_MESSAGES";
			CreateFolder(LocaleDir);
			fso.CopyFile(oFile.Path, LocaleDir + "\\stardict.mo");
		}
	}
	if(cnt == 0) {
		WScript.Echo("No translation files found: " + oFolder.Path + "\\*.gmo");
		usage();
		WScript.Quit(1);
	}
}
{
	var PixmapsDir = InstallDir + "pixmaps\\";
	CreateFolder(PixmapsDir);
	CopyFile(BaseDir + "pixmaps\\stardict.png", PixmapsDir);
	fso.CopyFile(BaseDir + "src\\pixmaps\\*.png", PixmapsDir);
	fso.DeleteFile(PixmapsDir + "docklet_*.png");
}

{
	var SoundsDir = InstallDir + "sounds\\";
	CreateFolder(SoundsDir);
	fso.CopyFile(BaseDir + "src\\sounds\\*.wav",  SoundsDir);
}

{
	var DicDir = InstallDir + "dic\\";
	CreateFolder(DicDir);
	CopyFolderContent("src\\dic\\", DicDir);
}

CreateFolder(InstallDir + "treedict\\");
CreateFolder(InstallDir + "skins\\");

{
	var InstallHelpDir = InstallDir + "help\\";
	CreateFolder(InstallHelpDir);
	var oFolder = fso.GetFolder(fso.BuildPath(UnixDistDir, "help\\"));
	var oFolders = new Enumerator(oFolder.SubFolders);
	var cnt = 0;
	for(; !oFolders.atEnd(); oFolders.moveNext())
	{
		var oLocaleFolder = oFolders.item();
		if(oLocaleFolder.name == ".svn") {
			continue;
		}
		var HtmlDir = oLocaleFolder.path + "\\html\\";
		if(!fso.FolderExists(HtmlDir)) {
			WScript.Echo("Html help folder is not found: " + HtmlDir);
			WScript.Quit(1);
		}
		var InstallLocaleDir = InstallHelpDir + oLocaleFolder.name + "\\";
		CreateFolder(InstallLocaleDir);
		CopyFolderContent(HtmlDir, InstallLocaleDir);
		++cnt;
	}
	if(cnt == 0) {
		WScript.Echo("Html help is not found: " + oFolder.Path + "\\C\\html");
		usage();
		WScript.Quit(1);
	}
}

{
	var PluginsDir = InstallDir + "plugins\\";
	CreateFolder(PluginsDir);
	{
		var files = FindFiles(MSVSOutputDir, /.*\.dll$/i);
		for(var i=0; i<files.length; i++) {
			if(!files[i].match(/stardict-.*-plugin\.dll/i))
				continue;
			//WScript.Echo("path: " + MSVSOutputDir + files[i]);
			CopyFile(MSVSOutputDir + files[i], PluginsDir);
		}
	}
}

CopyFile(MSVSOutputDir + "StarDict.api", InstallDir);
{
	var LibSigcDir = MSVSDir + "libsigc++\\";
	var files = FindFiles(LibSigcDir, /sigc-.*\.dll/i);
	var cnt = 0;
	for(var i=0; i<files.length; ++i) {
		var is_debug_dll = (files[i].match(/.*-d-.*/i) != null);
		var is_debug_build = (VSConfig.toLowerCase() == "debug");
		if(is_debug_dll != is_debug_build)
			continue;
		CopyFile(LibSigcDir + files[i], InstallDir);
		++cnt;
	}
	if(cnt == 0) {
		WScript.Echo("libsigc++ dll is not found in " + LibSigcDir);
		WScript.Quit(1);
	}
}
WScript.Echo("Done.");
