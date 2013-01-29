
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
   Return values for the bbs library functions. 
   DO NOT CHANGE ANY OF THESE!!!
*/

/* General purpose errors */
#define S_OK		0
#define S_SYSERR	1
#define S_DENIED	2
#define S_NOCONFIGFILE  3
#define S_FULL          4
#define S_LOGINFAIL     5
#define S_LOGGEDIN      6
#define S_LOGINLIMIT    7
#define S_LOGINMUSTBOOT 8
#define S_ACCTDISABLED  9
#define S_NOSUCHUSER    10
#define S_USEREXISTS    11
#define S_BADUSERID     12
#define S_BADPASSWD     13
#define S_NOSUCHBOARD   14
#define S_BOARDEXISTS   15
#define S_BADBOARDNAME  16
#define S_NOSUCHFILE    17
#define S_CANNOTMAIL    18
#define S_BOARDFULL     19
#define S_BADADDRESS    20
#define S_CANNOTZAP     21
#define S_NOTOPEN       22
#define S_ALREADYOPEN   23
#define S_WRONGTYPE     24
#define S_PAGEROFF      25
#define S_CANNOTPAGE    26
#define S_PAGEREFUSED   27
#define S_INTERRUPT     28
#define S_CMDTIMEOUT    29
#define S_TALKTOSELF    30
#define S_MODEVIOLATION 31
#define S_CHATIDINUSE   32
#define S_BADCHATID     33
#define S_CHATDERROR    34
#define S_NOSUCHEDITOR  35
#define S_NOSUCHPROTO   36
#define S_PROTONOGOOD   37
#define S_NOUPLOAD      38
#define S_BADFILENAME   39
#define S_BADPATH       40

/* Client-server errors */
#define S_NOTCONN       41
#define S_BADPACKET     42
#define S_TIMEOUT       43
#define S_UNAVAILABLE   44
#define S_HOSTERR       45

#define S_MAXERROR      S_HOSTERR

/* Non-errors, returnable through client-server socket */
#define S_GETFILE       101
#define S_PUTFILE       102

/* Internal use only */
#define S_NOUTABLE      -1
#define S_ATTACHED      -2
#define S_NOSUCHREC     -3
#define S_EMPTYREC      -4
#define S_RECEXISTS     -5
#define S_OUTOFRANGE    -6

/* 
   ENUM_QUIT is never returned by the library functions, but is to be
   returned by the functions called by the enumeration-type library
   functions when they want to quit enumerating.
*/

#define ENUM_QUIT	666


