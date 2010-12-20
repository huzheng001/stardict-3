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
var Portable = false;
var MakeNSISExe = "c:\\Program Files\\NSIS\\makensis.exe";

var BaseDir = GetBaseDir();
var InstallRootDir;
var InstallDir; // application directory, it contains stardict.exe, stardict.dll, etc.
var MSVSOutputDir;
var MSVSDir = BaseDir + "msvc_2008\\";
var NSISLog = "StarDictPortableLaucher.log";
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
	txt += "  portable:     Build portable version of StarDict [yes|no] (" + (Portable ? "yes" : "no") + ") \n";
	txt += "  distdir:      StarDict source directory after executing `make dist` or\n";
	txt += "                a directory with unpacked distribution tarball for Unix.\n";
	txt += "                Used to get html help files.\n";
	txt += "                (" + UnixDistDir + ") \n";
	txt += "  builddir:     StarDict build directory or.\n";
	txt += "                Used to get compiled translation files.\n";
	txt += "                (" + UnixBuildDir + ") \n";
	txt += "  makensis:     Path to makensis.exe.\n";
	txt += "                (" + MakeNSISExe + ") \n";
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
			else if (opt == "portable")
				Portable = strToBool(arg.substring(opt.length + 1, arg.length));
			else if (opt == "distdir")
				UnixDistDir = arg.substring(opt.length + 1, arg.length);
			else if (opt == "builddir")
				UnixBuildDir = arg.substring(opt.length + 1, arg.length);
			else if (opt == "makensis")
				MakeNSISExe = arg.substring(opt.length + 1, arg.length);
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

/* Helper function, transforms the argument string into a boolean
   value. */
function strToBool(opt)
{
	if (opt == 0 || opt == "no")
		return false;
	else if (opt == 1 || opt == "yes")
		return true;
	error = 1;
	return false;
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

/* Remove traling backslash if present */
function CanonizePath(path) {
	if(path.charAt(path.length-1) == "\\")
		return path.substr(0, path.length-1);
	else
		return path;
}

function DeleteFolder(path) {
	path = CanonizePath(path);
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

{	// assign global variables
	MSVSOutputDir = MSVSDir + VSConfig + "\\";
	if(Portable) {
		InstallRootDir = BaseDir + "win32-portable-install-dir\\";
		InstallDir = InstallRootDir + "StarDictPortable\\App\\StarDict\\";
	} else {
		InstallRootDir = BaseDir + "win32-install-dir\\";
		InstallDir = InstallRootDir;
	}
}

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
	if (fso.FolderExists(InstallRootDir)) {
		WScript.Echo("Directory " + InstallRootDir + " already exists, "
			+ "remove it and start this script again.");
		WScript.Quit(1);
	} 
} else {
	DeleteFolder(InstallRootDir);
}
CreateFolder(InstallRootDir);
CreateFolder(InstallDir);
CopyFile(MSVSOutputDir + "stardict-loader.exe", InstallDir + "stardict.exe");
CopyFile(MSVSOutputDir + "stardict.dll", InstallDir);
CopyFile(MSVSOutputDir + "TextOutSpy.dll", InstallDir);
CopyFile(MSVSOutputDir + "TextOutHook.dll", InstallDir);
CopyFile(MSVSOutputDir + "stardict-editor.dll", InstallDir);
CopyFile(MSVSOutputDir + "stardict-editor-loader.exe", InstallDir + "stardict-editor.exe");
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

if(!Portable) {
	CopyFile(MSVSOutputDir + "StarDict.api", InstallDir);
}
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

{ // Gtk runtime
	var files = FindFiles("redist", /^gtk2-runtime-.*\.exe$/i);
	if(files.length == 0) {
		WScript.Echo("Unable to find gtk2-runtime.");
		WScript.Quit(1);
	}
	if(files.length > 1) {
		WScript.Echo("Multiple gtk2-runtime's found.");
		WScript.Quit(1);
	}
	var gtk2_runtime = "redist\\" + files[0];
	var GtkInstallDir = InstallDir + "Gtk";
	CreateFolder(GtkInstallDir);
	var cmd = "\"" + gtk2_runtime + "\" /S /sideeffects=no /dllpath=bin /translations=yes /compatdlls=yes "
		+ "/D=" + GtkInstallDir + "";
	var status = shell.Run(cmd, 1, true);
	if(status != 0) {
		WScript.Echo("gtk2-runtime failed.");
		WScript.Quit(status);
	}
}

if(!Portable) {
	WScript.Echo("Done.");
	WScript.Quit(0);
}

{ // launcher
	var cmd = "\"" + MakeNSISExe + "\" /O" + NSISLog + " src\\win32\\nsis\\StarDictPortable.nsi";
	var status = shell.Run(cmd, 1, true);
	if(status != 0) {
		WScript.Echo("Building StarDict launcher failed.");
		WScript.Quit(status);
	}
	CopyFile(BaseDir + "src\\win32\\nsis\\StarDictPortable.exe", InstallRootDir + "StarDictPortable\\");
}

{ // stardict-dirs.cfg
	var contents =
			"[general]\n"
		+	"user_config_dir=..\\\\..\\\\Data\\\\settings\\\\stardict\n"
		+	"user_cache_dir=..\\\\..\\\\Data\\\\cache\n"
		+	"log_dir=..\\\\..\\\\Data\\\\log\n"
	;
	var CfgDir = InstallRootDir + "StarDictPortable\\App\\DefaultData\\settings\\"
	CreateFolder(CfgDir);
	var filePath = CfgDir + "stardict-dirs.cfg";
	var file = fso.CreateTextFile(filePath, true, false);
	file.Write(contents);
	file.Close();
}

{ // StarDictPortableSettings.ini
	var contents =
			"[Language]\n"
		+	"STARDICTLANG=\n"
	;
	var CfgDir = InstallRootDir + "StarDictPortable\\App\\DefaultData\\settings\\"
	CreateFolder(CfgDir);
	var filePath = CfgDir + "StarDictPortableSettings.ini";
	var file = fso.CreateTextFile(filePath, true, false);
	file.Write(contents);
	file.Close();
}

{ // gtkrc
	var contents =
			"gtk-theme-name = \"MS-Windows\"\n"
	;
	var CfgDir = InstallRootDir + "StarDictPortable\\App\\DefaultData\\settings\\"
	CreateFolder(CfgDir);
	var filePath = CfgDir + "gtkrc";
	var file = fso.CreateTextFile(filePath, true, false);
	file.Write(contents);
	file.Close();
}

/* StarDict-loader sets $HOME enviroment variable to "StarDictPortable\\App":
Set $HOME so that the GTK+ settings get stored in the right place 
GTK will store settings in "StarDictPortable\\App\GTK" subfolder. */
CreateFolder(InstallRootDir + "StarDictPortable\\App\\GTK");

WScript.Echo("Done.");
