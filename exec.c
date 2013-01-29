
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
#include <fcntl.h>
#include <signal.h>

#define MAXCOMSZ   (1024) /* Maximum length of do_exec command */
#define MAXFILELEN (80)   /* Maximum length of the executable file name */
#define MAXARGS    (40)   /* Maximum number of args to a do_exec command */

#define LOOKFIRST  (0)
#define LOOKLAST   (1)
#define QUOTEMODE  (2)

_set_io(fname, fd, append)
char *fname;
int fd;
int append;
{
  int newfd;
  int mode;
  if (fd == 0) mode = O_RDONLY;
  else {
    mode = O_WRONLY | O_CREAT;
    if (append) mode |= O_APPEND;
  }
  close(fd);
  newfd = open(fname, mode, 0600);
  if (newfd == -1) {
    bbslog(0, "ERROR _set_io: open failed: %s mode %d\n", fname, mode);
    return -1;
  }
  if (newfd != fd && dup2(newfd, fd) == -1) {
    bbslog(0, "ERROR _set_io: dup2 failed\n");
    return -1;
  }
  return 0;
}

/*
   Adapted from Pirates BBS 1.6, do_exec in stuff.c
   Copyright (C) 1990, Edward Luke, lush@Athena.EE.MsState.EDU
   This one is suitable for the server, doesn't try to set the screen
*/

execute(com, wd, infile, outfile, errfile, envp, append)
char *com, *wd, *infile, *outfile, *errfile;
char **envp;
int append;
{
  char path[MAXFILELEN] ;
  char pcom[MAXCOMSZ] ;
  char *arglist[MAXARGS] ;
  register int i,len ;
  register int argptr ;
  int status, pid, w ;
  int pmode ;
  int saved_alarm ;
  void (*isig)(), (*qsig)();
  
  strncpy(pcom,com,MAXCOMSZ) ;
  len = strlen(com)+1;
  if (len > MAXCOMSZ) len = MAXCOMSZ;
  pmode = LOOKFIRST ;
  for(i=0,argptr=0;i<len;i++) {
    if(pcom[i] == '\0')
      break ;
    if(pmode == QUOTEMODE) {
      if(pcom[i] == '\"') {
	pmode = LOOKFIRST ;
	pcom[i] = '\0' ;
	continue ;
      }
      continue ;
    }
    if(pcom[i] == '\"') {
      pmode = QUOTEMODE ;
      arglist[argptr++] = &pcom[i+1] ;
      if(argptr+1 == MAXARGS)
	break ;
      continue ;
    }
    if(pmode == LOOKFIRST)
      if(pcom[i] != ' ') {
	arglist[argptr++] = &pcom[i] ;
	if(argptr+1 == MAXARGS)
	  break ;
	pmode = LOOKLAST ;
      } else continue ;
    if(pcom[i] == ' ') {
      pmode = LOOKFIRST ;
      pcom[i] = '\0' ;
    }
  }
  arglist[argptr] = NULL ;
  if(argptr == 0)
    return -1 ;
  strncpy(path,arglist[0],MAXFILELEN) ;
  saved_alarm = alarm(0);
  if((pid = fork()) == 0) {
    if(wd)
      if(chdir(wd)) {
        bbslog(0, "ERROR execute: chdir failed: %s\n", wd);
	exit(-1) ;
      }
    if (infile) _set_io(infile, 0, append);
    if (outfile) _set_io(outfile, 1, append);
    if (errfile) _set_io(errfile, 2, append);
    execve(path, arglist, envp);
    bbslog(0, "ERROR execute: execve failed: %s\n", path);
    exit(-1) ;
  }
  isig = signal(SIGINT, SIG_IGN);
  qsig = signal(SIGQUIT, SIG_IGN);
  while ((w = wait(&status)) != pid && w != -1);
  if (saved_alarm) alarm(saved_alarm);
  signal(SIGINT, isig) ;
  signal(SIGQUIT, qsig) ;
  return((w == -1)? w: status) ;
}
