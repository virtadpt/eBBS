
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

char permtab[MAX_CLNTCMDS];
char *currentboard = currboard;

DoReadHelp(m, openflags)
NREADMENU *m;
int openflags;
{
  NREADMENUITEM *mi;
  for (mi = m->commlist; mi != NULL; mi = mi->next) {
    if (HasReadMenuPerm(mi->boardprivs, openflags) &&
        HasPerm(mi->mainprivs) && mi->help[0] != '\0') 
      prints("%s\n", mi->help);
  }
}

MovementReadHelp()
{
  standout();
  prints("Movement Commands\n");
  standend();
  prints("p               Previous Message\n");
  prints("n               Next Message\n");
  prints("P               Previous Page\n");
  prints("N               Next Page\n");
  prints("## <cr>         Goto Message ##\n");
  prints("$               Goto Last Message\n");
}

MiscReadHelp()
{
  prints("CTRL-L          Redraw Screen\n");
  prints("e               EXIT Read Menu\n");
}

ReadMenuHelp(menu, flags)
NREADMENU *menu;
int flags;
{
  clear();
  standout();
  prints("%s\n", menu->menu_helptitle);
  standend();
  move(2,0);
  MovementReadHelp();
  prints("\n");
  standout();
  prints("Miscellaneous Commands\n");
  standend();
  DoReadHelp(menu, flags);
  MiscReadHelp();
  pressreturn();
  clear();
  return FULLUPDATE;    
}

/*ARGSUSED*/
MailReadHelp(hptr, curr, num, flags)
void *hptr;
LONG curr, num, flags;
{
  return (ReadMenuHelp(MailReadMenu, flags));
}

/*ARGSUSED*/
MainReadHelp(hptr, curr, num, flags)
void *hptr;
LONG curr, num, flags;
{
  return (ReadMenuHelp(PostReadMenu, flags));
}

/*ARGSUSED*/
FileReadHelp(hptr, curr, num, flags)
void *hptr;
LONG curr, num, flags;
{
  return (ReadMenuHelp(FileReadMenu, flags));
}

NotImpl()
{
  clear();
  prints("This function is not yet implemented.\n");
  pressreturn();
  return FULLUPDATE;
}

EndMenu()
{
  return EXITMENU;
}

SetPermTable()
{
  memcpy(permtab, myinfo.access, MAX_CLNTCMDS);
}

HasPerm(perm)
int perm;
{
  if (perm == 0) return 1;
  else if (perm < 0 || perm >= MAX_CLNTCMDS) return 0;
  else return (permtab[perm]-'0');
}

HasReadMenuPerm(perm, flags)
int perm, flags;
{
  if (perm == 0) return 1;
  else return (perm & flags);
}

#define KEY_NORMAL (0)
#define KEY_ESCAPE (1)
#define KEY_VTKEYS (2)

int
MenuGetch()
{
  char c;
  int keymode = KEY_NORMAL;
  while (1) {
    c = igetch();
    switch (keymode) {
    case KEY_NORMAL:
      if (isprint(c)) return c;
      if (c == '\r' || c == '\n' || c == '\010' || c == '\177') {
	c = '\n';
	return c;
      }
      if (c == CTRL('L')) {
        redoscr() ;  
      }
      if (c == 27) keymode = KEY_ESCAPE;   /* ESC */
      break;
    case KEY_ESCAPE:
      if (c == '[') keymode = KEY_VTKEYS;
      else {
	bell();
	keymode = KEY_NORMAL;
      }
      break;
    case KEY_VTKEYS:
      if (c == 'A') return MENU_UP;
      else if (c == 'B') return MENU_DOWN;
      else {
	bell();
	keymode = KEY_NORMAL;
      }
    }
  }
}

