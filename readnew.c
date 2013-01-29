
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

/* Flags for Read New */
#define NEW_READ	001
#define NEW_PASS	002
#define NEW_SKIP	004
#define NEW_VISIT       010
#define NEW_QUIT	020

/* Are we reading mail or posts? */
#define FILE_TYPE_MAIL	1
#define FILE_TYPE_POST	2

extern int OpenMailbox(), CloseMailbox(), MailDisplay();
extern int CloseMailbox(), CloseBoard(), PostDisplay();
extern char *Ctime();

struct readnewstruct {
  int nummsgs;
  int numread;
  int openflags;
  int dispflags;
  int excode;
  char *thread;
};

ReadNewMessage(type, hptr, numleft, options, openflags)
int type;
HEADER *hptr;
int numleft;
int options;
int openflags;
{
  char ans[10], promptstr[80];
  clear();
  if (numleft > 0) {
    if (type == FILE_TYPE_MAIL)
      prints("[%d new mail message%s left]\n", numleft, numleft==1?"":"s");
    else 
      prints("[%d new message%s left on board '%s']\n", 
	     numleft, numleft==1?"":"s", currboard);
  }
  prints("%s %s on %s (%d bytes)\n", 
	 (type == FILE_TYPE_MAIL ? "Mail from" : "Posted by"),
	 hptr->owner, Ctime((time_t *)&hptr->mtime), hptr->size);
  prints("Subject: %s", hptr->title);
  
  strcpy(promptstr, "Read, Pass, ");
  if (BITISSET(options, NEW_SKIP)) strcat(promptstr, "SkipBoard, ");
  if (BITISSET(options, NEW_VISIT)) strcat(promptstr, "Visit, ");
  strcat(promptstr, "or Quit? [R]: ");
  getdata(3, 0, promptstr, ans, sizeof(ans), DOECHO, 0);
  
  if (*ans == 'Q' || *ans == 'q')
    return NEW_QUIT;
  if (*ans == 'P' || *ans == 'p')
    return NEW_PASS;
  if (BITISSET(options, NEW_SKIP) && (*ans == 'S' || *ans == 's'))
    return NEW_SKIP;
  if (BITISSET(options, NEW_VISIT) && (*ans == 'V' || *ans == 'v'))
    return NEW_VISIT;
  
  if (type == FILE_TYPE_MAIL) MailDisplay(hptr, 0, 0, openflags);
  else if (type == FILE_TYPE_POST) PostDisplay(hptr, 0, 0, openflags);
  return NEW_READ;
}

NewMailReadfn(indx, hdr, info)
int indx;
HEADER *hdr;
struct readnewstruct *info;
{
  info->excode = ReadNewMessage(FILE_TYPE_MAIL, hdr, info->nummsgs,
                                0, info->openflags);
  if (info->nummsgs > 0) info->nummsgs--;
  if (info->excode == NEW_READ) (info->numread)++;
  if (info->excode == NEW_QUIT) return ENUM_QUIT;
  return S_OK;
}  

/*ARGSUSED*/
SequentialReadMail(hptr, currmsg, numrecs, openflags)
HEADER *hptr;
int currmsg, numrecs, openflags;
{
  struct readnewstruct rns;
  rns.nummsgs = -1;
  rns.numread = 0;
  rns.openflags = openflags;
  rns.dispflags = 0;
  rns.thread = NULL;
  clear();
  if (currmsg) currmsg--;
  bbs_enum_headers(10, currmsg, 0, NewMailReadfn, &rns);
  return (rns.numread ? (FULLUPDATE | FETCHNEW) : FULLUPDATE);
}

ReadNewMail()
{
  struct readnewstruct rns;
  int servresp;
  clear();
  bbs_set_mode(M_MAIL);
  rns.nummsgs = OpenMailbox(&rns.openflags, 1, &servresp);
  if (servresp != S_OK) {
    bbperror(servresp, "Error opening mailbox");
    pressreturn();
  }
  else if (rns.nummsgs == 0) {
    prints("No new mail messages.\n");
    pressreturn();
    CloseMailbox();
  }
  else {
    bbs_enum_headers(10, 0, 1, NewMailReadfn, &rns);
    CloseMailbox();
  }
  bbs_set_mode(M_UNDEFINED);
  return FULLUPDATE;
}

IsSameThread(title1, title2)
char *title1;
char *title2;
{
  /* Function to see if two posts are in the same thread */
  if (!strncasecmp(title1, "Re: ", 4)) title1 += 4;
  if (!strncasecmp(title2, "Re: ", 4)) title2 += 4;
  return (!strcasecmp(title1, title2));
}    

NewPostReadfn(indx, hdr, info)
int indx;
HEADER *hdr;
struct readnewstruct *info;
{
  if (info->thread != NULL && !IsSameThread(info->thread, hdr->title)) {
    info->excode = NEW_PASS;      
  }
  else {
    info->excode = ReadNewMessage(FILE_TYPE_POST, hdr, info->nummsgs,
                                info->dispflags, info->openflags);
    if (info->nummsgs > 0) info->nummsgs--;
  }

  if (info->excode == NEW_READ) (info->numread)++;
  if (info->excode == NEW_QUIT || info->excode == NEW_VISIT ||
      info->excode == NEW_SKIP) return ENUM_QUIT;

  return S_OK;
}  

/*ARGSUSED*/
SequentialRead(hptr, currmsg, numrecs, openflags)
HEADER *hptr;
int currmsg, numrecs, openflags;
{
  char ans[7];
  TITLE threadtitle;
  struct readnewstruct rns;
  int newonly = 1;
  rns.nummsgs = -1;
  rns.numread = 0;
  rns.openflags = openflags;
  rns.dispflags = NEW_VISIT;
  rns.thread = NULL;
  clear();
  if (getdata(0, 0, 
      "(N)ew messages, (A)ll messages, or (F)ollow current thread? [N]: ",
      ans, sizeof ans, DOECHO, 1) == -1) return FULLUPDATE;
    
  if (*ans == 'F' || *ans == 'f') {
    strncpy(threadtitle, hptr->title, sizeof threadtitle);
    rns.thread = threadtitle;
    newonly = 0;
  }
  else if (*ans == 'A' || *ans == 'a') newonly = 0;

  if (currmsg) currmsg--;
  bbs_enum_headers(10, currmsg, newonly, NewPostReadfn, &rns);
  if (rns.excode == NEW_VISIT) {
    rns.numread++;  /* to force FETCHNEW */
    bbs_visit_board(currboard);
  }
  return (rns.numread ? (FULLUPDATE | FETCHNEW) : FULLUPDATE);
}

/*ARGSUSED*/
ReadNewPosts(indx, board, disp)
int indx;
BOARD *board;
int *disp;
{
  int openflags, newmsgs;
  struct readnewstruct rns;
  char msgbuf[80], ans[4];

  clear();
  sprintf(msgbuf, "Scanning %s ...\n", board->name);
  move(t_lines/2, t_columns/2-10);
  prints(msgbuf);
  refresh();
  strncpy(currboard, board->name, sizeof currboard);
  newmsgs = OpenBoard(&openflags, 1, NULL);
  if (newmsgs > 0) {
    rns.nummsgs = newmsgs;
    rns.numread = 0;
    rns.openflags = openflags;
    rns.dispflags = NEW_SKIP | NEW_VISIT;
    rns.thread = NULL;
    bbs_enum_headers(10, 0, 1, NewPostReadfn, &rns);
     *disp = rns.excode;
    if (*disp == NEW_VISIT) {
      bbs_visit_board(currboard);
    }
    if ((rns.excode != NEW_VISIT && rns.excode != NEW_SKIP && 
        rns.excode != NEW_QUIT) && (openflags & OPEN_POST)) {
      sprintf(msgbuf, "End of board '%s'. Post now (Y/N)? [N]: ", board->name);
      getdata(t_lines-1, 0, msgbuf, ans, sizeof ans, DOECHO, 0);
      if (*ans == 'Y' || *ans == 'y') {
	GenericPost(0);
      }
    }
  }   
  CloseBoard();
  return (*disp == NEW_QUIT ? ENUM_QUIT : S_OK);
}

ReadNew()
{
  NAME savecurr;
  int disp = NEW_READ;
  
  bbs_set_mode(M_READING);  

  /* Save the currboard, 'cuz we're gonna meddle with it */
  strcpy(savecurr, currboard);
  
  /* Now read the unzapped boards in sequence */
  bbs_enum_boards(10, 0, BE_UNZAPPED, ReadNewPosts, &disp);

  clear();
  if (disp == NEW_QUIT) prints("Quitting Read New.\n");
  else prints("End of new messages.\n");

  /* restore currbrd */
  strcpy(currboard, savecurr);
  bbs_set_mode(M_UNDEFINED);
  pressreturn();
  return FULLUPDATE;
}	            
