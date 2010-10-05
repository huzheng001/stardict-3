#ifndef _STARDICT_LIBCONFIG_H_
#define _STARDICT_LIBCONFIG_H_

//stardict
#define SD_STANDARD_EDITION

//stardictd
//#define SD_SERVER_EDITION


#ifdef SD_STANDARD_EDITION
#define SD_CLIENT_CODE
#undef SD_SERVER_CODE
#endif

#ifdef SD_SERVER_EDITION
#undef SD_CLIENT_CODE
#define SD_SERVER_CODE
#endif

#endif
