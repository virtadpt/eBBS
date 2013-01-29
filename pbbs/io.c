/*
    Pirate Bulletin Board System
    Copyright (C) 1990, Edward Luke, lush@Athena.EE.MsState.EDU
    Eagles Bulletin Board System
    Copyright (C) 1992, Raymond Rocker, rocker@datasync.com
                        Guy Vega, gtvega@seabass.st.usm.edu
                        Dominic Tynes, dbtynes@seabass.st.usm.edu

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
*/

#include <stdio.h>
#include "osdeps.h"
#include "io.h"
#include <sys/types.h>
#include <signal.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <errno.h>

#if USES_SYS_SELECT_H
# include <sys/select.h>
#endif

extern int input_active;

extern unsigned int conv_table[256][2];

#define OBUFSIZE  (4096)
#define IBUFSIZE  (256)

unsigned char outbuf[OBUFSIZE] ;
int obufsize = 0 ;

unsigned char inbuf[IBUFSIZE] ;
int ibufsize = 0 ;
int icurrchar = 0 ;

oflush()
{
    if(obufsize)
      write(1,outbuf,obufsize) ;
    obufsize = 0 ;
}

output(s,len)
char *s ;
{
    int i;
    for(i = 0; i < len; i++)
      ochar(s[i]);
}

ochar(c)
unsigned char c;
{
    if(obufsize > OBUFSIZE-1) {  /* doin a oflush */
        write(1,outbuf,obufsize) ;
        obufsize = 0 ;
    }
     outbuf[obufsize++] = conv_table[c][1] ;
}

int i_newfd = 0;
struct timeval i_to, *i_top = NULL ;
int (*flushf)() = NULL ;

#if 0
int serversock = 0;
int in_child = 0;
int (*sockf)() = NULL;

add_serversock(fd)
int fd;
{
    serversock = fd;
}

add_servhandler(func)
int (*func)();
{
    sockf = func;
}
#endif

add_io(fd,timeout)
int fd ;
int timeout ;
{
    i_newfd = fd ;
    if(timeout) {
        i_to.tv_sec = timeout ;
        i_to.tv_usec = 0 ;
        i_top = &i_to ;
    } else i_top = NULL ;
}

add_flush(flushfunc)
int (*flushfunc)() ;
{
    flushf = flushfunc ;
}

num_in_buf()
{
    return icurrchar - ibufsize ;
}

int
igetch()
{
igetagain:
    if(ibufsize == icurrchar) {
        fd_set readfds ;
        struct timeval to ;
        int nfds, sr ;

        to.tv_sec = 0 ;
        to.tv_usec = 0 ;
        FD_ZERO(&readfds) ;
        nfds = 1;
#if 0
        if (!in_child) 
#endif
	    FD_SET(0,&readfds) ;
        if(i_newfd) {
            FD_SET(i_newfd,&readfds) ;
	    nfds = i_newfd+1;
        }
#if 0
	if(serversock) {
	    FD_SET(serversock,&readfds);
            if (serversock >= nfds) nfds = serversock+1;
        }
#endif
        if((sr = select(nfds, &readfds, NULL, NULL, &to)) <= 0) {
            if(flushf)
                (*flushf)() ;
            if(dumb_term)
                oflush() ;
            else
                refresh() ;
selectagain:
            FD_ZERO(&readfds) ;
            nfds = 1;
#if 0
            if (!in_child)
#endif
  	        FD_SET(0,&readfds) ;
            if(i_newfd) {
                FD_SET(i_newfd,&readfds) ;
  	        nfds = i_newfd+1;
	    }
#if 0
	    if (serversock) {
		FD_SET(serversock,&readfds);
                if (serversock >= nfds) serversock+1;
            }
#endif
            while((sr = select(nfds, &readfds, NULL, NULL, i_top)) <0) {
                if(errno == EINTR) {
#if 0
                    if (in_child) return (I_SIGNAL);
		    else 
#endif
		      continue ;
		}
                else {
                    perror("select") ;
                    fprintf(stderr,"abnormal select conditions\n") ;
                    return -1 ;
                }
            }
            if(sr == 0)
                return I_TIMEOUT ;
        }
#if 0
        if(serversock && FD_ISSET(serversock,&readfds)) {
	    if (sockf) {
	  	(*sockf)(serversock);
		goto selectagain;
	    }
            else return I_OTHERDATA ;
        }
#endif
        if(i_newfd && FD_ISSET(i_newfd,&readfds)) {
            return I_OTHERDATA ;
        }
        while((ibufsize = read(0,inbuf,IBUFSIZE)) <= 0) {
            if(ibufsize == 0)
                generic_abort() ;
            if(ibufsize < 0 && errno != EINTR)
                generic_abort() ;
        }
        icurrchar = 0 ;
    }
#ifdef DO_ISTRIP
    inbuf[icurrchar] &= 0x7f ;
#endif
    input_active = 1;
    switch(inbuf[icurrchar]) {
      case CTRL('L'):
        redoscr() ;
        icurrchar++ ;
        goto igetagain ;
      default:
        break ;
    }
    return conv_table[inbuf[icurrchar++]][0] ;
}

getdata(line,col,prompt,buf,len,echo,cancel)
int line,col ;
char *prompt, *buf ;
int len, echo, cancel ;
{
    int ch ;
    int clen = 0 ;
    int x,y ;
    extern unsigned char scr_cols ;

    if(prompt) {
        move(line,col) ;
        prints("%s",prompt) ;
    }
    if(dumb_term) {
        while((ch = igetch()) != '\r') {
	    if((ch == CTRL('C') || ch == CTRL('D')) && cancel)
              return -1 ;
            if(ch == '\n')
              break ;
            if(ch == '\177' || ch == CTRL('H')) {
                if(clen == 0) {
                    bell() ;
                    continue ;
                }
                clen-- ;
                if(echo) {
                    ochar(CTRL('H')) ;
                    ochar(' ') ;
                    ochar(CTRL('H')) ;
                }
                continue ;
            }
            if(!isprint(ch)) {
                if(echo)
                  bell() ;
                continue ;
            }
            if(clen >= len-1) {
                if(echo)
                  bell() ;
                continue ;
            }
            buf[clen++] = ch ;
            if(echo)
              ochar(ch) ;
        }
        buf[clen] = '\0' ;
        prints("\n") ;
        oflush() ;
        return clen ;
    }
    clrtoeol() ;
    getyx(&y,&x) ;
    while((ch = igetch()) != '\r') {
        if((ch == CTRL('C') || ch == CTRL('D')) && cancel)
          return -1 ;
        if(ch == '\n')
          break ;
        if(ch == '\177' || ch == CTRL('H')) {
            if(clen == 0) {
                bell() ;
                continue ;
            }
            clen-- ;
            if(echo) {
                move(y,x+clen) ;
                prints(" ") ;
                move(y,x+clen) ;
            }
            continue ;
        }
        if(!isprint(ch)) {
            bell() ;
            continue ;
        }
        if(x+clen >= scr_cols || clen >= len-1) {
            bell() ;
            continue ;
        }
        buf[clen++] = ch ;
        if(echo)
          prints("%c",ch) ;
    }
    buf[clen] = '\0' ;
    if(echo)
      prints("\n") ;
    refresh() ;
    return clen ;
}
