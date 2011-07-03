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

#ifndef _STARDICT_SIGCPP_H_
#define _STARDICT_SIGCPP_H_

/* Please, include this file if you need to use sigc++ library.
Visual Studio 2008 uses external sigc++ library, it should not use headers
distributed with StarDict. In other cases private sigc++ library is OK.
It's important to avoid the case when local sigc++ headers are used, while the
project links with an external library.

Local sigc++ library uses angle brackets includes like '#include <sigc++/signal.h>'
(see sigc++/sigc++.h), so local sigc++ dir must be in include path. This makes possible
to use angle brackets include for sigc++.h for both local and system sigc++ library.
*/

#include <sigc++/sigc++.h>

#endif
