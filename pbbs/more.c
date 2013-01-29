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
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

int
readln(fp,buf)
FILE *fp ;
char *buf ;
{
    int i, j, k ;
    int ch ;
    int bytes ;
    i = 0 ;
    bytes = 0 ;
    while((ch = getc(fp)) != EOF) {
        bytes++ ;
        if(ch == '\n') {
            buf[i] = '\0' ;
            return bytes ;
        }
        if(ch == '\t') {
          k = (8 - (i % 8));
          if(k == 0) k = 8;
          for(j=0;j<k;j++) {
            buf[i++] = ' ';
            if(i == 80) {
              buf[i] = '\0' ;
              return bytes ;
            }
          }
        }
#if COLOR
        if(isprint(ch) || ch == '\033')
#else
        if(isprint(ch))
#endif
            buf[i++] = ch ;
        if(i == 80) {
            buf[i] = '\0' ;
            return bytes ;
        }
    }
    if(i == 0)
      return 0 ;
    else {
        buf[i] = '\0' ;
        return bytes ;
    }
}

int
more(filename,promptend)
char *filename ;
{
    FILE *fp ;
    int ch ;
    int i ;
    int viewed, pos ;
    long tsize ;
    struct stat st ;
    int numbytes ;
    int pagebreaks[1024];
    int pageno, linectr;
    char buf[81] ;
    extern int t_lines ;
    memset(pagebreaks, 0, sizeof(pagebreaks));

    if((fp = fopen(filename,"r")) == NULL) {
        return -1;
    }
    if(fstat(fileno(fp), &st)) {
        return -1;
    }
    tsize = st.st_size ;
    clear() ;
    i = 0 ;
    pos = 0 ;
    viewed = 0 ;
    pageno = linectr = 0;
    numbytes = readln(fp,buf) ;
    do {
        if(!numbytes)
          break ;
        viewed += numbytes ;
        prints("%s\n",buf) ;
        pos++ ;
        i++ ;
        if(pos == t_lines) {
            scroll() ;
            pos-- ;
        }
        if (++linectr == t_lines) {
            if (++pageno < 1024) pagebreaks[pageno] = viewed;
            linectr = 0;
        }
        numbytes = readln(fp,buf) ;
        if(!numbytes)
          break ;
        if(i == t_lines-1) {
            move(t_lines-1,0) ;
            if(!dumb_term) {
                standout() ;
                prints("--More-- (%d%%)",(viewed*100)/tsize) ;
                standend() ;
            } else
              bell() ;
            while((ch = igetch()) != EOF) {
                if(ch == ' ') {
                    move(t_lines-1,0) ;
                    clrtoeol() ;
                    refresh() ;
                    i = 1 ;
                    break ;
                }
                if(ch == 'q') {
                    move(t_lines-1,0) ;
                    clrtoeol() ;
                    refresh() ;
                    fclose(fp) ;
                    return  0;
                }
                if(ch == '\r') {
                    move(t_lines-1,0) ;
                    clrtoeol() ;
                    refresh() ;
                    i = t_lines-2 ;
                    break ;
                }
                if(ch == 'b') {
		    if (pageno >= 1 && pageno < 1024 && viewed < tsize) {
		        pageno--;
			viewed = pagebreaks[pageno];
		        fseek(fp, viewed, SEEK_SET);
                        move(t_lines-1,0);
			clrtoeol();
			i = linectr = 0;
			numbytes = readln(fp, buf);
			break;
		    }
                }
                bell() ;
            }
        }
    } while(numbytes) ;
    fclose(fp) ;
    move(t_lines-1,0) ;
    if(promptend)
      pressreturn() ;
    return 0 ;
}
