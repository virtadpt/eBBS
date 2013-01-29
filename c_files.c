
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
#include <unistd.h>
#include <time.h>
#include <ctype.h>
#if LACKS_MALLOC_H
#else
# if !defined(NeXT)
#  include <malloc.h>
# endif
#endif

/* The currently selected file board */
NAME currfileboard;
NAMELIST fileboardlist;

/* The currently selected protocol */
NAME currprotocol;
#ifndef REMOTE_CLIENT
NAMELIST protolist;
#endif

/*ARGSUSED*/
AllFileBoardsFunc(indx, board, info)
int indx;
BOARD *board;
struct enum_info *info;
{
  if (info->topline == info->currline) {
    move(info->topline-1, 0);
    prints(" %-16s   %-58s\n", "Name", "Description");
  }

  prints(" %-15s    %s\n", board->name, board->description);

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

FileBoards()
{
  struct enum_info info;
  info.count = 0;
  info.topline = info.currline = 4;
  info.bottomline = t_lines-2;
  move(3,0);
  clrtobot();    
  bbs_enum_fileboards(t_lines-5, 0, AllFileBoardsFunc, &info);
  clrtobot();
  move(t_lines-1, 0);
  prints("%d %s displayed\n", info.count, info.count==1?"board":"boards");
  return PARTUPDATE;
}

FileSelect()
{
  NAME newboard;
  move(2,0);
  clrtobot();
  bbs_fileboardnames(&fileboardlist, NULL);
  if (fileboardlist == NULL) {
    prints("No file boards to select.\n");
    pressreturn();
    return FULLUPDATE;
  }
  namecomplete(NULL, fileboardlist, "Enter board name: ", 
               newboard, sizeof(NAME));
  if (newboard[0] == '\0') return FULLUPDATE;
  else if (!is_in_namelist(fileboardlist, newboard)) {
    move(4,0);
    bbperror(S_NOSUCHBOARD, NULL);
    pressreturn();
    return FULLUPDATE;
  }
  strcpy(currfileboard, newboard);
  return (FULLUPDATE | NEWDIRECT);
}

OpenFileBoard(openflags, newonly, resp)
int *openflags;
int newonly;
int *resp;
{
  int code;
  OPENINFO openinfo;
  code = bbs_open_fileboard(currfileboard, &openinfo);    
  if (resp) *resp = code;
  if (code != S_OK) return -1;
  if (openflags) *openflags = openinfo.flags;
  return (newonly ? openinfo.newmsgs : openinfo.totalmsgs);
}

CloseFileBoard()
{
  int code;
  code = bbs_close_board();
  return (code == S_OK ? 0 : -1);
}

#ifndef REMOTE_CLIENT

SelectProtocol()
{
  NAME protoname;
  move(2,0);
  clrtobot();
  bbs_protonames(&protolist, NULL);
  if (protolist == NULL) {
    prints("No protocols to select!\n");
    pressreturn();
    return FULLUPDATE;
  }
  namecomplete(NULL, protolist, "Select a protocol: ", 
               protoname, sizeof(NAME));
  if (protoname[0] == '\0') return FULLUPDATE;
  else if (!is_in_namelist(protolist, protoname)) {
    move(4,0);
    bbperror(S_NOSUCHPROTO, NULL);
    pressreturn();
    return FULLUPDATE;
  }
  strcpy(currprotocol, protoname);
  bbs_set_protocol(protoname);  
  return FULLUPDATE;
}

FileView(hptr, currmsg, numrecs, openflags)
HEADER *hptr;
int currmsg, numrecs, openflags;
{
  int rc;
  PATH fname;

  if (BITISSET(hptr->flags, FILE_DIRECTORY)) {
    bell();
    return DONOTHING;
  }

  clear();
  if (BITISSET(hptr->flags, FILE_BINARY)) {
    char ans[4];
    getdata(0, 0, "This may be a binary file. Continue (Y/N)? [N]: ",
            ans, sizeof ans, DOECHO, 0);
    if (*ans != 'Y' && *ans != 'y')
      return FULLUPDATE;
  }      

  rc = bbs_download(hptr->title, NULL, fname);
  if (rc != S_OK) {
    bbperror(rc, "Error reading file");
    pressreturn();
    return FULLUPDATE;
  }

  More(fname, 1);
  return FULLUPDATE;
}

GetSavedProtocol()
{
  ACCOUNT acct;
  if (bbs_owninfo(&acct) == S_OK) {
    strncpy(currprotocol, acct.protocol, sizeof currprotocol-1);
  }
}  

#endif

FileUpload()
{
  PATH fname;
  char *base, *slash;
  char ans[4];
#ifdef REMOTE_CLIENT
  strcpy(currprotocol, "builtin");
#else
  if (currprotocol[0] == '\0') {
    GetSavedProtocol();
    if (currprotocol[0] == '\0') {
      clear();
      prints("You must select an upload protocol first.\n");      
      pressreturn();
      return FULLUPDATE;
    }
  }
#endif
  clear();
  if (getdata(0,0,"File to upload: ",fname,sizeof fname,DOECHO,1)==-1) {
    return FULLUPDATE;
  }

  if ((slash = strrchr(fname, '/')) != NULL) base = slash+1;
  else base = fname;

  prints("Uploading '%s'.\n", base);
  prints("Using %s protocol.\n", currprotocol);
  getdata(2, 0, "Are you sure (Y/N)? [Y]: ", ans, sizeof(ans), DOECHO, 0);
  if (*ans == 'N' || *ans == 'n') {
    prints("File NOT uploaded.\n");
  }
  else {
    int rc;
#ifdef REMOTE_CLIENT
    prints("Uploading...please stand by.\n");
    rc = bbs_upload(fname, base, currprotocol);
#else
    prints("Setup modem program and press any key to upload.\n");
    refresh();
    fgetc(stdin);
    reset_tty();
    rc = bbs_upload(NULL, base, currprotocol);
    printf("\nDone. Press any key.\n");
    fgetc(stdin);        
    restore_tty();
    clear();
#endif
    prints("\n");
    if (rc == S_OK) {
      prints("File uploaded!\n");
    }
    else bbperror(rc, "Upload error");
  }
  pressreturn();
  return FULLUPDATE;
}

FileReceive(hptr, currmsg, numrecs, openflags)
HEADER *hptr;
LONG currmsg, numrecs, openflags;
{
  char ans[4];
#ifdef REMOTE_CLIENT
  PATH fname;
#endif
  
  if (BITISSET(hptr->flags, FILE_DIRECTORY)) {
    bell();
    return DONOTHING;
  }

#ifdef REMOTE_CLIENT
  strcpy(currprotocol, "builtin");
#else
  if (currprotocol[0] == '\0') {
    GetSavedProtocol();
    if (currprotocol[0] == '\0') {
      clear();
      prints("You must select a download protocol first.\n");      
      pressreturn();
      return FULLUPDATE;
    }
  }
#endif
  clear();
  prints("Downloading '%s'.\n", hptr->title);
  prints("Using %s protocol.\n", currprotocol);
  getdata(2, 0, "Are you sure (Y/N)? [Y]: ", ans, sizeof(ans), DOECHO, 0);
  if (*ans == 'N' || *ans == 'n') {
    prints("File NOT downloaded.\n");
  }
  else {
    int rc;
#ifdef REMOTE_CLIENT
    if (getdata(3, 0, "Download to (RETURN = current directory): ",
                fname, sizeof fname, DOECHO, 1) == -1) {
      return FULLUPDATE;
    }      
    if (fname[0] == '\0') strncpy(fname, hptr->title, sizeof fname);
    prints("Receiving %d bytes. Please stand by.\n", hptr->size);
    rc = bbs_download(hptr->title, currprotocol, fname);
#else
    prints("Setup modem program and press any key to download.\n");
    refresh();
    fgetc(stdin);
    reset_tty();
    rc = bbs_download(hptr->title, currprotocol, NULL);
    printf("\nDone. Press any key.\n");
    fgetc(stdin);        
    restore_tty();
    clear();
#endif
    if (rc == S_OK) {
      prints("\nFile downloaded!\n");
    }
    else bbperror(rc, "Download error");
  }
  pressreturn();
  return FULLUPDATE;
}

/*ARGSUSED*/
FileForward(hptr, currmsg, numrecs, openflags)
HEADER *hptr;
int currmsg, numrecs, openflags;
{
  int code, rc = FULLUPDATE;
  ACCOUNT acct;
  char ans[4];

  if (BITISSET(hptr->flags, FILE_DIRECTORY)) {
    bell();
    return DONOTHING;
  }
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
    if ((code = bbs_forward_file(hptr->title)) == S_OK) {
      prints("Mail sent!\n");
    }
    else bbperror(code, "Forward failed");
  }
  pressreturn();
  return rc;
}

/*ARGSUSED*/
FileChdir(hptr, currmsg, numrecs, openflags)
HEADER *hptr;
int currmsg, numrecs, openflags;
{
  OPENINFO oinfo;
  if (!BITISSET(hptr->flags, FILE_DIRECTORY)) {
    bell();
    return DONOTHING;
  }
  /* Ostensibly, this should never fail */
  bbs_change_fileboard_dir(hptr->title, &oinfo);
  /* If oinfo->name were modified, we'd have to update currfileboard */
  return (FULLUPDATE | NEWDIRECT | NOCLOSE);
}

/*ARGSUSED*/
FileReadMenuSelect(hptr, currmsg, numrecs, openflags)
HEADER *hptr;
int currmsg, numrecs, openflags;
{
  return (FileSelect());
}

#ifndef REMOTE_CLIENT

/*ARGSUSED*/
FileReadMenuProto(hptr, currmsg, numrecs, openflags)
HEADER *hptr;
int currmsg, numrecs, openflags;
{
  return (SelectProtocol());
}

#endif
