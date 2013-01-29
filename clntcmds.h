
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
   All of the interface points between the bbs client and server parts
   are listed here. These must not be changed, but new ones can be added
   up to MAX_CLNTCMDS.
*/

#define C_INIT         0
#define C_CONNECT      1
#define C_DISCONNECT   2
#define C_ISSUE        3
#define C_WELCOME      4
#define C_NEWLOGIN     5
#define C_LOGIN        6
#define C_ADDACCT      7
#define C_DELACCT      8
#define C_QUERY        9
#define C_OWNACCT     10
#define C_GETACCT     11
#define C_SETMODE     12
#define C_SETPASSWD   13
#define C_SETUSERNAME 14
#define C_SETTERMINAL 15
#define C_SETEMAIL    16
#define C_SETPERMS    17
#define C_SETACCT     18
#define C_ALLUSERS    19
#define C_USERS       20
#define C_ALLNAMES    21
#define C_NAMES       22
#define C_INFOFILE    23
#define C_LICENSE     24
#define C_GETPLAN     25
#define C_SETPLAN     26
#define C_CLOAK       27
#define C_EXEMPT      28
#define C_GETPERMSTRS 29
#define C_CHKMAIL     30
#define C_MAIL        31
#define C_OPENMAIL    32
#define C_CLOSEBRD    33
#define C_ENUMHDRS    34
#define C_READMSG     35
#define C_DELMSG      36
#define C_DELRANGE    37
#define C_MARKMSG     38
#define C_SETWELCOME  39
#define C_ADDBOARD    40
#define C_DELBOARD    41
#define C_GETBOARD    42
#define C_SETBOARD    43
#define C_ENUMBRDS    44
#define C_BNAMES      45
#define C_VISITBRD    46
#define C_POST        47
#define C_OPENBRD     48
#define C_TESTBRD     49
#define C_REPLACEMSG  50
#define C_ZAPBOARD    51
#define C_GETBMGRLIST 52
#define C_SETBMGRLIST 53
#define C_ENUMFBRDS   54
#define C_FBNAMES     55
#define C_OPENFBRD    56
#define C_PROTONAMES  57
#define C_SETPROTO    58
#define C_UPLOAD      59
#define C_DOWNLOAD    60
#define C_CHAT        61
#define C_EXITCHAT    62
#define C_CHATLINE    63
#define C_KICK        64
#define C_TALK        65
#define C_EXITTALK    66
#define C_GETTALKREQ  67
#define C_TALKACCEPT  68
#define C_TALKREFUSE  69
#define C_SETPAGER    70
#define C_GETOVERLIST 71
#define C_SETOVERLIST 72
#define C_FORWARDMSG  73
#define C_FORWARDFILE 74
#define C_SETEDITOR   75
#define C_GETEDITOR   76
#define C_ENUMEDITORS 77
#define C_GETSIGFILE  78
#define C_SETSIGFILE  79
#define C_GETMODESTRS 80
#define C_GETMODECHRS 81
#define C_SETCLIOPTS  82
#define C_SEEALLAINFO 83
#define C_SEEALLBINFO 84
#define C_ALLBOARDMGR 85
#define C_SEECLOAK    86
#define C_SEEREALNAME 87
#define C_NOTIMEOUT   88
#define C_USERESERVED 89
#define C_EXTERNMAIL  90
#define C_ADMINMENU   91
#define C_FILEMENU    92
#define C_MAILMENU    93
#define C_TALKMENU    94
#define C_XYZMENU     95
#define C_SETCHARSET  96
#define C_FILECHDIR   97
#define C_MOVEMESSAGE 98

#define MAX_CLNTCMDS  256
