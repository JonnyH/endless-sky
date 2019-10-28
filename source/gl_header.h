/* gl_header.h
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef GL_HEADER_H_
#define GL_HEADER_H_

#define GLESWRAP_GLES2
#include "gleswrap/gleswrap.h"

using GL = gles_wrap::gles2;

extern gles_wrap::gles2 *gl;

#endif // GL_HEADER_H_
