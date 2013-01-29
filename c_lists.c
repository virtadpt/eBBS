
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

extern NAMELIST acctlist;
extern NAMELIST boardlist;

struct disp_list {
  int x;
  int y;
};

/*ARGSUSED*/
display_list_element(indx, name, arg)
int indx;
char *name;
void *arg;
{
  struct disp_list *info = arg;
  move(info->y, info->x);
  prints(name);
  if (++(info->y) == t_lines) {
    info->y = 4;
    info->x+=(NAMELEN + 2);
  }
}
  
display_list(header, list)
char *header;
NAMELIST list;
{
  struct disp_list info;
  info.x = 0;
  info.y = 4;
  move(3,0);
  clrtobot();
  prints("%s\n", header);
  apply_namelist(list, display_list_element, &info);
}

SetOverrides()
{
  int rc, done = 0, change = 0;
  char ans[7];
  NAME userid;
  NAMELIST overlist = NULL;
  clear();
  bbs_acctnames(&acctlist, NULL);
  if ((rc = bbs_get_overrides(&overlist)) != S_OK) {
    bbperror(rc, "Error fetching override list");
    pressreturn();
    return FULLUPDATE;
  }
  prints("Edit Talk Override List\n");
  while (!done) {
    display_list("*** Users on Override List ***", overlist);
    getdata(1,0,"Add name, Remove name, or Quit? [Q]: ",
	    ans, sizeof(ans), DOECHO, 0);
    switch (ans[0]) {
    case 'A': case 'a':
      move(2,0);
      namecomplete(NULL, acctlist, "Add which user: ", userid, sizeof(NAME));
      if (is_in_namelist(acctlist, userid) && 
          !is_in_namelist(overlist, userid)) {
        add_namelist(&overlist, userid, NULL);
        change++;
      }
      break;
    case 'R': case 'r':
      if (overlist == NULL) {
        prints("No one to remove!\n");
        clrtobot();
	pressreturn();
      }
      else {
	namecomplete(NULL, overlist, "Remove which user: ", 
                     userid, sizeof(NAME));
	if (remove_namelist(&overlist, userid) == S_OK) change++;
      }
      break;
    default:
      done = 1;
    }
    move(2,0);
    clrtoeol();    
  }
  if (change) {
    if ((rc = bbs_set_overrides(overlist)) == S_OK)
      prints("Override list was updated.\n");
    else bbperror(rc, "Error setting override list");
  }
  else prints("No changes to override list.\n");
  free_namelist(&overlist);
  pressreturn();
  return FULLUPDATE;
}

SetBoardMgrs()
{
  int rc, done = 0, change = 0;
  char ans[7];
  NAME namebuf;
  BOARD board;
  NAMELIST mgrlist = NULL;
  clear();
  bbs_boardnames(&boardlist, NULL);
  bbs_acctnames(&acctlist, NULL);

  namecomplete(NULL, boardlist, "Set manager for which board: ", 
               namebuf, sizeof(NAME));
  if (namebuf[0] == '\0') return FULLUPDATE;
  else if (!is_in_namelist(boardlist, namebuf)) {
    bbperror(S_NOSUCHBOARD, NULL);
    pressreturn();
    return FULLUPDATE;
  }
  if ((rc = bbs_get_board(namebuf, &board)) != S_OK) {
    bbperror(rc, NULL);
    pressreturn();
    return FULLUPDATE;
  }
  if ((rc = bbs_get_boardmgrs(board.name, &mgrlist)) != S_OK) {
    bbperror(rc, "Error retrieving manager list");
    pressreturn();
    return FULLUPDATE;
  }

  prints("Edit Board Manager List\n");
  while (!done) {
    display_list("*** Board Managers ***", mgrlist);
    getdata(1,0,"Add manager, Remove manager, or Quit? [Q]: ",
	    ans, sizeof(ans), DOECHO, 0);
    switch (ans[0]) {
    case 'A': case 'a':
      move(2,0);
      namecomplete(NULL, acctlist, "Add which user: ", namebuf, sizeof(NAME));
      if (is_in_namelist(acctlist, namebuf) && 
          !is_in_namelist(mgrlist, namebuf)) {
        add_namelist(&mgrlist, namebuf, NULL);
        change++;
      }
      break;
    case 'R': case 'r':
      if (mgrlist == NULL) {
        prints("No one to remove!\n");
        clrtobot();
	pressreturn();
      }
      else {
	namecomplete(NULL, mgrlist, "Remove which user: ", 
                     namebuf, sizeof(NAME));
	if (remove_namelist(&mgrlist, namebuf) == S_OK) change++;
      }
      break;
    default:
      done = 1;
    }
    move(2,0);
    clrtoeol();    
  }
  if (change) {
    if ((rc = bbs_set_boardmgrs(board.name, mgrlist)) == S_OK)
      prints("Manager list for '%s' was updated.\n", board.name);
    else bbperror(rc, "Error setting manager list");
  }
  else prints("No changes to manager list.\n");
  free_namelist(&mgrlist);
  pressreturn();
  return FULLUPDATE;
}
