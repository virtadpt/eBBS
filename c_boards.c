
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
#include <time.h>
#include <ctype.h>

/* The currently selected board */
NAME currboard;
NAMELIST boardlist;

/* stuff in c_users.c for the permission mask setting */
extern LONG SetPermMenu __P((LONG));

CheckBoardName(bname)
char *bname;
{
  BOARD buf;
  if (bname[0] == '\0') return 1;
  if (!is_valid_boardname(bname)) return 3;
  if (bbs_get_board(bname, &buf) != S_NOSUCHBOARD) return 2; 
  return 0;
}

/*ARGSUSED*/
AllBoardsFunc(indx, board, info)
int indx;
BOARD *board;
struct enum_info *info;
{
  if (info->topline == info->currline) {
    move(info->topline-1, 0);
    prints(" %-16s   %-58s\n", "Name", "Description");
  }

  prints("%c%-15s %c%c %s\n", 
	 BITISSET(board->flags, BOARD_ZAPPED) ? '*' : ' ',
	 board->name, 
	 board->readmask ? 'R' : ' ',
	 board->postmask ? 'P' : ' ',
	 board->description);

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

Boards()
{
  struct enum_info info;
  info.count = 0;
  info.topline = info.currline = 4;
  info.bottomline = t_lines-2;
  move(3,0);
  clrtobot();    
  bbs_enum_boards(t_lines-5, 0, BE_ALL, AllBoardsFunc, &info);
  clrtobot();
  move(t_lines-1, 0);
  prints("%d %s displayed\n", info.count, info.count==1?"board":"boards");
  return PARTUPDATE;
}

/*ARGSUSED*/
BoardCountsFunc(indx, board, info)
int indx;
BOARD *board;
struct enum_info *info;
{
  if (info->topline == info->currline) {
    move(info->topline-1, 0);
    prints(" %-16s   %6s %6s   %-30s\n", "Board", 
           "Total", "Unread", "Last Post");
  }

  prints("%c%-16s   %6d %6d   %s", 
	 BITISSET(board->flags, BOARD_ZAPPED) ? '*' : ' ',
	 board->name, 
	 board->totalposts,
       	 board->newposts,
	 (board->lastpost == 0 ? "\n" : ctime((time_t *)&board->lastpost)));

  info->currline++;
  info->count++;
  info->totals[0] += board->totalposts;
  info->totals[1] += board->newposts;

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

BoardCounts()
{
  struct enum_info info;
  SHORT flags;
  char ans[9];
  info.count = 0;
  info.topline = info.currline = 4;
  info.bottomline = t_lines-2;
  info.totals[0] = info.totals[1] = 0;
    
  move(3,0);
  clrtobot();
  if (getdata(3, 0, "(U)nzapped only, (Z)apped only, or (A)ll? [U]: ",
          ans, sizeof ans, DOECHO, 1) == -1) return FULLUPDATE;

  if (*ans == 'Z' || *ans == 'z') flags = BE_ZAPPED;
  else if (*ans == 'A' || *ans == 'a') flags = BE_ALL;
  else flags = BE_UNZAPPED;

  flags |= BE_DO_COUNTS;
  bbs_enum_boards(t_lines-5, 0, flags, BoardCountsFunc, &info);
  
  clrtobot();
  move(info.currline, 0);

  prints(" %-16s   %6d %6d\n", "*** TOTALS ***", 
         info.totals[0], info.totals[1]);

  return PARTUPDATE;
}

SelectBoard()
{
  NAME newboard;
  move(2,0);
  clrtobot();
  bbs_boardnames(&boardlist, NULL);
  if (boardlist == NULL) {
    prints("No boards to select.\n");
    pressreturn();
    return FULLUPDATE;
  }
  namecomplete(NULL, boardlist, "Enter board name: ", newboard, sizeof(NAME));
  if (newboard[0] == '\0') return FULLUPDATE;
  else if (!is_in_namelist(boardlist, newboard)) {
    move(4,0);
    bbperror(S_NOSUCHBOARD, NULL);
    pressreturn();
    return FULLUPDATE;
  }
  strcpy(currboard, newboard);
  return (FULLUPDATE | NEWDIRECT);
}

Zap()
{
  NAME zapboard;
  BOARD brec;
  SHORT notzapped;
  int rc;
  move(2,0);
  bbs_boardnames(&boardlist, NULL);
  namecomplete(NULL, boardlist, "Board to zap/unzap: ", 
               zapboard, sizeof(NAME));
  if (zapboard[0] == '\0') return PARTUPDATE;
  if (!is_in_namelist(boardlist, zapboard) || 
      bbs_get_board(zapboard, &brec) != S_OK) {
    move(4,0);
    bbperror(S_NOSUCHBOARD, NULL);
    pressreturn();
    return FULLUPDATE;
  }
  notzapped = !BITISSET(brec.flags, BOARD_ZAPPED);
  if ((rc = bbs_zap_board(brec.name, notzapped)) != S_OK) {
    move(3,0);
    bbperror(rc, "Zap failed");
  }
  return PARTUPDATE;
}

Visit()
{
  NAME visitboard;
  char ans[4];
  int code;
  move(2,0);
  clrtobot();
  bbs_boardnames(&boardlist, NULL);
  add_namelist(&boardlist, "*", NULL);
  namecomplete(NULL, boardlist, "Visit board (* = all unzapped boards): ", 
	       visitboard, sizeof(NAME));
  remove_namelist(&boardlist, "*");
  if (visitboard[0] == '\0') return FULLUPDATE;
  else if (visitboard[0] != '*' && !is_in_namelist(boardlist, visitboard)) {
    move(4,0);
    bbperror(S_NOSUCHBOARD, NULL);
    pressreturn();
    return FULLUPDATE;
  }
  getdata(3,0,"Are you sure (Y/N)? [N]: ", ans, sizeof(ans), DOECHO, 0);
  move(4,0);
  if (*ans != 'Y' && *ans != 'y') {
    prints("Visit NOT done.\n");
    pressreturn();
    return FULLUPDATE;
  }
  /* this can take a while, so...*/
  prints("Visit in progress...");
  refresh();
  code = bbs_visit_board(visitboard);
  if (code == S_OK) prints("done!\n");
  else bbperror(code, "visit failed");
  pressreturn();
  return FULLUPDATE;
}

SetBoardPerms(brec, postp)
BOARD *brec;
int postp;
{
  LONG newperms;
  move(2,0);
  clrtobot();
  prints("Set the %s permissions for board '%s'\n", 
	 (postp ? "post" : "read"), brec->name);
  newperms = SetPermMenu(postp ? brec->postmask : brec->readmask);	
  clear();
  if (newperms != (postp ? brec->postmask : brec->readmask)) {
    if (postp) brec->postmask = newperms;
    else brec->readmask = newperms;
    return 0;
  }
  else return -1;
}

AddBoard()
{
  BOARD board;
  int y, grok;
  int rc;
  char ans[4];
  move(3,0);
  clrtobot();
  memset(&board, 0, sizeof(BOARD));
  do {
    if (getdata(3,0,"New board name: ",board.name,NAMELEN+1,DOECHO,1)==-1)
      return FULLUPDATE;

    grok = CheckBoardName(board.name);
    switch (grok) {
    case 1: return FULLUPDATE;
    case 2: bbperror(S_BOARDEXISTS, NULL);
      break;
    case 3: bbperror(S_BADBOARDNAME, NULL);
    }
  } while (grok);
  getdata(4,0,"Board description: ", board.description, TITLELEN+1, DOECHO, 0);
  getdata(5,0,"Set read restrictions (Y/N)? [N]: ",ans,sizeof(ans),DOECHO,0);
  if (*ans == 'Y' || *ans == 'y') {
    SetBoardPerms(&board, 0);
    y = 0;
  }
  else y = 6;
  getdata(y,0,"Set post restrictions (Y/N)? [N]: ",ans,sizeof(ans),DOECHO,0);
  if (*ans == 'Y' || *ans == 'y') {
    SetBoardPerms(&board, 1);
    y = 0;
  }
  else y = 7;
  getdata(y,0,"Add board: are you sure (Y/N)? [N]: ",ans,sizeof(ans),
	  DOECHO, 0);
  if (*ans != 'Y' && *ans != 'y') {
    prints("Board not added.\n");
  }
  else {
    if ((rc = bbs_add_board(&board)) == S_OK) {
      prints("Board '%s' was added.\n", board.name);
    }
    else bbperror(rc, "Board add failed");
  }
  pressreturn();
  return FULLUPDATE;
}

DeleteBoard()
{
  NAME namebuf;
  char ans[4];
  int rc;
  move(2,0);
  clrtobot();
  bbs_boardnames(&boardlist, NULL);
  namecomplete(NULL, boardlist, "Delete which board: ", namebuf, sizeof(NAME));
  if (namebuf[0] == '\0') return FULLUPDATE;
  else if (!is_in_namelist(boardlist, namebuf)) {
    bbperror(S_NOSUCHBOARD, NULL);
    pressreturn();
    return FULLUPDATE;
  }
  prints("Deleting board '%s'.\n", namebuf);
  getdata(5,0,"Are you sure (Y/N)? [N]: ",ans,sizeof(ans), DOECHO, 0);    
  if (ans[0] != 'Y' && ans[0] != 'y') {
    prints("Board not deleted.\n");
    pressreturn();
    return FULLUPDATE;
  }
  if ((rc = bbs_delete_board(namebuf)) == S_OK)
    prints("Board has been deleted.\n");
  else
    bbperror(rc, "Board delete failed");
  pressreturn();
  return FULLUPDATE;
}

ChangeBoard()
{
  NAME namebuf, oldname;
  BOARD brec;
  int rc;
  int y, grok;
  SHORT flags = 0;
  char ans[4];
  move(2,0);
  clrtobot();
  bbs_boardnames(&boardlist, NULL);
  namecomplete(NULL, boardlist, "Change which board: ", namebuf, sizeof(NAME));
  if (namebuf[0] == '\0') return FULLUPDATE;
  else if (!is_in_namelist(boardlist, namebuf)) {
    bbperror(S_NOSUCHBOARD, NULL);
    pressreturn();
    return FULLUPDATE;
  }
  if ((rc = bbs_get_board(namebuf, &brec)) != S_OK) {
    bbperror(rc, NULL);
    pressreturn();
    return FULLUPDATE;
  }
  strncpy(oldname, brec.name, NAMELEN);
  move(3,0);
  clrtobot();
  move(4,0);
  prints("Board name: %s\n", brec.name);
  prints("Board description: %s\n", brec.description);
  prints("Use (M)anagers to view and set board managers.\n");
  if (brec.readmask) prints("Read restrictions are in effect.\n");
  if (brec.postmask) prints("Post restrictions are in effect.\n");
  do {
    getdata(9,0,"New board name (ENTER to leave unchanged): ", 
	    namebuf, NAMELEN+1, DOECHO, 0);
    grok = CheckBoardName(namebuf);
    switch (grok) {
    case 0: BITSET(flags, MOD_BNAME);
      strncpy(brec.name, namebuf, sizeof(brec.name));
      break;
    case 1: grok = 0;
      break;
    case 2: bbperror(S_BOARDEXISTS, NULL);
      break;
    case 3: bbperror(S_BADBOARDNAME, NULL);
    }
  } while (grok);
  getdata(10,0,"New description if any: ", brec.description, TITLELEN,
	  DOECHO, 0);
  if (brec.description[0] != '\0') BITSET(flags, MOD_BOARDDESC);
  getdata(11,0,"Alter read restrictions (Y/N)? [N]: ",
          ans,sizeof(ans),DOECHO,0);
  if (*ans == 'Y' || *ans == 'y') {
    if (SetBoardPerms(&brec, 0) != -1)
      BITSET(flags, MOD_READMASK);
    y = 0;
  }
  else y = 12;
  getdata(y,0,"Alter post restrictions (Y/N)? [N]: ",
          ans,sizeof(ans),DOECHO,0);
  if (*ans == 'Y' || *ans == 'y') {
    if (SetBoardPerms(&brec, 1) != -1)
      BITSET(flags, MOD_POSTMASK);
    y = 0;
  }
  else y = 14;
  getdata(y,0,"Change board: are you sure (Y/N)? [N]: ",ans,sizeof(ans),
	  DOECHO,0);
  if (*ans != 'Y' && *ans != 'y') {
    prints("Board not changed.\n");
  }
  else {
    rc = bbs_modify_board(oldname, &brec, flags);
    if (rc == S_OK) prints("Board '%s' was changed.\n", oldname);
    else bbperror(rc, "Board change failed");
  }
  pressreturn();
  return FULLUPDATE;
}
