
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
#if LACKS_MALLOC_H
# include <stdlib.h>
#else
# include <malloc.h>
#endif

extern LOGININFO myinfo;
extern char *currentboard;
char *_menudesc_file;

/* New Menu Function */

int GetMenuLetter(s)
char *s;
{
    char c;
    for (c = *s; *s; s++) if (*s >= 'A' && *s <= 'Z') return *s;
    return c;
}

int GetMenuIndex(t)
unsigned int t ;
{
    t = (t | 0x20) - 'a' ;
    return t % MAXMENUSZ ;
}

char InterpretMenuAction(action) 
char *action;
{
  if(action[0] != '$')
    return GetMenuLetter(action) ;
  if(bbs_check_mail())
    return action[2] ;
  return action[1] ;
}

extern NMENU *bigMenuList ;

NMENU *menuEnt[MAXMENUDEPTH] ;
int currMenuEnt = -1 ;

#if 0
/*ARGSUSED*/
int 
NDoMenu(menu_name)
char *menu_name ;
{
  int found, update = FULLUPDATE;
  int fieldsz = t_columns/3;
  char oldcmd = 'h', cmd ;
  int comm_loc ;
  NMENU *msp ;
  NMENUITEM *item = NULL ;
  
  for(msp = bigMenuList;msp;msp=msp->next)
    if(!strcmp(menu_name,msp->menu_id))
      break ;
  if(!msp || currMenuEnt == MAXMENUDEPTH)
    return 0 ;
  currMenuEnt++ ;
  menuEnt[currMenuEnt] = msp ;
  if (myinfo.lastlogin == 0 || BITISSET(myinfo.flags, FLG_SHARED)) cmd = 'H';
  else cmd = InterpretMenuAction(msp->menu_default) ;
  while (update != EXITMENU) {
      if (PagePending()) {
          if (Answer()) update = FULLUPDATE;
      }
      if (update == FULLUPDATE || update == PARTUPDATE) {
          update == FULLUPDATE ? clear() : move(0,0);
          if(!strcmp(msp->menu_title,"*")) {
              extern BBSINFO serverinfo ;
              prints("%s",serverinfo.boardname) ;
          } else
            prints("%s",msp->menu_title);
          clrtoeol();
          move(0, fieldsz*2);
          if (currentboard[0] == '\0')
            prints("No Board Currently Selected");
          else
            prints("Current Board: %s", currentboard);
          move(2,0);
          clrtoeol();
          move(1,0);
          prints("%s",msp->menu_prompt);
          comm_loc = strlen(msp->menu_prompt) ;
          clrtoeol();
      }
      if (bbs_check_mail()) {
          move(0, fieldsz);
          prints("(You have mail.)");
      }
      while(cmd != '\n') {
          item = msp->menucommands[GetMenuIndex(cmd)] ;
          if(item != NULL && HasPerm(item->enabled)) {
              move(1,comm_loc);
              standout();
              prints("%s", item->name) ;
              standend();
              clrtoeol();
          } else {
              bell();
              cmd = oldcmd;
          }
          oldcmd = cmd;
          cmd = MenuGetch();
          if (!isalpha(cmd) && cmd != '\n') {
              bell();
              cmd = oldcmd;
          }
      }
      item = msp->menucommands[GetMenuIndex(oldcmd)] ;
      if (item != NULL) {
          update = (item->action_func)(item->action_arg);
          BITCLR(update, FETCHNEW | NEWDIRECT | NOCLOSE);     
          if (update & MENUERROR) 
            cmd = InterpretMenuAction(item->error_action);
          else 
            cmd = InterpretMenuAction(item->default_action) ;
      }
  }
  currMenuEnt-- ;
  return FULLUPDATE;
}
#endif

int
GetMenuLine(msp, cmd)
NMENU *msp;
char cmd;
{
  int lineno = 4;
  char mcmd;
  NMENUITEM *item;
  cmd = tolower(cmd);
  for (item=msp->commlist; item; item=item->next)
  if(HasPerm(item->enabled)) {
    mcmd = tolower(GetMenuLetter(item->name));
    if (cmd == mcmd) return lineno;
    lineno++;
  }
  return 0;
}

/*ARGSUSED*/
int 
NDoMenu(menu_name)
char *menu_name ;
{
  int found, update = FULLUPDATE;
  int fieldsz = t_columns/3;
  char oldcmd, cmd ;
  int comm_loc ;
  NMENU *msp ;
  NMENUITEM *item = NULL;
  NMENUITEM *selitem, *oldselitem;
  int oldselline = 0, selline = 0;
  
  for(msp = bigMenuList;msp;msp=msp->next)
    if(!strcmp(menu_name,msp->menu_id))
      break ;
  if(!msp || currMenuEnt == MAXMENUDEPTH)
    return 0 ;
  currMenuEnt++ ;
  menuEnt[currMenuEnt] = msp ;
  cmd = InterpretMenuAction(msp->menu_default) ;
  oldcmd = GetMenuIndex(GetMenuLetter(msp->commlist->name));
  while (update != EXITMENU) {
      if (PagePending()) {
          if (Answer()) update = FULLUPDATE;
      }
      if (update == FULLUPDATE || update == PARTUPDATE) {
          if (update == PARTUPDATE) {
            if (myinfo.flags & FLG_EXPERT) move(0,0);
	    else {
	      pressreturn();
	      clear();
	    }
          }
          else clear();
          if(!strcmp(msp->menu_title,"*")) {
              extern BBSINFO serverinfo ;
              prints("%s",serverinfo.boardname) ;
          } else
            prints("%s",msp->menu_title);
          clrtoeol();
          move(0, fieldsz*2);
          if (currentboard[0] == '\0')
            prints("No Board Currently Selected");
          else
            prints("Current Board: %s", currentboard);

          if (!(myinfo.flags & FLG_EXPERT)) {
            move(4,0) ;
            clrtoeol() ;
            for(item=msp->commlist;item;item=item->next)
              if(HasPerm(item->enabled)) {
#if MENU_STANDOUT
                if (item == oldselitem) standout();
                prints("%s\n",item->help) ;
  	        if (item == oldselitem) standend();
#else
                if (item == oldselitem) prints("--> %s\n", item->help);
                else prints("    %s\n",item->help) ;
#endif
              }
            clrtobot() ;
          }

          move(2,0);
          clrtoeol();
          move(1,0);
          prints("%s",msp->menu_prompt);
          comm_loc = strlen(msp->menu_prompt) ;
          clrtoeol();
      }
      if (bbs_check_mail()) {
          move(0, fieldsz);
          prints("(You have mail.)");
      }
      while(cmd != '\n') {
          selitem = msp->menucommands[GetMenuIndex(cmd)] ;
          if(selitem != NULL && HasPerm(selitem->enabled)) {
            if (!(myinfo.flags & FLG_EXPERT)) {
              selline = GetMenuLine(msp, cmd);              
              if (selline != oldselline) {
                if (oldselline) {
                  move(oldselline, 0);
#if MENU_STANDOUT
                  clrtoeol();
                  prints("%s\n", oldselitem->help);
#else
		  prints("   ");
#endif
                }
		move(selline, 0);
#if MENU_STANDOUT
                clrtoeol();
		standout();
		prints("%s\n", selitem->help);
		standend();
#else
		prints("-->");
#endif
		oldselitem = selitem;
                oldselline = selline;
              }
            }
            move(1,comm_loc);
            standout();
            prints("%s", selitem->name) ;
            standend();
            clrtoeol();
          } else {
              bell();
              cmd = oldcmd;
          }
          oldcmd = cmd;
          cmd = MenuGetch();
          if (cmd == MENU_UP) {
              if (selitem != NULL)
                for (item=selitem->prev; item && cmd==MENU_UP; item=item->prev)
                  if (HasPerm(item->enabled)) cmd = GetMenuLetter(item->name);
          }
	  else if (cmd == MENU_DOWN) {
              if (selitem != NULL)
                for (item=selitem->next;item && cmd==MENU_DOWN;item=item->next)
                  if (HasPerm(item->enabled)) cmd = GetMenuLetter(item->name);
	  }
          if (!isalpha(cmd) && cmd != '\n') {
              bell();
              cmd = oldcmd;
          }
      }
      item = msp->menucommands[GetMenuIndex(oldcmd)] ;
      if (item != NULL) {
          update = (item->action_func)(item->action_arg);
          BITCLR(update, FETCHNEW | NEWDIRECT);     
          if (update & MENUERROR) 
            cmd = InterpretMenuAction(item->error_action);
          else 
            cmd = InterpretMenuAction(item->default_action) ;
      }
  }
  currMenuEnt-- ;
  return FULLUPDATE;
}

do_help()
{
    NMENU *mp ;
    NMENUITEM *mip ;
    
    if(currMenuEnt < 0)
      return ;
    mp = menuEnt[currMenuEnt] ;
    move(3,0) ;
    clrtoeol() ;
    standout() ;
    prints("%s Menu Help Screen\n", mp->menu_id) ;
    standend() ;
    for(mip=mp->commlist;mip;mip=mip->next)
      if(HasPerm(mip->enabled))
        prints("%s\n",mip->help) ;
    clrtobot() ;
    return PARTUPDATE;
}

do_echo(s)
char *s ;
{
    clear() ;
    prints("%s",s) ;
    pressreturn() ;
    return FULLUPDATE ;
}

exec_func(s)
char *s ;
{
    int i ;
    char buf[4096] ;
    char *p, *q ;
    SHORT mode ;

    parse_default() ;
    strncpy(buf,s,sizeof(buf)) ;
    p=strchr(buf,':') ;
    if(p) {
        *p='\0' ;
        q=strchr(p+1,':') ;
        if(q) {
            *q='\0';
            mode = atoi(q+1);            
	}
        parse_environment(p+1) ;
    }
      
    if (mode > 1) bbs_set_mode(mode);
    clear() ;
    refresh() ;
    i = do_exec(buf,NULL) ;
    clear() ;
    if (mode > 1) bbs_set_mode(M_UNDEFINED);
    return FULLUPDATE ;
}
 
exec_func_w_pause(s)
char *s ;
{
    int i ;
    
    char buf[4096] ;
    char *p, *q ;
    SHORT mode = 0;

    parse_default() ;
    strncpy(buf,s,sizeof(buf)) ;
    p=strchr(buf,':') ;
    if(p) {
        *p='\0' ;
        q=strchr(p+1,':') ;
        if(q) {
            *q='\0';
            mode = atoi(q+1);            
	}
        parse_environment(p+1) ;
    }
      
    if (mode > 1) bbs_set_mode(mode) ;
    clear() ;
    refresh() ;
    i = do_exec(buf,NULL) ;
    pressreturn() ;
    clear() ;
    if (mode > 1) bbs_set_mode(M_UNDEFINED) ;
    return FULLUPDATE ;
}

int do_pipe_more() ;

int revised_pipe_more(s)
char *s ;
{
    char buf[4096] ;
    char *p ;
    
    parse_default() ;
    strncpy(buf,s,sizeof(buf)) ;
    p=strchr(buf,':') ;
    if(p) {
        *p='\0' ;
        parse_environment(p+1) ;
    }
      
    do_pipe_more(buf) ;
}

struct funcs {
    char *funcname ;
    int (*funcptr)() ;
} ;

int NotImpl(), EndMenu(), XyzMenu(), AdminMenu(), MailMenu(), TalkMenu();
int MainHelp(), XyzHelp(), AdminHelp(), MailHelp(), TalkHelp();
int ShowDate(), Welcome(), BoardInfo(), GnuInfo(), EditWelcome();
int MainReadHelp(), MailReadHelp();
int FileMenu(), FileHelp(), FileReadHelp();
int AllUsers(), OnlineUsers(), SetPasswd(), SetUsername(), SetAddress();
int ShortList(), Monitor();
int SetTermtype(), ShowOwnInfo(), AddAccount(), DeleteAccount();
int SetCharset();
int SetUserData(), SetUserPerms(), ToggleCloak(), ToggleExempt();
int Query(), QueryEdit();
int MailSend(), GroupSend(), ReadNewMail(), MailRead();
int MailDisplay(), MailDelete(), MailDelRange();
int MailReply(), GroupReply(), Forward();
int Visit(), BoardCounts(), Zap(), ReadNew(), SequentialRead();
int Boards(), SelectBoard(), AddBoard(), DeleteBoard(), ChangeBoard();
int SetBoardMgrs();
int Post(),  MainRead(), ReadMenuSelect();
int PostDisplay(), PostDelete(), PostMessage(), PostDelRange(), PostMark();
int PostEdit();
int FileBoards(), FileSelect(), FileDownload();
int FileUpload(), FileReceive(), FileReadMenuSelect();
int Chat(), Kick(), Talk(), SetPager(), SetOverrides();
int SignatureEdit(), MenuConfig();
#ifndef REMOTE_CLIENT
int SelectEditor(), SelectProtocol(), FileReadMenuProto(), FileView();
#endif

struct funcs funclist[] = {
    "exec", exec_func,
    "exec.pause", exec_func_w_pause,
    "exec.more", revised_pipe_more,
    "echo", do_echo,
    "NotImpl",NotImpl,
    "EndMenu",EndMenu,
    "Help",do_help,
    "Menu",NDoMenu,
    "ShowDate",ShowDate,
    "Welcome", Welcome,
    "BoardInfo",BoardInfo,
    "GnuInfo",GnuInfo,
    "EditWelcome",EditWelcome,
    "AllUsers",AllUsers,
    "OnlineUsers",OnlineUsers,
    "SetPasswd",SetPasswd,
    "SetUsername",SetUsername,
    "SetAddress",SetAddress,
    "ShortList",ShortList,
    "Monitor",Monitor,
    "SetTermtype",SetTermtype,
    "SetCharset",SetCharset,
    "ShowOwnInfo",ShowOwnInfo,
    "AddAccount",AddAccount,
    "DeleteAccount",DeleteAccount,
    "SetUserData",SetUserData,
    "SetUserPerms",SetUserPerms,
    "ToggleCloak",ToggleCloak,
    "ToggleExempt",ToggleExempt,
    "Query",Query,
    "QueryEdit",QueryEdit,
    "MailSend",MailSend,
    "GroupSend",GroupSend,
    "ReadNewMail",ReadNewMail,
    "MailRead",MailRead,
    "Visit",Visit,
    "BoardCounts",BoardCounts,
    "Zap",Zap,
    "ReadNew",ReadNew,
    "Boards",Boards,
    "SelectBoard",SelectBoard,
    "AddBoard",AddBoard,
    "DeleteBoard",DeleteBoard,
    "ChangeBoard",ChangeBoard,
    "SetBoardMgrs",SetBoardMgrs,
    "Post",Post,
    "MainRead",MainRead,
    "FileBoards",FileBoards,
    "FileSelect",FileSelect,
    "FileDownload",FileDownload,
#ifndef REMOTE_CLIENT
    "SelectProtocol",SelectProtocol,
#endif
    "FileUpload",FileUpload,
    "Chat",Chat,
    "Kick",Kick,
    "Talk",Talk,
    "SetPager",SetPager,
    "SetOverrides",SetOverrides,
#ifndef REMOTE_CLIENT
    "SelectEditor",SelectEditor,
#endif
    "SignatureEdit",SignatureEdit,
    "MenuConfig",MenuConfig,
    NULL,NULL
} ;
    
int (*findfunc(s))()
char *s ;
{
    int i ;
    
    for(i=0;funclist[i].funcname;i++)
      if(!strcmp(funclist[i].funcname,s))
        return funclist[i].funcptr ;
    return NULL ;
}

#ifndef REMOTE_CLIENT 

/* NOTE: This MUST agree with ACCESSFILE location in init.c */
#define ACCESSFILE "etc/access" 

char *funcstrings[MAX_CLNTCMDS];

form_function_list()
{
    FILE *fp;
    int i;
    char buf[1024], *equals;
    for (i=0; i<MAX_CLNTCMDS; i++) funcstrings[i] = NULL;
    i = 0;
    if ((fp = fopen(ACCESSFILE, "r")) != NULL) {
      while (i<MAX_CLNTCMDS && fgets(buf, sizeof buf, fp) != NULL) {
        if (*buf == '#' || isspace(*buf)) continue;
        if ((equals = strchr(buf, '=')) != NULL) {
          *equals = '\0';
          if ((funcstrings[i] = (char *)malloc(strlen(buf)+1)) != NULL)
	    strcpy(funcstrings[i], buf);
	}
	i++;
      }
      fclose(fp);
    }
}

free_function_list()
{
    int i;
    for (i=0; i<MAX_CLNTCMDS; i++)
      if (funcstrings[i] != NULL) free(funcstrings[i]);
}

convert_cmd_to_int(s) 
char *s;
{
    int i ;
    
    for (i=0; i<MAX_CLNTCMDS; i++) {
      if (funcstrings[i] == NULL) continue;
      if (!strcmp(funcstrings[i], s)) return i;
    }
    fprintf(stderr, "does not grok '%s'\n", s);
    sleep(2);
    return -1;
}

#else /* REMOTE_CLIENT */

convert_cmd_to_int(s) 
char *s;
{
    /* In the remote client, we're assuming numbers will be here */
    return 0;
}

#endif /* !REMOTE_CLIENT */

int line_num ;

ParseMenu()
{
    FILE *fp ;
    extern FILE *yyin ;
    
    if((fp = fopen(_menudesc_file,"r")) == NULL) {
        perror("open menu file") ;
        return -1 ;
    }
#ifndef REMOTE_CLIENT
    form_function_list();    
#endif
    yyin = fp ;
    line_num = 1 ;
    yyparse() ;
    fclose(fp) ;
#ifndef REMOTE_CLIENT
    free_function_list();
#endif
    return 0;
}

/* Now things for the read menus */

int MainReadHelp(), MailReadHelp(), FileReadHelp();
int MailDisplay(), MailDelete(), MailDelRange(), SequentialReadMail();
int MailReply(), GroupReply(), Forward(), SequentialRead();
int PostDisplay(), PostDelete(), PostMessage(), PostDelRange(), PostMark();
int FileView(), FileReadMenuProto(), ReadMenuSelect(), PostEdit(), PostMove();
int FileReceive(), FileReadMenuSelect(), FileForward(), FileChdir();

struct funcs rfunclist[] = {
    "MainReadHelp",MainReadHelp,
    "MailReadHelp",MailReadHelp,
    "FileReadHelp",FileReadHelp,
    "MailDisplay",MailDisplay,
    "MailDelete",MailDelete,
    "MailDelRange",MailDelRange,
    "MailReply",MailReply,
    "GroupReply",GroupReply,
    "Forward",Forward,
    "SequentialRead",SequentialRead,
    "SequentialReadMail",SequentialReadMail,
    "ReadMenuSelect",ReadMenuSelect,
    "PostDisplay",PostDisplay,
    "PostDelete",PostDelete,
    "PostMessage",PostMessage,
    "PostDelRange",PostDelRange,
    "PostMark",PostMark,
    "PostEdit",PostEdit,
    "PostMove",PostMove,
#ifndef REMOTE_CLIENT
    "FileView",FileView,
    "FileReadMenuProto",FileReadMenuProto,
#endif
    "FileReceive",FileReceive,
    "FileReadMenuSelect",FileReadMenuSelect,
    "FileForward",FileForward,
    "FileChdir",FileChdir,
    NULL, NULL
};

int (*findrfunc(s))()
char *s ;
{
    int i ;
    
    for(i=0;rfunclist[i].funcname;i++)
      if(!strcmp(rfunclist[i].funcname,s))
        return rfunclist[i].funcptr ;
    return NULL ;
}

convert_key_to_int(s)
char *s;
{
    s++;
    if (*s == '^') {
        /* CTRL key */
        return((int)(toupper(*(s+1)) - 'A') + 1);      
    }
    return ((int)*s);
}

convert_openflag_to_int(s)
char *s;
{
    if (*s == 'P' || *s == 'p') return OPEN_POST;
    else if (*s == 'M' || *s == 'm') return OPEN_MANAGE;
    else return 0;
}
    
yywrap()
{
    return 1 ;
}

yyerror()
{
    char buf[512] ;
    sprintf(buf,"syntax error in '%s' %d.\n",_menudesc_file,line_num) ;
    do_echo(buf) ;
}

/* Error list: hardcoded for now. TODO: put in menu.desc */
char *bb_errlist[S_MAXERROR+1] = {
"No error",
"System error",
"Permission denied",
"Cannot open bbs config file",
"BBS system is full -- try again later",
"Invalid userid or password",
"Already logged in",
"Login limit for this account has been reached",
"Login limit for this account has been reached",
"Account is not available at this time",
"No such userid",
"Userid already exists",
"Invalid userid",
"Invalid password",
"No such board",
"Board already exists",
"Invalid board name",
"No such message or file",
"User cannot receive mail",
"Board or filesystem is full",
"Invalid mail address",
"Board cannot be zapped",
"No board or mailbox is open",
"Another board or mailbox is already open",
"Wrong type of board for this operation",
"User's pager is turned off",
"User cannot answer a page at this time",
"Talk request not accepted",
"Talk request was cancelled",
"Talk request timed out",
"Attempted to page yourself",
"Cannot complete this operation in the current mode",
"Chatid is in use",
"Invalid chatid",
"Chat daemon error",
"No such editor",
"No such protocol",
"Protocol cannot be used for this operation",
"Uploading is not allowed",
"Invalid filename",
"Bad file path",
"Not connected to server",
"Corrupt packet received",
"Socket request timed out",
"Command is unavailable",
"Host name lookup failure"
};
