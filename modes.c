 
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

#include "client.h"

char *global_modestrs[BBS_MAX_MODE+1];
char global_modechars[BBS_MAX_MODE+1];

char *
ModeToString(mode)
SHORT mode;
{
  if (global_modestrs[0] == NULL) {
    bbs_get_modestrings(global_modestrs);
  }

  if (mode <= BBS_MAX_MODE) {
    return(global_modestrs[mode]);
  }

  return("");
}

char
ModeToChar(mode)
SHORT mode;
{
  if (global_modechars[0] == '\0') {
    bbs_get_modechars(global_modechars);
  }

  if (mode <= BBS_MAX_MODE) {
    return(global_modechars[mode]);
  }

  return(' ');
}
