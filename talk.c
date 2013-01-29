
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
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>

#if USES_SYS_SELECT_H
# include <sys/select.h>
#endif

#ifndef INADDR_LOOPBACK
# define INADDR_LOOPBACK 0x7f000001
#endif
 
#define PAGE_INTERVAL 20      /* How often the page signal is sent */

int talkfd = -1;
int cloak_save;
USERDATA talk_theirdata, talk_mydata;

local_bbs_talk(pid, fd, sockp)
LONG pid;
LONG fd;
LONG *sockp;
{
  int s, sinlen, nfds, rc;
  struct sockaddr_in sin;
  fd_set readfds;
  struct timeval tv;
  char cbuf[80];
  SHORT mymode;

  mymode = my_real_mode();
  if (mymode == M_CHAT || mymode == M_TALK) {
    return S_MODEVIOLATION;
  }

  if (mymode != M_PAGE) {
    /* You can't page yourself! */
    if (pid == getpid()) return S_TALKTOSELF;

    if (utable_find_record(pid, &talk_theirdata) != S_OK) return S_NOSUCHUSER;

    if (!has_page_permission(&talk_theirdata)) return S_PAGEROFF;

    /* Don't interrupt an upload or download. */
    if (talk_theirdata.u.mode & MODE_FLG_NOPAGE) return S_CANNOTPAGE;  
    /* Also if the client says it's not in a place where it can hear you. */ 
    if (talk_theirdata.usermode & MODE_FLG_NOPAGE) return S_CANNOTPAGE;

    if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
      bbslog(0, "ERROR local_bbs_talk: socket failed\n");
      return S_SYSERR;
    }
    
    memset(&sin, 0, sizeof sin);
    sin.sin_family = PF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    if (bind(s, (struct sockaddr *)&sin, sizeof sin) == -1) {
      close(s);
      bbslog(0, "ERROR local_bbs_talk: bind failed\n");
      return S_SYSERR;
    }
    sinlen = sizeof sin;
    if (getsockname(s, (struct sockaddr *)&sin, &sinlen) == -1) {
      close(s);
      bbslog(0, "ERROR local_bbs_talk: getsockname failed\n");
      return S_SYSERR;
    }
    if (listen(s, 1) == -1) {
      close(s);
      bbslog(0, "ERROR local_bbs_talk: listen failed\n");
      return S_SYSERR;
    }

    set_real_mode(M_PAGE);
    utable_get_record(my_utable_slot(), &talk_mydata);
    talk_mydata.destpid = pid;
    talk_mydata.port = (SHORT)htons(sin.sin_port);
    utable_set_record(my_utable_slot(), &talk_mydata);
    talkfd = s;
      
    bbslog(4,"PAGE %s by %s\n",talk_theirdata.u.userid,talk_mydata.u.userid);
  }

  if (kill(pid, SIGUSR1) == -1) {
    local_bbs_exit_talk();
    return S_SYSERR;
  }

  nfds = (talkfd > fd ? talkfd+1 : fd+1);
  while (1) {
    tv.tv_sec = PAGE_INTERVAL;
    tv.tv_usec = 0;
    FD_ZERO(&readfds);
    FD_SET(talkfd, &readfds);
    FD_SET(fd, &readfds);
    rc = select(nfds, &readfds, NULL, NULL, &tv);
    switch (rc) {
    case -1:
      if (errno != EINTR) {
	local_bbs_exit_talk();
        bbslog(0, "ERROR local_bbs_talk: select failed\n");
        return S_SYSERR;
      }
      break;
    case 0:
      return S_CMDTIMEOUT;
    default:
      if (FD_ISSET(talkfd, &readfds)) {
        int talksock;
        sinlen = sizeof sin;
        talksock = accept(talkfd, (struct sockaddr *)&sin, &sinlen);
        if (talksock == -1 || recv(talksock, cbuf, 1, 0) != 1) {
          local_bbs_exit_talk();
          bbslog(0, "ERROR local_bbs_talk: accept or recv failed\n");
	  return S_SYSERR;
	}
        if (cbuf[0] != 'y') {
	  local_bbs_exit_talk();
          bbslog(4, "PAGE REFUSED from %s by %s\n", 
                 talk_mydata.u.userid, talk_theirdata.u.userid);
	  return S_PAGEREFUSED;
	}
        set_real_mode(M_TALK);
	utable_get_record(my_utable_slot(), &talk_mydata);
	talk_mydata.port = 0;
        /* Force pager's cloak setting to match pagee's cloak setting */
        if ((talk_mydata.u.flags & FLG_CLOAK) != 
            (talk_theirdata.u.flags & FLG_CLOAK)) {
          cloak_save = (talk_mydata.u.flags & FLG_CLOAK) ? 1 : -1;
          talk_mydata.u.flags ^= FLG_CLOAK;
        }
	utable_set_record(my_utable_slot(), &talk_mydata);
        bbslog(4, "PAGE ACCEPTED from %s by %s\n", 
               talk_mydata.u.userid, talk_theirdata.u.userid);
	*sockp = (LONG)talksock;
	return S_OK;
      }        
      else if (FD_ISSET(fd, &readfds)) {
	int i, cc = read(fd, cbuf, sizeof cbuf);
        if (cc <= 0) {
	  local_bbs_exit_talk();
	  return S_SYSERR;
	}
        for (i=0; i<cc; i++) 
	  if (cbuf[i] == 3 || cbuf[i] == 4) {
	    local_bbs_exit_talk();
	    return S_INTERRUPT;
	  }
      }
    }
  }  
  /*NOTREACHED*/
  return 0;
}  
  
local_bbs_exit_talk()
{
  USERDATA udata;
  if (talkfd == -1) return S_OK;
  close(talkfd);
  talkfd = -1;
  utable_get_record(my_utable_slot(), &udata);
  udata.destpid = 0;
  udata.port = 0;
  if (cloak_save) {
    if (cloak_save > 0) udata.u.flags |= FLG_CLOAK;
    else udata.u.flags &= ~FLG_CLOAK;
    cloak_save = 0;
  }
  utable_set_record(my_utable_slot(), &udata);
  
  set_real_mode(M_UNDEFINED);
  return S_OK;
}  

/*ARGSUSED*/
_find_whos_paging(indx, udata, urec)
int indx;
USERDATA *udata;
USEREC *urec;
{
  if (udata->destpid == urec->pid && udata->port != 0) {
    memcpy(urec, &udata->u, sizeof(*urec));
    /* Kludge: pass port back in flags field. */
    urec->flags = udata->port;
    return ENUM_QUIT;
  }
  return S_OK;
}

local_bbs_get_talk_request(urec, paddr, pport)
USEREC *urec;
LONG *paddr;      /* may be NULL */
SHORT *pport;     /* may be NULL */
{
  memset(urec, 0, sizeof(*urec));
  urec->pid = getpid();
  utable_enumerate(0, NULL, _find_whos_paging, urec);
  if (urec->userid[0] == '\0') {
    urec->pid = 0;    
    return S_INTERRUPT;
  }      
  if (paddr) *paddr = (LONG)htonl(INADDR_LOOPBACK);
  if (pport) *pport = urec->flags;
  urec->flags = 0;
  return S_OK;
}

_get_answer_socket(addr, port, psock)
LONG addr;
SHORT port;
LONG *psock;
{
  int s;
  struct sockaddr_in sin;

  if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    return S_SYSERR;
  }
    
  memset(&sin, 0, sizeof sin);
  sin.sin_family = PF_INET;
  sin.sin_port = ntohs((unsigned short)port);
  sin.sin_addr.s_addr = (unsigned long)addr;

  if (connect(s, (struct sockaddr *)&sin, sizeof sin) == -1) {
    close(s);
    return S_SYSERR;
  }

  *psock = (LONG)s;
  return S_OK;
}  

local_bbs_refuse_page(addr, port)
LONG addr;
SHORT port;
{
  int rc, sock;
  char shaddup = 'n';
  rc = _get_answer_socket(addr, port, &sock);
  if (rc != S_OK) return rc;
  if (send(sock, &shaddup, 1, 0) != 1) {
    close(sock);
    return S_SYSERR;
  }
  close(sock);
  return S_OK;
}

local_bbs_accept_page(addr, port, psock)
LONG addr;
SHORT port;
LONG *psock;
{
  int rc, sock;
  char talktome = 'y';
  SHORT mymode;

  mymode = my_real_mode();
  if (mymode == M_CHAT || mymode == M_TALK) {
    return S_MODEVIOLATION;
  }

  rc = _get_answer_socket(addr, port, &sock);
  if (rc != S_OK) return rc;
  if (send(sock, &talktome, 1, 0) != 1) {
    close(sock);
    return S_SYSERR;
  }
  set_real_mode(M_TALK);
  talkfd = sock;
  /* 
     (maybe) TODO: get destpid and port, set in utable record.
     Will have to do this before enum_users can return info about
     who is talking to whom.
  */     
  *psock = (LONG)sock;
  return S_OK;
}

/* Override list stuff */

#define OVERRIDE_FILE  "overrides"

get_override_file(userid, buf)
char *userid;
char *buf;
{
  get_home_directory(userid, buf);
  strcat(buf, "/");
  strcat(buf, OVERRIDE_FILE);
}

is_on_override_list(userid)
char *userid;
{
  PATH buf;
  get_override_file(userid, buf);
  if (_record_find(buf, _match_full, my_userid(), NULL, NULL) == S_OK) 
    return 1;
  else return 0;
}

local_bbs_set_overrides(list)
NAMELIST list;
{
  PATH buf;
  FILE *fp;
  get_override_file(my_userid(), buf);
  return (write_namelist(buf, list));
}

local_bbs_get_overrides(list)
NAMELIST *list;
{
  PATH buf;
  get_override_file(my_userid(), buf);
  return (read_namelist(buf, list));
}

/* Returns 1 if current user can page user, 0 if not */

has_page_permission(udata)
USERDATA *udata;
{
  int noovers, noothers;

  if (!_they_have_access(C_TALKACCEPT, udata->perms)) return 0;

  if (_has_perms(PERM_SYSOP)) return 1;

  noovers = udata->u.flags & FLG_NOOVERRIDE;
  noothers = udata->u.flags & FLG_NOPAGE;

  if (noovers && noothers) return 0;
  if (!noovers && !noothers) return 1;

  if (is_on_override_list(udata->u.userid)) {
    if (noovers) return 0;
  }
  else {
    if (noothers) return 0;
  }
  return 1;
}

#if FULL_USER_DELETE
/* For fixing override files when users get deleted or their names changed */

/*ARGSUSED*/
fix_override_entry(indx, rec, ncs)
int indx;
char *rec;
struct namechange *ncs;
{
  PATH overfile;
  NAME userid;

  /* Sleazy way to get the userid out of the passfile record */
  memset(userid, 0, sizeof userid);
  strncpy(userid, rec, NAMELEN);
  strip_trailing_space(userid);

  get_override_file(userid, overfile);
  
  if (ncs->newname == NULL)
    _record_delete(overfile, _match_full, ncs->oldname);
  else _record_replace(overfile, _match_full, ncs->oldname,
                       _change_name, ncs->newname);
  return S_OK;
}    
#endif
