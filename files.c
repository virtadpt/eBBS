
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

#include "server.h"
#include <sys/stat.h>
#include <fcntl.h>
#if LACKS_MALLOC_H
# include <stdlib.h>
#else
# include <malloc.h>
#endif
#include <time.h>
#include <utime.h>
#ifdef NeXT
#include <sys/dir.h>
#include <sys/dirent.h>
#include <sys/stat.h>
#else
#include <dirent.h>
#endif
#include <ctype.h>

#ifdef NeXT
typedef unsigned short mode_t;
struct utimbuf {
  time_t  actime;
  time_t  modtime;
};
#endif

extern LONG mail_file_to_outside __P((char *, char *, char *, int, int));
extern LONG forward_file_to_outside __P((char *, char *, int));

extern SERVERDATA server;    /* for append_sig */

#define MAIL_READBITS_NAME "$MAIL$"

typedef struct _FILENODE {
  SHORT fileid;
  SHORT flags;
  LONG size;
  time_t mtime;
  struct _FILENODE *next;
} FILENODE;

typedef struct _FILENODE *FILELIST;

struct _openboard {
  int btype;
  OPENINFO oi;
  READINFO ri;
  PATH bdir;
  PATH wdir;
  FILELIST bcache;
  time_t bsync;
} _currbrd;


#define UNMARKED_FILE_MODE  0660
#define MARKED_FILE_MODE    0640

free_filelist(list)
FILELIST *list;
{
  FILENODE *curr = *list, *next;
  while (curr != NULL) {
    next = curr->next;
    free(curr);
    curr = next;
  }
  *list = NULL;
}    

insert_filelist(list, node)
FILELIST *list;
FILENODE *node;
{
  FILENODE *curr = *list, *prev = NULL;
  
  if (*list == NULL || (*list)->mtime > node->mtime ||
     ((*list)->mtime == node->mtime && (*list)->fileid > node->fileid)) {
    node->next = *list;
    *list = node;
    return S_OK;
  }
  
  do {
    prev = curr;
    curr = curr->next;
  } while (curr && (curr->mtime < node->mtime ||
                (curr->mtime == node->mtime && curr->fileid < node->fileid)));

  node->next = curr;
  prev->next = node;
  return S_OK;
}

build_filelist(dir, list, fn, arg)
char *dir;
FILELIST *list;
int (*fn)();
void *arg;
{
  DIR *dp;
  struct dirent *dent;
  PATH fname;
  char *fnamebase;
  struct stat stbuf;
  FILENODE *node;
  SHORT fileid;

  free_filelist(list);

  strcpy(fname, dir);
  strcat(fname, "/");
  fnamebase = fname+strlen(fname);

  if ((dp = opendir(dir)) == NULL) {
    return S_SYSERR;
  }
              
  while ((dent = readdir(dp)) != NULL) {
    strcpy(fnamebase, dent->d_name);
    fileid = hex2SHORT(dent->d_name);
    if (fileid > 0 && fileid <= BBS_MAX_FILES && stat(fname, &stbuf) == 0) {
      if (stbuf.st_size == 0) continue;
      else if ((node = (FILENODE *)malloc(sizeof(*node))) != NULL) {
        node->fileid = fileid;
        node->flags = FILE_UNREAD;
        if ((stbuf.st_mode & 0777) == MARKED_FILE_MODE)
	  node->flags |= FILE_MARKED;
        node->size = stbuf.st_size;
        node->mtime = stbuf.st_mtime;        
        insert_filelist(list, node);
        if (fn) fn(node, arg);
      }
    }
  }

  closedir(dp);
  return S_OK;
}

_enum_files(dir, fn, arg)
char *dir;
int (*fn)();
void *arg;
{
  PATH fname;
  DIR *dp;
  struct dirent *dent;
  int indx = 0;
  int isdir;

  if ((dp = opendir(dir)) == NULL) {
    return S_SYSERR;
  }
              
  while ((dent = readdir(dp)) != NULL) {
    isdir = 0;
    if (dent->d_name[0] == '.') {
      if (strcmp(dent->d_name, "..") || !strcmp(dir, _currbrd.bdir))
        continue;
    }
    strcpy(fname, dir);
    strcat(fname, "/");
    strcat(fname, dent->d_name);
    if (is_directory(fname)) isdir++;
    if (fn(indx++, dir, dent->d_name, isdir, arg) == ENUM_QUIT) break;
  }

  closedir(dp);
  return S_OK;
}

FILENODE *
_bcache_find(fileid)
SHORT fileid;
{
  FILENODE *trav = _currbrd.bcache;
  while (trav) {
    if (trav->fileid == fileid) break;
    trav = trav->next;
  }
  return trav;
}    

_msg_tally(node, newinfo)
FILENODE *node;
READINFO *newinfo;
{
  _currbrd.oi.totalmsgs++;
  if ((node->mtime < (time_t)_currbrd.ri.stamp) &&
      test_readbit(&_currbrd.ri, node->fileid)) {
    set_readbit(newinfo, node->fileid);
    node->flags &= ~FILE_UNREAD;
  }
  else _currbrd.oi.newmsgs++;
  return S_OK;
}      

/*ARGSUSED*/
_file_tally(indx, dir, fname, isdir, arg)
int indx;
char *dir;
char *fname;
int isdir;
void *arg;
{
  _currbrd.oi.totalmsgs++;
  return S_OK;
}      

_sync_currbrd()
{
  struct stat stbuf;
  READINFO newinfo;
  if (stat(_currbrd.wdir, &stbuf)) {
    _currbrd.oi.totalmsgs = _currbrd.oi.newmsgs = 0;
    return S_SYSERR;
  }
  if (stbuf.st_mtime < _currbrd.bsync) {
    /* No files created or deleted, so we don't re-read */
    return S_OK;
  }
  _currbrd.oi.totalmsgs = _currbrd.oi.newmsgs = 0;
  switch (_currbrd.btype) {
  case BOARD_MAIL:
  case BOARD_POST:
    clear_all_readbits(&newinfo);
    build_filelist(_currbrd.bdir, &_currbrd.bcache, _msg_tally, &newinfo);
    time((time_t *)&newinfo.stamp);
    memcpy(&_currbrd.ri, &newinfo, sizeof(_currbrd.ri));
    break;
  case BOARD_FILE:
    _enum_files(_currbrd.wdir, _file_tally, NULL);
    break;
  default:
    return S_NOTOPEN;
  }
  time(&_currbrd.bsync);
  return S_OK;
}
  
get_filelist_ids(dir, rinfo)
char *dir;
READINFO *rinfo;
{
  DIR *dp;
  struct dirent *dent;
  SHORT fileid;

  clear_all_readbits(rinfo);

  if ((dp = opendir(dir)) == NULL) {
    return S_SYSERR;
  }
              
  while ((dent = readdir(dp)) != NULL) {
    fileid = hex2SHORT(dent->d_name);
    if (fileid > 0 && fileid <= BBS_MAX_FILES)
      set_readbit(rinfo, fileid);
  }

  closedir(dp);
  return S_OK;
}

fileid_to_fname(dir, fileid, fname)
char *dir;
SHORT fileid;
char *fname;
{
  char *eodir;
  strcpy(fname, dir);
  strcat(fname, "/");
  eodir = fname+strlen(fname);
  SHORTcpy(eodir, fileid);
  eodir[4] = '\0';
}

struct _sendmsgstruct {
  int btype;
  HEADER hdr;
  RNAME username;
  NAMELIST to_list;
  PATH destfile;
  char *srcfile;
  LONG errcode;
};

append_sig(fd)
int fd;
{
  FILE *fp;
  PATH sigfile;
  char buf[81];
  int i;
  /* 
     Append current user's sig to open file fd.
     Honor maxsiglines defined in the global SERVERDATA struct.
  */

  if (server.maxsiglines == 0) {
    return S_OK;
  }

  local_bbs_get_signature(sigfile);
  if ((fp = fopen(sigfile, "r")) != NULL) {
    for (i=0; i<server.maxsiglines; i++) {
      if (fgets(buf, sizeof(buf), fp)) {
        write(fd, buf, strlen(buf));
      }
    }      
    fclose(fp);
  }

  return S_OK;
}  

_do_message(indx, bname, sm)
int indx;
char *bname;
struct _sendmsgstruct *sm;
{
  PATH msgdir;
  PATH msgfile;
  READINFO readinfo;
  SHORT fileid;
  NAME goodbname;
  struct stat stbuf;
  int fd, ok = 0;
    
  if (indx >= BBS_MAX_MAILRECIPS) return S_OK;

  if (sm->btype == BOARD_MAIL) {
    ACCOUNT acct;
    /* If address has a : in it, assume it is going to an external mailer. */
    if (strchr(bname, ':') != NULL) {
      LONG rc;
      if (!_has_access(C_EXTERNMAIL)) rc = S_DENIED;
      else rc = mail_file_to_outside(sm->srcfile, sm->hdr.title, bname, 0, 0);
      if (rc != S_OK) {
        sm->hdr.size |= (1 << indx);
        sm->errcode = rc;
      }
      return S_OK;
    }    
    if (_lookup_account(bname, &acct) != S_OK) {
      sm->hdr.size |= (1 << indx);
      sm->errcode = S_NOSUCHUSER;
      return S_OK;
    }
    /* Deny if: account is marked shared, account has no mail permission,
       or mail is from outside and account has no external mail permission. */
    if (acct.flags & FLG_SHARED || 
        !_they_have_access(C_OPENMAIL, acct.perms) ||
        (strchr(sm->hdr.owner, ':') && 
         !_they_have_access(C_EXTERNMAIL, acct.perms))) {
      sm->hdr.size |= (1 << indx);
      sm->errcode = S_CANNOTMAIL;
      return S_OK;
    }
    strncpy(goodbname, acct.userid, sizeof(goodbname));    
    get_mail_directory(goodbname, msgdir);
  }
  else {
    BOARD board;
    if (_lookup_board(bname, &board) != S_OK) {
      sm->hdr.size |= (1 << indx);
      sm->errcode = S_NOSUCHBOARD;
      return S_OK;
    }
    strncpy(goodbname, board.name, sizeof(goodbname));
    get_board_directory(goodbname, msgdir);
  }

  if (stat(msgdir, &stbuf) == -1 || !S_ISDIR(stbuf.st_mode)) {
    /* A directory is missing! */
    bbslog(0, "ERROR _do_message: cannot access %s\n", msgdir);
    sm->hdr.size |= (1 << indx);
    sm->errcode = S_SYSERR;
    return S_OK;
  }
  get_filelist_ids(msgdir, &readinfo);

  for (fileid = 1; fileid <= BBS_MAX_FILES; fileid++) {
    if (test_readbit(&readinfo, fileid)) continue;
    fileid_to_fname(msgdir, fileid, msgfile);
    if (sm->destfile[0]) {
      /* Carbon copy! Hopefully we can just link. */
      if (link(sm->destfile, msgfile) == 0) {
        ok++;
        break;
      }
      /* Damn! Copy the stupid thing. */
      fd = open(msgfile, O_WRONLY|O_CREAT|O_EXCL, UNMARKED_FILE_MODE);
      if (fd != -1) {
        append_file(fd, sm->destfile);
	append_sig(fd);
        close(fd);
        ok++;
        break;
      }
    }
    else {
      /* First copy. We have to write the file with headers. */
      fd = open(msgfile, O_WRONLY|O_CREAT|O_EXCL, UNMARKED_FILE_MODE);
      if (fd != -1) {
        if (sm->btype == BOARD_MAIL)
          write_mail_headers(fd, &sm->hdr, sm->username, sm->to_list);
        else
          write_post_headers(fd, &sm->hdr, sm->username, goodbname);

        append_file(fd, sm->srcfile);
        append_sig(fd);
        close(fd);
        strncpy(sm->destfile, msgfile, PATHLEN);
        ok++;
        break;
      }
    }
  }
  
  if (ok) {
    if (sm->btype == BOARD_MAIL) notify_new_mail(bname, 1);
  }
  else {
    /* Too many files, I guess! */
    bbslog(0, "ERROR _do_message: cannot create message in %s\n", msgdir);
    sm->hdr.size |= (1 << indx);
    sm->errcode = S_BOARDFULL;
  }
  return S_OK;
}    
    
local_bbs_mail(from, fromname, to_list, subject, fname, success)
char *from;
char *fromname;
NAMELIST to_list;
char *subject;
char *fname;
LONG *success;
{
  struct _sendmsgstruct sm;
  
  sm.btype = BOARD_MAIL;
  memset(&sm.hdr, 0, sizeof sm.hdr);

  if (from == NULL) strncpy(sm.hdr.owner, my_userid(), sizeof sm.hdr.owner);
  else strncpy(sm.hdr.owner, from, sizeof sm.hdr.owner);
  if (fromname == NULL) strncpy(sm.username,my_username(),sizeof sm.username);
  else strncpy(sm.username, fromname, sizeof sm.username);
  strncpy(sm.hdr.title, subject, TITLELEN);
  sm.to_list = to_list;
  sm.srcfile = fname;
  sm.destfile[0] = '\0';

  apply_namelist(to_list, _do_message, &sm);
  
  /* how's this for a kludge? */
  *success = sm.hdr.size;
  return (*success == 0 ? S_OK : sm.errcode);
}

local_bbs_post(bname, subject, fname)
char *bname;
char *subject;
char *fname;
{
  struct _sendmsgstruct sm;
  
  sm.btype = BOARD_POST;
  memset(&sm.hdr, 0, sizeof sm.hdr);
  strncpy(sm.hdr.owner, my_userid(), sizeof sm.hdr.owner);
  strncpy(sm.username, my_username(), sizeof sm.username);
  strncpy(sm.hdr.title, subject, TITLELEN);
  sm.to_list = NULL;
  sm.srcfile = fname;
  sm.destfile[0] = '\0';
  _do_message(0, bname, &sm);
  bbslog(3, "POST '%s' in %s by %s\n", subject, bname, my_userid());
  return (sm.hdr.size == 0 ? S_OK : sm.errcode);
}

local_bbs_open_mailbox(oinfo)
OPENINFO *oinfo;
{
  if (_currbrd.btype != BOARD_NONE && _currbrd.btype != BOARD_MAIL) {
    return S_ALREADYOPEN;
  }
  if (_currbrd.btype == BOARD_MAIL) {
    _currbrd.oi.flags |= OPEN_REOPEN;
  }
  else {
    _currbrd.btype = BOARD_MAIL;
    strncpy(_currbrd.oi.name, my_userid(), sizeof _currbrd.oi.name);
    get_mail_directory(_currbrd.oi.name, _currbrd.bdir);
    strcpy(_currbrd.wdir, _currbrd.bdir);
    _currbrd.oi.flags = OPEN_POST | OPEN_MANAGE;
    get_bitfile_ent(MAIL_READBITS_NAME, &_currbrd.ri);
    _currbrd.bsync = 0;
  }
  _sync_currbrd();
  memcpy(oinfo, &_currbrd.oi, sizeof(*oinfo));
  return S_OK;
}  

local_bbs_open_board(bname, oinfo)
char *bname;
OPENINFO *oinfo;
{
  BOARD board;
  if (_currbrd.btype != BOARD_NONE && _currbrd.btype != BOARD_POST) {
    return S_ALREADYOPEN;
  }
  if (_currbrd.btype == BOARD_POST) {
    if (strcasecmp(_currbrd.oi.name, bname)) return S_ALREADYOPEN;
    _currbrd.oi.flags |= OPEN_REOPEN;
  }
  else {
    if (_lookup_board(bname, &board) != S_OK) return S_NOSUCHBOARD;
    if (!_has_read_access(&board)) return S_NOSUCHBOARD;
    _currbrd.btype = BOARD_POST;
    strcpy(_currbrd.oi.name, board.name);
    get_board_directory(_currbrd.oi.name, _currbrd.bdir);
    strcpy(_currbrd.wdir, _currbrd.bdir);
    _currbrd.oi.flags = 0;
    if (_has_post_access(&board)) _currbrd.oi.flags |= OPEN_POST;
    if (_has_manager_access(&board)) _currbrd.oi.flags |= OPEN_MANAGE;
    get_bitfile_ent(board.name, &_currbrd.ri);
    _currbrd.bsync = 0;
  }
  _sync_currbrd();
  memcpy(oinfo, &_currbrd.oi, sizeof(*oinfo));
  return S_OK;
}  

local_bbs_open_fileboard(bname, oinfo)
char *bname;
OPENINFO *oinfo;
{
  BOARD board;
  if (_currbrd.btype != BOARD_NONE && _currbrd.btype != BOARD_FILE) {
    return S_ALREADYOPEN;
  }
  if (_currbrd.btype == BOARD_FILE) {
    if (strcasecmp(_currbrd.oi.name, bname)) return S_ALREADYOPEN;
    _currbrd.oi.flags |= OPEN_REOPEN;
  }
  else {
    if (_lookup_ftpent(bname, &board) != S_OK) return S_NOSUCHBOARD;
    _currbrd.btype = BOARD_FILE;
    strcpy(_currbrd.oi.name, board.name);
    get_fileboard_directory(_currbrd.oi.name, _currbrd.bdir);
    strcpy(_currbrd.wdir, _currbrd.bdir);
    _currbrd.oi.flags = 0;
    _currbrd.bsync = 0;
  }
  _sync_currbrd();
  memcpy(oinfo, &_currbrd.oi, sizeof(*oinfo));
  return S_OK;
}  

fname_to_pathname(fname, buf)
char *fname;
char *buf;
{
  char *slash;
  strcpy(buf, _currbrd.wdir);
  if (!strcmp(fname, "..")) {
    slash = strrchr(buf, '/');
    if (slash != NULL && slash != buf) *slash = '\0';    
  }
  else {
    strcat(buf, "/");
    strcat(buf, fname);
  }
}

local_bbs_change_fileboard_dir(fname, oinfo)
char *fname;
OPENINFO *oinfo;
{
  BOARD board;
  PATH fullname;
  if (_currbrd.btype != BOARD_FILE) {
    return S_WRONGTYPE;
  }
  fname_to_pathname(fname, fullname);
  if (!is_directory(fullname)) return S_WRONGTYPE; /* should be S_NOTDIR */
  /* Should we change _currbrd.oi.name? Can't since it's a NAME */
  strcpy(_currbrd.wdir, fullname);
  _currbrd.oi.flags = 0;
  _currbrd.bsync = 0;
  _sync_currbrd();
  memcpy(oinfo, &_currbrd.oi, sizeof(*oinfo));
  return S_OK;
}  

local_bbs_test_board(bname, pflags)
char *bname;
SHORT *pflags;
{
  BOARD board;
  if (_lookup_board(bname, &board) != S_OK) return S_NOSUCHBOARD;
  if (!_has_read_access(&board)) return S_NOSUCHBOARD;
  *pflags = 0;
  if (_has_post_access(&board)) (*pflags) |= OPEN_POST;
  if (_has_manager_access(&board)) (*pflags) |= OPEN_MANAGE;
  return S_OK;
}  

local_bbs_close_board()
{
  free_filelist(&_currbrd.bcache);

  if (_currbrd.btype == BOARD_POST)
    set_bitfile_ent(_currbrd.oi.name, &_currbrd.ri);
  else if (_currbrd.btype == BOARD_MAIL)
    set_bitfile_ent(MAIL_READBITS_NAME, &_currbrd.ri);

  _currbrd.btype = BOARD_NONE;
  return S_OK;
}

struct fileenum {
  SHORT chunk;
  SHORT start;
  int (*fn)();
  void *arg;
};

_get_file_headers(indx, dir, fname, isdir, info)
int indx;
char *dir;
char *fname;
struct fileenum *info;
{
  struct stat stbuf;
  HEADER hdr;
  PATH path;
  strcpy(path, dir);
  strcat(path, "/");
  strcat(path, fname);
  if (indx < info->start) return S_OK;
  if (stat(path, &stbuf) || stbuf.st_size == 0) return S_OK;
  memset(&hdr, 0, sizeof hdr);
  strncpy(hdr.title, fname, sizeof(hdr.title));
  hdr.size = stbuf.st_size;
  hdr.flags = isdir ? FILE_DIRECTORY : (is_text_file(path) ? 0 : FILE_BINARY);
  return (info->fn(indx, &hdr, info->arg));
}

/*ARGSUSED*/
local_bbs_enum_headers(chunk, start, newonly, enumfn, arg)
SHORT chunk;
SHORT start;
SHORT newonly;
int (*enumfn)();
void *arg;
{
  FILENODE *node;
  PATH fname;
  HEADER hdr;
  SHORT indx;
  
  if (_currbrd.btype == BOARD_NONE) 
    return S_NOTOPEN;

  if (_currbrd.btype == BOARD_FILE) {
    /* this case is completely different -- blech */
    struct fileenum fe;
    fe.chunk = chunk;
    fe.start = start;
    fe.fn = enumfn;
    fe.arg = arg;
    return (_enum_files(_currbrd.wdir, _get_file_headers, &fe));
  }

  _sync_currbrd();  

  for (node=_currbrd.bcache, indx=0; node; node=node->next, indx++) {
    if (indx < start) continue;
    if (newonly && !(node->flags & FILE_UNREAD)) continue;
    fileid_to_fname(_currbrd.bdir, node->fileid, fname);
    hdr.fileid = node->fileid;
    hdr.flags = node->flags;
    if (!(_currbrd.oi.flags & OPEN_MANAGE)) hdr.flags &= ~FILE_MARKED;
    hdr.size = node->size;
    hdr.mtime = (LONG)node->mtime;
    read_headers(fname, &hdr);
    if (hdr.owner[0] == '\0') strcpy(hdr.owner, SYSOP_ACCOUNT);
    if (enumfn(indx, &hdr, arg) == ENUM_QUIT)
      break;
  }

  return S_OK;
}

local_bbs_read_message(fileid, fname)
SHORT fileid;
char *fname;
{
  FILENODE *node;
  PATH myfname;
  struct stat stbuf;
  mode_t mode;

  if (_currbrd.btype != BOARD_MAIL &&_currbrd.btype != BOARD_POST) 
    return S_WRONGTYPE;

  node = _bcache_find(fileid);
  fileid_to_fname(_currbrd.bdir, fileid, myfname);
  if (node == NULL || stat(myfname, &stbuf))
    return S_NOSUCHFILE;
  else node->flags &= ~FILE_UNREAD;

  if (!test_readbit(&_currbrd.ri, fileid)) {
    set_readbit(&_currbrd.ri, fileid);
    if (_currbrd.btype == BOARD_MAIL)
      notify_new_mail(_currbrd.oi.name, -1);
  }

  strcpy(fname, myfname);
  return S_OK;  
}

local_bbs_mark_message(fileid, mflag)
SHORT fileid;
SHORT mflag;
{
  FILENODE *node;
  PATH fname;
  struct stat stbuf;
  mode_t mode;

  if (_currbrd.btype != BOARD_POST)
    return S_WRONGTYPE;

  if (!(_currbrd.oi.flags & OPEN_MANAGE))
    return S_DENIED;

  node = _bcache_find(fileid);
  fileid_to_fname(_currbrd.bdir, fileid, fname);
  if (node == NULL || stat(fname, &stbuf))
    return S_NOSUCHFILE;

  if (mflag) {
    mode = MARKED_FILE_MODE;
    node->flags |= FILE_MARKED;
  }
  else {
    mode = UNMARKED_FILE_MODE;
    node->flags &= ~FILE_MARKED;
  }

  bbslog(2, "MARKMSG %s %d on %s by %s\n", mflag ? "ON" : "OFF", 
         fileid, _currbrd.oi.name, my_userid());

  return(chmod(fname, mode) ? S_SYSERR : S_OK);
}

local_bbs_delete_message(fileid)
SHORT fileid;
{
  FILENODE *node;
  PATH fname;
  HEADER hdr;
  int is_my_post;

  if (_currbrd.btype != BOARD_MAIL && _currbrd.btype != BOARD_POST) 
    return S_WRONGTYPE;

  node = _bcache_find(fileid);
  fileid_to_fname(_currbrd.bdir, fileid, fname);
  if (node == NULL) return S_NOSUCHFILE;

  read_headers(fname, &hdr);
  is_my_post = is_me(hdr.owner);

  if (!(_currbrd.oi.flags & OPEN_MANAGE)) {
    if (!is_my_post) return S_DENIED;
  }

  if (unlink(fname)) return S_SYSERR;

  if (_currbrd.btype == BOARD_MAIL && !test_readbit(&_currbrd.ri, fileid)) {
    notify_new_mail(_currbrd.oi.name, -1);
  }

  if (_currbrd.btype == BOARD_POST && !is_my_post)
    bbslog(2, "DELETEPOST '%s' (%s) on %s by %s\n", hdr.title, hdr.owner,
           _currbrd.oi.name, my_userid());

  return S_OK;
}

local_bbs_move_message(fileid, bname)
SHORT fileid;
char *bname;
{
  FILENODE *node;
  PATH oldmsgfile;
  HEADER hdr;
  int is_my_post;
  BOARD board;
  PATH newmsgdir, newmsgfile;
  SHORT newfileid;
  READINFO readinfo;
  struct stat stbuf;

  if (_currbrd.btype != BOARD_POST)
    return S_WRONGTYPE;

  /* See if the specified fileid exists and if so get the filename. */
  node = _bcache_find(fileid);
  fileid_to_fname(_currbrd.bdir, fileid, oldmsgfile);
  if (node == NULL) return S_NOSUCHFILE;

  read_headers(oldmsgfile, &hdr);
  is_my_post = is_me(hdr.owner);

  if (!(_currbrd.oi.flags & OPEN_MANAGE)) {
    if (!is_my_post) return S_DENIED;
  }

  if (_lookup_board(bname, &board) != S_OK) return S_NOSUCHBOARD;
  if (!_has_read_access(&board)) return S_NOSUCHBOARD;
  if (!_has_post_access(&board)) return S_DENIED;

  get_board_directory(board.name, newmsgdir);
  if (stat(newmsgdir, &stbuf) == -1 || !S_ISDIR(stbuf.st_mode)) {
    bbslog(0, "ERROR bbs_move_message: cannot access %s\n", newmsgdir);
    return S_SYSERR;
  }
  get_filelist_ids(newmsgdir, &readinfo);

  for (newfileid = 1; newfileid <= BBS_MAX_FILES; newfileid++) {
    if (test_readbit(&readinfo, newfileid)) continue;
    fileid_to_fname(newmsgdir, newfileid, newmsgfile);
    if (rename(oldmsgfile, newmsgfile)) return S_SYSERR;
    break;
  }

  bbslog(2, "MOVEPOST '%s' (%s) on %s to %s by %s\n", hdr.title, hdr.owner,
         _currbrd.oi.name, board.name, my_userid());

  return S_OK;
}

local_bbs_update_message(fileid, newfile)
SHORT fileid;
char *newfile;
{
  FILENODE *node;
  PATH fname;
  struct utimbuf utbuf;

  if (_currbrd.btype != BOARD_POST) 
    return S_WRONGTYPE;

  if (!(_currbrd.oi.flags & OPEN_MANAGE))
    return S_DENIED;
  
  node = _bcache_find(fileid);
  fileid_to_fname(_currbrd.bdir, fileid, fname);
  if (node == NULL) return S_NOSUCHFILE;

  /* Touch the message back to its original posting time */
  utbuf.actime = 0;
  utbuf.modtime = node->mtime;
  if (utime(fname, &utbuf)) return S_SYSERR;

  bbslog(2, "EDITMSG %d on %s by %s\n", fileid, _currbrd.oi.name, my_userid());
  return S_OK;
}

local_bbs_delete_range(start, finis, count)
SHORT start;
SHORT finis;
SHORT *count;
{
  FILENODE *node;
  PATH fname;
  SHORT indx = 0;

  if (_currbrd.btype != BOARD_MAIL && _currbrd.btype != BOARD_POST) 
    return S_WRONGTYPE;

  if (!(_currbrd.oi.flags & OPEN_MANAGE))
    return S_DENIED;

  *count = 0;
  for (node = _currbrd.bcache; node && (indx++ < finis); node = node->next) {
    if (indx < start) continue;
    if (node->flags & FILE_MARKED) continue;
    if (_currbrd.btype == BOARD_MAIL && (node->flags & FILE_UNREAD)) continue;
    fileid_to_fname(_currbrd.bdir, node->fileid, fname);
    if (unlink(fname) == 0) (*count)++;
  }

  if (_currbrd.btype == BOARD_POST)
    bbslog(2, "DELETERANGE %d-%d on %s by %s\n", start, finis, 
          _currbrd.oi.name, my_userid());

  return S_OK;
}

local_bbs_forward_message(fileid)
SHORT fileid;
{
  FILENODE *node;
  PATH fname;
  HEADER hdr;

  if (_currbrd.btype != BOARD_MAIL && _currbrd.btype != BOARD_POST) 
    return S_WRONGTYPE;
  
  node = _bcache_find(fileid);
  fileid_to_fname(_currbrd.bdir, fileid, fname);
  if (node == NULL) return S_NOSUCHFILE;
  read_headers(fname, &hdr);

  return (forward_file_to_outside(fname, hdr.title, 0));
}

local_bbs_download(fname, protoname, path)
char *fname;
char *protoname;
char *path;
{
  PATH fullname;
  FILE *fp;

  if (_currbrd.btype != BOARD_FILE) return S_WRONGTYPE;
  fname_to_pathname(fname, fullname);
  if (is_directory(fullname)) return S_WRONGTYPE; /* should be S_ISDIR */

  if ((fp = fopen(fullname, "r")) == NULL) return S_NOSUCHFILE;
  fclose(fp);

  if (protoname == NULL && path != NULL) {
    /* Special case -- just return the filename */
    strcpy(path, fullname);
    return S_OK;
  }
  else return (do_download(_currbrd.bdir, fname, protoname));
}

local_bbs_forward_file(fname)
char *fname;
{
  PATH fullname;
  TITLE title;

  if (_currbrd.btype != BOARD_FILE) return S_WRONGTYPE;
  fname_to_pathname(fname, fullname);
  if (is_directory(fullname)) return S_WRONGTYPE; /* should be S_ISDIR */
  sprintf(title, "%s: %s", _currbrd.oi.name, fname);
  return (forward_file_to_outside(fullname, title, !is_text_file(fullname)));
}

/* This is used by bbs_visit_board */

_mark_all_as_read(bname)
char *bname;
{
  if (_currbrd.btype == BOARD_POST && !strcmp(_currbrd.oi.name, bname)) {
    FILENODE *node;
    get_filelist_ids(_currbrd.bdir, &_currbrd.ri);
    time((time_t *)&_currbrd.ri.stamp);
    for (node = _currbrd.bcache; node != NULL; node = node->next) {
      node->flags &= ~FILE_UNREAD;
    }
    time(&_currbrd.bsync);
  }
  else {
    PATH bdir;
    READINFO ri;
    get_board_directory(bname, bdir);    
    get_filelist_ids(bdir, &ri);
    time((time_t *)&ri.stamp);
    set_bitfile_ent(bname, &ri);
  }
  return S_OK;
}  

/* This is used by bbs_enum_boards to do the counts */

_board_count(board, rinfo)
BOARD *board;
READINFO *rinfo;
{
  PATH bdir, fname;
  char *fnamebase;
  DIR *dp;
  struct dirent *dent;
  struct stat stbuf;
  SHORT fileid;

  board->totalposts = board->newposts = board->ownedposts = 0;
  board->lastpost = 0;
  
  get_board_directory(board->name, bdir);
  if ((dp = opendir(bdir)) == NULL) {
    return S_SYSERR;
  }
              
  strcpy(fname, bdir);
  strcat(fname, "/");
  fnamebase = fname+strlen(fname);

  while ((dent = readdir(dp)) != NULL) {
    strcpy(fnamebase, dent->d_name);
    fileid = hex2SHORT(dent->d_name);
    if (fileid > 0 && fileid <= BBS_MAX_FILES && stat(fname, &stbuf) == 0) {
      if (stbuf.st_size > 0) {
        (board->totalposts)++;
        if (!test_readbit(rinfo, fileid)) (board->newposts)++;
        if ((LONG)stbuf.st_mtime > board->lastpost)
          board->lastpost = (LONG)stbuf.st_mtime;
      }
      /* 
         To test and increment board->ownedposts, we'd have to read the 
         headers and compare the owner to our userid. In the interest of
         speed, I'm not doing that, for now. 
      */
    }
  }

  closedir(dp);
  return S_OK;
}
