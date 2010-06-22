#ifndef _STARDICT_SIGCPP_H_
#define _STARDICT_SIGCPP_H_

/* Please, include this file if you need to use sigc++ library.
Visual Studio 2005 uses external sigc++ library, it should not use headers
distributed with StarDict. In other cases private sigc++ library is OK.
It's important to avoid the case when local sigc++ headers are used, while the
project links with an external library.

Local sigc++ library uses angle brackets includes like '#include <sigc++/signal.h>'
(see sigc++/sigc++.h), so local sigc++ dir must be in include path. This makes possible
to use angle brackets include for sigc++.h for both local and system sigc++ library.
*/

#include <sigc++/sigc++.h>

#endif
