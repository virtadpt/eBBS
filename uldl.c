
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
#include <unistd.h>

#define PROTOFILE "etc/protos"

/* 
   The PROTOFILE lists the available protocols for uploading/downloading.
   Format is:

   protoname:send-binary:receive-binary
*/

typedef struct _PROTOCOL {
  NAME name;
  PATH sendbin;
  PATH recvbin;
  SHORT flags;
} PROTOCOL;

#define PROTO_FLAG_RECV_PIPE     0x001
#define PROTO_FLAG_RECV_FILENAME 0x002

protoent_to_proto(rec, proto)
char *rec;
PROTOCOL *proto;
{
  proto->flags = PROTO_FLAG_RECV_FILENAME;
  rec = _extract_quoted(rec, proto->name, sizeof(proto->name));
  rec = _extract_quoted(rec, proto->sendbin, sizeof proto->sendbin);
  if (*rec == '|') {
    proto->flags = PROTO_FLAG_RECV_PIPE;
    rec++;
  }    
  else if (*rec == '-') {
    proto->flags = 0;
    rec++;
  }
  _extract_quoted(rec, proto->recvbin, sizeof(proto->recvbin));
  return S_OK;
}

_lookup_protocol(pname, proto)
char *pname;
PROTOCOL *proto;
{
  int rc;
  memset(proto, 0, sizeof *proto);
  rc = _record_find(PROTOFILE, _match_first, pname, protoent_to_proto, proto);
  return rc;
}

/*ARGSUSED*/
_fill_protonames(indx, rec, lc)
int indx;
char *rec;
struct listcomplete *lc;
{
  PROTOCOL proto;
  protoent_to_proto(rec, &proto);
  if (!strncasecmp(proto.name, lc->str, strlen(lc->str)))
    add_namelist(lc->listp, proto.name, NULL);
 
  return S_OK;
}

local_bbs_protonames(list, complete)
NAMELIST *list;
char *complete;
{
  struct listcomplete lc;
  create_namelist(list);
  lc.listp = list;
  lc.str = complete == NULL ? "" : complete;
  _record_enumerate(PROTOFILE, 0, _fill_protonames, &lc);  
  return S_OK;
}

#define ULDLFILE  "etc/ftplist"

/* 
   The ULDLFILE lists the directories available for upload/download.
   Format, for now, is:

   boardname:directory:description
*/

#define UPLOADBOARD "UPLOAD"

/*
   This is a special entry in the ULDLFILE which, if present, specifies
   the directory for uploads to go to. Downloading from it is not allowed.
*/

ftpent_to_board(rec, board)
char *rec;
BOARD *board;
{
  PATH bdir;  /* thrown away in this function */
  rec = _extract_quoted(rec, board->name, sizeof(board->name));
  rec = _extract_quoted(rec, bdir, sizeof bdir);
  rec = _extract_quoted(rec, board->description, sizeof(board->description));
  return S_OK;
}

ftpent_to_dir(rec, dir)
char *rec;
char *dir;
{
  NAME bname;  /* thrown away in this function */
  rec = _extract_quoted(rec, bname, sizeof(bname));
  rec = _extract_quoted(rec, dir, PATHLEN+1);
  return S_OK;
}

_lookup_ftpent(bname, board)
char *bname;
BOARD *board;
{
  int rc;
  memset(board, 0, sizeof *board);
  rc = _record_find(ULDLFILE, _match_first, bname, ftpent_to_board, board);
  if (!strcmp(board->name, UPLOADBOARD)) return S_NOSUCHBOARD;

  return rc;
}

get_fileboard_directory(bname, dir)
char *bname;
char *dir;
{
  int rc;
  dir[0] = '\0';
  rc = _record_find(ULDLFILE, _match_first, bname, ftpent_to_dir, dir);
  return rc;
}

_enum_fileboards(indx, rec, en)
int indx;
char *rec;
struct enumstruct *en;
{
  BOARD board;
  memset(&board, '\0', sizeof board);
  ftpent_to_board(rec, &board);
  if (!strcmp(board.name, UPLOADBOARD)) return S_OK;
  if (en->fn(indx, &board, en->arg) == ENUM_QUIT) return ENUM_QUIT;
  return S_OK;
}

/*ARGSUSED*/
local_bbs_enum_fileboards(chunk, startrec, enumfn, arg)
SHORT chunk;
SHORT startrec;
int (*enumfn)();
void *arg;
{
  struct enumstruct en;
  en.fn = enumfn;
  en.arg = arg;
  _record_enumerate(ULDLFILE, startrec, _enum_fileboards, &en);
  return S_OK;
}

_fill_fileboardnames(indx, rec, lc)
int indx;
char *rec;
struct listcomplete *lc;
{
  BOARD board;
  ftpent_to_board(rec, &board);
  if (strcmp(board.name, UPLOADBOARD)) {
    if (!strncasecmp(board.name, lc->str, strlen(lc->str)))
      add_namelist(lc->listp, board.name, NULL);
  }
  return S_OK;
}

local_bbs_fileboardnames(list, complete)
NAMELIST *list;
char *complete;
{
  struct listcomplete lc;
  create_namelist(list);
  lc.listp = list;
  lc.str = complete == NULL ? "" : complete;
  _record_enumerate(ULDLFILE, 0, _fill_fileboardnames, &lc);  
  return S_OK;
}

_is_valid_fname(fname)
char *fname;
{
  /* To make sure no one tries to pull hanky panky, like asking to download
     /etc/passwd...*/

  if (strchr(fname, '/')) return 0;
  if (fname[0] == '\0' || fname[0] == '.') return 0;
  return 1;
}

local_bbs_upload(path, fname, protoname)
char *path;
char *fname;
char *protoname;
{
  int rc;
  char *outf = NULL;
  PROTOCOL proto;
  PATH execbuf;
  PATH upldir;

  if (get_fileboard_directory(UPLOADBOARD, upldir) != S_OK) {
    return S_NOUPLOAD;
  }

  if (protoname == NULL && path != NULL) {
    /* Special case -- just return the filename */
    strcpy(path, upldir);
    strcat(path, "/");
    strcat(path, fname);
    return S_OK;
  }

  if (_lookup_protocol(protoname, &proto) != S_OK)
    return S_NOSUCHPROTO;

  if (proto.recvbin[0] == '\0')
    return S_PROTONOGOOD;

  if (!_is_valid_fname(fname)) return S_BADFILENAME;

  strcpy(execbuf, proto.recvbin);
  if (proto.flags & PROTO_FLAG_RECV_FILENAME) {
    strcat(execbuf, " ");
    strcat(execbuf, fname);
  }

  if (proto.flags & PROTO_FLAG_RECV_PIPE) {
    outf = fname;
  }

  bbslog(3, "UPLOAD %s proto %s by %s\n", fname, proto.name, my_userid());

  set_real_mode(M_ULDL);
  rc = execute(execbuf, upldir, NULL, outf, NULL, NULL, 0);
  set_real_mode(M_UNDEFINED);
      
  return (rc == -1 ? S_SYSERR : S_OK);
}

do_download(dir, fname, protoname)
char *dir;
char *fname;
char *protoname;
{
  int rc;
  PROTOCOL proto;
  PATH execbuf;

  if (_lookup_protocol(protoname, &proto) != S_OK)
    return S_NOSUCHPROTO;

  if (proto.sendbin[0] == '\0')
    return S_PROTONOGOOD;

  if (!_is_valid_fname(fname)) return S_BADFILENAME;

  strcpy(execbuf, proto.sendbin);
  strcat(execbuf, " ");
  strcat(execbuf, fname);

  bbslog(3, "DOWNLOAD %s proto %s by %s\n", fname, proto.name, my_userid());

  set_real_mode(M_ULDL);
  rc = execute(execbuf, dir, NULL, NULL, NULL, NULL, 0);
  set_real_mode(M_UNDEFINED);
      
  return (rc == -1 ? S_SYSERR : S_OK);
}
