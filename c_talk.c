
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
#include <signal.h>

#define TALK_MAX_COLS 128

NAMELIST userlist;
int g_page_pending;
int g_page_need_notify;

struct talkwin {
  int firstln;
  int lastln;
  int currln;
  int firstcol;
  int lastcol;
  int currcol;
  char line[TALK_MAX_COLS];
};

#define MAX_HANDLED_LOGINS 20

struct usertopid {
  USEREC saved_urec;
  LONG pids[MAX_HANDLED_LOGINS];
  int count;
};

void
page_handler(sig)
int sig;
{
  bell();
  bell();
  g_page_pending = 1;
  g_page_need_notify = 1;
  signal(sig, page_handler);  
}

PagePending()
{
  return g_page_pending;
}

/* We need this function to handle page requests in talk and chat mode.
   We only want to notify once, but need to keep the page request 
   around in case we want to go off and answer it. */

NewPagePending()
{
  if (g_page_need_notify) {
    g_page_need_notify = 0;
    return 1;
  }
  return 0;
}

PrintLoginEntry(num, urec)
int num;
USEREC *urec;
{
  prints("%2d %-12s  %-24s %-30s %s\n", num, urec->userid, 
	 urec->username, urec->fromhost, ModeToString(urec->mode));
}

/*ARGSUSED*/
PickLogin(indx, urec, info)
int indx;
USEREC *urec;
struct usertopid *info;
{
  if (info->count >= MAX_HANDLED_LOGINS) return ENUM_QUIT;
  info->pids[info->count++] = urec->pid;
  if (info->count == 1) {
    memcpy(&info->saved_urec, urec, sizeof(info->saved_urec));
    return S_OK;
  }        
  if (info->count == 2) {
    clear();
    PrintLoginEntry(1, &info->saved_urec);
  }
  PrintLoginEntry(info->count, urec);
  return S_OK;
}  

LONG
UseridToPid(userid)
char *userid;
{
  struct usertopid u2p;
  int x, y, indx;
  char ans[3];
  memset(&u2p, 0, sizeof u2p);
  bbs_enum_users(MAX_HANDLED_LOGINS, 0, userid, PickLogin, &u2p);
  if (u2p.count == 0) return (LONG)-1;
  else if (u2p.count == 1) return u2p.pids[0];
  /* Else, more than one. We must decide. */
  getyx(&y, &x);
  getdata(y, x, "Which login [1]: ", ans, sizeof ans, DOECHO, 0);
  indx = atoi(ans);    
  if (indx < 1 || indx > u2p.count) indx = 1;
  return (u2p.pids[indx-1]);
}

Kick()
{
  NAME namebuf;
  LONG pid;
  char ans[4];
  int rc;
  move(2, 0);
  clrtobot();
  bbs_usernames(&userlist, NULL);
  namecomplete(NULL, userlist, "Kick whom: ", namebuf, sizeof(NAME));
  if (namebuf[0] == '\0') {
    return FULLUPDATE;
  }
  else if (!is_in_namelist(userlist, namebuf)) {
    bbperror(S_NOSUCHUSER, NULL);
    pressreturn();
    return FULLUPDATE;
  }
  pid = UseridToPid(namebuf);
  if (pid == (LONG)-1) {
    bbperror(S_NOSUCHUSER, NULL);
    pressreturn();
    return FULLUPDATE;
  }
  getdata(3, 0, "Are you sure (Y/N)? [N]: ", ans, sizeof ans, DOECHO, 0);
  if (*ans != 'Y' && *ans != 'y') {
    prints("User NOT kicked.\n");
    pressreturn();
    return FULLUPDATE;
  }

  if ((rc = bbs_kick_user(pid)) == S_OK) {
    prints("User has been kicked.\n");
  }
  else bbperror(rc, "Kick failed");

  pressreturn();
  return FULLUPDATE;
}

void
TalkAdvanceLine(ts)
struct talkwin *ts;
{
  if (++ts->currln > ts->lastln) ts->currln = ts->firstln;
  move((ts->currln == ts->lastln ? ts->firstln : ts->currln+1), 0);
  clrtoeol();
  move(ts->currln, 0);
  clrtoeol();
}

DoTalkChar(ch, ts)
char ch;
struct talkwin *ts;
{
  /* This function handles backspaces, newlines, and printables. */
  move(ts->currln, ts->currcol);
  if (ch == CTRL('H') || ch == 127) {
    if (ts->currcol == 0) return -1;
    move(ts->currln, --ts->currcol);
    addch(' ');
    ts->line[ts->currcol] = '\0';
    move(ts->currln, ts->currcol);
  }
  else if (ch == '\n' || ch == '\r') {
    ts->currcol = 0;
    TalkAdvanceLine(ts);
    memset(ts->line, 0, sizeof ts->line);
  }
  else if (isprint(ch)) {
    if (ts->currcol < ts->lastcol) {
      ts->line[ts->currcol++] = ch;
      addch(ch);
    }
    else {
      char save[TALK_MAX_COLS];
      int i = ts->currcol;
      ts->line[i] = ch;
      memcpy(save, ts->line, sizeof save);
      while (i && save[i] != ' ') i--;
      if (i == 0) {
	memset(ts->line, 0, sizeof(ts->line));
	ts->currcol = 1;
	TalkAdvanceLine(ts);
	addch(ch);
      }
      else {
	move(ts->currln, i);
	clrtoeol();
	ts->currcol -= i;
	memset(ts->line, 0, sizeof(ts->line));
	memcpy(ts->line, &save[i+1], ts->currcol);
	TalkAdvanceLine(ts);
	prints(ts->line);
      }
    }
  }
  else return -1;

  refresh();
  return 0;
}

DoTalkString(s, tw)
char *s;
struct talkwin *tw;
{
  for (; s && *s; s++) DoTalkChar(*s, tw);
  return 0;
}

talk_show_page_request(ln)
int ln;
{
  USEREC urec;
  char buf[80];
  if (bbs_get_talk_request(&urec, NULL, NULL) != S_OK) {
    /* Whoever was paging stopped. */
    return 0;
  }
  sprintf(buf, " Being paged by %s (%s)", urec.userid, urec.username);
  move(ln, 3);
  prints(buf);
  move(ln, t_columns-20);  
  prints("[CTRL-R to erase]");
  return 0;
}

_talk_enum_users(count, urec, tw)
int count;
USEREC *urec;
struct talkwin *tw;
{
  char buf[NAMELEN+10];
  sprintf(buf, ",%s%s [%c]", BITISSET(urec->flags, FLG_CLOAK) ? " #" : " ",
          urec->userid, ModeToChar(urec->mode));
  DoTalkString((tw->currcol == 0 ? buf+2 : buf), tw);
  return S_OK;
}
    
talk_user_list(tw)
struct talkwin *tw;
{
  DoTalkString("\n*** Users currently online ***\n", tw);
  bbs_enum_users(10, 0, NULL, _talk_enum_users, tw);
  DoTalkChar('\n', tw);
  return 0;
}    

DrawDivider(ln)
int ln;
{
  int i;
  move(ln, 0);
  for (i=0; i<t_columns; i++) {
    addch('-');
  }
  refresh();
}

DoTalk(sock)
int sock;
{
  int i, ch, cc;
  int divider;
  char c;  
  char incoming[80];
  struct talkwin me, them;

  divider = (t_lines-1) / 2;
  me.currln = me.firstln = 0;
  me.lastln = divider - 1;
  them.currln = them.firstln = divider + 1;
  them.lastln = t_lines - 1;
  me.currcol = me.firstcol = them.currcol = them.firstcol = 0;
  me.lastcol = them.lastcol = 
    (t_columns > TALK_MAX_COLS ? TALK_MAX_COLS : t_columns - 1);
  memset(me.line, 0, sizeof me.line);
  memset(them.line, 0, sizeof them.line);

  clear();
  DrawDivider(divider);
  move(0, 0);
  add_io(sock, 0);
  while (1) {
    ch = igetch();
    if (NewPagePending()) {
      talk_show_page_request(divider);
      move(me.currln, me.currcol);
    }
    if (ch == -1) {
      /* error in igetch! */
      break;
    }
    if (ch == I_OTHERDATA) {
      /* pending input on sock */
      cc = recv(sock, incoming, sizeof incoming, 0);
      if (cc <= 0) {
	/* other side closed connection */
	break;
      }
      for (i=0; i<cc; i++) {
	DoTalkChar(incoming[i], &them);
      }
    }
    else {
      /* something we typed */
      c = (char)ch;
      if (DoTalkChar(c, &me) == 0) {
	if (send(sock, &c, 1, 0) != 1) {
	  /* connection broken */
	  break;
	}
      }
      else if (c == CTRL('C') || c == CTRL('D')) {
	/* we're ending the talk */
	break;
      }
      else if (c == CTRL('R')) {
	/* clear message on divider line */
	DrawDivider(divider);
	move(me.currln, me.currcol);
      }
      else if (c == CTRL('U')) {
	/* show a brief user list */
	talk_user_list(&me);
      }
      else {
	/* a character we don't like, at this time */
	bell();
      }
    }
  }
  add_io(0,0);

  bbs_exit_talk();
  close(sock);
  return 0;
}

Talk()
{
  NAME namebuf;
  LONG pid;
  LONG sock;
  int rc;
  move(2, 0);
  clrtobot();
  bbs_usernames(&userlist, NULL);
  namecomplete(NULL, userlist, "Page whom: ", namebuf, sizeof(NAME));
  if (namebuf[0] == '\0') {
    return FULLUPDATE;
  }
  else if (!is_in_namelist(userlist, namebuf)) {
    bbperror(S_NOSUCHUSER, NULL);
    pressreturn();
    return FULLUPDATE;
  }
  pid = UseridToPid(namebuf);
  if (pid == (LONG)-1) {
    bbperror(S_NOSUCHUSER, NULL);
    pressreturn();
    return FULLUPDATE;
  }
  clear();
  prints("Paging %s. Press CTRL-D to abort.\n", namebuf);
  refresh();
  do {
    bell();
    switch (rc = bbs_talk(pid, 0, &sock)) {
    case S_OK:
      DoTalk((int)sock);
      break;
    case S_CMDTIMEOUT:
      prints("No answer. Ringing party again.\n");
      refresh();
      break;      
    case S_INTERRUPT:
      return FULLUPDATE;
    default:
      bbperror(rc, "Page failed");
    }
  } while (rc == S_CMDTIMEOUT);
  if (rc != S_OK && rc != S_CMDTIMEOUT) pressreturn();
  return FULLUPDATE;
}

Answer()
{
  USEREC urec;
  LONG addr, sock;
  SHORT port;
  char ans[4];
  int rc;
  
  g_page_pending = g_page_need_notify = 0;

  if (bbs_get_talk_request(&urec, &addr, &port) != S_OK) {
    /* Whoever was paging stopped. */
    return 0;
  }

  clear();
  prints("Would you like to talk to %s (%s)?", urec.userid, urec.username);
  getdata(1, 0, "Yes or No [Y]: ", ans, sizeof ans, DOECHO, 0);
  if (*ans == 'n' || *ans == 'N') {
    bbs_refuse_page(addr, port);
    return 1;
  }

  if ((rc = bbs_accept_page(addr, port, &sock)) != S_OK) {
    bbperror(rc, "Answer failed");
    pressreturn();
    return 1;
  }

  DoTalk((int)sock);
  return 1;
}   
