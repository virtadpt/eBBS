
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


/* 
   Legal values for the mode field in a userec structure, to tell what
   a user is doing. Some of these are set by the server but by the client
   because this is a transaction-based system for the most part.
*/

#define M_EMPTY		0    /* Empty slot in utable */
#define M_CONNECTING	1    
#define M_UNDEFINED     2

#define M_MAIL          3
#define M_READING       4
#define M_POSTING       5
#define M_ULDL          6
#define M_CHAT          7
#define M_MONITOR       8
#define M_TALK          9
#define M_PAGE         10
