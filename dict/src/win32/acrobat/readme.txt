You need Microsoft Visual Studio 2003 (2005 or 2008) to compile the Acrobat plug-in.
The Acrobat SDK 7 is needed too.
1. Install Acrobat SDK (SDK705Installer.zip) into "C:\Program Files (x86)\Adobe\Adobe Acrobat 7.0.5 SDK\".
2. Copy headers files from "C:\Program Files\Adobe\Adobe Acrobat 7.0.5 SDK\PluginSupport\Headers" (from "C:\Program Files (x86)\Adobe\Adobe Acrobat 7.0.5 SDK\PluginSupport\Headers" on x64) to "Acrobat_Headers" in this directory.

fix acrobat_headers\api\pimain.c file
Near the line 385 replace 
<<<
		char err[255];
		wsprintf(err,"failed on %s version %s%x\n",table,(requiredVer & kHFT_IN_BETA_FLAG) ?"(beta)":"",requiredVer & ~kHFT_IN_BETA_FLAG);
>>>
with
<<<
		TCHAR err[255];
		wsprintf(err, TEXT("failed on %s version %s%x\n"),table,(requiredVer & kHFT_IN_BETA_FLAG) ?"(beta)":"",requiredVer & ~kHFT_IN_BETA_FLAG);
>>>

You can try sdk91_v2_win.zip too.
Exctract "Adobe/Acrobat 9 SDK/Version 1/PluginSupport/Headers" to "Acrobat_Headers" in this directory.

This plug-in only work on Abode Acrobat Professional currently. For Adobe Reader, a License is needed, which will cost $2,500, this has not be done yet.
