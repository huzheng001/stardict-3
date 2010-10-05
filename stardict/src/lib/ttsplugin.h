#ifndef _STARDICT_TTS_PLUGIN_H_
#define _STARDICT_TTS_PLUGIN_H_

struct StarDictTtsPlugInObject{
	StarDictTtsPlugInObject();

	typedef void (*saytext_func_t)(const char *text);
	saytext_func_t saytext_func;
	const char *tts_name;
};

#endif
