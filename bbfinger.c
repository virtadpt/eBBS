#include "server.h"

extern char *optarg;
extern int optind;

extern USERDATA user_params;
extern SERVERDATA server;

extern char *ModeToString __P((SHORT));

usage(prog)
char *prog;
{
  fprintf(stderr, 
	  "Usage: %s [-d bbs-dir]\n", prog);
}

/*ARGSUSED*/
one_line_display(indx, urec, count)
int indx;
USEREC *urec;
int *count;
{
  printf("%-12s    %-25s %-25s   %s\n", urec->userid, 
	 urec->username, urec->fromhost, 
         ModeToString(urec->mode));

  (*count)++;
  return S_OK;
}

main(argc, argv)
int argc;
char *argv[];
{
    char *bbshome = NULL;
    int c, count = 0;

    while ((c = getopt(argc, argv, "d:?")) != -1)
      {
	switch (c)
	  {
	  case 'd':
	    bbshome = optarg;
	    break;
	  case '?':
	    usage(argv[0]);
	    return 2;
	  }
      }

    if (home_bbs(bbshome) == -1) {
        fprintf(stderr, "%s: Cannot chdir to %s\n", argv[0], bbshome);
        return 1;
    }

    if (local_bbs_initialize(NULL) != S_OK) {
        fprintf(stderr, "%s: local_bbs_initialize failed\n", argv[0]);
	return 1;
    }

    /* Identify ourself for the log file, just in case */
    strcpy(user_params.u.userid, "[bbfinger]");
    /* assume lowest possible priviliges */    
    user_params.perms = 0;

    /* Do it! */
    printf("[%s]\n\n", server.name);
    printf("%-12s    %-25s %-25s   %s\n", 
	   "User Id", "User Name", "From", "Mode");
    local_bbs_enum_users(50, 0, NULL, one_line_display, &count);
    printf("\n%d %s displayed\n\n", count, count==1?"user":"users");

    local_bbs_disconnect();

    return 0;
}    

