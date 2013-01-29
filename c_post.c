
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
#include <stdlib.h>
#include <sys/stat.h>

int
OpenBoard(openflags, newonly, resp)
int *openflags;
int newonly;
int *resp;
{
  int code;
  OPENINFO openinfo;
  code = bbs_open_board(currboard, &openinfo);    
  if (resp) *resp = code;
  if (code != S_OK) return -1;
  if (openflags) *openflags = openinfo.flags;
  return (newonly ? openinfo.newmsgs : openinfo.totalmsgs);
}

CloseBoard()
{
  int code;
  code = bbs_close_board();
  return (code == S_OK ? 0 : -1);
}

/* 
   Return codes for DoPostSend:
   0 == success
   -2 == post was manually aborted
   -3 == post is zero length -- aborted
   other == error code from bbs_post
*/

DoPostSend(subject, textfile, doedit)
char *subject;    /* subject string */
char *textfile;   /* file to send: if NULL invoke editor */
int doedit;       /* edit even if textfile != NULL */
{
  int rc = 0;
  struct stat stbuf;
  
  if (textfile == NULL || doedit) {
    if (textfile == NULL) textfile = c_tempfile;
    if (Edit(textfile)) { 
      if (textfile == c_tempfile) unlink(c_tempfile);
      return -2;
    }
  }
  if (stat(textfile, &stbuf) != 0 || stbuf.st_size == 0) {
    rc = -3;
  }
  else rc = bbs_post(currboard, subject, textfile);

  if (textfile == c_tempfile) unlink(c_tempfile);
  return rc;
}            

GenericPost(docheck)
int docheck;
{
  int rc;
  LONG retcode = FULLUPDATE;
  TITLE subject;
  clear();
  if (docheck) {
    SHORT flags;
    if (currboard[0] == '\0') {
      prints("Use (S)elect to select a board first.\n");
      pressreturn();
      return FULLUPDATE;
    }
    if ((rc = bbs_test_board(currboard, &flags)) != S_OK) {
      bbperror(rc, "Can't access board");
      pressreturn();
      return FULLUPDATE;
    }
    if (!BITISSET(flags, OPEN_POST)) {
      prints("Posting on the '%s' board is restricted.\n", currboard);
      pressreturn();
      return FULLUPDATE;
    }
  }
  bbs_set_mode(M_POSTING);
  prints("Posting message on board '%s'\n", currboard);
  getdata(1, 0, "Subject: ", subject, sizeof(subject), DOECHO, 0);
  rc = DoPostSend(subject, (char *)NULL, 0);
  clear();
  move(0,0);
  switch (rc) {
  case -3:
    prints("Zero length file NOT sent.\n");
    break;
  case -2:        
    prints("File NOT posted.\n");
    break;
  case 0:
    prints("File posted!\n");
    retcode |= FETCHNEW;
    break;
  default:
    bbperror(rc, "Error saving post");
    break;
  }
  pressreturn();
  /* If docheck is true,  we came in from the main menu -- else, read menu */
  bbs_set_mode(docheck ? M_UNDEFINED : M_READING);
  return retcode;
}

Post()
{
  return (GenericPost(1));
}

/*ARGSUSED*/
PostMessage(hptr, currmsg, numrecs, openflags)
HEADER *hptr;
int currmsg, numrecs, openflags;
{
  if (!BITISSET(openflags, OPEN_POST)) {
    bell();
    return DONOTHING;
  }
  return (GenericPost(0));   /* no need to check post privilege -- we do */
}

/*ARGSUSED*/
ReadMenuSelect(hptr, currmsg, numrecs, openflags)
HEADER *hptr;
int currmsg, numrecs, openflags;
{
  return (SelectBoard());
}

GenericPostReply(hptr, msgsrc)
HEADER *hptr;
char *msgsrc;
{
  TITLE subject;
  char ans[4];
  int rc, retcode = FULLUPDATE;
  PATH msgfile;

  clear();

  if (msgsrc == NULL) {
    if ((rc = bbs_read_message(hptr->fileid, msgfile)) != S_OK) {
      bbperror(rc, "Error reading post");
      return retcode;
    }
  }
  else strcpy(msgfile, msgsrc);

  move(1,0);
  bbs_set_mode(M_POSTING);
  prints("[RETURN to Re: current title]\n");
  getdata(0, 0, "Title: ", subject, sizeof(subject), DOECHO, 0);
  if (*subject == '\0') {
    if (strncasecmp(hptr->title, "Re:", 3)) {
      strcpy(subject, "Re: ");
      strncat(subject, hptr->title, TITLELEN-5);
    }
    else strncpy(subject, hptr->title, TITLELEN-1);
    move(0,0);
    prints("Title: %s\n", subject);
  }    

  getdata(1, 0, "Include message text in followup (Y/N)? [N]: ",
	  ans, sizeof(ans), DOECHO, 0);
  if (*ans == 'Y' || *ans == 'y') {
    AnnotateMessageBody(c_tempfile, msgfile);
  }

  rc = DoPostSend(subject, c_tempfile, 1);
  clear();
  move(0,0);
  switch (rc) {
  case -3:
    prints("Zero length file NOT sent.\n");
    break;
  case -2:        
    prints("File NOT posted.\n");
    break;
  case 0:
    prints("File posted!\n");
    retcode |= FETCHNEW;
    break;
  default:
    bbperror(rc, "Post failed");
    break;
  }
  pressreturn();
  bbs_set_mode(M_READING);
  return retcode;
}    

/*ARGSUSED*/
PostReply(hptr, currmsg, numrecs, openflags)
HEADER *hptr;
LONG currmsg, numrecs, openflags;
{
  return (GenericPostReply(hptr, NULL));
}

/*ARGSUSED*/
PostDelete(hptr, currmsg, numrecs, openflags)
HEADER *hptr;
int currmsg, numrecs, openflags;
{
  int code, rc = FULLUPDATE;
  char ans[4];
  if (strcmp(myinfo.userid,hptr->owner) && !BITISSET(openflags,OPEN_MANAGE)) {
    bell();
    return DONOTHING;
  }
  clear();
  prints("Delete message '%s'\n", hptr->title);
  getdata(2,0,"Are you sure (Y/N)? [N]: ",ans,sizeof(ans),DOECHO, 0);
  move(4,0);
  if (*ans != 'Y' && *ans != 'y') {
    prints("Message not deleted.\n");
  }        
  else {
    if ((code = bbs_delete_message(hptr->fileid)) == S_OK) {
      prints("Message deleted.\n");
      rc |= FETCHNEW;
    }
    else {
      bbperror(code, "Deletion failed");
    }
  }
  pressreturn();
  return rc;
}

/*ARGSUSED*/
PostDelRange(hptr, currmsg, numinbox, openflags)
HEADER *hptr;
int currmsg, numinbox, openflags;
{
  int code, rc = FULLUPDATE;
  char ans[5];
  SHORT n1 = 0, n2 = 0, deleted;
  if (!BITISSET(openflags, OPEN_MANAGE)) {
    bell();
    return DONOTHING;
  }
  clear();
  if (numinbox <= 0) {
    prints("No messages to delete!\n");
    pressreturn();
    return rc;
  }
  prints("Delete Range\n");
  prints("[NOTE: Marked messages will NOT be deleted]\n");
  do {
    if (getdata(2,0,"Enter number of first message to delete: ",
	    ans,sizeof(ans),DOECHO,1) == -1) return rc;
    n1 = atoi(ans);
    if (n1 <= 0 || n1 > numinbox) {
      bell();
      move(3,0);
      prints("Not valid! Must be in range 1-%d.\n", numinbox);
    }        
  } while (n1 <= 0 || n1 > numinbox);
  do {
    if (getdata(3,0,"Enter number of last message to delete: ",
	    ans,sizeof(ans),DOECHO,1) == -1) return rc;
    n2 = atoi(ans);
    if (n2 < n1 || n2 > numinbox) {
      bell();
      move(4,0);
      prints("Not valid! Must be in range %d-%d.\n", n1, numinbox);
    }        
  } while (n2 < n1 || n2 > numinbox);
  
  move(4,0);
  clrtobot();
  getdata(4,0,"Are you sure (Y/N)? [N]: ",ans,sizeof(ans),DOECHO,0);
  move(6,0);
  if (*ans != 'Y' && *ans != 'y') {
    prints("No messages deleted.\n");
  }        
  else {
    if ((code = bbs_delete_range(n1, n2, &deleted)) == S_OK) {
      prints("%d unmarked messages deleted.\n", deleted);
      rc |= FETCHNEW;
    }
    else bbperror(code, "Delete range failed");
  }
  pressreturn();
  return rc;
}

/*ARGSUSED*/
PostMark(hptr, currmsg, numrecs, openflags)
HEADER *hptr;
int currmsg, numrecs, openflags;
{
  SHORT ismarked = BITISSET(hptr->flags, FILE_MARKED);
  if (bbs_mark_message(hptr->fileid, !ismarked) != S_OK) return DONOTHING;
  if (ismarked) BITCLR(hptr->flags, FILE_MARKED);
  else BITSET(hptr->flags, FILE_MARKED);
  return PARTUPDATE;
}            

/*ARGSUSED*/
PostMove(hptr, currmsg, numrecs, openflags)
HEADER *hptr;
int currmsg, numrecs, openflags;
{
  int code, rc = FULLUPDATE;
  NAME bname;
  extern NAMELIST boardlist;
  char ans[4];
  if (strcmp(myinfo.userid,hptr->owner) && !BITISSET(openflags,OPEN_MANAGE)) {
    bell();
    return DONOTHING;
  }
  clear();
  prints("Moving post '%s' to another board\n", hptr->title);
  getdata(2,0,"Are you sure (Y/N)? [N]: ",ans,sizeof(ans),DOECHO, 0);
  move(4,0);
  if (*ans != 'Y' && *ans != 'y') {
    prints("Message not moved.\n");
  }        
  else {
    bbs_boardnames(&boardlist, NULL);
    if (boardlist == NULL) {
      prints("No boards to select.\n");
      pressreturn();
      return FULLUPDATE;
    }
    move(2,0);
    namecomplete(NULL, boardlist, "Move to board: ", bname, sizeof(NAME));
    if (bname[0] == '\0') return FULLUPDATE;
    else if (!is_in_namelist(boardlist, bname)) {
      bbperror(S_NOSUCHBOARD, NULL);
    }
    else if ((code = bbs_move_message(hptr->fileid, bname)) == S_OK) {
      prints("Message moved to '%s'.\n", bname);
      rc |= FETCHNEW;
    }
    else {
      bbperror(code, "Move failed");
    }
  }
  pressreturn();
  return rc;
}

/*ARGSUSED*/
PostEdit(hptr, currmsg, numrecs, openflags)
HEADER *hptr;
int currmsg, numrecs, openflags;
{
  struct stat stbuf;
  PATH msgfile;
  int rc;
  if ((rc = bbs_read_message(hptr->fileid, msgfile)) != S_OK) {
    bbperror(rc, "Error reading post");
    pressreturn();
    return FULLUPDATE;
  }
  if (Edit(msgfile)) { 
    prints("File NOT edited.\n");
    return FULLUPDATE;
  }
  if (stat(msgfile, &stbuf) != 0 || stbuf.st_size == 0) {
    prints("New file is zero length -- not updated.\n");
    return FULLUPDATE;
  }
  if ((rc = bbs_update_message(hptr->fileid, msgfile)) == S_OK) {
    prints("File was edited!\n");
  }
  else bbperror(rc, "File edit failed");
  return FULLUPDATE;
}            

/*ARGSUSED*/
PostDisplay(hptr, currmsg, numrecs, openflags)
HEADER *hptr;
int currmsg, numrecs, openflags;
{
  char ans[9], promptstr[80];
  PATH msgfile;
  int code, rc = FULLUPDATE;
  int done = 0, replied = 0, marked = 0, can_delete = 0;
#ifndef NO_POST_FOLLOWUP
  int posted = 0;
#endif
#ifdef REMOTE_CLIENT
  int saved = 0;
#endif
  if ((code = bbs_read_message(hptr->fileid, msgfile)) != S_OK) {
    bbperror(code, "Error reading file");
    pressreturn();
    return rc;
  }
  BITCLR(hptr->flags, FILE_UNREAD);

  if (!HasPerm(C_MAIL)) replied = 1;
#ifndef NO_POST_FOLLOWUP
  if (!BITISSET(openflags, OPEN_POST)) posted = 1;
#endif
  if (!BITISSET(openflags, OPEN_MANAGE)) marked = 1;
  if (BITISSET(openflags, OPEN_MANAGE) || !strcmp(hptr->owner, myinfo.userid))
    can_delete = 1;

  clear();
  More(msgfile, 0);
  while (!done) {
    promptstr[0] = '\0';
    if (!replied) strcat(promptstr, "Mail reply, ");
#ifndef NO_POST_FOLLOWUP
    if (!posted) strcat(promptstr, "Post followup, ");
#endif
#ifdef REMOTE_CLIENT
    if (!saved) strcat(promptstr, "Save, ");
#endif
    if (!marked) strcat(promptstr, 
       BITISSET(hptr->flags, FILE_MARKED) ? "unmarK, " : "marK, ");
    if (can_delete) strcat(promptstr, "Delete, ");

    if (promptstr[0] == '\0') {
      pressreturn();
      break;
    }
    else strcat(promptstr, "or Continue? [C]: ");
    getdata(t_lines-1, 0, promptstr, ans, sizeof(ans), DOECHO, 0);
    switch (*ans) {
    case 'm': case 'M': 
      if (replied) done = 1;
      else {
	rc |= MailReply(hptr, currmsg, numrecs, openflags);
	replied = 1;
      }
      break;
#ifndef NO_POST_FOLLOWUP
    case 'p': case 'P':
      if (posted) done = 1;
      else {
	rc |= GenericPostReply(hptr, msgfile);
	posted = 1;
      }
      break;
#endif
#ifdef REMOTE_CLIENT
    case 's': case 'S':
      if (saved) done = 1;
      else {
	SaveFile(msgfile);
	saved = 1;
      }
      break;
#endif
    case 'k': case 'K': 
      if (marked) done = 1;
      else {
	PostMark(hptr, currmsg, numrecs, openflags);
	marked = 1;
      }
      break;
    case 'd': case 'D': 
      if (can_delete) rc |= PostDelete(hptr, currmsg, numrecs, openflags);
      /* fall through */
    default:
      done = 1;
      break;
    }
  }
  return rc;
}
