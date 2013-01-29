#include "server.h"
#include <pwd.h>

/* Header types, so we can deal with continuations */
#define HEADER_NONE    0
#define HEADER_OTHER   1
#define HEADER_SUBJECT 2
#define HEADER_FROM    3
#define HEADER_REPLYTO 4

extern USERDATA user_params;
extern SERVERDATA server;

extern char *optarg;
extern int optind;

/* Are we bbsmail or bbsmaild? Assume bbsmaild */
int daemon = 1;

/* Information we need from the headers */
ADDR sender;
TITLE subject;
RNAME sendername;

/* sender, without the prefix */
char *sptr;
int ssize;

char *
identify_header(str, hdr_type)
char *str;
int *hdr_type;
{
    char *cp, *colon;
    colon = strchr(str, ':');
    if (colon == NULL) {
      *hdr_type = HEADER_NONE;
      return str;
    }    
    *colon = '\0';
    for (cp = str; cp != colon; cp++) {
      if (*cp == ' ' || *cp == '\t') {
        *hdr_type = HEADER_NONE;
        return str;
      }    
    }

    if (!strcasecmp(str, "Subject")) *hdr_type = HEADER_SUBJECT;
    else if (!strcasecmp(str, "From")) *hdr_type = HEADER_FROM;
    else if (!strcasecmp(str, "Reply-To")) *hdr_type = HEADER_REPLYTO;
    else *hdr_type = HEADER_OTHER;

    for (cp = colon+1; *cp == ' ' || *cp == '\t'; cp++) ;    
    return cp;
}

split_sender(type, str)
int type;
char *str;
{
    /* 
        This splits a From: or Reply-To: header into the address and 
        full name parts, according to RFC 822. Give the Reply-To precedence.
    */
    char *cp, *sp;
    cp = strchr(str, '<');
    if (cp == NULL) {
        /* Format should be "address (Real Name)" */
        cp = strchr(str, ' ');
        if (cp != NULL) *cp++ = '\0';
        if (type == HEADER_REPLYTO || *sptr == '\0')
            strncpy(sptr, str, ssize);
        if (cp != NULL && *cp == '(') {
            sp = strchr(cp, ')');
            if (sp != NULL) {
                *sp = '\0';
                if (type == HEADER_REPLYTO || sendername[0] == '\0')
                    strncpy(sendername, cp, sizeof sendername);
            }
        }            
    }
    else {
        /* Format should be "Real Name <address>" */
        if (cp > str) {
            *(cp-1) = '\0';
            if (type == HEADER_REPLYTO || sendername[0] == '\0')
                strncpy(sendername, str, sizeof sendername);
	}
        sp = strchr(cp, '>');
        if (sp != NULL) {
            *sp = '\0';
            if (type == HEADER_REPLYTO || *sptr == '\0')
                strncpy(sptr, cp+1, ssize);
	}
    }    
 
    sender[strlen(sender)] = '\0';
    sendername[strlen(sendername)] = '\0';
    return 0;
}

eat_header(str)
char *str;
{
    /* 
       This attempts to determine if we are reading headers. It is 
       not foolproof -- if something looks like a header but really 
       isn't, it won't get into the delivered message. As long as the
       message is in the standard format (headers, blank line, text)
       it'll be ok.
    */
    static int lastheader = HEADER_NONE;
    int header, is_cont;
    char *nl, *htext;

    if (str[0] == '\n' || str[0] == '\0') return 0;

    /* Chop newline */
    nl = strchr(str, '\n');
    if (nl != NULL) *nl = '\0';

    if (str[0] == ' ' || str[0] == '\t') {
        header = lastheader;
        for (htext = str; *htext == ' ' || *htext == '\t'; htext++) ;
        is_cont = 1;
    }
    else {
        htext = identify_header(str, &header);
        is_cont = 0;
    }
    lastheader = header;
     
    switch (header) {
    case HEADER_NONE: return 0;
    case HEADER_OTHER: break;
    case HEADER_SUBJECT:
        if (!is_cont) strncpy(subject, htext, sizeof subject - 1);
        break;
    case HEADER_FROM:
    case HEADER_REPLYTO:
        if (!is_cont) split_sender(header, htext);
        break;
    }
    return 1;
}

show_failures(indx, userid, mask)
int indx;
char *userid;
LONG *mask;
{
    if ((*mask) & (LONG)(1<<indx)) {
        fprintf(stderr, "%s: bbs_mail failed\n", userid);
    }
    return S_OK;
}

usage(prog)
char *prog;
{
    if (daemon)
        fprintf(stderr, "Usage: %s [-d bbs-dir] user ...\n", prog);
    else
        fprintf(stderr, "Usage: %s [-d bbs-dir] [-s subject] user ...\n", 
                prog);
}

main(argc, argv)
int argc;
char *argv[];
{
    char *bbshome = NULL, *prog;
    char copybuf[256];
    int c, rc;
    int in_header = 1;
    FILE *fp;
    NAMELIST userid_list;
    LONG failmask;

    /* Prefix sender with INTERNET: */
    strcpy(sender, MAILER_PREFIX);
    strcat(sender, ":");
    sptr = sender + strlen(sender);
    ssize = sizeof(sender) - strlen(sender);
 
    /* Find out if we are bbsmail or bbsmaild */
    prog = strrchr(argv[0], '/');
    if (prog == NULL) prog = argv[0];
    else prog++;
    if (!strcmp(prog, "bbsmail")) {
        /* We do not expect headers. Get sender info from passwd file. */
        struct passwd *pw = getpwuid(getuid());
        if (pw != NULL) {
            char *comma;
            strncpy(sptr, pw->pw_name, ssize);
            strncpy(sendername, pw->pw_gecos, sizeof(sendername)+1);
            if ((comma = strchr(sendername, ',')) != NULL) *comma = '\0';
	}
        daemon = in_header = 0;     
    }

    /* Scan argv for option flags */
    while ((c = getopt(argc, argv, "d:s:?")) != -1)
      {
	switch (c)
	  {
	  case 'd':
	    bbshome = optarg;
	    break;
          case 's':
	    if (daemon) {
	      usage(prog);
	      return 2;
            }
	    strncpy(subject, optarg, sizeof(subject)-1);
            break;
	  case '?':
	    usage(prog);
	    return 2;
	  }
      }

    /* Must have at least one recipient */
    if (optind >= argc) {
        usage(prog);
        return 2;
    }

    /* Form recipients into namelist */
    userid_list = NULL;
    while (optind < argc) {
        if (!is_in_namelist(userid_list, argv[optind]))
            add_namelist(&userid_list, argv[optind], NULL);
        optind++;
    }
    
    /* Home and initialize bbs library */
    if (home_bbs(bbshome) == -1) {
        fprintf(stderr, "%s: Cannot chdir to %s\n", prog, bbshome);
        return 1;
    }

    if (local_bbs_initialize(NULL) != S_OK) {
        fprintf(stderr, "%s: local_bbs_initialize failed\n", prog);
        return 1;
    }

    /* Identify ourself for the log file */
    strcpy(user_params.u.userid, "[bbsmail]");
    user_params.perms = ~0;

    /* Copy message to temporary file, eating headers first */
    fp = fopen(server.tempfile, "w");
    if (fp == NULL) {
        free_namelist(&userid_list);
        fprintf(stderr, "%s: ", prog);
        perror("spool file open failed");
        return 1;
    }
    while (fgets(copybuf, sizeof copybuf, stdin) != NULL) {
        if (in_header) {
            in_header = eat_header(copybuf);
        }
        if (!in_header) fputs(copybuf, fp);
    }
    fclose(fp);            

    /* Make sure we know who the sender is */
    if (*sptr == '\0') {
        free_namelist(&userid_list);
        remove(server.tempfile);
        fprintf(stderr, "%s: cannot determine sender\n", prog);
        return 1;
    }

    /* Now mail it and let our caller know what happened */
    rc = local_bbs_mail(sender, sendername, userid_list, subject,
			server.tempfile, &failmask);

    if (rc != S_OK) {
        apply_namelist(userid_list, show_failures, &failmask);
        if (rc == S_NOSUCHUSER)
	  fprintf(stderr, "bbs_mail: No such user\n");
        else if (rc == S_CANNOTMAIL)
	  fprintf(stderr, "bbs_mail: User cannot receive Internet mail\n");
    }

    /* That's all, folks */
    free_namelist(&userid_list);
    remove(server.tempfile);
    local_bbs_disconnect();

    return (rc == S_OK ? 0 : 1);
}    


