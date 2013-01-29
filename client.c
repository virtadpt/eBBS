
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

#include "client.h"
#include <signal.h>
#include <time.h>
#ifdef REMOTE_CLIENT
# include <pwd.h>
# include <stdlib.h>
#endif

BBSINFO serverinfo;		/* server's info structure */
LOGININFO myinfo;               /* user's info structure */

PATH c_tempfile;                /* work file for the client side */
int input_active;               /* for the idle timer */

#ifdef REMOTE_CLIENT
char *tmpdir;
char *shell;
char *pager;
char *editor;
char *termtype;
char *charset;

extern char *optarg;
extern int optind;
#endif

extern PromptForAccountInfo();
extern SetPermTable();
extern char *_menudesc_file;
extern NDoMenu();
extern void page_handler __P((int));
extern char *Ctime __P((time_t *));

bbperror(code, str)
LONG code;
char *str;
{
  char *errstr;
  if (code > S_MAXERROR) errstr = "unknown error";
  else errstr = bb_errlist[code];
  if (str == NULL) prints("%s\n", errstr);
  else prints("%s: %s\n", str, errstr);
  return 0;
}

disconnect(status)
int status;
{
  unlink(c_tempfile);
  bbs_disconnect();
  if (fullscreen()) {
    clear();
    refresh();
  }
  else oflush();
  reset_tty();
  printf("\n");
  switch (status) {
  case EXIT_LOGOUT:
    printf("Goodbye!\n");
#ifdef AUX
    clear();
#endif
    break;
  case EXIT_CLIERROR:
    printf("Something's wrong. Exiting.\n");
    break;
  case EXIT_LOSTCONN:
    printf("Server closed connection. Exiting.\n");
    break;
  case EXIT_TIMEDOUT:
    printf("Idle timeout exceeded. Exiting.\n");
    break;
  default:     /* assumed to be a signal */
    if (g_child_pid != -1) {
      kill(g_child_pid, status);
    }
    switch (status) {
    case SIGHUP: break;
    case SIGINT: 
      printf("Interrupt signal received. Goodbye!\n");
      break;
    case SIGQUIT: 
      printf("Quit signal received. Goodbye!\n");
      break;
    case SIGTERM: 
      printf("Terminate signal received. Goodbye!\n");
      break;
    default: 
      printf("Bad news: signal %d received. Run! Flee!\n", status);
      break;
    }
    break;
  } 
  exit(status);
}

generic_abort()		/* general "bail and exit" */
{
  disconnect(EXIT_CLIERROR);
}

#ifdef REMOTE_CLIENT
bbslib_abort()
{
  disconnect(EXIT_LOSTCONN);
}
#endif

void
sig_handler(sig)
int sig;
{
  disconnect(sig == SIGALRM ? EXIT_TIMEDOUT : sig);
}

void
hit_alarm_clock()
{
  if (!input_active) {
    disconnect(EXIT_TIMEDOUT);
  }
  input_active = 0;
  signal(SIGALRM, hit_alarm_clock);
  if (myinfo.idletimeout) alarm((int)myinfo.idletimeout*60);
}

set_idle_alarm()
{
  if (myinfo.idletimeout != 0) {
    signal(SIGALRM, hit_alarm_clock);
    alarm((int)myinfo.idletimeout*60);
  }
}

cancel_idle_alarm()
{
  alarm(0);
}

void
InitializeTerminal()
{
  TERM t;
#ifdef REMOTE_CLIENT
  if (termtype == NULL) {
#else /* !REMOTE_CLIENT */
  ACCOUNT acct;
  if (bbs_owninfo(&acct) != S_OK) {
#endif
    prints("Can't determine your terminal type!\n");
    prints("Defaulting to 'dumb'.\n");
    term_init("dumb");
    return;
  }
#ifdef REMOTE_CLIENT
  strncpy(t, termtype, TERMLEN);
#else
  strncpy(t, acct.terminal, TERMLEN);
#endif
  if (term_init(t) == -1) {
    prints("I don't know about '%s' terminals!\n", t);
    prints("Defaulting terminal type to 'dumb'.\n");
    term_init("dumb");
  }
}
      
void
InitializeCharset()
{
  CSET c;
#ifdef REMOTE_CLIENT
  if (charset == NULL) {
#else /* !REMOTE_CLIENT */
  ACCOUNT acct;
  if (bbs_owninfo(&acct) != S_OK) {
#endif
    prints("Can't determine your character set!\n");
    prints("Defaulting to 'ascii'.\n");
    conv_init("ascii");
    return;
  }
#ifdef REMOTE_CLIENT
  strncpy(c, charset, TERMLEN);
#else
  strncpy(c, acct.charset, TERMLEN);
#endif

  if (conv_init(c) == -1) {
    prints("I don't know about '%s' charset!\n", c);
    prints("Defaulting character set to 'ascii'.\n");
    conv_init("ascii");
  }
}

Login(newok)
SHORT newok;
{
  ACCOUNT acct;
  int rc;

  char *ustr = (newok ? "userid ('new' for new user): " : "userid: ");
  alarm(60);   /* You get one minute to log in succesfully */
  do {
    if (getdata(0, 0, ustr, acct.userid, sizeof(acct.userid), DOECHO, 1) == -1)
      disconnect(EXIT_LOGOUT);

    if (newok && !strcmp(acct.userid, "new")) {
      /* Give them five minutes to enter new user data */
      alarm(300);
      PromptForAccountInfo(&acct, 1);
      rc = bbs_newlogin(&acct, &myinfo);
    }
    else {
      getdata(0, 0, "Password: ", acct.passwd, sizeof(acct.passwd), NOECHO, 0);
      rc = bbs_login(acct.userid, acct.passwd, 0, &myinfo);
    }
login_switch:
    if (rc != S_OK) {
      prints("\n");
      bbperror(rc, NULL);
      prints("\n");
      switch (rc) {
      case S_FULL:
      case S_ACCTDISABLED:
      case S_LOGINLIMIT:
        disconnect(EXIT_LOGOUT);
        break;
      case S_LOGINMUSTBOOT:      
        {
          char ans[4];
          getdata(0,0,"Kill other login (Y/N)? [Y]: ",ans,sizeof ans,DOECHO,0);
          if (*ans == 'N' || *ans == 'n') disconnect(EXIT_LOGOUT);
          rc = bbs_login(acct.userid, acct.passwd, 1, &myinfo);
	  goto login_switch;
        }
      }
    }          
  } while (rc != S_OK);
  alarm(0);
  set_idle_alarm();
  return 0;
}

#ifdef REMOTE_CLIENT
usage(prog)
char *prog;
{
  fprintf(stderr, 
  "Usage: %s [-c charset] [-d tmpdir] [-e editor] [-m menu-file]\n", prog);
  fprintf(stderr,
  "       [-p pager] [-s shell] [-t terminal-type] [bbs-server] [port]\n");
}
#endif

main(argc, argv)
int argc;
char *argv[];
{
  int rc;
  PATH fname;
  char *bbshomedir = NULL;
  char *servername = "localhost";
  SHORT port = EBBS_PORT;
  INITINFO initinfo;
  
#ifdef REMOTE_CLIENT
  int c;
  struct passwd *pw;

  while ((c = getopt(argc, argv, "c:d:e:m:p:s:t:?")) != -1) {
    switch (c) {
    case 'c':
      charset = optarg;
      break;
    case 'd':
      tmpdir = optarg;
      break;
    case 'e':
      editor = optarg;
      break;
    case 'm':
      _menudesc_file = optarg;
      break;
    case 'p':
      pager = optarg;      
      break;
    case 's':
      shell = optarg;
      break;
    case 't':
      termtype = optarg;
      break;
    case '?':
      usage(argv[0]);
      return 2;
    }
  }

  if (optind < argc) servername = argv[optind];
  if (optind+1 < argc) port = (SHORT)atoi(argv[optind+1]);

  if (shell == NULL) shell = getenv("SHELL");
  if (shell == NULL) shell = "/bin/sh";

  if (tmpdir == NULL) tmpdir = getenv("TMPDIR");
  if (tmpdir == NULL) tmpdir = "/tmp";

  if (pager == NULL) pager = getenv("BBS_PAGER");
  if (editor == NULL) editor = getenv("BBS_EDITOR");
  if (termtype == NULL) termtype = getenv("TERM");
  if (_menudesc_file == NULL) _menudesc_file = ".ebbsmenu";

  if ((pw = getpwuid(getuid())) != NULL)
    bbshomedir = pw->pw_dir;

  initinfo.abortfn = bbslib_abort;
  initinfo.tmpdir = tmpdir;
#else
  _menudesc_file = "etc/menu.desc";
  initinfo.abortfn = NULL;
  initinfo.tmpdir = NULL;
#endif

  if (get_tty() == -1) {
    perror("tty");
    fprintf(stderr, "Can't get terminal settings!\n");
    return 1;
  }
  
  home_bbs(bbshomedir);
  if ((rc = bbs_initialize(&initinfo)) != S_OK) {
    bbperror(rc, "bbs_initialize");
    oflush();
    return 1;
  }

  if ((rc = bbs_connect(servername, port, &serverinfo)) != S_OK) {
    bbperror(rc, "bbs_connect");
    oflush();
    return 1;
  }
    
#ifdef REMOTE_CLIENT
  sprintf(c_tempfile, "%s/bbs%05d", tmpdir, getpid());
#else
  sprintf(c_tempfile, "tmp/bbl%05d", getpid());
#endif

  signal(SIGHUP, sig_handler);
  signal(SIGINT, sig_handler);
  signal(SIGQUIT, sig_handler);
  signal(SIGILL, sig_handler);
  signal(SIGIOT, sig_handler);
#ifdef SIGEMT
  signal(SIGEMT, sig_handler);
#endif
  signal(SIGFPE, sig_handler);
#ifdef SIGBUS
  signal(SIGBUS, sig_handler);
#endif
  signal(SIGSEGV, sig_handler);
#ifdef SIGSYS
  signal(SIGSYS, sig_handler);
#endif
  signal(SIGPIPE, sig_handler);
  signal(SIGTERM, sig_handler);
  signal(SIGALRM, sig_handler);
  signal(SIGCHLD, SIG_DFL);
#ifdef SIGIO
  signal(SIGIO, SIG_IGN);
#endif
#ifdef SIGURG
  signal(SIGURG, SIG_IGN);
#endif
#ifdef SIGPWR
  signal(SIGPWR, sig_handler);
#endif

#ifndef REMOTE_CLIENT
  /* This is how the local client gets paged -- via SIGUSR1. */
  signal(SIGUSR1, page_handler);
#endif

  init_tty();
  
  prints("\nWelcome to %s\n", serverinfo.boardname);
  if (bbs_get_issue(fname) == S_OK) {
    FILE *fp = fopen(fname, "r");
    char buf[80];
    if (fp) {
      while (fgets(buf, sizeof(buf), fp)) 
	prints(buf);
      fclose(fp);
    }
  }
  
  Login(serverinfo.newok);
  SetPermTable();

  if (ParseMenu() == -1) {
    disconnect(EXIT_LOGOUT);
  }

  InitializeCharset();

  InitializeTerminal();
  initscr();          /* initscr() HAS to follow InitializeTerminal */

  /*  use this if time is not being set --  putenv("TZ=CST6CDT"); tzset(); */

  clear();
  if (bbs_get_welcome(fname) == S_OK) {
    if (myinfo.lastlogin == 0) More(fname, 1);
    else {
      char buf[80], host[21], ans;
      memset(host, '\0', sizeof host);
      strncpy(host, myinfo.fromhost, sizeof(host)-1);
      More(fname, 0);
      sprintf(buf, "Last login %s from %s [RETURN]: ",
              Ctime((time_t *)&myinfo.lastlogin), host);
      getdata(t_lines-1, 0, buf, &ans, 1, NOECHO, 0);
    }
  }
  
  NDoMenu("Main") ; /* Start at the Main Menu */

  disconnect(EXIT_LOGOUT);
}  
