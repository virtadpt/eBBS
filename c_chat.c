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
#include <ctype.h>
#include <varargs.h>

#define CHAT_PROMPT  "-->"
#define CHAT_HELP_FILE "etc/chathlp.txt"
#define CHAT_XTRA_HELP_FILE "etc/chatxhlp.txt"

extern BBSINFO serverinfo;
extern char *Ctime __P((time_t *));      /* for query */

int g_currline;
int g_echatwin;
int g_you_have_mail;

print_chatid(chatid)
char *chatid;
{
  char buf[CHATID_MAX+2];
  int i;
  memset(buf, 0, sizeof buf);
  strncpy(buf, chatid, CHATID_MAX);
  i = strlen(buf);
  buf[i] = ':';
  for (i++; i<=CHATID_MAX; i++) buf[i] = ' ';
  buf[CHATID_MAX+1] = '\0';
  move(g_echatwin+1, 0);
  prints("%s", buf);
  return 0;
}

printchatline(str)
char *str;
{
  int linelen = t_columns-1, len = strlen(str);
#if COLOR
  int color = 0;
#endif
  if (!g_you_have_mail && bbs_check_mail()) {
    int i;
    move(g_echatwin, 0);
    prints("---(You have mail.)");
    for (i=19; i<t_columns; i++) prints("-");
    g_you_have_mail++;
  }  
#if COLOR
    if (*str == '*') 
      color = (strncmp(str, "***", 3) ? COLOR_GREEN : COLOR_LIGHTBLUE);
#endif
  while (len) {
    move(g_currline, 0);
    clrtoeol();
    if (linelen < t_columns-1) move(g_currline, CHATID_MAX+2);
    if (len >= linelen) {
      int eoln;
      char c;
      for (eoln = linelen-1; eoln >= t_columns/4; eoln--)
	if (isspace(str[eoln])) break;
      if (!isspace(str[eoln])) eoln = linelen-1;
      c = str[eoln];
      str[eoln] = '\0';
#if COLOR
      if (color) colorstart(color);
#endif
      prints("%s", str);
#if COLOR
      if (color) colorend();
#endif
      str[eoln] = c;
      if (isspace(c)) eoln++;
      len -= eoln;
      str += eoln;
      if (linelen == t_columns-1) linelen -= (CHATID_MAX+2);
    }
    else {
#if COLOR
      if (color) colorstart(color);
#endif
      prints("%s", str);
#if COLOR
      if (color) colorend();
#endif
      len = 0;
    }
    if (++g_currline == g_echatwin) g_currline = 0;
    move(g_currline, 0);
    standout();
    prints(CHAT_PROMPT);
    standend();
    clrtoeol();
  }
}

chat_help(helpfile)
char *helpfile;
{
  FILE *fp;
  char buf[84];
  fp = fopen(helpfile, "r");
  if (fp == NULL) {
    printchatline("*** Cannot open help file\n");
  }
  else {
    strcpy(buf, "*** ");
    while (fgets(buf+4, sizeof(buf)-4, fp)) 
      if (buf[4] != '#') printchatline(buf);
    fclose(fp);
  }
  return 0;
}

chat_resetscreen(chatid)
char *chatid;
{
  int i;
  char buf[80];
  clear();
  g_currline = 0;
  g_echatwin = t_lines - 2;
  move(g_echatwin, 0);
  for (i=0; i<t_columns; i++) prints("-");
  sprintf(buf, "%s Chat System -- type /h for Help", serverinfo.boardname);
  printchatline(buf);
  print_chatid(chatid);
  return 0;
}

struct _chatlist {
  int start;
  int stop;
  int col;
  int verbose;
  char buf[80];
};

chat_list_users_func(indx, urec, cl)
int indx;
USEREC *urec;
struct _chatlist *cl;
{
  indx++;    /* Start counting at one, not zero. */
  if (indx < cl->start) return S_OK;
  else if (cl->stop && (indx > cl->stop)) return ENUM_QUIT;
  if (cl->verbose) {
    char fromhost[24];
    memset(fromhost, '\0', sizeof fromhost);
    strncpy(fromhost, urec->fromhost, sizeof(fromhost)-1);
    sprintf(cl->buf, "*** %-14s %-25s %c%-8s %-23s", 
  	    urec->userid, urec->username,
	    BITISSET(urec->flags, FLG_CLOAK) ? '#' : ' ',
	    ModeToString(urec->mode), fromhost);
    printchatline(cl->buf);
  }
  else {
    if (cl->col == 0) {
      strcpy(cl->buf, "*** ");
      cl->col = 4;
    }     
    sprintf(cl->buf+cl->col, "[%c]%s%-13s ", ModeToChar(urec->mode), 
            BITISSET(urec->flags, FLG_CLOAK) ? " #" : " ", urec->userid);
    if ((cl->col += 18) > 70) {
      printchatline(cl->buf);
      cl->col = 0;
    }          
  }
  return S_OK;
}

chat_list_users(cbuf, verbose)
char *cbuf;
int verbose;
{
  struct _chatlist cl;
  extern char global_modechar_key[];
  if (!HasPerm(C_USERS)) {
    printchatline("*** You do not have permission to list users");
    return 0;
  }
  while (*cbuf && !isspace(*cbuf)) cbuf++;
  while (*cbuf && isspace(*cbuf)) cbuf++;
  cl.start = atoi(cbuf);
  while (*cbuf && isdigit(*cbuf)) cbuf++;
  while (*cbuf && !isdigit(*cbuf)) cbuf++;
  cl.stop = atoi(cbuf);
  cl.col = 0;
  cl.verbose = verbose;
  printchatline("***");
  if (verbose) {
    sprintf(cl.buf, "*** %-14s %-25s  %-8s %s", "Userid", "Username", 
            "Mode", "From");
    printchatline(cl.buf);
    sprintf(cl.buf, "*** %-14s %-25s  %-8s %s", "------", "--------",
  	    "----", "----");
    printchatline(cl.buf);
  }
  else {
    if (global_modechar_key[0] == '\0') form_modechar_key();
    sprintf(cl.buf, "*** %s", global_modechar_key);
    printchatline(cl.buf);
    printchatline("*** ------------------------------------------------------------------------");
  }
  bbs_enum_users(t_lines-5, 0, NULL, chat_list_users_func, &cl);
  if (!verbose && cl.col > 0) printchatline(cl.buf);
  return 0;
}

extern int _query_if_logged_in __P((int, USEREC *, int *));

chat_query_user(cbuf)
char *cbuf;
{
  ACCOUNT acct;
  char buf[80];
  int in_now = 0;
  while (*cbuf && !isspace(*cbuf)) cbuf++;
  while (*cbuf && isspace(*cbuf)) cbuf++;
  strip_trailing_space(cbuf);
  if (!HasPerm(C_QUERY)) {
    printchatline("*** You do not have permission to query");
    return 0;
  }
  if (*cbuf == '\0') {
    printchatline("*** You must specify a userid to query");
    return 0;
  }
  if (bbs_query(cbuf, &acct) != S_OK) {
    sprintf(buf, "*** Userid '%s' not found", cbuf);
    printchatline(buf);
    return 0;
  }     
  bbs_enum_users(20, 0, acct.userid, _query_if_logged_in, &in_now);
  printchatline("***");
  sprintf(buf, "*** %s (%s):", acct.userid, acct.username);
  printchatline(buf);
  if (acct.lastlogin == 0)
    strcpy(buf, "*** Never logged in.");
  else sprintf(buf, "*** %s from %s %s %s", 
               in_now ? "Logged in" : "Last login", 
               acct.fromhost, in_now ? "since" : "at", 
               Ctime((time_t *)&acct.lastlogin));
  printchatline(buf);
  if (acct.realname[0] != '\0') {
    sprintf(buf, "*** Real name: %s", acct.realname);
    printchatline(buf);
  }
  return 0;
}

chat_show_page_request()
{
  USEREC urec;
  char buf[80];
  if (bbs_get_talk_request(&urec, NULL, NULL) != S_OK) {
    /* Whoever was paging stopped. */
    return 0;
  }
  sprintf(buf, "*** Being paged by %s (%s)", urec.userid, urec.username);
  printchatline(buf);
  return 0;
}

chat_process_incoming(fd, chatid)
int fd;
char *chatid;
{
  static char buf[CHATLINE_MAX*2+1];
  static int bufstart = 0;
  int c, len;
  char *bptr;

  len = sizeof(buf) - bufstart - 1;
  if ((c = recv(fd, buf+bufstart, len, 0)) <= 0) return -1;
  c += bufstart;

  bptr = buf;
  while (c > 0) {
    len = strlen(bptr)+1;
    if (len > c && len < (sizeof buf / 2)) break;
    if (!strncmp(bptr, CHAT_CTRL_CHATID, 3)) {
      memset(chatid, 0, CHATID_MAX+1);
      strncpy(chatid, bptr+4, CHATID_MAX);
      print_chatid(chatid);
    }
    else printchatline(bptr);
    c -= len;
    bptr += len;
  }        

  if (c > 0) {
    char temp[CHATLINE_MAX*2+1];
    strcpy(temp, bptr);
    strcpy(buf, temp);
    bufstart = len-1;
  }            
  else bufstart = 0;
  return 0;  
}

chat_exit(buf)
char *buf;
{
  /* Send the line, then return 1 so we exit. */
  bbs_chat_send(buf);
  return 1;
}

chat_cmd_match(buf, str)
char *buf;
char *str;
{
  if (*buf != '/') return 0;
  for (buf++; *str && *buf && !isspace(*buf); buf++, str++) {
    if (tolower(*buf) != *str) return 0;
  }      
  return 1;
}

chat_process_local(cbuf, chatid)
char *cbuf;
char *chatid;
{
  /* 
     See if the typed line should be handled locally. If not, return -1.
     If so, return 1 if we should exit, 0 if we keep going.
  */
  if (!strncmp(cbuf, "Goodbye!", 8)) {
    chat_exit("/e\n");
    return 1;
  }

  if (chat_cmd_match(cbuf, "clear")) 
    return (chat_resetscreen(chatid));
  else if (chat_cmd_match(cbuf, "exit"))
    return (chat_exit(cbuf));
  else if (chat_cmd_match(cbuf, "help"))
    return (chat_help(CHAT_HELP_FILE));
  else if (chat_cmd_match(cbuf, "long"))
    return (chat_list_users(cbuf, 1));
  else if (chat_cmd_match(cbuf, "query"))
    return (chat_query_user(cbuf));
  else if (chat_cmd_match(cbuf, "users"))
    return (chat_list_users(cbuf, 0));
  else if (chat_cmd_match(cbuf, "xhelp"))
    return (chat_help(CHAT_XTRA_HELP_FILE));

  return -1;
}

Chat()
{
  CHATID chatid;
  char cbuf[CHATLINE_TEXT_MAX+1];
  int cfd;
  int rc;
  int done_chatting = 0;
  int margin, curspos, maxpos, endpos;
  int ch, i;
  int metakey = 0;
  int shift, shiftdelta;

  move(2, 0);
  clrtobot();

  do {  
    if (getdata(2,0,"Enter chatid: ",chatid,sizeof chatid,DOECHO,1)==-1)
      return FULLUPDATE;

    if (*chatid == '\0') {
      strncpy(chatid, myinfo.userid, CHATID_MAX);
      chatid[CHATID_MAX] = '\0';
    }
    rc = bbs_chat(chatid, (LONG *)&cfd);
    switch (rc) {
    case S_OK: break;
    case S_BADCHATID:
    case S_CHATIDINUSE:
      bbperror(rc, NULL);
      break;
    default:
      bbperror(rc, "Error entering chat");
      return PARTUPDATE;
    }
  } while (rc != S_OK);

  add_io(cfd, 0);
  chat_resetscreen(chatid);

  memset(cbuf, '\0', sizeof cbuf);
  margin = CHATID_MAX+2;
  maxpos = sizeof(cbuf) - margin - 1;
  curspos = 0;
  endpos = 0;
  shift = 0;
  shiftdelta = (t_columns - margin) / 2;
    
  while (!done_chatting) {
    move(g_echatwin+1, margin+(curspos-shift));

    ch = igetch();
    if (NewPagePending()) {
      chat_show_page_request();
    }      
    if (ch == -1) done_chatting = 1;
    else if (ch == I_OTHERDATA) {
      /* Incoming! */
      if (chat_process_incoming(cfd, chatid) == -1)
        done_chatting = 1;
    }
    else if (metakey) {
      switch (ch) {
      case 'b': case 'B':
	if (curspos == 0) bell();
	else {
	  while (curspos && isspace(cbuf[curspos-1])) curspos--;
	  while (curspos && !isspace(cbuf[curspos-1])) curspos--;
	}
	break;
      case 'f': case 'F':
	if (curspos == endpos) bell();
	else {
	  while (curspos < endpos && isspace(cbuf[curspos])) curspos++;
	  while (curspos < endpos && !isspace(cbuf[curspos])) curspos++;
	}
	break;
      default: 
	bell();
      }
      metakey = 0;
    }
    else switch (ch) {
    case '\r': 
    case '\n':
      if (endpos > 0) {
        cbuf[endpos] = '\n';
        done_chatting = chat_process_local(cbuf, chatid);
        if (done_chatting == -1) {
          done_chatting = (bbs_chat_send(cbuf) != S_OK);
        }
      }
      memset(cbuf, '\0', sizeof cbuf);
      endpos = curspos = 0;
      move(g_echatwin+1, margin);
      clrtoeol();
      break;
    case CTRL('A'):
      if (curspos == 0) bell();
      else curspos = 0;
      break;
    case CTRL('B'):
      if (curspos == 0) bell();
      else curspos--;
      break;
    case CTRL('C'):
      bbs_chat_send("/e\n");
      done_chatting = 1;
      break;
    case CTRL('D'):
      if (curspos == endpos) bell();
      else {
        for (i=curspos; i<endpos; i++) cbuf[i] = cbuf[i+1];        
        endpos--;
        move(g_echatwin+1, margin+(curspos-shift));
        prints("%s", cbuf+curspos);
        clrtoeol();
      }
      break;
    case CTRL('E'):
      if (curspos == endpos) bell();
      else curspos = endpos;
      break;
    case CTRL('F'):
      if (curspos == endpos) bell();
      else curspos++;
      break;
    case CTRL('H'):
    case 127:
      if (curspos == 0) bell();
      else {
        for (i=curspos; i<=endpos; i++) cbuf[i-1] = cbuf[i];        
        endpos--;
        curspos--;
        move(g_echatwin+1, margin+(curspos-shift));
        prints("%s", cbuf+curspos);
	clrtoeol();
      }
      break;
    case CTRL('U'):
      if (endpos == 0) bell();
      else {
        memset(cbuf, '\0', sizeof cbuf);
        curspos = endpos = 0;
        move(g_echatwin+1, margin);
        clrtoeol();
      }
      break;
    case CTRL('W'):
      if (curspos == 0) bell();
      else {
        while (curspos && isspace(cbuf[curspos-1])) {
          for (i=curspos; i<=endpos; i++) cbuf[i-1] = cbuf[i];
	  curspos--, endpos--;
	}
        while (curspos && !isspace(cbuf[curspos-1])) {
          for (i=curspos; i<=endpos; i++) cbuf[i-1] = cbuf[i];
	  curspos--, endpos--;
	}
        move(g_echatwin+1, margin+(curspos-shift));
        prints("%s", cbuf+curspos);
        clrtoeol();
      }
      break;
    case 27:   /* ESC */
      metakey = 1;
      break;
    default:
      if (isprint(ch) && endpos < maxpos) {
        for (i=endpos; i>curspos; i--) cbuf[i] = cbuf[i-1];
        cbuf[curspos] = ch;        
        move(g_echatwin+1, margin+(curspos-shift));
        prints("%s", cbuf+curspos);
        curspos++;
        endpos++;
      }
      else bell();      
    }                              
    if (curspos-shift >= (t_columns-margin)) {
      while (curspos-shift >= (t_columns-margin)) shift += shiftdelta;
      move(g_echatwin+1, margin);
      prints("%s", cbuf+shift);
      clrtoeol();
    }
    else if (curspos < shift) {
      while (curspos < shift) shift -= shiftdelta;
      move(g_echatwin+1, margin);
      prints("%s", cbuf+shift);
      clrtoeol();
    }
  }

  bbs_exit_chat();
  g_you_have_mail = 0;
  add_io(0, 0);
  return FULLUPDATE;
}





