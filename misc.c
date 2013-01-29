
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

/* Miscellaneous server functions. */

#define HOME        "home"

get_home_directory(userid, buf)
char *userid;
char *buf;
{
  strcpy(buf, HOME);
  strcat(buf, "/");
  strcat(buf, userid);
}

#define MAILDIR     "mail"

get_mail_directory(userid, buf)
char *userid;
char *buf;
{
  get_home_directory(userid, buf);
  strcat(buf, "/");
  strcat(buf, MAILDIR);
}

#define PLANFILE    "plan"

local_bbs_get_plan(userid, buf)
char *userid;
char *buf;
{
  get_home_directory(userid, buf);
  strcat(buf, "/");
  strcat(buf, PLANFILE);
  return S_OK;
}

#define SIGFILE     "signature"

local_bbs_get_signature(buf)
char *buf;
{
  get_home_directory(my_userid(), buf);
  strcat(buf, "/");
  strcat(buf, SIGFILE);
  return S_OK;
}

#define BOARDHOME  "boards"

get_board_directory(bname, buf)
char *bname;
char *buf;
{
  strcpy(buf, BOARDHOME);
  strcat(buf, "/");
  strcat(buf, bname);
  return S_OK;
}

#define ISSUEFILE   "etc/issue"

local_bbs_get_issue(buf)
char *buf;
{
  strcpy(buf, ISSUEFILE);
  return S_OK;
}

#define INFOFILE   "etc/info"

local_bbs_get_info(buf)
char *buf;
{
  strcpy(buf, INFOFILE);
  return S_OK;
}

#define GNUFILE    "etc/COPYING"

local_bbs_get_license(buf)
char *buf;
{
  strcpy(buf, GNUFILE);
  return S_OK;
}

#define WELCFILE   "etc/welcome"

local_bbs_get_welcome(buf)
char *buf;
{
  strcpy(buf, WELCFILE);
  return S_OK;
}

local_bbs_set_welcome(buf)
char *buf;
{
  int rc = 0;
  if (buf == NULL) {
    unlink(WELCFILE);
  }
  else if (strcmp(WELCFILE, buf)) {
    rc = copy_file(buf, WELCFILE, 0660, 0);
  }
  return (rc == 0 ? S_OK : S_SYSERR);
}
