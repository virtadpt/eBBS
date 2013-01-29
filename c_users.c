 
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
#if LACKS_MALLOC_H
# include <stdlib.h>
#else
# include <malloc.h>
#endif
#include <signal.h>
#include <time.h>

NAMELIST acctlist;
extern LOGININFO myinfo;     /* for idle timeout in Monitor */

CheckUserid(userid)
char *userid;
{
  ACCOUNT acct;
  if (userid[0] == '\0') return 1;
  if (!is_valid_userid(userid)) return 3;
  if (bbs_query(userid, &acct) != S_NOSUCHUSER) return 2; 
  return 0;
}

PromptForAccountInfo(acct, for_self)
ACCOUNT *acct;
int for_self;
{
  PASSWD passcfm;
  char ans[4];
  int lineno, userok = 0, passok, cfm = 1;
  memset(acct, 0, sizeof(*acct));
  if (for_self) {
    prints("Welcome, new user! Enter a userid, 1-%d characters, no spaces.\n",
	   NAMELEN);
    lineno = 0;
  }
  else lineno = 3;
  while (!userok) {
    if (getdata(lineno,0,"Userid: ",acct->userid,NAMELEN+1,DOECHO,1) == -1)
      return -1;

    switch (CheckUserid(acct->userid)) {
    case 0: userok = 1;
      break;
    case 1: if (!for_self) return -1;   /* bail out */
      break;
    case 2: bbperror(S_USEREXISTS, NULL);
      break;
    case 3: bbperror(S_BADUSERID, NULL);
      break;
    }
  }
  if (lineno) lineno++;
  while (cfm) {
    do {
      getdata(lineno,0,"Enter Passwd: ", acct->passwd, PASSLEN+1, NOECHO, 0);
      passok = is_valid_password(acct->passwd);
      if (!passok) bbperror(S_BADPASSWD, NULL);
    } while (!passok);
    getdata(lineno+1,0,"Confirm Passwd: ", passcfm, PASSLEN+1, NOECHO, 0);
    if (cfm = strcmp(acct->passwd, passcfm))
      prints("\nPasswords did not match! Try again.\n");
  }
  if (lineno) lineno++;
  getdata(lineno,0,"User Name: ", acct->username, UNAMELEN+1, DOECHO, 0);
  if (lineno) lineno++;
  getdata(lineno,0,"Terminal type (default=vt100): ",
	  acct->terminal, TERMLEN+1, DOECHO, 0);
  getdata(lineno,0,"Charset (default=ascii): ",
          acct->charset, CSETLEN+1, DOECHO, 0);
  if (!for_self) {
    if (lineno) lineno++;
    getdata(lineno,0,"Real Name: ", acct->realname, RNAMELEN+1,DOECHO,0);
    if (lineno) lineno++;
    getdata(lineno,0,"Postal Address: ", acct->address, ADDRLEN+1,DOECHO,0);
  }
  if (lineno) lineno++;
  getdata(lineno,0,"E-mail address, if any: ",acct->email,MAILLEN+1,DOECHO,0);
  return 0;
}

/*ARGSUSED*/
AllUsersFunc(indx, acct, info)
int indx;
ACCOUNT *acct;
struct enum_info *info;
{
  if (info->topline == info->currline) {
    move(info->topline-1, 0);
    prints("%-14s %-30s   %s\n","User Id", "User Name", "Last Login");
  }

  prints("%-14s %-30s %c %s", acct->userid, acct->username,
   BITISSET(acct->flags, FLG_EXEMPT) ? 'X': ' ',
   (acct->lastlogin == 0) ? "\n":ctime((time_t *)&acct->lastlogin));

  info->currline++;
  info->count++;

  if (info->currline > info->bottomline) {
    int ch;
    standout();
    prints("--MORE--");
    standend();
    clrtoeol();
    while((ch = igetch()) != EOF) {
      if(ch == '\n' || ch == '\r' || ch == ' ')
	break;
      if(toupper(ch) == 'Q') {
	move(info->currline, 0);
	clrtoeol();
	return ENUM_QUIT;
      }
      else bell();
    }
    info->currline = info->topline;
  }
  return S_OK;
}

AllUsers()
{
  struct enum_info info;
  info.count = 0;
  info.topline = info.currline = 4;
  info.bottomline = t_lines-2;
  move(3,0);
  clrtobot();
  bbs_enum_accounts(t_lines-5, 0, AllUsersFunc, &info);
  clrtobot();
  move(t_lines-1, 0);
  prints("%d %s displayed\n", info.count, info.count==1?"user":"users");
  return PARTUPDATE;
}

/*ARGSUSED*/
OnlineUsersFunc(indx, urec, info)
int indx;
USEREC *urec;
struct enum_info *info;
{
  if (info->topline == info->currline) {
    move(info->topline-1, 0);
    prints("%-12s    %-25s %-25s %s %s\n", 
	   "User Id", "User Name", "From", "P", "Mode");
  }

  prints("%-12s  %c %-25s %-25s %c %s\n", urec->userid, 
	 BITISSET(urec->flags, FLG_CLOAK) ? '#': ' ', 
	 urec->username, urec->fromhost, 
	 BITISSET(urec->flags, FLG_NOPAGE) ? 'N': ' ', 
         ModeToString(urec->mode));

  info->currline++;
  info->count++;

  if (info->currline > info->bottomline) {
    int ch;
    standout();
    prints("--MORE--");
    standend();
    clrtoeol();
    while((ch = igetch()) != EOF) {
      if(ch == '\n' || ch == '\r' || ch == ' ')
	break;
      if(toupper(ch) == 'Q') {
	move(info->currline, 0);
	clrtoeol();
	return ENUM_QUIT;
      }
      else bell();
    }
    info->currline = info->topline;
  }
  return S_OK;
}

OnlineUsers()
{
  struct enum_info info;
  info.count = 0;
  info.topline = info.currline = 4;
  info.bottomline = t_lines-2;
  move(3,0);
  clrtobot();
  bbs_enum_users(t_lines-5, 0, NULL, OnlineUsersFunc, &info);
  clrtobot();
  move(t_lines-1, 0);
  prints("%d %s displayed\n", info.count, info.count==1?"user":"users");
  return PARTUPDATE;
}

struct shorturec {
  NAME userid;
  LONG pid;
  SHORT flags;
  SHORT mode;
  short wasfound;
  short found;
  short active;
} *global_ulist;

int global_ulist_sz;

SetupGlobalList()
{
  global_ulist_sz = (t_lines - 4) * 4;
  global_ulist = 
    (struct shorturec *)calloc(global_ulist_sz, sizeof(struct shorturec));
  if (global_ulist == NULL)
    {
      move(3,0);
      prints("Not enough memory to fetch user list!\n");
      return -1;
    }
  return 0;
}

/*ARGSUSED*/
FillShortUserList(indx, urec, arg)
int indx;
USEREC *urec;
void *arg;
{
  int i;
  for (i=0; i<global_ulist_sz; i++)
    if (global_ulist[i].pid == urec->pid) {
      global_ulist[i].flags = urec->flags;
      global_ulist[i].mode = urec->mode;
      global_ulist[i].found = 1;
      break;
    }
  if (i >= global_ulist_sz)
    for (i=0; i<global_ulist_sz; i++)
      if (!global_ulist[i].active)
	{
	  strcpy(global_ulist[i].userid, urec->userid);
	  global_ulist[i].pid = urec->pid;
	  global_ulist[i].flags = urec->flags;
	  global_ulist[i].mode = urec->mode;
	  global_ulist[i].active = global_ulist[i].found = 1;
	  break;
	}

  return S_OK;
}

DoShortUserList()
{
  int i, y = 3, x = 0, ucount = 0;
  time_t now;

  for (i=0; i<global_ulist_sz; i++) {
    global_ulist[i].wasfound = global_ulist[i].found;
    global_ulist[i].found = 0;
  }
  move(y, x);
  clrtobot();
  time(&now);
  bbs_enum_users(global_ulist_sz, 0, NULL, FillShortUserList, NULL);
  for (i=0; i<global_ulist_sz; i++)
    {
      if (global_ulist[i].found)
	{
	  prints("[%c]%s%-14s", ModeToChar(global_ulist[i].mode), 
                 BITISSET(global_ulist[i].flags, FLG_CLOAK) ? " #" : " ",
		 global_ulist[i].userid);
	  ucount++;
	}
      else if (global_ulist[i].wasfound) prints("%18s", " ");

      x+=18;
      if ((x+18) > t_columns)
	{
	  x=0; y++;
	}
      move(y, x);
    }
  move(t_lines-1, 0);
  prints("%d user%s online at %s", ucount, (ucount==1?"":"s"), ctime(&now));
  refresh();
  return ucount;
}

ShortList()
{
  int i;
  if (global_ulist == NULL) {
    if (SetupGlobalList() == -1) return PARTUPDATE;
  }
  DoShortUserList();
  memset(global_ulist, 0, global_ulist_sz * sizeof(*global_ulist));
  return PARTUPDATE;
}

struct shorturec *monitor_data;
int monitor_max;
int monitor_idle;
char global_modechar_key[75];

form_modechar_key()
{
  SHORT i;
  int left;
  char c, *s, buf[20];
  strcpy(global_modechar_key, "Key:");
  left = sizeof(global_modechar_key) - 5;
  for (i=0; i<=BBS_MAX_MODE; i++) {
    c = ModeToChar(i);
    if (c == ' ') continue;
    s = ModeToString(i);
    sprintf(buf, " [%c]", c);
    if (toupper(c) == toupper(*s)) strncat(buf+4, s+1, 10);
    else strncat(buf+4, s, 10);
    if (left > strlen(buf)) {
      strcat(global_modechar_key, buf);
      left -= strlen(buf);
    }
  }
  return 0;
}

void
monitor_refresh(sig)
{
  int i, boottime;
  if (sig) signal(sig, SIG_IGN);
  boottime = myinfo.idletimeout*120;
  if (boottime && ((monitor_idle += MONITOR_REFRESH) > boottime)) {
    disconnect(EXIT_TIMEDOUT);
  }
  if (bbs_check_mail()) {
    move(0, t_columns/3);
    prints("(You have mail.)");
  }
  DoShortUserList();
  for (i=0; i<global_ulist_sz; i++)
    if (!global_ulist[i].found) global_ulist[i].active = 0;

  signal(SIGALRM, monitor_refresh);
  alarm(MONITOR_REFRESH);
}

Monitor()
{
  void (*asig)();
  char ch;
  int saved_alarm;
  if (global_modechar_key[0] == '\0') form_modechar_key();
  clear();
  prints("Monitor Mode                                  Press CTRL-C \
or CTRL-D to exit.\n");
  prints("%s\n", global_modechar_key);
  prints("-----------------------------------------------------------\
------------------\n");
  if (global_ulist == NULL) {
    if (SetupGlobalList() == -1) return PARTUPDATE;
  }
  monitor_idle = 0;
  bbs_set_mode(M_MONITOR);
  saved_alarm = alarm(0);
  asig = signal(SIGALRM, SIG_IGN);
  monitor_refresh(0);
  while (1)
    {
      ch = igetch();
      monitor_idle = 0;
      if (ch == CTRL('C') || ch == CTRL('D')) break;
    }
  alarm(0);
  signal(SIGALRM, asig);
  if (saved_alarm) alarm(saved_alarm);
  bbs_set_mode(M_UNDEFINED);
  memset(global_ulist, 0, global_ulist_sz * sizeof(*global_ulist));
  return FULLUPDATE;
}

SetPasswd()
{
  ACCOUNT acct;
  int rc;
  PASSWD passbuf, passcfm;
  move(3,0);
  clrtobot();
  if ((rc = bbs_owninfo(&acct)) != S_OK) {
    move(4,0);
    prints("\n");
    bbperror(rc, "Cannot fetch old password");
    return PARTUPDATE;
  }
  if (getdata(3,0,"Old password: ",passbuf,sizeof passbuf,NOECHO,1) == -1)
    return FULLUPDATE;

  if (passbuf[0] == '\0' || !is_passwd_good(acct.passwd, passbuf)) {
    move(4,0);
    prints("\nSorry.\n");
    return PARTUPDATE;
  } 
  getdata(4,0,"New Password: ",passbuf,sizeof passbuf,NOECHO,0);
  if(!is_valid_password(passbuf)) {
    move(5,0);
    prints("\nBad Password\n");
    return PARTUPDATE;
  }
  getdata(5,0,"Confirm Password: ",passcfm,sizeof passbuf,NOECHO,0);
  move(6,0);
  if(strcmp(passbuf,passcfm)) {
    prints("Error entering password.\n");
    return PARTUPDATE;		
  }
  rc = bbs_set_passwd(passbuf);
  if (rc == S_OK)
    prints("Password was changed.\n");
  else 
    bbperror(rc, "Password change failed");

  return PARTUPDATE;
}

SetUsername()
{
  UNAME username;
  int rc;
  move(3,0);
  clrtobot();
  if (getdata(3,0,"Enter your name: ",username,sizeof username,DOECHO,1)==-1)
    return FULLUPDATE;
  rc = bbs_set_username(username);    
  if (rc == S_OK)
    prints("Name was changed.\n");
  else 
    bbperror(rc, "Name change failed");

  return PARTUPDATE;
}

SetAddress()
{
  MAIL email;
  int rc;
  move(3,0);
  clrtobot();
  if (getdata(3, 0, "Enter your e-mail address: ", email,
      sizeof email,DOECHO,1) == -1) return FULLUPDATE;
  rc = bbs_set_email(email);
  if (rc == S_OK)
    prints("Address was changed.\n");
  else 
    bbperror(rc, "Address change failed");

  return PARTUPDATE;
}

SetTermtype()
{
  TERM terminal;
  int rc;
  move(3,0);
  clrtobot();
  if (getdata(3,0, "Enter new terminal type: ", terminal, 
      sizeof terminal, DOECHO, 1) == -1) return FULLUPDATE;
  if(terminal[0] == '\0')
    return PARTUPDATE;
  if(term_init(terminal) == -1) {
    prints("Invalid terminal type.\n");
#ifndef REMOTE_CLIENT
    return PARTUPDATE;
#endif
  }
  else {
    initscr();
    clear();
  }
  rc = bbs_set_terminal(terminal);
  if (rc == S_OK)
    prints("New terminal type was saved.\n");
  else 
    bbperror(rc, "New terminal type not saved.");

  pressreturn();
  return FULLUPDATE;
}

SetCharset()
{
  CSET charset;
  int rc;
  move(3,0);
  clrtobot();
  if (getdata(3,0, "Enter new charset: ", charset,
      sizeof charset, DOECHO, 1) == -1) return FULLUPDATE;
  if(charset[0] == '\0')
    return PARTUPDATE;
  if(conv_init(charset) == -1) {
    prints("Invalid character set.\n");
#ifndef REMOTE_CLIENT
    return PARTUPDATE;
#endif
  }
  else {
    initscr();
    clear();
  }
  rc = bbs_set_charset(charset);
  if (rc == S_OK)
    prints("New character set was saved.\n");
  else
    prints("New character set NOT saved.\n");
  pressreturn();
  return FULLUPDATE;
}

UserDisplay(acct)
ACCOUNT *acct;
{
  prints("[%s]\n", acct->userid);
  prints("User name:       %s\n", acct->username);
  if (acct->lastlogin) {
    prints("Last login time: %s", ctime((time_t *)&acct->lastlogin));
    prints("Last login from: %s\n", acct->fromhost);
  }
  else prints("Never logged in.\n");
  if (*acct->terminal) prints("Terminal type:   %s\n", acct->terminal);
  if (*acct->charset)  prints("Charset:         %s\n", acct->charset);
  if (*acct->realname) prints("Real name:       %s\n", acct->realname);
  if (*acct->address)  prints("Address:         %s\n", acct->address);
  if (*acct->email)    prints("E-mail address:  %s\n", acct->email);
  clrtobot();
}        

ShowOwnInfo()
{
  ACCOUNT acct;
  int rc;
  move(3,0);
  clrtobot();
  rc = bbs_owninfo(&acct);
  if (rc != S_OK) {
    bbperror(rc, "Can't get user info:\n");
  }
  else UserDisplay(&acct);
  return PARTUPDATE;
}

AddAccount()
{
  int rc;
  ACCOUNT acct;
  char ans[4];
  move(3,0);
  clrtobot();
  if (PromptForAccountInfo(&acct, 0) == -1) {
    return PARTUPDATE;
  }
  getdata(12, 0, "Are you sure (Y/N)? [N]: ", ans, sizeof ans, DOECHO, 0);
  move(13,0);
  if (ans[0] != 'Y' && ans[0] != 'y') {
    prints("Account not added.\n");
    return PARTUPDATE;
  }
  rc = bbs_add_account(&acct, 0);  
  switch (rc) {
  case S_OK:
    prints("New account added.\n");
    break;
  default:
    bbperror(rc, "Account add failed");
  }
  return PARTUPDATE;
}

DeleteAccount()
{
  NAME namebuf;
  int rc;
  char ans[4];
  move(2,0);
  clrtobot();
  bbs_acctnames(&acctlist, NULL);
  namecomplete(NULL, acctlist, "Userid to delete: ", namebuf, sizeof(NAME));
  if (namebuf[0] == '\0' || !is_in_namelist(acctlist, namebuf)) {
    bbperror(S_NOSUCHUSER, NULL);
    pressreturn();
    return FULLUPDATE;
  }
  prints("Deleting user '%s'.\n", namebuf);
  getdata(5,0,"Are you sure (Y/N)? [N]: ",ans,sizeof(ans),DOECHO,0);    
  if (ans[0] != 'Y' && ans[0] != 'y') {
    prints("Account not deleted.\n");
    pressreturn();
    return FULLUPDATE;
  }
  rc = bbs_delete_account(namebuf);
  if (rc == S_OK)
    prints("Account deleted.\n");
  else
    bbperror(rc, "Account deletion failed");

  pressreturn();
  return FULLUPDATE;
}

SetUserData()
{
  NAME userid;
  ACCOUNT acct, nr;
  int rc;
  SHORT flags = 0;
  PASSWD passcfm;
  int x, y, grok;
  char genbuf[256], ans[4];
  move(2,0);
  memset(&nr, 0, sizeof nr);
  bbs_acctnames(&acctlist, NULL);
  namecomplete(NULL, acctlist, "Userid to set: ", userid, sizeof(NAME)); 
  if (userid[0] == '\0' || !is_in_namelist(acctlist, userid)) {
    bbperror(S_NOSUCHUSER, NULL);
    pressreturn();
    return FULLUPDATE;
  }
  if ((rc = bbs_get_userinfo(userid, &acct)) != S_OK) {
    bbperror(rc, NULL);
    pressreturn();
    return FULLUPDATE;
  }
  move(3,0);
  UserDisplay(&acct);
  getyx(&y, &x);
  getdata(++y,0,"Change any user information (Y/N)? [N]: ",ans,sizeof(ans),
	  DOECHO, 0);
  if (ans[0] != 'Y' && ans[0] != 'y') {
    prints("Record not changed.\n");
    pressreturn();
    return FULLUPDATE;
  }
  
  sprintf(genbuf, "New userid [%s]: ", acct.userid);
  do {
    getdata(y+1, 0, genbuf, nr.userid, sizeof(nr.userid), DOECHO, 0);
    if (!nr.userid[0])
      break;
    if (grok = CheckUserid(nr.userid)) {
      move(y+2, 0);
      prints("Invalid or taken userid. Try again.\n");
    }
    else BITSET(flags, MOD_USERID);
  } while (grok);
  y++;
  
  do {
    getdata(y+1,0,"New Password: ",nr.passwd,sizeof(nr.passwd),NOECHO, 0);
    if (!nr.passwd[0]) break;
    getdata(y+2,0,"Confirm Pass: ", passcfm,sizeof(passcfm),NOECHO, 0);
    if (grok = strcmp(nr.passwd, passcfm)) {
      move(y+3, 0);
      prints("Passwords don't match. Try again.\n");
    }
    else BITSET(flags, MOD_PASSWD);
  } while (grok);
  move(y+2, 0);    
  clrtobot();
  y+=2;
  
  sprintf(genbuf, "New username [%s]: ", acct.username);
  getdata(y++, 0, genbuf, nr.username, sizeof(nr.username), DOECHO, 0);
  if (nr.username[0]) BITSET(flags, MOD_USERNAME);
  
  sprintf(genbuf, "New terminal type [%s]: ", acct.terminal);
  getdata(y++, 0, genbuf, nr.terminal, sizeof(nr.terminal), DOECHO, 0);
  if (nr.terminal[0]) BITSET(flags, MOD_TERMINAL);
  
  sprintf(genbuf, "New charset [%s]: ", acct.charset);
  getdata(y++, 0, genbuf, nr.charset, sizeof(nr.charset), DOECHO, 0);
  if (nr.charset[0]) BITSET(flags, MOD_CHARSET);
 
  sprintf(genbuf, "New real name [%s]: ", acct.realname);
  getdata(y++, 0, genbuf, nr.realname, sizeof(nr.realname), DOECHO, 0);
  if (nr.realname[0]) BITSET(flags, MOD_REALNAME);
  
  sprintf(genbuf, "New address: ");
  getdata(y++, 0, genbuf, nr.address, sizeof(nr.address), DOECHO, 0);
  if (nr.address[0]) BITSET(flags, MOD_ADDRESS);
  
  sprintf(genbuf, "New mail address: ");
  getdata(y++, 0, genbuf, nr.email, sizeof(nr.email), DOECHO, 0);
  if (nr.email[0]) BITSET(flags, MOD_EMAIL);
  
  getdata(y, 0, "Are you sure (Y/N)? [N]: ", ans, sizeof(ans), DOECHO, 0);
  if (ans[0] == 'Y' || ans[0] == 'y') {    
    rc = bbs_modify_account(acct.userid, &nr, flags);
    if (rc == S_OK)
      prints("User data was changed.\n");
    else
      bbperror(rc, "User modify failed");
  }
  else prints("User data not changed.\n");
  
  pressreturn();
  return FULLUPDATE;
}

char *global_permstrs[32];

#define PERMMENULETTER(i) ((i)<26?('A'+(i)):('1'+(i)-26))
#define PERMMENUNUMBER(c) ((c)>='A'?((c)-'A'):((c)-'1'+26))
#define PBITSET(m,i)       (((m)>>(i))&1)

LONG
SetPermMenu(pbits)
LONG pbits;
{
  int i, rc, done = 0;
  char buf[80], choice[2];
  move(4,0);

  if (global_permstrs[0] == NULL)
    if ((rc = bbs_get_permstrings(global_permstrs)) != S_OK) {
      move(3,0);
      bbperror(rc, "Can't get permission strings");
      pressreturn();
      return pbits;
    }

  prints("Enter the letter/number to toggle, RETURN when done.\n");
  move(6,0);
  for (i=0; i<16; i++) {
    sprintf(buf, "%c. %-20s %3s      %c. %-20s %3s\n", 
	    PERMMENULETTER(i), global_permstrs[i], 
            PBITSET(pbits,i) ? "YES" : "NO",
	    PERMMENULETTER(i+16), global_permstrs[i+16], 
            PBITSET(pbits,i+16) ? "YES" : "NO");
    prints(buf);
  }
  clrtobot(); 
  while (!done) {
    getdata(t_lines-1, 0, "Choice (ENTER to quit): ",choice,2,DOECHO,0);
    *choice = toupper(*choice);
    if (*choice == '\n' || *choice == '\0') done = 1;
    else if (!isalnum(*choice) || (*choice>='7' && *choice<='9') ||
	     *choice == '0') bell();
    else {
      i = PERMMENUNUMBER(*choice);
      if (PBITSET(pbits,i))
	BITCLR(pbits,1<<i);
      else BITSET(pbits,1<<i);
      i%=16;
      sprintf(buf, "%c. %-20s %3s      %c. %-20s %3s\n", 
   	      PERMMENULETTER(i), global_permstrs[i], 
              PBITSET(pbits,i) ? "YES" : "NO",
	      PERMMENULETTER(i+16), global_permstrs[i+16], 
              PBITSET(pbits,i+16) ? "YES" : "NO");
      move(i+6,0);
      prints(buf);
    }
  }				
  return (pbits);
}

SetUserPerms()
{
  int rc;
  NAME namebuf;
  LONG newperms;
  ACCOUNT acct;
  move(2,0);
  bbs_acctnames(&acctlist, NULL);
  namecomplete(NULL, acctlist, "Userid to set: ", namebuf, sizeof(NAME));
  if (namebuf[0] == '\0' || !is_in_namelist(acctlist, namebuf)) {
    bbperror(S_NOSUCHUSER, NULL);
    pressreturn();
    return FULLUPDATE;
  }
  if ((rc = bbs_get_userinfo(namebuf, &acct)) != S_OK) {
    bbperror(rc, NULL);
    pressreturn();
    return FULLUPDATE;
  }
  move(2,0);
  clrtobot();
  prints("Set the permissions for user '%s'\n", acct.userid);
  newperms = SetPermMenu(acct.perms);	
  move(2,0);
  if (newperms == acct.perms)
    prints("User '%s' permissions not changed.\n", acct.userid);
  else {
    rc = bbs_modify_perms(acct.userid, newperms);
    if (rc == S_OK) {
      prints("User '%s' permissions changed.\n", acct.userid);
    }
    else {
      bbperror(rc, "Permission change failed");
    }
  }
  pressreturn();
  return FULLUPDATE;
}

QueryEdit()
{
  PATH planfile;
  char ans[7];
  int rc;
  move(3,0);
  clrtobot();
  if (getdata(4,0,"Edit or Delete plan? [E]: ",ans,sizeof(ans),DOECHO,1)==-1)
    return FULLUPDATE;

  if (*ans == 'D' || *ans == 'd') {
    bbs_set_plan(NULL);
    move(6,0);
    prints("Plan deleted.\n");
    return PARTUPDATE;
  }
  bbs_get_plan(myinfo.userid, planfile);    
  if (Edit(planfile)) {
    clear();
    prints("Plan NOT updated.\n");
  }
  else {
    clear();
    if ((rc = bbs_set_plan(planfile)) == S_OK) prints("Plan updated.\n");
    else bbperror(rc, "Plan update failed");
  }
  pressreturn();
  return FULLUPDATE;
}

/*ARGSUSED*/
_query_if_logged_in(indx, urec, loggedin)
int indx;
USEREC *urec;
int *loggedin;
{
  (*loggedin)++;
  return ENUM_QUIT;
}

Query()
{
  NAME namebuf;
  ACCOUNT acct;
  PATH planfile;
  char buf[80];
  FILE *fp;
  int i, rc, in_now = 0, firstsig = 6;
  bbs_acctnames(&acctlist, NULL);
  move(3,0);
  clrtobot();
  prints("<Enter Userid>\n");    
  move(2,0);
  namecomplete(NULL, acctlist, "Query who: ", namebuf, sizeof(NAME));
  move(2,0);
  clrtoeol();
  move(3,0);
  if (namebuf[0] == '\0' || !is_in_namelist(acctlist, namebuf)) {
    if (namebuf[0]) {
      bbperror(S_NOSUCHUSER, NULL);
    }
    return PARTUPDATE;
  }

  if ((rc = bbs_query(namebuf, &acct)) != S_OK) {
    bbperror(rc, NULL);
    return PARTUPDATE;
  }

  bbs_enum_users(20, 0, acct.userid, _query_if_logged_in, &in_now);

  prints("%s (%s):\n", acct.userid, acct.username);
  if (acct.lastlogin == 0)
    prints("Never logged in.\n");
  else prints("%s from %s %s %s", in_now ? "On" : "Last login", acct.fromhost,
	      in_now ? "since" : "at", ctime((time_t *)&acct.lastlogin));

  if (acct.realname[0] != '\0') {
    prints("Real name: %s\n", acct.realname);
    firstsig++;
  }

  if (bbs_get_plan(acct.userid, planfile) != S_OK) {
    prints("No plan.\n");
  }
  else {
    /* For now, just print one screen of the plan. In the future maybe
       prompt to ask if they want to page thru the whole plan, since
       we have it. */
    if (fp = fopen(planfile, "r")) {
      prints("Plan:\n");
      for (i=firstsig; i<t_lines; i++) {
	if (!fgets(buf, sizeof buf, fp)) break;
	prints("%s", buf);
      }
      fclose(fp);
    }
    else prints("No plan.\n");
  }
  return PARTUPDATE;
}

ToggleCloak()
{
  int rc;
  move(3,0);
  clrtobot();
  rc = bbs_toggle_cloak();
  if (rc != S_OK) {
    bbperror(rc, "Cloak toggle failed");
    return PARTUPDATE;
  }
  prints("Cloak has been toggled.\n");
  return PARTUPDATE;
}

ToggleExempt()
{
  int rc;
  NAME namebuf;
  ACCOUNT acct;
  move(2,0);
  bbs_acctnames(&acctlist, NULL);
  namecomplete(NULL, acctlist, "Userid to exempt/unexempt: ", 
               namebuf, sizeof(NAME));
  if (namebuf[0] == '\0' || !is_in_namelist(acctlist, namebuf)) {
    bbperror(S_NOSUCHUSER, NULL);
    pressreturn();
    return FULLUPDATE;
  }
  if ((rc = bbs_get_userinfo(namebuf, &acct)) != S_OK) {
    bbperror(rc, NULL);
    pressreturn();
    return FULLUPDATE;
  }
  rc = bbs_toggle_exempt(acct.userid);
  if (rc == S_OK) {
    if (BITISSET(acct.flags, FLG_EXEMPT))
      prints("User '%s' is now subject to user clean.\n", acct.userid);
    else 
      prints("User '%s' is now exempt from user clean.\n", acct.userid);
  }
  else {
    bbperror(rc, "Exempt toggle failed");
  }
  pressreturn();
  return FULLUPDATE;
}

SetPager()
{
  int rc;
  char ans[3], buf[40];
  ACCOUNT acct;
  SHORT setting = 1, pageroff = 0, overrideoff = 0;
  move(3,0);
  clrtobot();
  if ((rc = bbs_owninfo(&acct)) != S_OK) {
    bbperror(rc, NULL);
    return PARTUPDATE;
  }    
  if (acct.flags & FLG_NOPAGE) setting += 1;
  if (acct.flags & FLG_NOOVERRIDE) setting += 2;  
  prints("Current pager setting is %d. Select new setting:\n", setting);
  prints("1) Anyone can page\n");
  prints("2) Only users on override list can page\n");
  prints("3) Only users NOT on override list can page\n");
  prints("4) Nobody can page\n");
  sprintf(buf, "Your choice (1-4)? [%d]: ", setting);
  if (getdata(8, 0, buf, ans, sizeof ans, DOECHO, 1)==-1)
    return FULLUPDATE;
  if (*ans != '1' && *ans != '2' && *ans != '3' && *ans != '4') {
    prints("Pager setting not changed.\n");
    return PARTUPDATE;
  }
  if (*ans == '2' || *ans == '4') pageroff = 1;
  if (*ans == '3' || *ans == '4') overrideoff = 1;  
  rc = bbs_set_pager(pageroff, overrideoff);
  if (rc != S_OK) {
    bbperror(rc, "Pager setting failed");
    return PARTUPDATE;
  }
  prints("Pager has been set.\n");
  return PARTUPDATE;
}

SignatureEdit()
{
  PATH sigfile;
  char ans[7];
  int rc;
  move(3,0);
  clrtobot();
  if (getdata(4,0,"Edit or Delete signature? [E]: ", 
      ans, sizeof(ans), DOECHO, 1) == -1) return FULLUPDATE;

  if (*ans == 'D' || *ans == 'd') {
    bbs_set_signature(NULL);
    move(6,0);
    prints("Signature deleted.\n");
    return PARTUPDATE;
  }
  bbs_get_signature(sigfile);    
  if (Edit(sigfile)) {
    clear();
    prints("Signature NOT updated.\n");
  }
  else {
    clear();
    if ((rc = bbs_set_signature(sigfile)) == S_OK) 
      prints("Signature updated.\n");
    else bbperror(rc, "Signature update failed");
  }
  pressreturn();
  return FULLUPDATE;
}

MenuConfig()
{
  int rc;
  SHORT expert = (myinfo.flags & FLG_EXPERT);
  SHORT newsetting;
  char ans[3], buf[40];
  move(3,0);
  clrtobot();
  prints("Current menu level is: %s\n", expert ? "Expert" : "Novice");
  sprintf(buf, "Use (N)ovice or (E)xpert menus? [%c]: ", expert ? 'N' : 'E');
  if (getdata(5, 0, buf, ans, sizeof ans, DOECHO, 1) == -1)
    return FULLUPDATE;
  if (*ans == 'N' || *ans == 'n') newsetting = 0;
  else if (*ans == 'E' || *ans == 'e') newsetting = 1;
  else newsetting = (expert ? 0 : 1);  
  if (expert == newsetting) {
    prints("Menu setting not changed.\n");
    return PARTUPDATE;
  }
  rc = bbs_set_cliopts(newsetting);
  if (rc != S_OK) {
    bbperror(rc, "Menu setting failed");
    return PARTUPDATE;
  }
  prints("Changing to %s menus...\n", newsetting ? "Expert" : "Novice");
  if (newsetting) myinfo.flags |= FLG_EXPERT;
  else myinfo.flags &= ~FLG_EXPERT;
  pressreturn();
  return FULLUPDATE;
}
