
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
#include <signal.h>
#include <errno.h>          /* this may be just temporary */
#include <sys/socket.h>
#include <netinet/in.h>
#ifdef NeXT
# include <sys/wait.h>
# include <sys/time.h>
# include <sys/resource.h>
# define waitpid wait3
#endif

#ifndef INADDR_LOOPBACK
# define INADDR_LOOPBACK 0x7f000001
#endif

extern SERVERDATA server;

int chatfd = -1;
int inchat = 0;

unsigned short
_read_daemoninfo(fname)
char *fname;
{
  FILE *fp;
  char buf[5];
  unsigned short number;
  if ((fp = fopen(fname, "r")) == NULL) {
    return 0;
  }
  fgets(buf, sizeof buf, fp);
  buf[4] = '\0';
  number = (unsigned short)hex2SHORT(buf);
  fclose(fp);
  return number;
}

_start_chat_daemon()
{
  int pid;
  char *argv0 = "bbs.chatd";
  char argv1[8];
  sprintf(argv1, "%d", server.maxutable);

  switch (pid = fork()) {
  case -1: 
    return -1;
  case 0: 
    execl(PATH_CHATD, argv0, argv1, NULL);
    bbslog(0, "ERROR _start_chat_daemon: execl failed: %s\n", PATH_CHATD);
    exit(1);
  default:
    /* The chat daemon forks so we can wait on it here. */
    waitpid(pid, NULL, 0);
  }
  return 0;
}

local_bbs_chat(chatid, pfd)
char *chatid;
LONG *pfd;
{
  CHATLINE sendbuf;
  int rc, daemon_started = 0;
  unsigned short port;
  if (inchat) return S_OK;

  if (my_real_mode() == M_TALK) return S_MODEVIOLATION;

  if (chatfd == -1) {
    int s;
    struct sockaddr_in sin;
setupchat:
    memset(&sin, 0, sizeof sin);
    sin.sin_family = PF_INET;
    sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    sin.sin_port = port = ntohs(_read_daemoninfo(PATH_CHATPORT));
    if (sin.sin_port == 0) {
      _start_chat_daemon();
      daemon_started++;
      sin.sin_port = port = ntohs(_read_daemoninfo(PATH_CHATPORT));
      if (sin.sin_port == 0) {
        return S_SYSERR;
      }
    }
    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      bbslog(0, "ERROR local_bbs_chat: socket failed\n");
      return S_SYSERR;
    }
    rc = connect(s, (struct sockaddr *)&sin, sizeof sin);
    if (rc == -1) {
      if (errno == SIGINT) goto setupchat;
      bbslog(0, "ERROR local_bbs_chat: connect failed: errno %d\n", errno);
      if (!daemon_started) {
        unsigned long currport;
        close(s);
        currport = ntohs(_read_daemoninfo(PATH_CHATPORT));
        if (currport == port) {
          /* We may have crashed and left the chatport file there.
             Try to kill the running daemon if it's hung. */
          pid_t pid = (pid_t)_read_daemoninfo(PATH_CHATPID);
          if (pid > 0) kill(pid, SIGQUIT);
	  _start_chat_daemon();
          daemon_started++;
        }
        goto setupchat;
      }	
      close(s);
      return S_SYSERR;
    }            
    chatfd = s;
  }
  
  set_real_mode(M_CHAT);
  sprintf(sendbuf, "/! %d %s\n", my_utable_slot(), chatid);  
  if (send(chatfd, sendbuf, strlen(sendbuf), 0) != strlen(sendbuf)) {
    local_bbs_exit_chat();
    bbslog(0, "ERROR local_bbs_chat: send failed\n");
    return S_SYSERR;
  }
  if (recv(chatfd, sendbuf, 3, 0) != 3) {
    local_bbs_exit_chat();
    bbslog(0, "ERROR local_bbs_chat: recv failed\n");
    return S_SYSERR;
  }
  if (!strcmp(sendbuf, CHAT_LOGIN_OK)) {
    inchat = 1;
    *pfd = (LONG)chatfd;
    bbslog(4, "CHAT ENTER %s as %s\n", my_userid(), chatid);
    return S_OK;
  }
  else if (!strcmp(sendbuf, CHAT_LOGIN_EXISTS)) {
    set_real_mode(M_UNDEFINED);
    return S_CHATIDINUSE;
  }
  else if (!strcmp(sendbuf, CHAT_LOGIN_INVALID)) {
    set_real_mode(M_UNDEFINED);
    return S_BADCHATID;
  }

  /* else, server really didn't like us */
  local_bbs_exit_chat();
  bbslog(0, "ERROR local_bbs_chat: daemon sent back '%s'\n", sendbuf);
  return S_CHATDERROR;
}

local_bbs_exit_chat()
{
  if (chatfd != -1) {
    close(chatfd);
    chatfd = -1;
    inchat = 0;
    set_real_mode(M_UNDEFINED);
    bbslog(4, "CHAT EXIT %s\n", my_userid());
  }
  return S_OK;
}

local_bbs_chat_send(buf)
char *buf;
{
  int len = strlen(buf);
  if (chatfd == -1 || inchat == 0) return S_DENIED;
  if (send(chatfd, buf, len, 0) != len) {
    local_bbs_exit_chat();
    bbslog(0, "ERROR local_bbs_chat_send: send failed\n");
    return S_SYSERR;
  }
  return S_OK;
}  

remote_bbs_chat(chatid, pport, magicstr)
char *chatid;
unsigned short *pport;
char *magicstr;
{
  /* Assumes magicstr is big enough (should be 256 chars) */
  int daemon_started = 0;
  unsigned short port;
  pid_t pid;

  if (inchat) return S_OK;

  if (my_real_mode() == M_TALK) return S_MODEVIOLATION;

  pid = (pid_t)_read_daemoninfo(PATH_CHATPID);
  if (pid == 0 || kill(pid, 0) == -1) {
    _start_chat_daemon();
    daemon_started++;
  }

  port = ntohs(_read_daemoninfo(PATH_CHATPORT));
  if (port == 0) {
    if (daemon_started == 0) {
      _start_chat_daemon();
      daemon_started++;
      port = ntohs(_read_daemoninfo(PATH_CHATPORT));
    }
    if (port == 0) {
      return S_SYSERR;
    }
  }

  sprintf(magicstr, "/! %d %s\n", my_utable_slot(), chatid);  
  bbslog(4, "CHAT ENTER %s as %s\n", my_userid(), chatid);
  *pport = port;
  inchat = 1;
  set_real_mode(M_CHAT);
  return S_OK;
}

remote_bbs_exit_chat()
{
  if (inchat) {
    inchat = 0;
    set_real_mode(M_UNDEFINED);
    bbslog(4, "CHAT EXIT %s\n", my_userid());
  }
  return S_OK;
}
