
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

#include "pbbs/io.h"

/*
   Header file for the user interface. This plus the remote libbbs will 
   make up the client in a future version.
*/

#if COLOR
/* ANSI Color constants */
# define COLOR_BLACK 0
# define COLOR_RED 1
# define COLOR_GREEN 2
# define COLOR_ORANGE 3
# define COLOR_DARKBLUE 4
# define COLOR_PURPLE 5
# define COLOR_LIGHTBLUE 6
# define COLOR_WHITE 7
#endif

/* Flags for the menu commands */

#define DONOTHING	0
#define FULLUPDATE      0001
#define PARTUPDATE      0002
#define FETCHNEW	0004
#define NEWDIRECT	0010
#define EXITMENU        0020
#define MENUERROR	0040
#define NOCLOSE         0100

struct enum_info {
  int count;
  int topline;
  int bottomline;
  int currline;
  int totals[2];   /* general purpose counters */
};

/* sleep time between Monitor refreshes */
#define MONITOR_REFRESH 10         

/* Additions for new_menu items */

#define MAXMENUSZ     (26)
#define MAXMENUDEPTH  (5)

#define MENU_UP   (16)
#define MENU_DOWN (14)

typedef struct _NMENUITEM {
    char *name ;
    int   enabled ;
    char *default_action ;
    char *error_action ;
    int (*action_func)() ;
    char *action_arg ;
    char *help ;
    struct _NMENUITEM *prev ;
    struct _NMENUITEM *next ;
} NMENUITEM ;

typedef struct _NMENU {
    char *menu_id ;
    char *menu_title ;
    char *menu_default ;
    char *menu_prompt ;
    NMENUITEM *menucommands[MAXMENUSZ] ;
    NMENUITEM *commlist ;
    struct _NMENU *next ;
} NMENU ;

typedef struct _NREADMENUITEM {
    int   key ;
    int   mainprivs ;
    int   boardprivs ;
    int (*action_func)() ;
    char *help ;
    struct _NREADMENUITEM *next ;
} NREADMENUITEM ;

typedef struct _NREADMENU {
    char *menu_helptitle ;
    char *menu_title ;
    char *menu_message ;
    char *menu_line2 ;
    char *menu_line3 ;
    char *menu_field1 ;
    char *menu_field2 ;
    char *menu_field3 ;
    char *menu_field4 ;
    NREADMENUITEM *commlist ;
} NREADMENU ;

/* A few global variables */
extern LOGININFO myinfo;
extern char c_tempfile[];
extern NAME currboard;
extern NAME currfileboard;
extern NREADMENU *PostReadMenu, *MailReadMenu, *FileReadMenu;
extern char *bb_errlist[];

/* Functions to check for page requests */
extern int PagePending __P((void));
extern int NewPagePending __P((void));

/* Other common functions */
extern char ModeToChar __P((SHORT));
extern char *ModeToString __P((SHORT));
extern int bbperror __P((LONG, char *));
