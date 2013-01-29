#include "server.h"

extern char *optarg;
extern int optind;

usage(prog)
char *prog;
{
  fprintf(stderr, 
	  "Usage: %s [-d bbs-dir] [-h log-header] log-level message\n", prog);
}

main(argc, argv)
int argc;
char *argv[];
{
    char *bbshome = NULL;
    char *logheader = NULL;
    char *msg;
    int c;
    int loglevel, rc;

    while ((c = getopt(argc, argv, "d:h:?")) != -1)
      {
	switch (c)
	  {
	  case 'd':
	    bbshome = optarg;
	    break;
	  case 'h':
	    logheader = optarg;
	    break;
	  case '?':
	    usage(argv[0]);
	    return 2;
	  }
      }

    if (optind+2 > argc) {
        usage(argv[0]);
        return 2;
    }

    if (logheader == NULL) logheader = "BBSLOG";
    loglevel = atoi(argv[optind++]);
    msg = argv[optind];
    
    if (home_bbs(bbshome) == -1) {
        fprintf(stderr, "%s: Cannot chdir to %s\n", argv[0], bbshome);
        return 1;
    }

    if (local_bbs_initialize(NULL) != S_OK) {
        fprintf(stderr, "%s: local_bbs_initialize failed\n", argv[0]);
	return 1;
    }

    set_log_header(logheader);
    bbslog(loglevel, "%s\n", msg);    

    local_bbs_disconnect();

    return 0;
}    

