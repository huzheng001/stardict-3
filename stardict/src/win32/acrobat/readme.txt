You need Microsoft Visual studio 2003 to compile the Acrobat plug-in.
The Acrobat SDK 7 is needed too, copy headers files from "C:\Program Files\Adobe\Adobe Acrobat 7.0.5 SDK\PluginSupport\Headers" to "Acrobat_Headers" in this directory.

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

This plug-in only work on Abode Acrobat Professional currently. For Adobe Reader, a License is needed, which will cost $2,500, this has not be done yet.
