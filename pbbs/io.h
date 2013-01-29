/*
    Pirate Bulletin Board System
    Copyright (C) 1990, Edward Luke, lush@Athena.EE.MsState.EDU
    Eagles Bulletin Board System
    Copyright (C) 1992, Raymond Rocker, rocker@datasync.com
                        Guy Vega, gtvega@seabass.st.usm.edu
                        Dominic Tynes, dbtynes@seabass.st.usm.edu

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 1, or (at your option)
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef MIN
#define MIN(a,b) ((a<b)?a:b)
#endif

#ifndef MAX
#define MAX(a,b) ((a>b)?a:b)
#endif
 
#define DOECHO (1)      /* Flags to getdata input function */
#define NOECHO (0)

#define I_TIMEOUT   (-2)         /* Used for the getchar routine select call */
#define I_OTHERDATA (-3)         /* interface */
#define I_SIGNAL    (-4)	 

#define MAXCOMSZ   (1024) /* Maximum length of do_exec command */
#define MAXFILELEN (80)   /* Maximum length of the executable file name */
#define MAXARGS    (40)   /* Maximum number of args to a do_exec command */

#ifndef CTRL
#define CTRL(x)	(x&037)
#endif

#define YEA     1
#define NA      0

#if AUX
# define clear aux_clear
#endif

extern int dumb_term;
extern int t_lines;
extern int t_columns;
extern int scrint;
extern int g_child_pid;
