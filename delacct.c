
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
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>

extern char *optarg;
extern int optind;

extern USERDATA user_params;

struct userclean {
  time_t cutoff;
  time_t age;
  int inactives;
  int nologins;
  int for_grins;
};

do_delete(userid, really)
char *userid;
int really;
{
  int rc;
  if (really) {
    rc = local_bbs_delete_account(userid);
    if (rc == S_OK) printf("DELETED  %s\n", userid);
    else printf("ERROR %d deleting %s\n", rc, userid);
  }
  else printf("would have DELETED  %s\n", userid);
}

user_clean_func(indx, userid, info)
int indx;
char *userid;
struct userclean *info;
{
  ACCOUNT acct;
  time_t lastlog;
  
  if (local_bbs_get_userinfo(userid, &acct) != S_OK) return 0;

  if (info->inactives) {
    if (acct.flags & FLG_EXEMPT) {
      return 0;
    }
    if (acct.perms & PERM_SYSOP) {
      return 0;
    }
  }

  if (acct.lastlogin == 0) {
    /* Never logged in, so go by creation time of home directory */
    /* I wish I was saving creation time somewhere besides the log */
    PATH buf;
    struct stat stbuf;
    get_home_directory(acct.userid, buf);
    if (stat(buf, &stbuf) == 0) acct.lastlogin = (LONG)stbuf.st_mtime;
  }
  else {
    if (info->nologins) return 0;
  }

  if (!info->inactives || acct.lastlogin + info->age < info->cutoff) {
    do_delete(acct.userid, !info->for_grins);
  }

  return 0;
}

usage(prog)
char *prog;
{
  fprintf(stderr,
    "Usage: %s [-a age] [-c] [-d bbsdir] [-n] [-t] userid ...\n", prog);
  fprintf(stderr,
    "       -c means clean all inactive accounts (subject to age)\n");
  fprintf(stderr,
    "       -n means only clean accounts that have never been used\n");
  fprintf(stderr,
    "       -t is test mode: only show what would happen\n");
}

main(argc, argv)
int argc;
char *argv[];
{
    struct userclean info;
    char *homedir = NULL;
    NAMELIST names;
    int c, cflg = 0;

    info.age = 30;          /* 30 days default */
    info.for_grins = 0;
    info.nologins = 0;
    info.inactives = 0;

    /* I should copy the passfile somewhere for safety */

    while ((c = getopt(argc, argv, "a:cd:nt?")) != -1)
      {
	switch (c)
	  {
	  case 'a':
	    info.age = atoi(optarg);
	    break;
	  case 'c':
            cflg++;
            break;
	  case 'd':
            homedir = optarg;
            break;
	  case 'n':
	    info.nologins++;
	    break;
	  case 't':
	    info.for_grins++;
	    break;
      	  case '?':
	    usage(argv[0]);
	    return 2;
	  }
      }
    
    /* Just a safety feature, delete it if you wish */
    if (info.age < 7) {
      fprintf(stderr, "%s: age parameter must be at least 7\n", argv[0]);
      return 1;
    }

    if (home_bbs(homedir) == -1) {
      fprintf(stderr, "%s: Cannot chdir to %s\n", argv[0], homedir);
      return -1;
    }

    if (local_bbs_initialize(NULL) != S_OK) {
        fprintf(stderr, "%s: local_bbs_initialize failed\n", argv[0]);
	return 1;
    }

    /* Identify ourself for the log file */
    strcpy(user_params.u.userid, "[delacct]");
    user_params.perms = ~0;
    user_params.access[C_SEEALLAINFO] = '1';

    names = NULL;
    while (optind < argc) {
      add_namelist(&names, argv[optind], NULL);
      optind++;
    }
  
    if (names == NULL) {
      if (!cflg) {
        fprintf(stderr, "%s: -c not specified and no names given\n", argv[0]);
	local_bbs_disconnect();
        return 2;
      }
      /* Delete all inactive accounts */
      info.inactives++;
      local_bbs_acctnames(&names, NULL);
    }

    time(&info.cutoff);
    info.age *= 86400;   /* convert to seconds */

    apply_namelist(names, user_clean_func, &info);
    
    free_namelist(&names);
    local_bbs_disconnect();
    return 0;
}

