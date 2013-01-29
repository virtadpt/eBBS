/*
    Pirate Bulletin Board System
    Copyright (C) 1990, Edward Luke

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.


    Email address:
    lush@Athena.EE.MsState.EDU
*/

#include <stdio.h>
#include <sys/param.h>
#include <signal.h>
#if LACKS_MALLOC_H
# include <stdlib.h>
#else
# include <malloc.h>
#endif
#include <fcntl.h>
#include "osdeps.h"
#include "io.h"

#define MAXENVS (200)

#if NO_VFORK
#define vfork fork
#endif

#if 0
extern int in_child;     /* to let igetch() know when to select on fd 0 */
int gotsigchld;          /* for communication with SIGCHLD handler */
#endif

int g_child_pid = -1;

dumbreturn()
{
    int ch ;
    prints("Press [RETURN] to continue") ;
    while((ch = igetch()) != EOF)
        if(ch == '\n' || ch == '\r')
    	    break ;
}

pressreturn()
{
    char c ;
    if (dumb_term) return (dumbreturn());
    getdata(t_lines-1,0,"Press [RETURN] to continue",&c,1,NOECHO,0) ;
    move(t_lines-1,0) ;
    clrtoeol() ;
    refresh() ;
    return 0 ;
}

bell()
{
    fprintf(stderr,"%c",CTRL('G')) ;
}

char *bbsenv[MAXENVS] ;
int numbbsenvs = 0 ;
 
bbssetenv(env,val)
char *env, *val ;
{
    register int i,len ;
 
    if(numbbsenvs == 0)
      bbsenv[0] = NULL ;
    len = strlen(env) ;
    for(i=0;bbsenv[i];i++)
      if(!strncasecmp(env,bbsenv[i],len) && bbsenv[i][len] == '=')
        break ;
    if(i>=MAXENVS)
      return -1 ;
    if(bbsenv[i])
      free(bbsenv[i]) ;
    else
      bbsenv[++numbbsenvs] = NULL ;
    bbsenv[i] = (char *)malloc(strlen(env)+strlen(val)+2) ;
    strcpy(bbsenv[i],env) ;
    strcat(bbsenv[i],"=") ;
    strcat(bbsenv[i],val) ;
}

#define LOOKFIRST  (0)
#define LOOKLAST   (1)
#define QUOTEMODE  (2)

#if 0
void
sigchld_handler(sig)
int sig;
{
    if (sig == SIGCHLD) {
        gotsigchld = 1;
    }
    return;
}
#endif

do_exec(com,wd)
char *com, *wd ;
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
#if 0
    void (*csig)() ;
#endif

    strncpy(pcom,com,MAXCOMSZ) ;
    len = MIN(strlen(com)+1,MAXCOMSZ) ;
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
    refresh();
    reset_tty() ;
    saved_alarm = alarm(0);
    if((pid = vfork()) == 0) {
        if(wd)
          if(chdir(wd)) {
              fprintf(stderr,"Unable to chdir to '%s'\n",wd) ;
              exit(-1) ;
          }
        if(numbbsenvs == 0)
          bbsenv[0] = NULL ;
        execve(path,arglist,bbsenv) ;
        fprintf(stderr,"EXECVP FAILED... path = '%s'\n",path) ;
        exit(-1) ;
    }
    g_child_pid = pid;
    isig = signal(SIGINT, SIG_IGN);
    qsig = signal(SIGQUIT, SIG_IGN);
#if 0
    csig = signal(SIGCHLD, sigchld_handler);

    in_child = 1;   /* tell igetch() not to select on 0 */
    gotsigchld = 0; /* keep looping till we get the sigchld */
    
    while (!gotsigchld) {
        if ((w = igetch()) != I_SIGNAL) {     
            fprintf(stderr, "WARNING: igetch returned %d\n", w);
            break;
        }
#endif
        if ((w = wait(&status)) != pid && w != -1) {     /* wrong child! */
/*            gotsigchld = 0; */
        }
#if 0
    }

    in_child = 0;
    signal(SIGCHLD, csig);
#endif
    if (saved_alarm) alarm(saved_alarm);
    signal(SIGINT, isig) ;
    signal(SIGQUIT, qsig) ;
    g_child_pid = -1;
    restore_tty() ;
    return((w == -1)? w: status) ;
}

#define BINDIR "bin/"

do_pipe_more(com)
char *com ;
{
    char path[MAXPATHLEN] ;
    char pcom[MAXCOMSZ] ;
    char *arglist[MAXARGS] ;
    register int i,len ;
    register int argptr ;
    register char *lparse ;
    int status, pid, w, fd[2] ;
    int pmode ;
    int saved_alarm ;
    void (*isig)(), (*qsig)() ;
    extern int t_lines ;
    char buf[80] ;
    int buflen, bufchar ;
    strncpy(path,BINDIR,MAXPATHLEN) ;
    strncpy(pcom,com,MAXCOMSZ) ;
    len = MIN(strlen(com)+1,MAXCOMSZ) ;
    pmode = LOOKFIRST ;
    for(i=0,argptr=0;i<len;i++) {
        if(pcom[i] == '\0')
          break ;
        if(pmode == QUOTEMODE) {
            if(pcom[i] == '\001') {
                pmode = LOOKFIRST ;
                pcom[i] = '\0' ;
                continue ;
            }
            continue ;
        }

        if(pcom[i] == '\001') {
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
    if(*arglist[0] == '/')
      strncpy(path,arglist[0],MAXPATHLEN) ;
    else
      strncat(path,arglist[0],MAXPATHLEN) ;
    if(pipe(fd) < 0) {
        perror("Pipe!") ;
        return -1 ;
    }
    saved_alarm = alarm(0);
    if((pid = vfork()) == 0) {
        close(fd[0]) ;
        close(0) ;
        open("/dev/null",O_RDONLY,0) ;
        dup2(fd[1],1) ;
        close(fd[1]) ;
        if(numbbsenvs == 0)
          bbsenv[0] = NULL ;
        execve(path,arglist,bbsenv) ;
        fprintf(stderr,"EXECV FAILED... path = '%s'\n",path) ;
        exit(-1) ;
    }
    close(fd[1]) ;
    buflen = 0 ;
    bufchar = 0 ;
    while(YEA) {
        int ch ;
        int chars ;
 
        move(3,0) ;
        clrtobot();
        for(i=3;i<t_lines-1;i++) {
            chars = 0 ;
            while(YEA) {
                if(bufchar == buflen) {
                    buflen = read(fd[0], buf, sizeof(buf)) ;
                    if(buflen <= 0) {
                        buflen = -1 ;
                        break ;
                    }
                    bufchar = 0 ;
                }
                while(bufchar!=buflen && buf[bufchar] != '\n') {
                    outc(buf[bufchar++]) ;
                    if(++chars == 78) {
                        outc('\n') ;
                        break ;
                    }
                }
                if(chars == 78)
                  break ;
                if(buf[bufchar] == '\n') {
                    outc(buf[bufchar++]) ;
                    break ;
                }
            }
            if(buflen == -1)
              break ;
        }
        if(buflen == -1 || i != t_lines-1)
          break ;
        standout() ;
        prints("-- More --") ;
        standend() ;
        clrtoeol() ;
        while((ch = igetch()) != EOF) {
            if(ch == '\n' || ch == '\r' || ch == ' ')
              break ;
            if(ch == 'q') {
                move(t_lines-1,0) ;
                clrtoeol() ;
                goto outahere ;
            }
            bell() ;
        }
    }
  outahere:
 
    clrtobot() ;
    close(fd[0]) ;
            
    isig = signal(SIGINT, SIG_IGN) ;
    qsig = signal(SIGQUIT, SIG_IGN) ;
    while((w = wait(&status)) != pid && w != 1)
      /* NULL STATEMENT */ ;
    if (saved_alarm) alarm(0);
    signal(SIGINT, isig) ;
    signal(SIGQUIT, qsig) ;
    return((w == -1)? w: status) ;
}

