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
#if NO_TERMIO
# include <sgtty.h>
#else
# include <termio.h>
#endif

#if NO_TERMIO

struct sgttyb tty_state, tty_new ;

get_tty()
{   
    if(gtty(1,&tty_state) < 0) {
        return(-1) ;
    }
    return 1 ;
}
  
init_tty()
{
    bcopy(&tty_state, &tty_new, sizeof(tty_new)) ;
    tty_new.sg_flags |= RAW ;
    tty_new.sg_flags &= ~(TANDEM|CBREAK|LCASE|ECHO|CRMOD) ;
    stty(1,&tty_new) ;
}

reset_tty()
{
    stty(1,&tty_state) ;
}
    
restore_tty()
{
    stty(1,&tty_new) ;
}

#else /* ! NO_TERMIO */

extern char *tgetstr();

struct termio tty_state, tty_new;

get_tty()
{
    if(ioctl(1,TCGETA,&tty_state) < 0) {
        return -1;
    }
#if LINUX
    /* Hopefully temporary workaround for buggy login program. */
    tty_state.c_oflag |= OPOST;
    tty_state.c_iflag |= BRKINT;
    tty_state.c_lflag |= ISIG;
    tty_state.c_cc[VMIN] = 1;
#endif
    return 0;
}

init_tty()
{
    memcpy(&tty_new, &tty_state, sizeof(tty_new));
    tty_new.c_iflag &= IXANY;
    tty_new.c_oflag &= ~(OPOST | OLCUC | OCRNL);
    tty_new.c_lflag &= ~(ICANON | ISIG | XCASE | ECHO);
    tty_new.c_lflag |= TOSTOP;
    tty_new.c_cc[VMIN] = 1;
    tty_new.c_cc[VTIME] = 0;
    if (ioctl(1,TCSETA,&tty_new) == -1) printf("ioctl failed\n");
}

reset_tty()
{
    ioctl(1,TCSETA,&tty_state) ;
}

restore_tty()
{
    ioctl(1,TCSETA,&tty_new) ;
}

#endif /* ! NO_TERMIO */

term_ok(term)
char *term;
{
   char tbuf[1024];
   if (tgetent(tbuf, term) != 1) return 0;
   return 1; 
}

#define TERMCOMSIZE (80)

int dumb_term = 1 ;

char clearbuf[TERMCOMSIZE] ;
int clearbuflen ;

char cleolbuf[TERMCOMSIZE] ;
int cleolbuflen ;

char cursorm[TERMCOMSIZE] ;
char *cm ;

char scrollrev[TERMCOMSIZE] ;
int scrollrevlen ;

char strtstandout[TERMCOMSIZE] ;
int strtstandoutlen ;

char endstandout[TERMCOMSIZE] ;
int endstandoutlen ;

#if COLOR
char strtcolor[TERMCOMSIZE] ;
int strtcolorlen;

char endcolor[TERMCOMSIZE] ;
int endcolorlen ;
#endif

int t_lines = 24;
int t_columns = 80 ;

int automargins ;

char *outp ;
int  *outlp ;

outcf(ch)
char ch ;
{
    if(*outlp >= TERMCOMSIZE)
        return ;
    (*outlp)++ ;
    *outp++ = ch ;
}

term_init(term)
char *term ;
{
#ifndef HPUX_TERMCAP
    extern char PC, *UP, *BC ;
    extern short ospeed ;
    static char UPbuf[TERMCOMSIZE] ;
    static char BCbuf[TERMCOMSIZE] ;
#endif
    static char buf[4096] ;
    char sbuf[4096] ;
    char *sbp, *s ;
    char *tgetstr() ;

#ifndef HPUX_TERMCAP
# if NO_TERMIO
    ospeed = tty_state.sg_ospeed ;
# else
    ospeed = tty_state.c_cflag & CBAUD ;
# endif
#endif
    if(tgetent(buf, term) != 1)
        return -1;
    sbp = sbuf ;
#ifndef HPUX_TERMCAP
    s = tgetstr("pc", &sbp) ; /* get pad character */
    if(s)
        PC = *s ;
#endif
    t_lines = tgetnum("li") ;
    t_columns = tgetnum("co") ;
    automargins = tgetflag("am") ;
    outp = clearbuf ;         /* fill clearbuf with clear screen command */
    outlp = &clearbuflen ;
    clearbuflen = 0 ;
    sbp = sbuf ;
    s = tgetstr("cl", &sbp) ;
    if(s)
        tputs(s,t_lines, outcf) ;
    outp = cleolbuf ;         /* fill cleolbuf with clear to eol command */
    outlp = &cleolbuflen ;
    cleolbuflen = 0 ;
    sbp = sbuf ;
    s = tgetstr("ce",&sbp) ;
    if(s)
        tputs(s,1,outcf) ;
    outp = scrollrev ;
    outlp = &scrollrevlen ;
    scrollrevlen = 0 ;
    sbp = sbuf ;
    s = tgetstr("sr",&sbp) ;
    if(s)
        tputs(s,1,outcf) ;
    outp = strtstandout ;
    outlp = &strtstandoutlen ;
    strtstandoutlen = 0 ;
    sbp = sbuf ;
    s = tgetstr("so",&sbp) ;
    if(s)
        tputs(s,1,outcf) ;
    outp = endstandout ;
    outlp = &endstandoutlen ;
    endstandoutlen = 0 ;
    sbp = sbuf ;
    s = tgetstr("se",&sbp) ;
    if(s)
        tputs(s,1,outcf) ;
    sbp = cursorm ;
    cm = tgetstr("cm",&sbp) ;
    if(cm)
        dumb_term = 0 ;
    else
        dumb_term = 1 ;
#if COLOR
    /* 
     Does termcap even have the strings for color? Mine doesn't. 
     Let's skip the BS and do the right thing for ansi 
     and not worry with the rest. At any rate -- strtcolor MUST
     have one and only one %d in it, and no other %-directives.
    */
    if (!strcmp(term, "ansi")) {
      strcpy(strtcolor, "\033[3%dm");
      strcpy(endcolor, "\033[0m");
      strtcolorlen = strlen(strtcolor);
      endcolorlen = strlen(endcolor);
    }
    else {
      strtcolorlen = endcolorlen = 0;
    }
#endif
#ifndef HPUX_TERMCAP
    sbp = UPbuf ;
    UP = tgetstr("up",&sbp) ;
    sbp = BCbuf ;
    BC = tgetstr("bc",&sbp) ;
#endif
    if(dumb_term) {
        t_lines = 24 ;
        t_columns = 80 ;
    }
    return 0;
}

do_move(destcol,destline,outc)
int destcol,destline ;
int (*outc)() ;
{
    tputs(tgoto(cm,destcol,destline), 1, outc) ;
}


