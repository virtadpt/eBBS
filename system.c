
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
#include <string.h>
#include <fcntl.h>
#include <time.h>

#ifdef REMOTE_CLIENT
extern char *editor;
extern char *pager;
extern char *shell;
#endif

ShowDate()
{
  time_t now;
  move(3,0);
  clrtobot();
  time(&now);
  prints("Date and time on this system: %s", ctime(&now));
  return PARTUPDATE;
}

/* 
   The builtin more should be improved a bit. I decided to leave it
   non-configurable because (a) no one has complained loudly, and (b)
   why bother with all that forking and execing when reading posts.
*/

More(filename, promptend)
char *filename;
int promptend; 
{
  return (more(filename, promptend));
}

#ifndef REMOTE_CLIENT
SelectEditor()
{
  NAME editorname;
  NAMELIST editorlist = NULL;
  move(2,0);
  bbs_enum_editors(&editorlist, NULL);
  if (editorlist == NULL) {
    prints("Builtin editor is the only one available.\n");
    pressreturn();
    return FULLUPDATE;
  }
  namecomplete(NULL, editorlist, "Select editor: ", editorname, sizeof(NAME));
  if (editorname[0] == '\0') return FULLUPDATE;
  else if (!is_in_namelist(editorlist, editorname)) {
    move(4,0);
    bbperror(S_NOSUCHEDITOR, NULL);
    pressreturn();
    return FULLUPDATE;
  }
  bbs_set_editor(editorname);  
  return FULLUPDATE;
}
#endif

Edit(filename)
char *filename;
{
  int rc;
#ifndef REMOTE_CLIENT
  ACCOUNT acct;
  PATH editbin;
  PATH editorenv;
  PATH execbuf;
  acct.editor[0] = '\0';
  bbs_owninfo(&acct);
  if (acct.editor[0] != '\0') {
    if ((rc = bbs_get_editor(acct.editor, editbin, editorenv)) == S_OK) {
      if (editorenv[0] != '\0') parse_environment(editorenv);
      sprintf(execbuf, "%s %s", editbin, filename);
      bbs_set_mode(1);    /* Magic value to block paging */
      rc = do_exec(execbuf, NULL);
      bbs_set_mode(0);    /* Magic value to allow paging */
      return rc;
    }
    else {
      clear();
      bbperror(rc, NULL);
      prints("Using builtin editor.\n");
      pressreturn();
    }
  }
  return (vedit(filename));
#else /* REMOTE_CLIENT */
  char buf[MAXCOMSZ];
  char ans[9];
  int done = 0;
  if (editor == NULL) {
    return(vedit(filename));
  }
  sprintf(buf, "%s %s", editor, filename);
  while (!done) {    
    rc = do_exec(buf, NULL);
    clear();
    if (rc == -1) {
      prints("Exec of '%s' failed. You might try a different EDITOR.\n");
      pressreturn();
      break;
    }
    getdata(0,0,"Continue, Abort, or Edit? [C]: ",ans,sizeof(ans),DOECHO,0);
    switch (ans[0]) {
    case 'E': case 'e':
      break;
    case 'A': case 'a':
      unlink(filename);
      rc = done = 1;
      break;
    default:
      rc = 0;
      done = 1;
    }
  }
  return rc; 
#endif
}
 
#ifdef REMOTE_CLIENT
ShellEscape()
{
  clear();
  do_exec(shell, NULL);
  return FULLUPDATE;
}
#endif

Welcome()
{
  PATH welcfile;
  int rc;
  clear();
  if ((rc = bbs_get_welcome(welcfile)) != S_OK) {
    bbperror(rc, "Welcome file is unavailable");
    pressreturn();
  }
  if (More(welcfile, 1)) {
    prints("Welcome file is unavailable.");
    pressreturn();
  }
  return FULLUPDATE;
}
    
EditWelcome()
{
  PATH welcfile;
  int rc;
  if ((rc = bbs_get_welcome(welcfile)) != S_OK) {
    bbperror(rc, "Cannot access welcome file");
    pressreturn();
    return FULLUPDATE;
  }
  switch (Edit(welcfile)) {
  case -1:
    break;
  case 1:
    move(1,0);
    prints("Welcome screen NOT updated.\n");
    pressreturn();
    break;        
  case 0:
    move(1,0);
    if ((rc = bbs_set_welcome(welcfile)) == S_OK)
      prints("Welcome screen has been updated.\n");
    else
      bbperror(rc, "Welcome screen update failed");

    pressreturn();
  }
  return FULLUPDATE;
}

BoardInfo()
{   
  PATH infofile;
  int rc;
  clear();
  if ((rc = bbs_get_info(infofile)) != S_OK) {
    bbperror(rc, "Server information page is not available");
    pressreturn();
  }
  if (More(infofile, 1)) {
    prints("Server information page is not available.\n");
    pressreturn();
  }
  return FULLUPDATE;
}

GnuInfo()
{   
  PATH gnufile;
  int rc;
  clear();
  if ((rc = bbs_get_license(gnufile)) != S_OK) {
    bbperror(rc, "GNU General Public License is not available");
    pressreturn();
  }
  if (More(gnufile, 1)) {
    prints("GNU General Public License is not available.\n");
    pressreturn();
  }
  return FULLUPDATE;
}

#define LINELEN 80

AnnotateMessageBody(dest, src)
char *dest, *src;
{
  char buf[LINELEN], overflow[LINELEN];
  FILE *ofp, *ifp;
  int ch, lastch = 0;
  int i, inbody = 0, newline = 1, overflowed = 0;
  char *space;

  if ((ofp = fopen(dest, "w")) == NULL) return -1;
  if ((ifp = fopen(src, "r")) == NULL) {
    fclose(ofp);
    return -1;
  }

  while ((ch = fgetc(ifp)) != EOF) {
    if (!inbody) {
      if (ch == '\n' && lastch == '\n') inbody = 1;
      else lastch = ch;
      continue;
    }
    if (newline) {
      fprintf(ofp, "> ");
      newline = 0;
      if (overflowed) {
        memcpy(buf, overflow, LINELEN);
        overflowed = 0;
      }              
      else memset(buf, 0, LINELEN);
      i = strlen(buf);
    }
    buf[i++] = ch;
    if (i > LINELEN-4 && ch != '\n') {
      if ((space = strrchr(buf, ' ')) != NULL) {
        *space++ = '\0';
        memset(overflow, 0, LINELEN);
        strcpy(overflow, space);
        overflowed = 1;        
      }
      fprintf(ofp, "%s\n", buf);
      newline = 1;
    }
    if (ch == '\n') {
      fprintf(ofp, "%s", buf);
      newline = 1;
    }
  }
  fclose(ifp);
  fclose(ofp);
}

filecopy(dest, src)
char *dest, *src;
{
  int ifd, ofd, rc = 0, cc;
  char buf[1024];
  if ((ofd = open(dest, O_WRONLY | O_CREAT, 0600)) == -1)
    return -1;
  if ((ifd = open(src, O_RDONLY)) == -1)
    {
      close(ofd);
      return -2;
    }
  while (cc = read(ifd, buf, sizeof(buf)))
    if (write(ofd, buf, cc) != cc) {
      rc = -3;
      break;
    }
  close(ofd);
  close(ifd);
  return rc;
}

#ifdef REMOTE_CLIENT
SaveFile(fname)
char *fname;
{
  char savename[PATHLEN];
  clear();
  getdata(0, 0, "Save to file: ", savename, sizeof(savename), DOECHO, 0);
  if (savename[0] == '\0')
    {
      prints("File not saved.\n");
    }
  else
    switch(filecopy(savename, fname))
      {
      case -3:
	prints("Error writing to file '%s'.\n", savename);
	break;
      case -2:
	prints("Cannot open input file '%s'.\n", fname);
	break;
      case -1:
	prints("Error creating output file '%s'.\n", savename);
	break;
      case 0:
	prints("File saved to '%s'.\n", savename);
      }
}
#endif


