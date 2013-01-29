
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

OpenMailbox(openflags, newonly, resp)
int *openflags;
int newonly;
int *resp;
{
  int code;
  OPENINFO openinfo;
  code = bbs_open_mailbox(&openinfo);    
  if (resp) *resp = code;
  if (code != S_OK) return -1;
  if (openflags) *openflags = (int)openinfo.flags;
  return (newonly ? (int)openinfo.newmsgs : (int)openinfo.totalmsgs);
}

CloseMailbox()
{
  int code;
  code = bbs_close_board();
  return (code == S_OK ? 0 : -1);
}

/* 
  If mail to ANY recipients fails, return error with retcode holding the mask.
  Abort mail, return -2. 
  Zero length file, return -3.
  Otherwise return 0.
*/

DoMailSend(recips, subject, textfile, doedit, retcode)
NAMELIST recips;  /* list of intended recipients */
char *subject;    /* subject string */
char *textfile;   /* file to send: if NULL invoke editor */
int doedit;       /* edit even if textfile != NULL */
LONG *retcode;    /* bit mask of failed destinations */
{
  int rc = 0;
  struct stat stbuf;
  
  if (textfile == NULL || doedit) {
    if (textfile == NULL) textfile = c_tempfile;
    if (Edit(textfile)) { 
      rc = -2;
    }
  }
  if (rc == 0) {
    if (stat(textfile, &stbuf) != 0 || stbuf.st_size == 0) {
      rc = -3;
    }
    else rc = bbs_mail(NULL, NULL, recips, subject, textfile, retcode);
  }
  if (textfile == c_tempfile) unlink(c_tempfile);
  return rc;
}            

show_names(indx, userid, mask)
int indx;
char *userid;
LONG *mask;
{
  int x, y;
  if ((*mask) & (LONG)(1<<indx)) {
    getyx(&y, &x);
    if (strlen(userid)+3 > (t_columns-x)) prints("\n");
    else if (x > 0) prints(", ");
    prints("%s", userid);
  }
  return S_OK;
}

GenericMailSend(group)
int group;
{
  int rc;
  ADDR nbuf;
  NAMELIST recips = NULL;
  TITLE subject;
  int max, count = 0;
  char *prompt;
  LONG failmask = ~0;
  ACCOUNT acct;

  bbs_set_mode(M_MAIL);
  if (group) {
    max = BBS_MAX_MAILRECIPS;
    prompt = "Enter recipient (RETURN when done): ";
  }
  else {
    max = 1;
    prompt = "Enter recipient: ";
  }

  move(2,0);
  clrtobot();
  for (count = 0; count < max; count++) {
    move(2,0);
    namecomplete(bbs_acctnames, NULL, prompt, nbuf, sizeof nbuf);
    if (nbuf[0] == '\0') break;
    if (strchr(nbuf, ':') != NULL) {
      if (!HasPerm(C_EXTERNMAIL)) {
        move(3,0);
	bbperror(S_NOSUCHUSER, NULL);
	continue;
      }
    }
    else {
      if ((rc = bbs_query(nbuf, &acct)) != S_OK) {
        move(3,0);
        bbperror(rc, NULL);
	continue;
      }
      strcpy(nbuf, acct.userid);      
    }
    if (!is_in_namelist(recips, nbuf))
      add_namelist(&recips, nbuf, NULL);
    if (group) {
      move(3,0);
      prints("\nSending to:\n");
      apply_namelist(recips, show_names, &failmask);
    }
  }

  if (recips != NULL) {
    getdata(3,0,"Subject: ", subject, sizeof subject, DOECHO, 0);
    rc = DoMailSend(recips, subject, (char *)NULL, 0, &failmask);
    clear();
    move(0,0);
    switch (rc) {
    case -3:
      prints("Zero length file NOT sent.\n");
      break;
    case -2:
      prints("Mail NOT sent.\n");
      break;
    case 0:
      prints("Mail sent!\n");
      break;
    default:
      if (group) {
        bbperror(rc, "Mail failed to the following");
        apply_namelist(recips, show_names, &failmask);
      }
      else bbperror(rc, "Mail failed");
      break;
    }
  }
  pressreturn();
  bbs_set_mode(M_UNDEFINED);
  return FULLUPDATE;
}

GroupSend()
{
  return (GenericMailSend(1));
}

MailSend()
{
  return (GenericMailSend(0));
}

/*ARGSUSED*/
MailDelete(hptr, currmsg, numrecs, openflags)
HEADER *hptr;
int currmsg, numrecs, openflags;
{
  int code, rc = FULLUPDATE;
  char ans[4];
  clear();
  prints("Delete message from %s\n", hptr->owner);
  prints("entitled '%s'\n", hptr->title);
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
MailDelRange(hptr, currmsg, numinbox, openflags)
HEADER *hptr;
int currmsg, numinbox, openflags;
{
  int code, rc = FULLUPDATE;
  char ans[5];
  SHORT n1 = 0, n2 = 0, deleted;
  clear();
  if (numinbox <= 0) {
    prints("No messages to delete!\n");
    pressreturn();
    return rc;
  }
  prints("Delete Range\n");
  prints("[NOTE: Unread messages will NOT be deleted]\n");
  do {
    if (getdata(2,0,"Enter number of first message to delete: ",
	    ans,sizeof(ans), DOECHO, 1) == -1) return rc;
    n1 = atoi(ans);
    if (n1 <= 0 || n1 > numinbox) {
      bell();
      move(3,0);
      prints("Not valid! Must be in range 1-%d.\n", numinbox);
    }        
  } while (n1 <= 0 || n1 > numinbox);
  do {
    if (getdata(3,0,"Enter number of last message to delete: ",
	    ans,sizeof(ans), DOECHO, 1) == -1) return rc;
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
      prints("%d messages deleted.\n", deleted);
      rc |= FETCHNEW;
    }
    else bbperror(code, "Delete range failed\n");
  }
  pressreturn();
  return rc;
}

/*ARGSUSED*/
Forward(hptr, currmsg, numrecs, openflags)
HEADER *hptr;
int currmsg, numrecs, openflags;
{
  int code, rc = FULLUPDATE;
  ACCOUNT acct;
  char ans[4];
  clear();
  if ((code = bbs_owninfo(&acct)) != S_OK) {
    bbperror(code, "Can't fetch address");
    pressreturn();
    return rc;
  }
  prints("Mailing '%s' to:\n", hptr->title);
  prints("%s\n", acct.email);
  getdata(2,0,"Are you sure (Y/N)? [Y]: ",ans,sizeof(ans),DOECHO,0);
  move(4,0);
  if (*ans == 'N' || *ans == 'n') {
    prints("Message not forwarded.\n");
  }        
  else {
    if ((code = bbs_forward_message(hptr->fileid)) == S_OK) {
      prints("Mail sent!\n");
    }
    else bbperror(code, "Forward failed");
  }
  pressreturn();
  return rc;
}

GenericMailReply(hptr, group, msgsrc)
HEADER *hptr;
int group;
char *msgsrc;
{
  NAMELIST recips = NULL;
  TITLE subject;
  char ans[4];
  int rc, retcode = FULLUPDATE;
  LONG failmask;
  PATH msgfile;  

  clear();

  if (msgsrc == NULL) {
    if ((rc = bbs_read_message(hptr->fileid, msgfile)) != S_OK) {
      bbperror(rc, "Can't read message");      
      return retcode;
    }
  }
  else strcpy(msgfile, msgsrc);

  add_namelist(&recips, hptr->owner, NULL);
  if (group) parse_to_list(&recips, msgfile, myinfo.userid);

  if (strncasecmp(hptr->title, "Re:", 3)) {
    strcpy(subject, "Re: ");
    strncat(subject, hptr->title, TITLELEN-5);
  }
  else strncpy(subject, hptr->title, TITLELEN-1);
  
  getdata(0,0,"Include message text in reply (Y/N)? [N]: ",
	  ans, sizeof(ans), DOECHO, 0);
  if (*ans == 'Y' || *ans == 'y') {
    AnnotateMessageBody(c_tempfile, msgfile);
  }
  
  rc = DoMailSend(recips, subject, c_tempfile, 1, &failmask);
  clear();
  move(0,0);
  switch (rc) {
  case -3:
    prints("Zero length reply not sent.\n");
    break;
  case -2:        
    prints("Reply NOT sent.\n");
    break;
  case 0:
    prints("Reply sent!\n");
    if (is_in_namelist(recips, myinfo.userid)) BITSET(retcode, FETCHNEW);
    break;
  default:
    if (group) {
      bbperror(rc, "Reply not delivered to following");
      apply_namelist(recips, show_names, &failmask);
    }
    else bbperror(rc, "Reply could not be delivered.\n");
    break;
  }
  unlink(c_tempfile);
  pressreturn();
  return retcode;
}

/*ARGSUSED*/
MailReply(hptr, currmsg, numrecs, openflags)
HEADER *hptr;
int currmsg, numrecs, openflags;
{
  return (GenericMailReply(hptr, 0, NULL));
}    

/*ARGSUSED*/
GroupReply(hptr, currmsg, numrecs, openflags)
HEADER *hptr;
int currmsg, numrecs, openflags;
{
  return (GenericMailReply(hptr, 1, NULL));
}    

/*ARGSUSED*/
MailDisplay(hptr, currmsg, numrecs, openflags)
HEADER *hptr;
int currmsg, numrecs, openflags;
{
  char ans[9], promptstr[80];
  PATH msgfile;
  int code, rc = FULLUPDATE;
  int done = 0, replied = 0;
#ifdef REMOTE_CLIENT
  int saved = 0;
#else
  int forwarded = 0;
#endif
  if ((code = bbs_read_message(hptr->fileid, msgfile)) != S_OK) {
    bbperror(code, "Error retrieving file");
    pressreturn();
    return rc;
  }
  BITCLR(hptr->flags, FILE_UNREAD);
  clear();
  More(msgfile, 0);
  while (!done) {
    promptstr[0] = '\0';
    if (!replied) strcat(promptstr, "Reply, Group reply, ");
#ifdef REMOTE_CLIENT
    if (!saved) strcat(promptstr, "Save, ");
#else
    if (!forwarded) strcat(promptstr, "Forward, ");
#endif
    strcat(promptstr, "Delete, or Continue? [C]: ");
    getdata(t_lines-1, 0, promptstr, ans, sizeof ans, DOECHO, 0);
    switch (*ans) {
    case 'r': case 'R': 
    case 'g': case 'G':
      if (replied) done = 1;
      else {
	rc |= GenericMailReply(hptr, toupper(*ans)=='G'?1:0, msgfile);
	replied = 1;
      }
      break;
#ifdef REMOTE_CLIENT
    case 's': case 'S':
      if (saved) done = 1;
      else {
	rc |= SaveFile(msgfile);
	saved = 1;
      }
      break;
#else
    case 'f': case 'F':
      if (forwarded) done = 1;
      else {
        rc |= Forward(hptr, currmsg, numrecs, openflags);
        forwarded = 1;
      }
      break;
#endif
    case 'd': case 'D': 
      rc |= MailDelete(hptr, currmsg, numrecs, openflags);
      /* fall through */
    default:
      done = 1;
      break;
    }
  }
  return rc;
}

