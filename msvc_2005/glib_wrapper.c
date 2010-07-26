#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif
#include <errno.h>
#include <glib.h>
#include <glib/gstdio.h>

FILE *
my_g_fopen (const gchar *filename,
	 const gchar *mode)
{
#ifdef _WIN32
  if (G_WIN32_HAVE_WIDECHAR_API ())
    {
      wchar_t *wfilename = g_utf8_to_utf16 (filename, -1, NULL, NULL, NULL);
      wchar_t *wmode;
      FILE *retval;
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
	
      retval = _wfopen (wfilename, wmode);
      save_errno = errno;

      g_free (wfilename);
      g_free (wmode);

      errno = save_errno;
      return retval;
    }
  else
    {
      gchar *cp_filename = g_locale_from_utf8 (filename, -1, NULL, NULL, NULL);
      FILE *retval;
      int save_errno;

      if (cp_filename == NULL)
	{
	  errno = EINVAL;
	  return NULL;
	}

      retval = fopen (cp_filename, mode);
      save_errno = errno;

      g_free (cp_filename);

      errno = save_errno;
      return retval;
    }
#else
  return fopen (filename, mode);
#endif
}
