
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

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "osdeps.h"
#include "modes.h"
#include "retval.h"
#include "clntcmds.h"

/* 
   Common header file for all pieces of the bbs, client and server.
*/

/* The port bbsd listens on. */
#define EBBS_PORT       5150

/* Exit values for lbbs. */

#define EXIT_LOGOUT	0
#define EXIT_LOSTCONN	-1
#define EXIT_CLIERROR   -2
#define EXIT_TIMEDOUT   -3
#define EXIT_KICK	-4

/* Lengths for string data items. */

#define BBSNAMELEN      39
#define NAMELEN		12
#define PASSLEN		14
#define UNAMELEN	25
#define HOSTLEN		39
#define TERMLEN		11
#define CSETLEN         11
#define RNAMELEN	29
#define ADDRLEN		99
#define MAILLEN		79
#define PATHLEN		255
#define TITLELEN	63

/* I like typedefs. */

typedef char BBSNAME[BBSNAMELEN+1];
typedef char NAME[NAMELEN+1];
typedef char PASSWD[PASSLEN+1];
typedef char UNAME[UNAMELEN+1];
typedef char HOST[HOSTLEN+1];
typedef char TERM[TERMLEN+1];
typedef char CSET[CSETLEN+1];
typedef char RNAME[RNAMELEN+1];
typedef char ADDR[ADDRLEN+1];
typedef char MAIL[MAILLEN+1];
typedef char PATH[PATHLEN+1];
typedef char TITLE[TITLELEN+1];
typedef char ACCESSCODES[MAX_CLNTCMDS];

/* Structures used in the bbs library functions. */

typedef struct _INITINFO {
  int (*abortfn)();
  char *tmpdir;
} INITINFO;

typedef struct _BBSINFO {
  BBSNAME boardname;
  SHORT majver;
  SHORT minver;
  SHORT newok;
} BBSINFO;

typedef struct _LOGININFO {
  NAME userid;
  SHORT flags;
  LONG idletimeout;
  LONG lastlogin;
  HOST fromhost;
  ACCESSCODES access;
} LOGININFO;

typedef struct _ACCOUNT {
  /* in the passwds file */ 
  NAME userid;
  PASSWD passwd;
  UNAME username;
  LONG perms;
  SHORT flags;
  /* in home/<userid>/lastlogin */
  LONG lastlogin;
  HOST fromhost;
  /* in home/<userid>/profile */
  TERM terminal;
  CSET charset;
  RNAME realname;
  ADDR address;
  MAIL email;
  NAME protocol;
  NAME editor;
} ACCOUNT;

typedef struct _USEREC {
  NAME userid;
  UNAME username;
  HOST fromhost;
  LONG pid;
  SHORT flags;
  SHORT mode;
} USEREC;

/* Account and userec flags. */
#define FLG_CLOAK	0x001     /* Invisibility */
#define FLG_EXEMPT	0x002     /* For use by user clean utilities */
#define FLG_DISABLED    0x004     /* Account is temporarily disabled */
#define FLG_SHARED      0x008     /* Account does not belong to one user */
#define FLG_NOPAGE      0x010     /* User not accepting page requests... */
#define FLG_NOOVERRIDE  0x020     /* ...not even friends! */
#define FLG_EXPERT      0x040     /* Verbose menus or not? */

#ifdef FOR_FUTURE_USE
/* More userec flags */
#define FLG_IN_BBS      0x100     /* User is connected to bbs */
#define FLG_IN_CHAT     0x200     /* User is connected to chat daemon */
#endif

/* Flags for bbs_modify_account */
#define MOD_USERID      0x001
#define MOD_PASSWD      0x002
#define MOD_USERNAME    0x004
#define MOD_TERMINAL    0x008
#define MOD_REALNAME    0x010
#define MOD_ADDRESS     0x020
#define MOD_EMAIL       0x040
#define MOD_PROTOCOL    0x080
#define MOD_EDITOR      0x100
#define MOD_CHARSET     0x200

/* Mode constants */
#define BBS_MAX_MODE       31       /* not including flags */
#define MODE_FLG_NOPAGE    0x0100   /* must be greater than BBS_MAX_MODE */

/* Max number of recipients for a mail message */
#define BBS_MAX_MAILRECIPS 32

typedef struct _OPENINFO {
  NAME name;
  SHORT flags;
  LONG totalmsgs;
  LONG newmsgs;
} OPENINFO;

/* Flags for OPENINFO */
#define OPEN_POST       0x001
#define OPEN_MANAGE     0x002
#define OPEN_REOPEN     0x004

typedef struct _HEADER {
  SHORT fileid;
  ADDR owner;
  TITLE title;
  SHORT flags;
  LONG size;
  LONG mtime;
} HEADER;

/* flags for HEADER structures */
#define FILE_UNREAD	0x1
#define FILE_MARKED	0x2
#define FILE_BINARY     0x4
#define FILE_DIRECTORY  0x8
 
/* General-purpose board type constants */
#define BOARD_NONE      0
#define BOARD_MAIL      1
#define BOARD_POST      2
#define BOARD_FILE      3

typedef struct _BOARD {
    NAME name;
    TITLE description;
    LONG readmask;
    LONG postmask;
    SHORT flags;
    LONG totalposts;
    LONG newposts;
    LONG ownedposts;     /* not supported at the moment! */
    LONG lastpost;
} BOARD;

/* Flags for BOARD structs */
#define BOARD_ZAPPED    0x001
#define BOARD_NOZAP     0x002

/* Flags for bbs_modify_board. */
#define MOD_BNAME       0x001
#define MOD_BOARDDESC   0x002
#define MOD_READMASK    0x004
#define MOD_POSTMASK    0x008

/* Flags for bbs_enum_boards. */
#define BE_UNZAPPED     0x001
#define BE_ZAPPED       0x002
#define BE_ALL          (BE_ZAPPED | BE_UNZAPPED)
#define BE_UNDEFINED    0x004
#define BE_DO_COUNTS    0x008

/* Standard mailer-prefix for Internet mail messages. */
#define MAILER_PREFIX   "INTERNET"

/* Common stuff for chat. */
#define CHATID_MAX         8
typedef char CHATID[CHATID_MAX+1];

#define CHATLINE_MAX       255  /* Text plus /cmd at beginning */
#define CHATLINE_TEXT_MAX  200  /* Max size of test only */
typedef char CHATLINE[CHATLINE_MAX+1];

#define CHAT_CTRL_CHATID "**C"  /* Chatd-->client control message */

/* Initial responses sent by the chat daemon */
#define CHAT_LOGIN_OK       "OK"
#define CHAT_LOGIN_EXISTS   "EX"
#define CHAT_LOGIN_INVALID  "IN"
#define CHAT_LOGIN_BOGUS    "BG"

/* Namelists are very useful. */

typedef struct _NAMENODE {
  char *word;
  struct _NAMENODE *next;
} NAMENODE;

typedef NAMENODE *NAMELIST;

/* Useful bit-manipulation macros. */

#define BITISSET(mask,bit)     ((mask)&(bit))
#define BITSET(mask,bit)       ((mask)|=(bit))
#define BITCLR(mask,bit)       ((mask)&=~(bit))
#define BITTOGGLE(mask,bit)    ((mask)^=(bit))

/* Prototypes. */

void strip_trailing_space __P((char *));
int recursive_rmdir __P((char *));
int is_valid_userid __P((char *));
int is_valid_password __P((char *));
int is_valid_boardname __P((char *));
LONG hex2LONG __P((char *));
SHORT hex2SHORT __P((char *));
char *LONGcpy __P((char *, LONG));
char *SHORTcpy __P((char *, SHORT));
int is_text_file __P((char *));
int is_directory __P((char *));

int is_passwd_good __P((char *, char *));
void encrypt_passwd __P((char *, char *));

void free_namelist __P((NAMELIST *));
void create_namelist __P((NAMELIST *));
int add_namelist __P((NAMELIST *, char *, char *));
int remove_namelist __P((NAMELIST *, char *));
int is_in_namelist __P((NAMELIST, char *));
int apply_namelist __P((NAMELIST, int(), void *));
int read_namelist __P((char *, NAMELIST *));
int write_namelist __P((char *, NAMELIST));

int read_headers __P((char *, HEADER *));
int write_mail_headers __P((int, HEADER *, char *, NAMELIST));
int write_post_headers __P((int, HEADER *, char *, char *));
int parse_to_list __P((NAMELIST *, char *, char *));
