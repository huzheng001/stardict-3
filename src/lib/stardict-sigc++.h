#ifndef _STARDICT_SIGCPP_H_
#define _STARDICT_SIGCPP_H_

/* Please, include this file if you need to use sigc++ library.
Visual Studio 2005 uses external sigc++ library, it should not use headers
distributed with StarDict. In other cases private sigc++ library is OK.
It's important to avoid the case when local sigc++ headers are used, while the
project links with an external library.
*/

#ifdef _MSC_VER
#include <sigc++/sigc++.h>
#else
#include "../sigc++/sigc++.h"
#endif

#endif
