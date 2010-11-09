#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <errno.h>
#include <glib.h>
#include <glib/gstdio.h>

FILE *
stardict_g_fopen (const gchar *filename,
						const gchar *mode)
{
#ifdef _WIN32
#if !G_WIN32_HAVE_WIDECHAR_API()
#error Widechar API is not available
#endif
	{
		wchar_t *wfilename = g_utf8_to_utf16 (filename, -1, NULL, NULL, NULL);
		wchar_t *wmode;
		FILE *fp;
		int save_errno;

		if (wfilename == NULL)
		{
			errno = EINVAL;
			return NULL;
		}

		wmode = g_utf8_to_utf16 (mode, -1, NULL, NULL, NULL);

		if (wmode == NULL)
		{
			g_free (wfilename);
			errno = EINVAL;
			return NULL;
		}

		if(_wfopen_s(&fp, wfilename, wmode))
			fp = NULL;
		save_errno = errno;

		g_free (wfilename);
		g_free (wmode);

		errno = save_errno;
		return fp;
	}
#else
	return fopen (filename, mode);
#endif
}
