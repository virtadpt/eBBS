
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

/*
   Adapted from Pirates BBS 1.8 "read.c"
   Copyright (C) 1990, Edward Luke, lush@Athena.EE.MsState.EDU
*/

#include "client.h"
#if LACKS_MALLOC_H
# include <stdlib.h>
#else
# include <malloc.h>
#endif

#define MAIL_KEEP_NAME " $MAIL "

int BoardType;

extern OpenMailbox(), CloseMailbox();
extern OpenBoard(), CloseBoard();
extern OpenFileBoard(), CloseFileBoard();

#define PUTCURS   move(4+locmem->crs_line-locmem->top_line,0);prints(">");
#define RMVCURS   move(4+locmem->crs_line-locmem->top_line,0);prints(" ");

HEADER *headers = NULL;

struct keeploc {
    int btype;
    char *key ;
    int top_line ;
    int crs_line ;
    struct keeploc *next ;
} ;

struct keeploc *
getkeep(btype,s,def_topline,def_cursline)
char *s ;
LONG def_topline, def_cursline;
{
  static struct keeploc *keeplist = NULL ;
  struct keeploc *p ;

  for(p = keeplist; p!= NULL; p = p->next) {
    if(p->btype == btype && !strcmp(s,p->key))
      return p ;
  }
  p = (struct keeploc *) malloc(sizeof (*p)) ;
  p->btype = btype;
  p->key = (char *) malloc(strlen(s)+1) ;
  strcpy(p->key,s) ;
  p->top_line = def_topline ;
  p->crs_line = def_cursline ;
  p->next = keeplist ;
  keeplist = p ;
  return p ;
}

void
fixkeep(locmem, maxline, def_lines)
struct keeploc *locmem;
int maxline;
int def_lines;
{
  if (maxline < locmem->top_line) {
    if ((locmem->top_line = maxline - def_lines) < 1)
      locmem->top_line = 1;
  }
  if (maxline < locmem->crs_line) locmem->crs_line = maxline;
}

ReadMenuTitle(m)
NREADMENU *m;
{
  clear();
  move(0,0);
  prints("%s", m->menu_helptitle);
  move(0,52);
  if (BoardType == BOARD_MAIL) prints("%s\n", m->menu_message);
  else if (BoardType == BOARD_FILE) 
    prints("%s '%s'\n", m->menu_message, currfileboard);
  else prints("%s '%s'\n", m->menu_message, currboard);
  prints("%s\n", m->menu_line2);
  prints("%s\n", m->menu_line3);
  if (BoardType == BOARD_FILE) 
    prints("%5s %1s %5s  %s\n", m->menu_field1, m->menu_field2, 
           m->menu_field3, m->menu_field4) ;
  else
    prints("%5s %7s %-16s %s\n", m->menu_field1, m->menu_field2, 
           m->menu_field3, m->menu_field4) ;
  clrtobot() ;
}

struct get_recs_struct {
  SHORT want;
  SHORT got;
};

GetRecsFunc(indx, hdr, info)
int indx;
HEADER *hdr;
struct get_recs_struct *info;
{
  if (info->got >= info->want) return ENUM_QUIT;
  memcpy(&headers[info->got], hdr, sizeof(HEADER));
  info->got++;
  return S_OK;
}  

MenuGetRecords(first_line, num_lines)
int first_line, num_lines;
{
  int result;
  struct get_recs_struct gr;
  gr.want = (SHORT)num_lines;
  gr.got = 0;
  first_line--;
  result = bbs_enum_headers(gr.want, (SHORT)first_line, 0, GetRecsFunc, &gr);
  return (result == S_OK ? (int)gr.got : 0);
}

MenuDrawScreen(first_line, num_lines)
int first_line, num_lines;
{
  int i, k;
  char c;
  char buf[TITLELEN+40];
  move(4,0) ;
  for (i=0; i<num_lines; i++) {
    if (BoardType == BOARD_FILE) {
      if (BITISSET(headers[i].flags, FILE_DIRECTORY)) c = 'D';
      else c = (BITISSET(headers[i].flags, FILE_BINARY) ? 'B' : 'A');
    }
    else {
      if (headers[i].flags & FILE_MARKED)
	c = (BITISSET(headers[i].flags, FILE_UNREAD) ? 'M' : 'm');
      else
	c = (BITISSET(headers[i].flags, FILE_UNREAD) ? 'N' : ' ');
    }

    k = (headers[i].size + 512) / 1024;
    if (BoardType == BOARD_FILE)
      sprintf(buf, " %4d %c %4dk  %s", first_line+i, c, k, headers[i].title);  
    else 
      sprintf(buf, " %4d %c %4dk %-16s %s", first_line+i, c, k,
	         headers[i].owner, headers[i].title);

    if(strlen(buf) >= 79) {
      buf[78] = '^' ;
      buf[79] = '\0' ;
    }
    prints("%s\n", buf) ;
  }
}

NoHeaders()
{
  switch (BoardType) {
  case BOARD_MAIL:
    prints("No messages in this mailbox\n");
    break;
  case BOARD_FILE:
    prints("No files on this board\n");
    break;
  default:
    prints("No messages on this board\n");
  }
}

#define KEY_NORMAL (0)
#define KEY_ESCAPE (1)
#define KEY_VTKEYS (2)

ReadMenu(m, openfn, closefn, keepname)
NREADMENU *m;
int (*openfn)();
int (*closefn)();
char *keepname;
{
  char lbuf[11];
  NREADMENUITEM *mi;
  int lbc, ch, resp;
  struct keeploc *locmem;
  int screen_len = t_lines - 5;
  int num_entries;
  int last_line;
  int openflags;
  int mode = DONOTHING;
  int keymode = KEY_NORMAL;
  
  if (m == NULL) {
    move(3,0);
    prints("Menu is not defined in menu description file!\n");
    return PARTUPDATE;
  }

  if (!headers)
    headers = (HEADER *)calloc(screen_len, sizeof(*headers));
  
  ReadMenuTitle(m);
  last_line = (*openfn)(&openflags, 0, &resp);
  if (last_line == -1) {
    bbperror(resp, "Cannot access this board");
    return PARTUPDATE;
  }
  if(last_line == 0) {
    NoHeaders();
    (*closefn)();
    return PARTUPDATE;
  }
  
  locmem = getkeep(BoardType, keepname,
	   (last_line-screen_len+1 < 1)?1:last_line-screen_len+1,last_line) ;
  
  fixkeep(locmem, last_line, screen_len-3);
  num_entries = MenuGetRecords(locmem->top_line, screen_len);
  MenuDrawScreen(locmem->top_line, num_entries);
  
  PUTCURS ;
  lbc = 0 ;
  while((ch = igetch()) != EOF) {
    if (PagePending()) {
      Answer();
      mode = FULLUPDATE;
      goto endofloop;
    }
    if (keymode == KEY_ESCAPE) {
      if (ch == '[') keymode = KEY_VTKEYS;
      else {
	bell();
	keymode = KEY_NORMAL;
      }
      continue;
    }
    else if (keymode == KEY_VTKEYS) {
      keymode = KEY_NORMAL;
      if (ch == 'A') ch = 'p';       /* move up */
      else if (ch == 'B') ch = 'n';  /* move down */
      else {
	bell();
	continue;
      }
    }
    if(isdigit(ch)) {
      if(lbc < 9)
	lbuf[lbc++] = ch ;
      goto endofloop ;
    }
    if(ch != '\n' && ch != '\r')
      lbc = 0 ;
    switch(ch) {
      int val ;
    case 27: /* ESC */
      keymode = KEY_ESCAPE;
      break;
    case 'q':
    case 'e':
      mode = EXITMENU;
      break ;
    case '\n':
    case '\r':
      if(lbc == 0)
	goto defaultlabel ;
      lbuf[lbc] = '\0' ;
      val = atoi(lbuf) ;
      lbc = 0 ;
      if(val > last_line)
	val = last_line ;
      if(val <= 0)
	val = 1 ;
      if(val >= locmem->top_line && val < locmem->top_line+screen_len) {
	RMVCURS ;
	locmem->crs_line = val ;
	PUTCURS ;
	continue ;
      }
      locmem->top_line = val - 10 ;
      if(locmem->top_line <= 0)
	locmem->top_line = 1 ;
      locmem->crs_line = val ;
      mode = PARTUPDATE | FETCHNEW;
      goto endofloop ;
    case 'p':
      if(locmem->crs_line == locmem->top_line) {
	if(locmem->crs_line == 1) {
	  bell() ;
	  break ;
	}
	locmem->top_line -= screen_len - 2 ;
	if(locmem->top_line <= 0)
	  locmem->top_line = 1 ;
	locmem->crs_line-- ;
	mode = PARTUPDATE | FETCHNEW;
	break ;
      }
      RMVCURS ;
      locmem->crs_line-- ;
      PUTCURS ;
      break ;
    case CTRL('L'):
      redoscr() ;
      break ;
    case 'n':
      if(locmem->crs_line == last_line) {
	bell() ;
	break ;
      }
      if(locmem->crs_line+1 == locmem->top_line+screen_len) {
	locmem->top_line += screen_len - 2 ;
	locmem->crs_line++ ;
	mode = PARTUPDATE | FETCHNEW;
	break ;
      }
      RMVCURS ;
      locmem->crs_line++ ;
      PUTCURS ;
      break ;
    case 'N':
      if(locmem->top_line + screen_len - 1 >= last_line) {
	bell() ;
	break ;
      }
      locmem->top_line += screen_len - 1 ;
      locmem->crs_line = locmem->top_line ;
      mode = PARTUPDATE | FETCHNEW;
      break ;
    case 'P':
      if(locmem->top_line == 1) {
	bell() ;
	break ;
      }
      locmem->top_line -= screen_len - 1 ;
      if(locmem->top_line <= 0)
	locmem->top_line = 1 ;
      locmem->crs_line = locmem->top_line ;
      mode = PARTUPDATE | FETCHNEW;
      break ;
    case '$':
      if(last_line < locmem->top_line + screen_len) {
	RMVCURS ;
	locmem->crs_line = last_line ;
	PUTCURS ;
	break ;
      }
      locmem->top_line = last_line - screen_len + 1 ;
      if(locmem->top_line <= 0)
	locmem->top_line = 1 ;
      locmem->crs_line = last_line ;
      mode = PARTUPDATE | FETCHNEW;
      break ;
defaultlabel:
    default:
      for (mi = m->commlist; mi != NULL; mi = mi->next) {  
	if(mi->key == ch && 
           HasReadMenuPerm(mi->boardprivs, openflags) &&
	   HasPerm(mi->mainprivs)) {
	  mode = (*(mi->action_func))
	    (&headers[locmem->crs_line - locmem->top_line],
	     locmem->crs_line, last_line, openflags);
	  break ;
	}
      }
      if(mi == NULL)
	bell();
      break ;
    }
  endofloop:
    if (mode & (FETCHNEW | NEWDIRECT)) {
      if (mode & NEWDIRECT && !(mode & NOCLOSE)) (*closefn)();
      last_line = (*openfn)(&openflags, 0, &resp);
      if (mode & NEWDIRECT)
	locmem = getkeep(BoardType, keepname,
			 (last_line-screen_len < 1)?1:last_line-screen_len+1,
			 last_line) ;
      fixkeep(locmem, last_line, screen_len-3);
      num_entries = MenuGetRecords(locmem->top_line, screen_len);
    }
    if (mode & (FULLUPDATE | PARTUPDATE)) {
      if (mode & FULLUPDATE) {
	clear() ;
	ReadMenuTitle(m);
      }
      if(last_line <= 0) {
	NoHeaders();
	num_entries = 0 ;
	break;
      }
      MenuDrawScreen(locmem->top_line, num_entries);
      if(locmem->crs_line > last_line)
	locmem->crs_line = last_line ;
      clrtobot() ;
      PUTCURS ;
    }
    if(mode == EXITMENU || num_entries == 0)
      break ;
    mode = DONOTHING ;
  }
  (*closefn)();
  return FULLUPDATE;
}

MailRead()
{
  int rc;
  BoardType = BOARD_MAIL;
  bbs_set_mode(M_MAIL);
  rc = ReadMenu(MailReadMenu, OpenMailbox, CloseMailbox, MAIL_KEEP_NAME);
  bbs_set_mode(M_UNDEFINED);  
  return rc;
}

MainRead()
{
  int rc = PARTUPDATE;
  if (*currboard == '\0') {
    move(3,0);
    prints("Use (S)elect to select a board first.\n");
  }
  else {
    BoardType = BOARD_POST;
    bbs_set_mode(M_READING);
    rc = ReadMenu(PostReadMenu, OpenBoard, CloseBoard, currboard);
    bbs_set_mode(M_UNDEFINED);
  }
  return rc;
}

FileDownload()
{
  int rc = PARTUPDATE;
  if (*currfileboard == '\0') {
    move(3,0);
    prints("Use (S)elect to select a file board first.\n");
  }
  else {
    BoardType = BOARD_FILE;
    rc = ReadMenu(FileReadMenu, OpenFileBoard, CloseFileBoard, currfileboard);
  }
  return rc;
}
