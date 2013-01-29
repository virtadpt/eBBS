
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

extern USERDATA user_params;

extern char *optarg;

usage(prog)
char *prog;
{
  fprintf(stderr, 
	  "Usage: %s -i userid -p passwd [-u username] [-t termtype]\n", prog);
  fprintf(stderr,
	  "       [-e email] [-r realname] [-a address] [-d bbs-dir]\n");
}

main(argc, argv)
int argc;
char *argv[];
{
    char *bbshome = NULL;
    int iflg, pflg, uflg;
    int c;
    ACCOUNT acct;
    int rc;

    memset(&acct, 0, sizeof(acct));
    iflg = pflg = uflg = 0;

    while ((c = getopt(argc, argv, "a:d:e:i:p:r:t:u:?")) != -1)
      {
	switch (c)
	  {
	  case 'a':
	    strncpy(acct.address, optarg, sizeof acct.address-1);
	    break;
	  case 'd':
	    bbshome = optarg;
	    break;
	  case 'e':
	    strncpy(acct.email, optarg, sizeof acct.email-1);
	    break;
	  case 'i':
	    iflg++;
	    strncpy(acct.userid, optarg, sizeof acct.userid-1);
	    break;
	  case 'p':
	    pflg++;
	    strncpy(acct.passwd, optarg, sizeof acct.passwd-1);
	    break;
	  case 'r':
	    strncpy(acct.realname, optarg, sizeof acct.realname-1);
	    break;
	  case 't':
	    strncpy(acct.terminal, optarg, sizeof acct.terminal-1);
	    break;
	  case 'u':
	    uflg++;
	    strncpy(acct.username, optarg, sizeof acct.username-1);
	    break;
	  case '?':
	    usage(argv[0]);
	    return 2;
	  }
      }

    if (!iflg || !pflg) {
        fprintf(stderr, "%s: userid and passwd args must be given\n", argv[0]);
	return 1;
    }

    if (home_bbs(bbshome) == -1) {
        fprintf(stderr, "%s: Cannot chdir to %s\n", argv[0], bbshome);
        return 1;
    }

    if (local_bbs_initialize(NULL) != S_OK) {
        fprintf(stderr, "%s: local_bbs_initialize failed\n", argv[0]);
	return 1;
    }

    /* Identify ourself for the log file */
    strcpy(user_params.u.userid, "[addacct]");
    user_params.perms = ~0;

    if (!uflg) strncpy(acct.username, acct.userid, sizeof acct.username-1);

    rc = local_bbs_add_account(&acct, 0);
    switch (rc) {
    case S_BADUSERID:
    case S_BADPASSWD:
      fprintf(stderr, "%s: invalid userid or password\n", acct.userid);
      break;
    case S_USEREXISTS:
      fprintf(stderr, "%s: userid already exists\n", acct.userid);
      break;
    case S_SYSERR:
      fprintf(stderr, "%s: error creating account\n", acct.userid);
      fprintf(stderr, "Wrong -d argument or $BBSHOME?\n");
    }

    local_bbs_disconnect();

    return (rc == S_OK ? 0 : 1);
}    








