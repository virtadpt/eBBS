
/*
Eagles Bulletin Board System
Copyright (C) 1995, Ray Rocker, rocker@datasync.com

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdio.h>
#include <stdlib.h>
#include "osdeps.h"

#define BBSHOMEENV "BBSHOME"

/*
   This function is used by the local bbs client and all utilities
   to chdir to the bbs home directory.
*/

home_bbs(dir)
char *dir;
{
  char *bbshome;
  if (dir != NULL) bbshome = dir;
  else {
    bbshome = getenv(BBSHOMEENV);
    if (bbshome == NULL) {
      /* We must be there already. */
      return 0;
    }
  }
  return (chdir(bbshome));
}
  
  
      
