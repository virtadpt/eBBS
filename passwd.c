
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


#include "common.h"
#include <time.h>
#if NEEDS_CRYPT_DECLARED
char *crypt __P((char *, char *));
#else
# include <unistd.h>
#endif

#define REALPASSLEN 8   /* number of significant characters */

is_passwd_good(crypted, clear)
char *crypted;
char *clear;
{
  char *pw;
  char copy[REALPASSLEN+1];
  memset(copy, 0, sizeof copy);
  strncpy(copy, clear, REALPASSLEN);
  pw = crypt(copy, crypted);    
  return (!strcmp(pw, crypted));  
}

void
encrypt_passwd(crypted, clear)
char *crypted;
char *clear;
{
  char copy[REALPASSLEN+1];
  char saltc[2];
  time_t now;
  char *pw;
  int i;
  if (*clear == '\0') {
    *crypted = '\0';
    return;
  }
  memset(copy, 0, sizeof copy);
  strncpy(copy, clear, REALPASSLEN);    
  time(&now);
  saltc[0] = (char)(now & 000177);
  saltc[1] = (char)((now >> 7) & 000177);
  for (i=0; i<2; i++) {
    if (saltc[i] < 0x2e) saltc[i]+=0x41;
    if (saltc[i] > 0x39 && saltc[i] < 0x41) saltc[i]-=0x7;
    if (saltc[i] > 0x5a && saltc[i] < 0x61) saltc[i]+=0x6;
    if (saltc[i] > 0x7a) saltc[i]-=0x5;
  }
  pw = crypt(copy, saltc);
  strncpy(crypted, pw, PASSLEN);
}
