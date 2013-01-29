
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


#include "common.h"
#include "perms.h"

/* 
   Header file of stuff just of use to the server side, which is for now
   just libbbs. 
*/

#define SERVER_VERSION_MAJ	0
#define SERVER_VERSION_MIN	1

/* This userid owns posts/mail that have no headers */

#define SYSOP_ACCOUNT "SYSOP"

typedef struct _USERDATA {
  USEREC u;
  LONG perms;
  ACCESSCODES access;
  int newmailmsgs;     /* easy (You have mail.) checking */
  SHORT usermode;
  SHORT port;          /* this and destpid are for Talk */
  LONG destpid;        /* this tells who I am paging */
} USERDATA;

typedef struct _SERVERDATA {
  BBSNAME name;
  PATH logfile;
  PATH tempfile;
  PATH encodebin;
  PATH locale;
  int maxusers;
  int reservedslots;
  int maxutable;
  int newok;
  int loglevel;
  int maxlogons;
  int idletimeout;
  int maxsiglines;
  int queryreal;
} SERVERDATA;

#define LOG_LEVEL_MAX     10
#define LOG_LEVEL_DEFAULT 1

/* 
   Maximum number of files per board. Be careful not to extend this beyond
   BBS_MAX_RECORD in record.c! The readbits files will break things if so.
*/

#define BBS_MAX_FILES  2048

#define READBITSIZE BBS_MAX_FILES   /* future plan: BBS_MAX_FILES / 8 */

typedef struct _READINFO {
  LONG stamp;
  char bits[READBITSIZE];
} READINFO;
  
/*
   Read order is not implemented at this time, but it can be added later
   without ill effect.
*/

#define READ_ORDER_ZAPPED     0xffff   /* means board is zapped */
#define READ_ORDER_UNSET      0x0000   /* not zapped, or unzapped */

struct enumstruct {
  int (*fn)();
  void *arg;
  SHORT flags;
};

struct namechange {
  char *oldname;
  char *newname;
};

struct listcomplete {
  NAMELIST *listp;
  char *str;
};

/* This mask must encompass all the MOD_* flags for account in common.h */
#define MOD_ACCOUNT_MASK 0x0FFF

#define _MOD_PERMS     0x8000
#define _TOGGLE_FLAGS  0x4000

/* Server stuff for chat. */
#define PATH_CHATD     "bin/chatd"
#define PATH_CHATPORT  "etc/.chatport"
#define PATH_CHATPID   "etc/.chatpid"

/* Are we an ordinary program or bbsd? */
#define BBSLIB_DEFAULT  0
#define BBSLIB_BBSD     1
extern int bbslib_user;

int _match_first __P((char *, char *));
int _match_full __P((char *, char *));
int _change_name __P((char *, char *, char *));
char *_append_quoted __P((char *, char *));
char *_extract_quoted __P((char *, char *, int));
int _record_add __P((char *, int(), void *, int(), void *));
int _record_delete __P((char *, int(), void *));
int _record_delete_many __P((char *, int(), void *));
int _record_find __P((char *, int(), void *, int(), void *));
int _record_enumerate __P((char *, int, int(), void *));
int _record_replace __P((char *, int(), void *, int(), void *));

int get_bitfile_ent __P((char *, READINFO *));
int set_bitfile_ent __P((char *, READINFO *));
int clear_all_readbits __P((READINFO *));
int set_readbit __P((READINFO *, SHORT));
int test_readbit __P((READINFO *, SHORT));

#if NO_SHARED_MEMORY
# define utable_attach f_utable_attach
# define utable_detach f_utable_detach
# define utable_lock_record f_utable_lock_record
# define utable_free_record f_utable_free_record
# define utable_get_record f_utable_get_record
# define utable_set_record f_utable_set_record
# define utable_find_record f_utable_find_record
# define utable_enumerate f_utable_enumerate
#endif

int utable_attach __P((int));
int utable_detach __P((int));
int utable_lock_record __P((int *));
int utable_free_record __P((int));
int utable_get_record __P((int, USERDATA *));
int utable_set_record __P((int, USERDATA *));
int utable_find_record __P((LONG, USERDATA *));
int utable_enumerate __P((int, char *, int(), void *));

SHORT my_real_mode __P((void));
char *my_userid __P((void));
char *my_username __P((void));
char *my_host __P((void));

