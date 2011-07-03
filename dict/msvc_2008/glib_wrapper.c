/*
 * Copyright 2011 kubtek <kubtek@mail.com>
 *
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
