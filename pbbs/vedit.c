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
#include <sys/stat.h>
#include "io.h"
#include "edit.h"

struct textline *firstline = NULL ;
struct textline *lastline = NULL ;

struct textline *currline = NULL ;
int currpnt = 0 ;

struct textline *top_of_win = NULL ;
int curr_window_line ;
int redraw_everything ;

void
indigestion(i)
{
  fprintf(stderr,"SERIOUS INTERNAL INDIGESTION CLASS %d\n",i) ;
}

struct textline *
back_line(pos,num)
     struct textline *pos ;
{
  while(num-- > 0)
    if(pos && pos->prev)
      pos = pos->prev ;
  return pos ;
}

struct textline *
forward_line(pos,num)
     struct textline *pos ;
{
  while(num-- > 0)
    if(pos && pos->next)
      pos = pos->next ;
  return pos ;
}

int
getlineno()
{
  int cnt = 0 ;
  struct textline *p = currline ;
  
  while(p != top_of_win) {
    if(p == NULL)
      break ;
    cnt++ ;
    p = p->prev ;
  }
  return cnt ;
}
char *
killsp(s)
     char *s ;
{
  while(*s == ' ')
    s++ ;
  return s ;
}

struct textline *
alloc_line()
{
  extern void *malloc() ;
  register struct textline *p ;
  
  p = (struct textline *) malloc(sizeof(*p)) ;
  if(p == NULL) {
    indigestion(13) ;
    generic_abort() ;
  }
  p->next = NULL ;
  p->prev = NULL ;
  p->data[0] = '\0' ;
  p->len = 0 ;
  return p ;
}

/*
   Appends p after line in list.  keeps up with last line as well.
   */

void
append(p,line)
     register struct textline *p, *line ;
{
  p->next = line->next ;
  if(line->next)
    line->next->prev = p ;
  else
    lastline = p ;
  line->next = p ;
  p->prev = line ;
}

/*
   delete_line deletes 'line' from the list and maintains the lastline, and 
   firstline pointers.
   */


void
delete_line(line)
     register struct textline *line ;
{
  if(!line->next && !line->prev) {
    line->data[0] = '\0' ;
    line->len = 0 ;
    return ;
  }
  if(line->next)
    line->next->prev = line->prev ;
  else
    lastline = line->prev ;
  if(line->prev)
    line->prev->next = line->next ;
  else
    firstline = line->next ;
  free(line) ;
}

/*
   split splits 'line' right before the character pos
   */

void
split(line,pos)
     register struct textline *line ;
     register int pos ;
{
  register struct textline *p = alloc_line() ;
  
  if(pos > line->len) {
    free(p) ;
    return ;
  }
  p->len = line->len - pos ;
  line->len = pos ;
  strcpy(p->data,(line->data + pos)) ;
  *(line->data + pos) = '\0' ;
  append(p,line) ;
  if(line == currline && pos <= currpnt) {
    currline = p ;
    currpnt -= pos ;
    curr_window_line++ ;
  }
  redraw_everything = YEA ;
}

/*
   join connects 'line' and the next line.  It returns true if:
   
   1) lines were joined and one was deleted
   2) lines could not be joined
   3) next line is empty
   
   returns false if:
   
   1) Some of the joined line wrapped
   */

int 
join(line)
     register struct textline *line ;
{
  register int ovfl ;
  
  if(!line->next)
    return YEA ;
  if(*killsp(line->next->data) == '\0')
    return YEA ;
  ovfl = line->len + line->next->len - WRAPMARGIN ;
  if(ovfl < 0) {
    strcat(line->data, line->next->data) ;
    line->len += line->next->len ;
    delete_line(line->next) ;
    return YEA ;
  } else {
    register char *s ;
    register struct textline *p = line->next ;
    
    s = p->data + p->len - ovfl -1 ;
    while(s!=p->data && *s == ' ')
      s-- ;
    while(s!=p->data && *s != ' ')
      s-- ;
    if(s == p->data)
      return YEA ;
    split(p,(s - p->data) + 1) ;
    if(line->len + p->len >= WRAPMARGIN) {
      indigestion(0) ;
      return YEA ;
    }
    join(line) ;
    p = line->next ;
    if(p->len >= 1 && p->len+1 < WRAPMARGIN) {
      if(p->data[p->len-1] != ' ') {
	strcat(p->data," ") ;
	p->len++ ;
      }
    }
    return NA ;
  }
}

void
insert_char(ch)
     register int ch ;
{
  register int i ;
  register char *s ;
  register struct textline *p = currline ;
  int wordwrap = YEA ;
  
  if(currpnt > p->len) {
    indigestion(1) ;
    return ;
  }
  for(i = p->len; i >= currpnt; i--)
    p->data[i+1] = p->data[i] ;
  p->data[currpnt] = ch ;
  p->len++ ;
  currpnt++ ;
  if(p->len < WRAPMARGIN)
    return ;
  s = p->data + (p->len - 1) ;
  while(s!=p->data && *s == ' ')
    s-- ;
  while(s!=p->data && *s != ' ')
    s-- ;
  if(s==p->data) {
    wordwrap = NA ;
    s = p->data + (p->len -2) ;
  }
  split(p,(s - p->data) + 1) ;
  p = p->next ;
  if(wordwrap && p->len >= 1) {
    i = p->len ;
    if(p->data[i-1] != ' ') {
      p->data[i] = ' ' ;
      p->data[i+1] = '\0' ;
      p->len++ ;
    }
  }
  while(!join(p)) {
    p = p->next ;
    if(p == NULL) {
      indigestion(2) ;
      break ;
    }
  }
}

void
delete_char()
{
  register int i ;
  
  if(currline->len == 0)
    return ;
  if(currpnt >= currline->len) {
    indigestion(1) ;
    return ;
  }
  for(i=currpnt;i!=currline->len;i++)
    currline->data[i] = currline->data[i+1] ;
  currline->len-- ;
}

void
vedit_init()
{
  register struct textline *p = alloc_line() ;
  
  firstline = p ;
  lastline = p ;
  currline = p ;
  currpnt = 0 ;
  top_of_win = p ;
  curr_window_line = 0 ;
  redraw_everything = NA ;
}

void
read_file(filename)
     char *filename ;
{
  FILE *fp ;
  int ch ;
  
  if(currline == NULL)
    vedit_init() ;
  if((fp = fopen(filename,"r+")) == NULL) {
    if((fp = fopen(filename,"w+")) != NULL) {
      fclose(fp) ;
      return ;
    }
    indigestion(4) ;
    generic_abort() ;
  }
  while((ch = getc(fp)) != EOF)
      if(isprint(ch))
        insert_char(ch) ;
      else if(ch == '\n')
        split(currline,currpnt) ;
  fclose(fp) ;
}

#define KEEP_EDITING -2

int
write_file(filename)
     char *filename ;
{
  FILE *fp ;
  struct textline *p = firstline ;
  char abort[6];
  int aborted = 0;
  getdata(0,0,"(S)ave, (A)bort, or (E)dit? [S]: ",abort,6,DOECHO,NULL,0);
  if (abort[0] == 'a' || abort[0] == 'A') {
    struct stat stbuf;
#if 0
    prints("File NOT saved\n");
    refresh();
    sleep(2);
#endif
    if (stat(filename, &stbuf) || stbuf.st_size == 0) 
      unlink(filename);
    aborted = -1;
  }
  else if (abort[0] == 'e' || abort[0] == 'E')
    return KEEP_EDITING;
    firstline = NULL ;
  if (!aborted) {
    if((fp = fopen(filename,"w")) == NULL) {
      indigestion(5) ;
      generic_abort() ;
    }
  }
  while(p != NULL) {
    struct textline *v = p->next ;
    if (!aborted) if(p->next != NULL || p->data[0] != '\0')
      fprintf(fp,"%s\n",p->data) ;
    free(p) ;
    p = v ;
  }
  if (!aborted) fclose(fp) ;
  currline = NULL ;
  lastline = NULL ;
  firstline = NULL ;
  return aborted;
}

void
display_buffer()
{
  register struct textline *p ;
  register int i ;
  
  
  for(p=top_of_win,i=0;i < 24;i++) {
    move(i,0) ;
    if(p) {
      prints("%s",p->data) ;
      p = p->next ;
    } else prints("~") ;
    clrtoeol() ;
  }
  return ;
}

#define ESCAPE  (01)
#define VTKEYS  (02)
#define NORMAL  (00) 

vedit(filename)
     char *filename ;
{
  int ch ;
  int foo ;
  int lastcharindent = -1 ;
  int mode = NORMAL ;
  int firstkey = YEA ;
  
  read_file(filename) ;
  top_of_win = firstline ;
  currline = firstline ;
  curr_window_line = 0 ;
  currpnt = 0 ;
  clear() ;
  display_buffer() ;
  move(0,0) ;
  standout() ;
  prints("Type Ctrl-Z for help\n") ;
  standend() ;
  clrtoeol() ;
  move(1,0) ;
  clrtoeol() ;
  move(curr_window_line,currpnt) ;
  
  while((ch = igetch()) != EOF) {
    if(firstkey) {
      firstkey = NA ;
      move(0,0) ;
      clrtoeol() ;
      prints("~\n~") ;
    }
#ifdef DO_ISTRIP
    ch = ch & 0x7f ;
#endif
    switch(mode) {
    case ESCAPE:
      switch(ch) {
      case '[':
	mode = VTKEYS ;
	break ;
      case '>':
	top_of_win = back_line(lastline,23) ;
	currline = lastline ;
	curr_window_line = getlineno() ;
	currpnt = 0 ;
	redraw_everything = YEA ;
	mode = NORMAL ;
	break ;
      case '<':
	top_of_win = firstline ;
	currline = top_of_win ;
	currpnt = 0 ;
	curr_window_line = 0 ;
	redraw_everything = YEA ;
	mode = NORMAL ;
	break ;
      case 'v':
      case 'V':
	top_of_win = back_line(top_of_win,22) ;
	currline = top_of_win ;
	currpnt = 0 ;
	curr_window_line = 0 ;
	redraw_everything = YEA ;
	mode = NORMAL ;
	break ;
      default:
	bell() ;
	mode = NORMAL ;
	break ;
      }
      break ;
    case VTKEYS:
      switch(ch) {
      case 'A':
	ch = CTRL('P') ;
	break ;
      case 'B':
	ch = CTRL('N') ;
	break ;
      case 'C':
	ch = CTRL('F') ;
	break ;
      case 'D':
	ch = CTRL('B') ;
	break ;
      default:
	bell() ;
	mode = NORMAL ;
	break ;
      }
      if(mode == NORMAL)
	break ;
      mode = NORMAL ;
    default:
	if(isprint(ch)) {
	  insert_char(ch) ;
	  lastcharindent = -1 ;
	} else switch(ch) {
	case CTRL('P'):
	  if(lastcharindent == -1)
	    lastcharindent = currpnt ;
	  if(!currline->prev) {
	    bell() ;
	    break ;
	  }
	  curr_window_line-- ;
	  currline = currline->prev ;
	  currpnt = (currline->len > lastcharindent)?lastcharindent:currline->len ;
	  break ;
	case CTRL('N'):
	  if(lastcharindent == -1)
	    lastcharindent = currpnt ;
	  if(!currline->next) {
	    bell() ;
	    break ;
	  }
	  currline = currline->next ;
	  curr_window_line++ ;
	  currpnt = (currline->len > lastcharindent)?lastcharindent:currline->len ;
	  break ;
	default:
	  lastcharindent = -1 ;
	  switch(ch) {
	  case CTRL('F'):
	    if(currline->len == currpnt) {
	      if(!currline->next) {
		bell() ;
		break ;
	      }
	      currpnt = 0 ;
	      curr_window_line++ ;
	      currline = currline->next ;
	      break ;
	    }
	    currpnt++ ;
	    break ;
	  case CTRL('B'):
	    if(currpnt == 0) {
	      if(!currline->prev) {
		bell() ;
		break ;
	      }
	      curr_window_line-- ;
	      currline = currline->prev ;
	      currpnt = currline->len ;
	      break ;
	    }
	    currpnt-- ;
	    break ;
	  case CTRL('G'):
	    clear() ;
	    redraw_everything = YEA ;
	    break ;
	  case CTRL('Z'):
	    vedit_help() ;
	    break ;
	  case CTRL('V'):
	    {
	      struct textline *temp = forward_line(top_of_win,22) ;
	      if(temp == lastline) {
		bell() ;
		break ;
	      }
	      top_of_win = temp ;
	    }
	    currline = top_of_win ;
	    curr_window_line = 0 ;
	    currpnt = 0 ;
	    redraw_everything = YEA ;
	    break ;
	  case CTRL('A'):
	    currpnt = 0 ;
	    break ;
	  case CTRL('E'):
	    currpnt = currline->len ;
	    break ;
	  case CTRL('X'): case CTRL('W'):
	    clear() ;
	    foo = write_file(filename);
	    if (foo != KEEP_EDITING) return foo;
	  case '\r':
	  case '\n':
	    split(currline,currpnt) ;
	    break ;
	  case '\177':
	  case CTRL('H'):
	    if(currpnt == 0) {
	      struct textline *p ;
	      
	      if(!currline->prev) {
		bell() ;
		break ;
	      }
	      curr_window_line-- ;
	      currline = currline->prev ;
	      currpnt = currline->len ;
	      if(*killsp(currline->next->data) == '\0') {
		delete_line(currline->next) ;
		redraw_everything = YEA ;
		break;
	      }
	      
	      p = currline ;
	      while(!join(p)) {
		p = p->next ;
		if(p == NULL) {
		  indigestion(2) ;
		  generic_abort() ;
		}
	      }
	      redraw_everything = YEA ;
	      break ;
	    }
	    currpnt-- ;
	    delete_char() ;
	    break ;
	  case CTRL('D'):
	    if(currline->len == currpnt) {
	      struct textline *p = currline ;
	      
	      while(!join(p)) {
		p = p->next ;
		if(p == NULL) {
		  indigestion(2) ;
		  generic_abort() ;
		}
	      }
	      redraw_everything = YEA ;
	      break ;
	    }
	    delete_char() ;
	    break ;
	  case CTRL('K'):
	    if(currline->len == 0) {
	      struct textline *p = currline->next ;
	      
	      if(!p) {
		p = currline->prev ;
		if(!p) {
		  bell() ;
		  break ;
		}
		curr_window_line-- ;
	      }
	      if(currline == top_of_win)
		top_of_win = p ;
	      delete_line(currline) ;
	      currline = p ;
	      redraw_everything = YEA ;
	      break ;
	    }
	    if(currline->len == currpnt) {
	      struct textline *p = currline ;
	      
	      while(!join(p)) {
		p = p->next ;
		if(p == NULL) {
		  indigestion(2) ;
		  generic_abort() ;
		}
	      }
	      redraw_everything = YEA ;
	      break ;
	    }
	    currline->len = currpnt ;
	    currline->data[currpnt] = '\0' ;
	    break ;
	  case '\033':   /* ESC */
	    mode = ESCAPE ;
	    break ;
	  default:
	    bell() ;
	    break ;
	  }
	}
      break ;
    }
    if(curr_window_line == -1) {
      curr_window_line = 0 ;
      if(!top_of_win->prev) {
	indigestion(6) ;
	bell() ;
      } else {
	top_of_win = top_of_win->prev ;
	rscroll() ;
      }
    }
    if(curr_window_line == 24) {
      curr_window_line = 23 ;
      if(!top_of_win->next) {
	indigestion(7) ;
	bell() ;
      } else {
	top_of_win = top_of_win->next ;
	scroll() ;
      }
    }
    move(curr_window_line,0) ;
    prints("%s",currline->data) ;
    clrtoeol() ;
    if(redraw_everything)
      display_buffer() ;
    redraw_everything = NA ;
    move(curr_window_line,currpnt) ;
  }
}

char *helptxt[] = {
  "\01General Commands:",
  "Ctrl-X  Save and Exit (or Ctrl-W)",
  "Ctrl-L  Redraw Screen",
  "Ctrl-Z  Call up this help screen",
  "",
  "\01Cursor and Movement Commands:",
  "Ctrl-F  Forward character",
  "Ctrl-B  Backward character",
  "Ctrl-P  Previous line",
  "Ctrl-N  Next line",
  "Ctrl-V  Next Page",
  "ESC v   Previous Page",
  "ESC >   Goto end of file",
  "ESC <   Goto beginning of file",
  "Ctrl-A  Beginning of line",
  "Ctrl-E  End of line",
  "",
  "\01Deletion Commands:",
  "Ctrl-D  Delete character at cursor",
  "Ctrl-K  Delete to end of line",
  NULL } ;

vedit_help()
{
  int i ,off = 0, pos = 2  ;
  
  clear() ;
  standout() ;
  prints("Editor Help Screen --------------------------------------- Press Any Key to Exit");
  standend() ;
  for(i=0;helptxt[i];i++) {
    move(pos,off) ;
    if(helptxt[i][0] == '\01') {
      standout() ;
      prints("%s",&helptxt[i][1]) ;
      standend() ;
    } else prints("%s",helptxt[i]) ;
    clrtoeol() ;
    pos++ ;
  }
  move(23,0) ;
  igetch() ;
  clear() ;
  redraw_everything = YEA ;
}
