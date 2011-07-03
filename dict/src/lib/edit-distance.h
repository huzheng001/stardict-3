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

#ifndef EDIT_DISTANCE_H
#define EDIT_DISTANCE_H

#include <glib.h>

class EditDistance {
private:
    int *d;
    int currentelements;
    /*Gets the minimum of three values */
    inline int minimum( const int a, const int b, const int c )
    {
        int min = a;
        if ( b < min )
              min = b;
        if ( c < min )
              min = c;
          return min;
    };
public:
    EditDistance(  );
    ~EditDistance(  );
    int CalEditDistance( const gunichar *s, const gunichar *t, const int limit );
};

#endif
