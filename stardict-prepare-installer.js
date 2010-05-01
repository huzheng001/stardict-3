/* prepares directory structure for NSIS installer (stardict-installer.nsi file)
To run the script, execute the command in Windows Console:
cscript //nologo stardict-prepare-installer.js
this script must run in the root StarDict directory
*/

// options
/* use Dev-C++ output files - true, or use MS Visual Studio output files - false */
var UseDevC = false; 
var InstallDirMustNotExist = false;
var MSVSConfig = "Release"; // Release or Debug

var BaseDir = GetBaseDir();
var InstallDir = BaseDir + "win32-install-dir\\";
var MSVSDir = BaseDir + "mvsc\\";
var MSVSOutputDir = MSVSDir + MSVSConfig + "\\";
var fso = WScript.CreateObject("Scripting.FileSystemObject");

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
	WScript.Quit(1);
}

if(InstallDirMustNotExist) {
	if (fso.FolderExists(BaseDir + "win32-install-dir")) {
		WScript.Echo("Directory win32-install-dir already exists, "
			+ "remove it and start this script again.");
		WScript.Quit(1);
	} 
} else {
	fso.DeleteFolder(BaseDir + "win32-install-dir");
}
CreateFolder(InstallDir);
if(UseDevC)
	CopyFile(BaseDir + "src\\stardict.exe", InstallDir);
else
	CopyFile(MSVSOutputDir + "stardict.exe", InstallDir);
if(UseDevC) {
	CopyFile(BaseDir + "src\\win32\\TextOutSpy.dll", InstallDir);
	CopyFile(BaseDir + "src\\win32\\TextOutHook.dll", InstallDir);
} else {
	CopyFile(MSVSOutputDir + "TextOutSpy.dll", InstallDir);
	CopyFile(MSVSOutputDir + "TextOutHook.dll", InstallDir);
}
{
	var oFolder = fso.GetFolder(BaseDir + "\\po");
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
		WScript.Echo("Translation files are not found: " + oFolder.Path + "\\*.gmo");
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
CreateFolder(InstallDir + "dic\\");
CreateFolder(InstallDir + "treedict\\");
CreateFolder(InstallDir + "skins\\");
CreateFolder(InstallDir + "help\\");
{
	var PluginsDir = InstallDir + "plugins\\";
	CreateFolder(PluginsDir);
	if(UseDevC) {
		var oFolder = fso.GetFolder(BaseDir + "\\stardict-plugins\\");
		var oFolders = new Enumerator(oFolder.SubFolders);
		for (; !oFolders.atEnd(); oFolders.moveNext())
		{
			var oSubFolder = oFolders.item();
			if(oSubFolder.Name.match(/^stardict-.*-plugin$/i)) {
				var files = FindFiles(oSubFolder.Path + "\\", /.*\.dll$/i);
				for(var i=0; i<files.length; i++) {
					//WScript.Echo("path: " + oSubFolder.Path + "\\" + files[i]);
					CopyFile(oSubFolder.Path + "\\" + files[i], PluginsDir);
				}
			}
		}
	} else {
		var files = FindFiles(MSVSOutputDir, /.*\.dll$/i);
		for(var i=0; i<files.length; i++) {
			if(files[i].toLowerCase() == "textoutspy.dll" 
			|| files[i].toLowerCase() == "textouthook.dll")
				continue;
			//WScript.Echo("path: " + MSVSOutputDir + files[i]);
			CopyFile(MSVSOutputDir + files[i], PluginsDir);
		}
	}
}

CopyFile(MSVSOutputDir + "StarDict.api", InstallDir);
if(!UseDevC) {
	var LibSigcDir = MSVSDir + "libsigc++\\";
	var files = FindFiles(LibSigcDir, /sigc-.*\.dll/i);
	if(files.length == 0) {
		WScript.Echo("libsigc++ dll is not found in " + LibSigcDir);
		WScript.Quit(1);
	}
	for(var i=0; i<files.length; ++i) {
		CopyFile(LibSigcDir + files[i], InstallDir);
	}
}
WScript.Echo("Done.");
