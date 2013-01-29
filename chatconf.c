 
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

#define CHATCONFIGFILE "etc/chatconfig"

/*
   This is a piece of the chat server; separated because chatserv.c
   is too damn big already.
*/

extern NAMELIST manager_list;
extern NAMELIST restricted_list;
extern NAME mainroom;
extern int no_punct_in_chatids;

_chat_config_form_list(list, str)
NAMELIST *list;
char *str;
{
  char *userid;  
  while ((userid = strtok(str, ", \t")) != NULL) {
    str = NULL;
    add_namelist(list, userid, NULL);
  }
  return 0;
}

/*ARGSUSED*/
_chat_init_config_func(indx, rec, arg)
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

  if (!strcasecmp(rec, "mainroom")) {
    if (*equals != '\0') strncpy(mainroom, equals, NAMELEN);
  }
  else if (!strcasecmp(rec, "operators")) {
    _chat_config_form_list(&manager_list, equals);
  }
  else if (!strcasecmp(rec, "restricted")) {
    _chat_config_form_list(&restricted_list, equals); 
  }
  else if (!strcasecmp(rec, "nopunct")) {
    no_punct_in_chatids = (*equals == 'Y' || *equals == 'y');
  }
  return S_OK;
}

chat_init_config()
{
  _record_enumerate(CHATCONFIGFILE, 0, _chat_init_config_func, NULL);
  return S_OK;
}

chat_get_ignore_file(userid, buf)
char *userid;
char *buf;
{
  get_home_directory(userid, buf);
  strcat(buf, "/");
  strcat(buf, "chatignores");
}
  
