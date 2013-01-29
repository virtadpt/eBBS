
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

extern char *ExpandString __P((char *));

/* 
   List of available editors for the local client. 
   This stuff is completely moot for the remote client.
   bbs_set_editor is in login.c.
*/

#define EDITORFILE "etc/editors"

/* Struct for getting entries from the etc/editors file. */

typedef struct _EDITSTRUCT {
  NAME name;
  PATH bin;
  PATH envp;
} EDITSTRUCT;

editent_to_es(rec, es)
char *rec;
EDITSTRUCT *es;
{
  rec = _extract_quoted(rec, es->name, sizeof(es->name));
  rec = _extract_quoted(rec, es->bin, sizeof(es->bin));
  rec = _extract_quoted(rec, es->envp, sizeof(es->envp));
  return S_OK;
}

local_bbs_get_editor(name, path, envp)
char *name;
char *path;
char *envp;
{
  int rc;
  EDITSTRUCT es;
  memset(&es, 0, sizeof es);
  rc = _record_find(EDITORFILE, _match_first, name, editent_to_es, &es);
  if (rc == S_OK) {
    strncpy(path, es.bin, PATHLEN);
    strncpy(envp, ExpandString(es.envp), PATHLEN);
  }
  else rc = S_NOSUCHEDITOR;
  return rc;
}

/*ARGSUSED*/
_fill_editornames(indx, rec, lc)
int indx;
char *rec;
struct listcomplete *lc;
{
  EDITSTRUCT es;
  editent_to_es(rec, &es);
  if (!strncasecmp(es.name, lc->str, strlen(lc->str)))
    add_namelist(lc->listp, es.name, NULL);

  return S_OK;
}

local_bbs_enum_editors(list, complete)
NAMELIST *list;
char *complete;
{
  struct listcomplete lc;
  create_namelist(list);
  lc.listp = list;
  lc.str = complete == NULL ? "" : complete;
  _record_enumerate(EDITORFILE, 0, _fill_editornames, &lc);  
  return S_OK;
}


