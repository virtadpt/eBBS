
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
#ifndef NO_LOCALE
# include <locale.h>
#endif
#if LACKS_MALLOC_H
# include <stdlib.h>
#else
# include <malloc.h>
#endif

int bbslib_user = BBSLIB_DEFAULT;

SERVERDATA server;
extern USERDATA user_params;

char *perm_strs[PERMBIT_MAX+1];
LONG perm_table[MAX_CLNTCMDS];

char *mode_strs[BBS_MAX_MODE+1];
char mode_chars[BBS_MAX_MODE+1];
char mode_pageable[BBS_MAX_MODE+1];

#define PERMSTRFILE	"etc/permstrs"
#define ACCESSFILE	"etc/access"
#define CONFIGFILE	"etc/bbconfig"
#define MODEFILE        "etc/modes"

/*ARGSUSED*/
_init_modes_func(indx, rec, arg)
int indx;
char *rec;
void *arg;
{
  /*
     Modes file format:
     mode-val:mode-char:mode-string:allow-page
  */
  char buf[10];
  int mode, len;
  strip_trailing_space(rec);
  rec = _extract_quoted(rec, buf, sizeof(buf));
  mode = atoi(buf);
  if (mode >= 0 && mode <= BBS_MAX_MODE && mode_strs[mode] == NULL) {
    rec = _extract_quoted(rec, buf, sizeof(buf));
    mode_chars[mode] = *buf;
    rec = _extract_quoted(rec, buf, sizeof(buf));
    len = strlen(buf)+1;
    if ((mode_strs[mode] = (char *)malloc(len)) == NULL) return ENUM_QUIT;
    strcpy(mode_strs[mode], buf);  
    if (*rec == 'N' || *rec == 'n')
       mode_pageable[mode] = 'N';
  }  
  return S_OK;
}

init_mode_strs_chars()
{
  int count;
  _record_enumerate(MODEFILE, 0, _init_modes_func, NULL);
  return S_OK;
}

/*ARGSUSED*/
_init_strs_func(indx, rec, arg)
int indx;
char *rec;
void *arg;
{
  int len;
  strip_trailing_space(rec);
  len = strlen(rec)+1;
  if ((perm_strs[indx] = (char *)malloc(len)) == NULL) return ENUM_QUIT;
  strcpy(perm_strs[indx], rec);  
  return (indx == PERMBIT_MAX ? ENUM_QUIT : S_OK);
}

init_perm_strs()
{
  int count;
  count = _record_enumerate(PERMSTRFILE, 0, _init_strs_func, NULL);
  for (; count<=PERMBIT_MAX; count++) {
    if ((perm_strs[count] = (char *)malloc(9)) != NULL)
      strcpy(perm_strs[count], "(unused)");      
  }
  return S_OK;
}

/*ARGSUSED*/
_init_perms_func(indx, rec, arg)
int indx;
char *rec;
void *arg;
{
  char *str, *equals;
  int i;
  perm_table[indx] = 0;
  strip_trailing_space(rec);
  if ((equals = strchr(rec, '=')) != NULL) {
    equals++;
    while (str = strtok(equals, " \t,")) {
      if (!strcmp(str, "ALL")) {
        perm_table[indx] = PERM_ALL;
        break;
      }
      for (i=0; i<=PERMBIT_MAX; i++) {
        if (perm_strs[i] && !strcmp(perm_strs[i], str))
          perm_table[indx] |= PERMBIT(i);
      }
      equals = NULL;
    }
  }
  return (indx == MAX_CLNTCMDS-1 ? ENUM_QUIT : S_OK);
}

init_perms()
{
  int count;
  count = _record_enumerate(ACCESSFILE, 0, _init_perms_func, NULL);
  for (; count<MAX_CLNTCMDS; count++) {
    perm_table[count] = 0;
  }
  return S_OK;
}

/*ARGSUSED*/
_init_config_func(indx, rec, arg)
int indx;
char *rec;
void *arg;
{
  char *equals;
  int i;
  strip_trailing_space(rec);

  if ((equals = strchr(rec, '=')) == NULL) return S_OK;
  *equals++ = '\0';
  strip_trailing_space(rec);
  strip_trailing_space(equals);
  while (*rec && isspace(*rec)) rec++;
  while (*equals && isspace(*equals)) equals++;

  if (!strcasecmp(rec, "new")) {
    if (toupper(*equals) == 'Y') server.newok = 1;
  }
  else if (!strcasecmp(rec, "users")) {
    i = atoi(equals);
    if (i > 0) server.maxusers = i;
  }
  else if (!strcasecmp(rec, "reservedslots")) {
    i = atoi(equals);
    if (i > 0) server.reservedslots = i;
  }
  else if (!strcasecmp(rec, "usertablesize")) {
    i = atoi(equals);
    if (i > 0) server.maxutable = i;
  }
  else if (!strcasecmp(rec, "name")) {
    strncpy(server.name, equals, BBSNAMELEN);
  }
  else if (!strcasecmp(rec, "logfile")) {
    strncpy(server.logfile, equals, PATHLEN);
  }
  else if (!strcasecmp(rec, "loglevel")) {
    i = atoi(equals);
    if (i >= 0 && i < LOG_LEVEL_MAX) server.loglevel = i;
  }
  else if (!strcasecmp(rec, "encoder")) {
    strncpy(server.encodebin, equals, PATHLEN);
  }
  else if (!strcasecmp(rec, "locale")) {
    strncpy(server.locale, equals, PATHLEN);
  }
  else if (!strcasecmp(rec, "logons")) {
    i = atoi(equals);
    if (i >= 0) server.maxlogons = i;
  }
  else if (!strcasecmp(rec, "siglines")) {
    i = atoi(equals);
    if (i >= 0) server.maxsiglines = i;
  }
  else if (!strcasecmp(rec, "showreal")) {
    if (toupper(*equals) == 'Y') server.queryreal = 1;
  }
  else if (!strcasecmp(rec, "timeout")) {
    i = atoi(equals);
    if (i >= 0) server.idletimeout = i;
  }
  return S_OK;
}

init_config()
{
  _record_enumerate(CONFIGFILE, 0, _init_config_func, NULL);
  return S_OK;
}

_determine_access(mask, access)
LONG mask;
char *access;
{
  int i;
  memset(access, '0', MAX_CLNTCMDS);
  for (i=0; i<MAX_CLNTCMDS; i++) {
    if (mask == 0) access[i] = (perm_table[i] == PERM_ALL ? '1' : '0');
    else access[i] = (perm_table[i] & mask ? '1' : '0');
  }
  return S_OK;
}  

_has_access(cmd)
LONG cmd;
{
  if (cmd < 0 || cmd >= MAX_CLNTCMDS) return 0;
  return(user_params.access[cmd] == '1');
}

_they_have_access(cmd, mask)
LONG cmd;
LONG mask;
{
  if (cmd < 0 || cmd >= MAX_CLNTCMDS) return 0;
  if (mask == 0) return(perm_table[cmd] == PERM_ALL);
  else return(perm_table[cmd] & mask);
}

local_bbs_initialize(initinfo)
INITINFO *initinfo;
{
  char loghdr[16];
  char *ident;
  char *tmpdir;
  FILE *fp;
  if ((fp = fopen(CONFIGFILE, "r")) == NULL) {
    /* We're in the wrong directory, probably. */
    return S_NOCONFIGFILE;
  }
  fclose(fp);
  if (initinfo != NULL && initinfo->tmpdir != NULL) 
    tmpdir = initinfo->tmpdir;
  else tmpdir = "tmp";
  umask(0007);
  init_mode_strs_chars();
  init_perm_strs();
  init_perms();
  server.newok = 0;
  server.loglevel = LOG_LEVEL_DEFAULT;
  server.maxusers = 80;
  server.reservedslots = 0;
  server.maxutable = 80;
  server.maxlogons = 0;
  server.maxsiglines = 4;
  server.queryreal = 0;
  server.idletimeout = 0;
  strcpy(server.name, "The Unknown BBS");
  strcpy(server.logfile, "log");
  sprintf(server.tempfile, "%s/bbs%05d", tmpdir, getpid());
  strcpy(server.encodebin, "/usr/bin/uuencode");
  strcpy(server.locale, "");
  init_config();
  if (server.maxutable < server.maxusers) 
    server.maxutable = server.maxusers;
  open_bbslog(server.logfile, server.loglevel);
  ident = (bbslib_user == BBSLIB_BBSD ? "BBSD" : "LOCAL");
  sprintf(loghdr, "%s(%05d)", ident, getpid());
  set_log_header(loghdr);
  if (utable_attach(server.maxusers) != S_OK) {
    close_bbslog();    
    return S_SYSERR;
  }
  return S_OK;
}

local_bbs_disconnect()
{
  local_logout();
  close_bbslog();
  unlink(server.tempfile);
  utable_detach(0);
  return S_OK;
}
  
/*ARGSUSED*/
local_bbs_connect(host, port, bbsinfo)
char *host;
SHORT port;
BBSINFO *bbsinfo;
{
  strcpy(bbsinfo->boardname, server.name);
  bbsinfo->majver = SERVER_VERSION_MAJ;
  bbsinfo->minver = SERVER_VERSION_MIN;
  bbsinfo->newok = server.newok;
#ifndef NO_LOCALE
  setlocale(LC_COLLATE, server.locale); /* XXX a error message upon fail? */
  setlocale(LC_CTYPE, server.locale);   /* XXX a error message upon fail? */
#endif

  return S_OK;
}

local_bbs_get_permstrings(ppstrs)
char **ppstrs;
{
  int i;
  for (i=0; i<=PERMBIT_MAX; i++) {
    ppstrs[i] = perm_strs[i];
  }
  return S_OK;
}

local_bbs_get_modestrings(pmstrs)
char **pmstrs;
{
  int i;
  for (i=0; i<=BBS_MAX_MODE; i++) {
    pmstrs[i] = (mode_strs[i] == NULL ? "" : mode_strs[i]);
  }
  return S_OK;
}

local_bbs_get_modechars(mchars)
char *mchars;
{
  int i;
  for (i=0; i<=BBS_MAX_MODE; i++) {
    mchars[i] = (mode_chars[i] == '\0' ? ' ': mode_chars[i]);
  }
  return S_OK;
}
