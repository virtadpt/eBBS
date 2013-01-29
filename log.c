
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

#include "server.h"
#include <time.h>
#include <unistd.h>
#if WANTS_VARARGS_H
# include <varargs.h>
#else
# include <stdarg.h>
#endif

PATH bbs_logfile;
int bbs_loglevel;
char bbs_logsource[16];
FILE *bbs_log;

open_bbslog(logfile, loglevel)
char *logfile;
int loglevel;
{
  if (logfile == NULL) return S_BADFILENAME;
  strncpy(bbs_logfile, logfile, PATHLEN-1);
  bbs_log = fopen(bbs_logfile, "a");
  if (bbs_log == NULL) {
      return S_SYSERR;
  }
  bbs_loglevel = loglevel;
  return S_OK;
}

close_bbslog()
{
  fclose(bbs_log);
  return S_OK;
}

void
set_log_header(str)
char *str;
{
  char *p = str;
  int indx = 0;
  memset(bbs_logsource, ' ', 15);
  while (p && *p && indx < 16) {
    bbs_logsource[indx++] = *p++;
  }                       
  bbs_logsource[15] = '\0';
}

void
#if WANTS_VARARGS_H
bbslog(va_alist)
va_dcl
#else
bbslog(int level, char *fmt, ...)
#endif
{
  va_list args;
  time_t now;
  char timestr[40];
#if WANTS_VARARGS_H
  int level;
  char *fmt;
#endif

#if WANTS_VARARGS_H
  va_start(args);
  level = va_arg(args, int);
  fmt = va_arg(args, char *);
#else
  va_start(args, fmt);
#endif
  
  if (level > bbs_loglevel) {
    va_end(args);
    return;
  }

  fseek(bbs_log, 0, SEEK_END);
  if (fprintf(bbs_log, "%-16s ", bbs_logsource) == EOF) {
    close_bbslog();
    open_bbslog(bbs_logfile, bbs_loglevel);
    fprintf(bbs_log, "%-16s ", bbs_logsource);
  }
  time(&now);
  strftime(timestr, sizeof timestr, "%x %X", localtime(&now));
  fprintf(bbs_log, "%s ", timestr);    
  vfprintf(bbs_log, fmt, args);
  fflush(bbs_log);
  va_end(args);
}
