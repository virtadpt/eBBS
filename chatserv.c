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

/*-------------------------------------------------------------------------*/

#include "server.h"
#include "stdlib.h"
#include <ctype.h>
#if LACKS_MALLOC_H
# include <stdlib.h>
#else
# include <malloc.h>
#endif
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>

#ifndef RELIABLE_SELECT_FOR_WRITE
# include <fcntl.h>
#endif

#if USES_SYS_SELECT_H
# include <sys/select.h>
#endif

#if NO_SETPGID
# define setpgid setpgrp
#endif

#ifndef SOMAXCONN
# define SOMAXCONN 5
#endif

#define FLG_CHATOP       0x0100
#define FLG_CHATMGR      0x0200
#define FLG_CHATGUEST    0x0400
#define FLG_RESTORECLOAK 0x1000

#define RESTRICTED(u)   (users[(u)].flags & FLG_CHATGUEST)
#define OPERATOR(u)     (users[(u)].flags & FLG_CHATOP)
#define MANAGER(u)      (users[(u)].flags & FLG_CHATMGR)

#define ROOM_LOCKED      0x1
#define ROOM_SECRET      0x2

#define LOCKED(r)       (rooms[(r)].flags & ROOM_LOCKED)
#define SECRET(r)       (rooms[(r)].flags & ROOM_SECRET)

#define ROOM_ALL        (-2)

#define CHAT_MOTD_FILE  "etc/chat.motd"

struct chatuser {
  int sockfd;            /* socket to bbs server */
  int utent;             /* utable entry for this user */
  int client_pid;        /* client pid */
  int room;              /* room: -1 means none, 0 means main */
  int flags;             /* FLG_CHATOP, FLG_CHATMGR, FLG_CHATGUEST, etc. */
  NAME userid;           /* real userid */
  NAME chatid;           /* chat id */
  CHATLINE ibuf;         /* buffer for sending/receiving */
  int ibufsize;          /* current size of ibuf */
  NAMELIST ignorelist;   /* list of userids being ignored */
  char *ignoring;        /* list of chat user numbers ignoring me */
} *users;

struct chatroom {
  NAME name;             /* name of room; room 0 is "main" */
  short occupants;       /* number of users in room */
  short flags;           /* ROOM_LOCKED, ROOM_SECRET */
  char *invites;         /* Keep track of invites to rooms */
} *rooms;

struct chatcmd {
  char *cmdstr;
  int (*cmdfunc)();
  int exact;
};  

int bbs_max_users;          /* Passed in at exec time as argv[1] */
int sock = -1;              /* the socket for listening */
int nfds;                   /* number of sockets to select on */
int num_conns;              /* current number of connections */
fd_set allfds;              /* fd set for selecting */
struct timeval zerotv;      /* timeval for selecting */
CHATLINE genbuf;            /* general purpose buffer */

/* These are set up in the chatconfig file. */
NAME mainroom;              /* name of the main room (always exists) */
NAMELIST manager_list;      /* list of chat super-operators */
NAMELIST restricted_list;   /* list of restricted access accounts */
int no_punct_in_chatids;    /* are punctuation characters allowed? */

is_valid_chatid(id)
char *id;
{
  int i;
  if (*id == '\0') return 0;

  for (i=0; i<CHATID_MAX && *id; i++, id++) {
    if (!isprint(*id) || *id == '*' || *id == '+' || *id == ':') return 0;
    if (no_punct_in_chatids && ispunct(*id) && *id != '-' && *id != '_') 
      return 0;
  }

  return 1;
}

is_valid_roomname(roomid)
char *roomid;
{
  int i;
  if (*roomid == '\0') return 0;

  for (i=0; i<NAMELEN && *roomid; i++, roomid++)
    if (!isprint(*roomid) || *roomid == '+') return 0;

  return 1;
}

char *
nextword(str)
char **str;
{
  char *p;
  while (isspace(**str)) (*str)++;    
  p = *str;
  while (**str && !isspace(**str)) (*str)++;
  if (**str) {
    **str = '\0';
    (*str)++;
  }
  return p;
}

chatid_to_indx(chatid)
char *chatid;
{
  register int i;
  for (i=0; i<bbs_max_users; i++) {
    if (users[i].sockfd == -1) continue;
    if (!strcasecmp(chatid, users[i].chatid)) return i;
  }
  return -1;
}

fuzzy_chatid_to_indx(chatid)
char *chatid;
{
  register int i, indx = -1;
  int len = strlen(chatid);
  for (i=0; i<bbs_max_users; i++) {
    if (users[i].sockfd == -1) continue;
    if (!strncasecmp(chatid, users[i].chatid, len)) {
      if (len == strlen(users[i].chatid)) return i;
      if (indx == -1) indx = i; 
      else indx = -2;
    }
  }
  return indx;
}

userid_to_indx(userid)
char *userid;
{
  register int i;
  for (i=0; i<bbs_max_users; i++) {
    if (users[i].sockfd == -1) continue;
    if (!strcasecmp(userid, users[i].userid)) return i;
  }
  return -1;
}

roomid_to_indx(roomid)
char *roomid;
{
  register int i;
  for (i=0; i<bbs_max_users; i++) {
    if (i && rooms[i].occupants == 0) continue;
    if (!strcasecmp(roomid, rooms[i].name)) return i;
  }
  return -1;
}

do_send(writefds, str)
fd_set *writefds;
char *str;
{
  register int i;
  int len = strlen(str);
  /* Remove the string +++ anywhere we see it, for modem users */
  for (i=0; i<len-2; i++) {
    if (str[i] == '+' && str[i+1] == '+' && str[i+2] == '+')
      str[i] = str[i+1] = str[i+2] = ' ';
  }
  if (select(nfds, NULL, writefds, NULL, &zerotv) > 0) {
    for (i=0; i<nfds; i++)
      if (FD_ISSET(i, writefds)) send(i, str, len+1, 0);
  }
}          

send_to_room(room, str, fromunum)
int room;
char *str;
int fromunum;
{
  int i, checkign, rc = 0;
  fd_set writefds;
  checkign = (fromunum != -1 && users[fromunum].ignoring != NULL);
  FD_ZERO(&writefds);
  for (i=0; i<bbs_max_users; i++) {
    if (users[i].sockfd == -1) continue;
    if (room == ROOM_ALL || room == users[i].room)
      if (!checkign || !users[fromunum].ignoring[i]) {
	FD_SET(users[i].sockfd, &writefds);
        rc++;
      }
  }
  do_send(&writefds, str);
  return rc;
}  

send_to_unum(unum, str, fromunum)
int unum;
char *str;
int fromunum;
{
  fd_set writefds;
  if (fromunum == -1 || users[fromunum].ignoring == NULL ||
      users[fromunum].ignoring[unum] == 0) {
    FD_ZERO(&writefds);
    FD_SET(users[unum].sockfd, &writefds);
    do_send(&writefds, str);
    return 1;
  }
  else return 0;
}  

exit_room(unum, disp, msg)
int unum;
int disp;
char *msg;
{
  int oldrnum = users[unum].room;
  if (oldrnum != -1) {
    if (--rooms[oldrnum].occupants) {
      switch (disp) {
      case EXIT_LOGOUT:
        sprintf(genbuf, "*** %s has left the room", users[unum].chatid);
        if (msg && *msg) {
          strcat(genbuf, ": ");
          strncat(genbuf, msg, CHATLINE_TEXT_MAX);
          strcat(genbuf, "\n");
        }
 	break;
      case EXIT_LOSTCONN:
        sprintf(genbuf, "*** %s has been disconnected\n", users[unum].chatid);
	break;
      case EXIT_KICK:
        sprintf(genbuf, "*** %s has been kicked out ", users[unum].chatid);
        if (msg && *msg) {
          strcat(genbuf, "(");
          strncat(genbuf, msg, CHATLINE_TEXT_MAX-(strlen(genbuf)+3));
          strcat(genbuf, ")");
        }
        break;
      }
      send_to_room(oldrnum, genbuf, unum);
    }
  }
  users[unum].flags &= ~FLG_CHATOP;
  users[unum].room = -1;
}

enter_room(unum, room, msg)
int unum;
char *room;
char *msg;
{
  int rnum = roomid_to_indx(room);
  int op = 0;
  register int i;
  if (rnum == -1) {
    /* new room */
    for (i=1; i<bbs_max_users; i++) {
      if (rooms[i].occupants == 0) {
        rnum = i;
        if (rooms[rnum].invites == NULL) {
          rooms[rnum].invites = (char *)malloc(bbs_max_users);
          if (rooms[rnum].invites == NULL) {
            send_to_unum(unum, "*** Not enough memory\n", -1);
            return 0;
          }
        }
        memset(rooms[rnum].invites, 0, bbs_max_users);
        strncpy(rooms[rnum].name, room, NAMELEN);
        rooms[rnum].name[NAMELEN] = '\0';
        rooms[rnum].flags = 0;
        op++;
        break;
      }
    }
    if (rnum == -1) {
      send_to_unum(unum, "*** No more rooms available\n", -1);
      return 0;
    }
  }
  if (!MANAGER(unum))
    if (LOCKED(rnum) && rooms[rnum].invites[unum] == 0) {
      send_to_unum(unum, "*** Cannot enter locked room without a key\n", -1);
      return 0;
    }

  exit_room(unum, EXIT_LOGOUT, msg);
  users[unum].room = rnum;
  if (op) users[unum].flags |= FLG_CHATOP;
  rooms[rnum].occupants++;
  rooms[rnum].invites[unum] = 0;
  sprintf(genbuf, "*** %s has entered room '%s'\n", 
          users[unum].chatid, rooms[rnum].name);
  send_to_room(rnum, genbuf, unum);
  return 0;
}

logout_user(unum)
{
  USERDATA udata;
  int i, sockfd = users[unum].sockfd;
  PATH ignorefile;
#ifdef DEBUG
  printf("Logout on slot %2d, fd %d\n", unum, sockfd);
#endif
  close(sockfd);
  FD_CLR(sockfd, &allfds);

  utable_get_record(users[unum].utent, &udata);
  if (users[unum].flags & FLG_RESTORECLOAK) {
    udata.u.flags |= FLG_CLOAK;
  }
  utable_set_record(users[unum].utent, &udata);

  if (!RESTRICTED(unum)) {
    chat_get_ignore_file(users[unum].userid, ignorefile);  
    write_namelist(ignorefile, users[unum].ignorelist);
  }
  free_namelist(&users[unum].ignorelist);
  memset(&users[unum], 0, sizeof(users[unum]));
  users[unum].sockfd = users[unum].utent = users[unum].room = -1;
  users[unum].ignorelist = NULL;
  if (users[unum].ignoring != NULL) {
    free(users[unum].ignoring);
    users[unum].ignoring = NULL;
  }
  for (i=0; i<bbs_max_users; i++) {
    if (rooms[i].invites != NULL) rooms[i].invites[unum] = 0;
    if (users[i].sockfd != -1 && users[i].ignoring != NULL) {
      users[i].ignoring[unum] = 0;
    }
  }
  num_conns--;
}

display_motd(unum)
int unum;
{
  FILE *fp = fopen(CHAT_MOTD_FILE, "r");
  if (fp != NULL) {
    char buf[80];
    strcpy(buf, "*** ");
    while (fgets(buf+4, sizeof(buf)-4, fp) != NULL) {
      buf[sizeof(buf)-1] = '\0';
      send_to_unum(unum, buf, -1);
    }
    fclose(fp);
  }
  return 0;
}

print_user_counts(unum)
int unum;
{
  int i, c, userc = 0, suserc = 0, roomc = 0;
  for (i=0; i<bbs_max_users; i++) {
    c = rooms[i].occupants;
    if (i > 0 && c > 0) {
      if (!SECRET(i) || MANAGER(unum)) roomc++;
    }
    c = users[i].room;
    if (users[i].sockfd != -1 && c != -1) {
      if (SECRET(c) && !MANAGER(unum)) suserc++;
      else userc++;
    }
  }
  sprintf(genbuf, "*** %d other user(s) present", userc);
  if (suserc) 
    sprintf(genbuf+strlen(genbuf), " (+ %d in secret rooms)", suserc);
  strcat(genbuf, "\n");
  send_to_unum(unum, genbuf, -1);
  sprintf(genbuf, "*** %d other visible room(s) in use\n", roomc);
  send_to_unum(unum, genbuf, -1);
  return 0;
}    

login_user(unum, msg)
int unum;
char *msg;
{
  int i, utent, fd = users[unum].sockfd;
  char accessbytes[MAX_CLNTCMDS];
  char *utentstr = nextword(&msg);
  char *chatid = nextword(&msg);
  USERDATA udata;
  PATH ignorefile;
      
  utent = atoi(utentstr);
  if (utable_get_record(utent, &udata) != S_OK || udata.u.mode != M_CHAT) {
    send_to_unum(unum, CHAT_LOGIN_BOGUS, -1);
    return -1;
  }
  for (i=0; i<bbs_max_users; i++) {
    if (users[i].sockfd != -1 && users[i].utent == utent) {
      /* Either a "ghost" or a hacker */
      if (kill(users[i].client_pid, 0) == 0) {
        send_to_unum(unum, CHAT_LOGIN_BOGUS, -1);
        return -1;
      }
      else {
        exit_room(i, EXIT_LOSTCONN, (char *)NULL);
        logout_user(i);
      }
    }
  }
  if (strlen(chatid) > CHATID_MAX) chatid[CHATID_MAX] = '\0';
  if (!is_valid_chatid(chatid)) {
    send_to_unum(unum, CHAT_LOGIN_INVALID, -1);
    return 0; /* TODO change to -1 when possible */
  }
  if (chatid_to_indx(chatid) != -1) {
    /* userid in use */
    send_to_unum(unum, CHAT_LOGIN_EXISTS, -1);
    return 0; /* TODO: change to -1 when possible */
  }    

  /* Login is ok. Set flags and fill in user record. */
  utable_get_record(utent, &udata);
  if (udata.u.flags & FLG_CLOAK) {
    udata.u.flags &= ~FLG_CLOAK;
    users[unum].flags |= FLG_RESTORECLOAK;
  }  
  utable_set_record(utent, &udata);

  if (is_in_namelist(manager_list, udata.u.userid))
    users[unum].flags |= FLG_CHATMGR;    
  else if (is_in_namelist(restricted_list, udata.u.userid))
    users[unum].flags |= FLG_CHATGUEST;

  users[unum].utent = utent;
  users[unum].client_pid = udata.u.pid;
  strncpy(users[unum].userid, udata.u.userid, NAMELEN);
  if (!RESTRICTED(unum)) {
    chat_get_ignore_file(users[unum].userid, ignorefile);  
    read_namelist(ignorefile, &users[unum].ignorelist);
  }
  users[unum].ignoring = NULL;
  for (i = 0; i < bbs_max_users; i++) {
    if (users[i].sockfd == -1) continue;
    if (is_in_namelist(users[i].ignorelist, udata.u.userid)) {
      if (users[unum].ignoring == NULL)
        users[unum].ignoring = (char *)calloc(bbs_max_users, sizeof(char));
      if (users[unum].ignoring != NULL)
        users[unum].ignoring[i] = 1;
    }
    if (is_in_namelist(users[unum].ignorelist, users[i].userid)) {
      if (users[i].ignoring == NULL)
        users[i].ignoring = (char *)calloc(bbs_max_users, sizeof(char));
      if (users[i].ignoring != NULL)
        users[i].ignoring[unum] = 1;
    }
  }
  strcpy(users[unum].chatid, chatid);
  send_to_unum(unum, CHAT_LOGIN_OK, -1);
  sprintf(genbuf, "*** Welcome to Chat, %s\n", users[unum].chatid);
  send_to_unum(unum, genbuf, -1);
  display_motd(unum);
  print_user_counts(unum);
  enter_room(unum, mainroom, (char *)NULL);
  return 0;  
}  

/*ARGSUSED*/
chat_list_rooms(unum, msg)
int unum;
char *msg;
{
  int i, occupants;
  if (RESTRICTED(unum)) {
    send_to_unum(unum, "*** You do not have permission to list rooms\n", -1);
    return 0;
  }
  send_to_unum(unum, "***\n", -1);
  send_to_unum(unum, "*** Room           Users\n", -1);
  send_to_unum(unum, "*** ----           -----\n", -1);
  for (i=0; i<bbs_max_users; i++) {
    occupants = rooms[i].occupants;
    if (occupants > 0) {
      if (!MANAGER(unum))
        if ((rooms[i].flags & ROOM_SECRET) && (users[unum].room != i))
          continue;
      sprintf(genbuf, "*** %-15s %3d", rooms[i].name, occupants);
      if (rooms[i].flags & ROOM_LOCKED) strcat(genbuf, " (LOCKED)");
      if (rooms[i].flags & ROOM_SECRET) strcat(genbuf, " (SECRET)");
      strcat(genbuf, "\n");
      send_to_unum(unum, genbuf, -1);
    }
  }
  return 0;
}

chat_do_user_list(unum, msg, whichroom)
int unum;
char *msg;
int whichroom;
{
  int start, stop, curr = 0;
  int i, rnum, myroom = users[unum].room;
  while (*msg && isspace(*msg)) msg++;
  start = atoi(msg);
  while (*msg && isdigit(*msg)) msg++;
  while (*msg && !isdigit(*msg)) msg++;
  stop = atoi(msg);
  send_to_unum(unum, "***\n", -1);
  send_to_unum(unum, "*** Nick      Userid          Room\n", -1);
  send_to_unum(unum, "*** ----      ------          ----\n", -1);
  for (i=0; i<bbs_max_users; i++) {
    rnum = users[i].room;
    if (users[i].sockfd != -1 && rnum != -1) {
      if (whichroom != -1 && whichroom != rnum) continue;
      if (myroom != rnum) {
        if (RESTRICTED(unum)) continue;
        if ((rooms[rnum].flags & ROOM_SECRET) && !MANAGER(unum)) continue;
      }
      curr++;
      if (curr < start) continue;
      else if (stop && (curr > stop)) break;
      sprintf(genbuf, "*** %-9s %-15s %s", users[i].chatid, 
              users[i].userid, rooms[rnum].name);
      if (OPERATOR(i)) strcat(genbuf, " (Op)");
      strcat(genbuf, "\n");
      send_to_unum(unum, genbuf, -1);
    }
  }
  return 0;
}

chat_list_by_room(unum, msg)
int unum;
char *msg;
{
  int whichroom;
  char *roomstr = nextword(&msg);
  if (*roomstr == '\0') whichroom = users[unum].room;
  else {
    if ((whichroom = roomid_to_indx(roomstr)) == -1) {
      sprintf(genbuf, "*** No such room '%s'\n", roomstr);
      send_to_unum(unum, genbuf, -1);
      return 0;
    }    
    if ((rooms[whichroom].flags & ROOM_SECRET) && !MANAGER(unum)) {
      send_to_unum(unum, "*** Cannot list users in secret rooms\n", -1);
      return 0;
    }    
  }
  return (chat_do_user_list(unum, msg, whichroom));
}

chat_list_users(unum, msg)
int unum;
char *msg;
{
  return (chat_do_user_list(unum, msg, -1));
}

chat_map_chatids(unum)
int unum;
{
  int i, c, myroom, rnum;
  myroom = users[unum].room;
  send_to_unum(unum, 
  "*** Chatid   Userid         Chatid   Userid         Chatid   Userid\n", -1);
  send_to_unum(unum,
  "*** ------   ------         ------   ------         ------   ------\n", -1);
  strcpy(genbuf, "***  ");    
  for (i=0, c=0; i<bbs_max_users; i++) {
    rnum = users[i].room;
    if (users[i].sockfd != -1 && rnum != -1) {
      if (myroom != rnum) {
        if (RESTRICTED(unum)) continue;
        if ((rooms[rnum].flags & ROOM_SECRET) && !MANAGER(unum)) continue;
      }
      sprintf(genbuf+(c*24)+4, "%-8s %-12s   \n", 
	      users[i].chatid, users[i].userid);
      if (++c == 3) {
        send_to_unum(unum, genbuf, -1);
	c = 0;
      }
    }
  }
  if (c > 0) send_to_unum(unum, genbuf, -1);
  return 0;    
}

chat_query_chatid(unum, msg)
int unum;
char *msg;
{
  USERDATA udata;
  int recunum;
  char *recipient = nextword(&msg);
  send_to_unum(unum, "***\n", -1);
  if (*recipient == '\0') {
    /* map all chatids to userids */
    return (chat_map_chatids(unum));
  }
  recunum = fuzzy_chatid_to_indx(recipient);
  if (recunum < 0) {  
    /* no such user, or ambiguous */
    if (recunum == -1) sprintf(genbuf, "*** No such chatid '%s'\n", recipient);
    else sprintf(genbuf, "*** '%s' is ambiguous: try more letters", recipient);
    send_to_unum(unum, genbuf, -1);
    return 0;
  }    
  if (utable_get_record(users[recunum].utent, &udata) != S_OK ||
      strcmp(users[recunum].userid, udata.u.userid) ||
      udata.u.mode != M_CHAT) {
    sprintf(genbuf, "*** '%s' is apparently no longer here\n", 
	    users[recunum].userid);
    send_to_unum(unum, genbuf, -1);
    exit_room(recunum, EXIT_LOSTCONN, (char *)NULL);
    logout_user(recunum);        
    return 0;
  }
  sprintf(genbuf, "*** %s is %s (%s)\n", users[recunum].chatid,
          udata.u.userid, udata.u.username);
  send_to_unum(unum, genbuf, -1);
  sprintf(genbuf, "*** Logged in from %s\n", udata.u.fromhost);          
  send_to_unum(unum, genbuf, -1);
  return 0;
}

chat_setroom(unum, msg)
int unum;
char *msg;
{
  char *modestr = nextword(&msg);
  int rnum = users[unum].room;
  int sign = 1;
  int flag;
  char *fstr;
  if (!OPERATOR(unum)) {
    send_to_unum(unum, "*** You're not operator\n", -1);
    return 0;
  }
  if (*modestr == '+') modestr++;
  else if (*modestr == '-') {
    modestr++;
    sign = 0;
  }
  if (*modestr == '\0') {
    send_to_unum(unum, "*** No flags specified\n", -1);
    return 0;
  }
  while (*modestr) {
    flag = 0;
    switch (*modestr) {
    case 'l': case 'L':
      flag = ROOM_LOCKED;
      fstr = "Locked";
      break;
    case 's': case 'S':
      flag = ROOM_SECRET;
      fstr = "Secret";
      break;
    default:
      sprintf(genbuf, "*** Unknown flag '%c'\n", *modestr);
      send_to_unum(unum, genbuf, -1);
    }
    if (flag && ((rooms[rnum].flags & flag) != sign*flag)) {
      rooms[rnum].flags ^= flag;
      sprintf(genbuf, "*** Mode change by %s to%s%s\n",
              users[unum].chatid, sign ? " " : " NOT ", fstr);
      send_to_room(rnum, genbuf, -1);
    }
    modestr++;
  }  
  return 0;  
}  

chat_nick(unum, msg)
int unum;
char *msg;
{
  char *chatid = nextword(&msg);
  int othernum;
  if (!is_valid_chatid(chatid)) {
    send_to_unum(unum, "*** Invalid chatid\n", -1);
    return 0;
  }
  if (strlen(chatid) > CHATID_MAX) chatid[CHATID_MAX] = '\0';
  othernum = chatid_to_indx(chatid);
  if (othernum != -1 && othernum != unum) {
    send_to_unum(unum, "*** Chatid is in use\n", -1);
    return 0;
  }    
  sprintf(genbuf, "*** %s is now known as ", users[unum].chatid);
  strcpy(users[unum].chatid, chatid);
  strcat(genbuf, users[unum].chatid);
  strcat(genbuf, "\n");
  send_to_room(users[unum].room, genbuf, unum);
  sprintf(genbuf, "**C %s", users[unum].chatid);
  send_to_unum(unum, genbuf, -1);
  return 0;
}

chat_private(unum, msg)
int unum;
char *msg;
{
  char *recipient = nextword(&msg);
  int recunum = fuzzy_chatid_to_indx(recipient);
  if (recunum < 0) {  
    /* no such user, or ambiguous */
    if (recunum == -1) sprintf(genbuf, "*** No such chatid '%s'\n", recipient);
    else sprintf(genbuf, "*** '%s' is ambiguous: try more letters", recipient);
    send_to_unum(unum, genbuf, -1);
    return 0;
  }    
  if (*msg) {
    sprintf(genbuf, "*%s* ", users[unum].chatid);
    strncat(genbuf, msg, CHATLINE_TEXT_MAX);
    strcat(genbuf, "\n");
    if (send_to_unum(recunum, genbuf, unum)) {
      sprintf(genbuf, "%s> ", users[recunum].chatid);
      strncat(genbuf, msg, CHATLINE_TEXT_MAX);
      strcat(genbuf, "\n");
    }
    else sprintf(genbuf, "*** You are being ignored by '%s'\n", 
                 users[recunum].chatid);
    send_to_unum(unum, genbuf, -1);
  }
  return 0;  
}  

put_chatid(unum, str)
int unum;
char *str;
{
  int i;
  char *chatid = users[unum].chatid;
  memset(str, ' ', 10);
  for (i=0; chatid[i]; i++) str[i] = chatid[i];
  str[i] = ':';
  str[10] = '\0';
}

chat_allmsg(unum, msg)
int unum;
char *msg;
{
  if (*msg) {
    put_chatid(unum, genbuf);
    strncat(genbuf, msg, CHATLINE_TEXT_MAX);
    strcat(genbuf, "\n");
    send_to_room(users[unum].room, genbuf, unum);
  }
  return 0;  
}  

chat_emote(unum, msg)
int unum;
char *msg;
{
  if (*msg) {
    strcpy(genbuf, users[unum].chatid);
    strcat(genbuf, " ");
    strncat(genbuf, msg, CHATLINE_TEXT_MAX);
    strcat(genbuf, "\n");
    send_to_room(users[unum].room, genbuf, unum);
  }
  return 0;  
}  

/*ARGSUSED*/
chat_time(unum, msg)
int unum;
char *msg;
{
  time_t now;
  time(&now);
  sprintf(genbuf, "*** Date and time on this server: %s", ctime(&now));
  send_to_unum(unum, genbuf, -1);
}

#if EXTRA_CHAT_STUFF

diceroll(die)
int die;
{
  int r;
  int max = (RAND_MAX / die) * die - 1;
  while ((r = rand()) > max) ;
  return (r % die + 1);
}

chat_roll_dice(unum, msg)
int unum;
char *msg;
{
  static int seeded;
  int ndice = 2;
  int die = 6;
  int total = 0;
  int i, roll;
  char diebuf[8];
  char *rollspec = nextword(&msg);
  char *d;
  if (!seeded) {
    srand((int)time(NULL));
    seeded++;
  }
  if ((d = strchr(rollspec, 'd')) != NULL) {
    *d = '\0';
    die = atoi(d+1);
  }
  if (*rollspec != '\0') {
    ndice = atoi(rollspec);
  }
  if (ndice < 0 || ndice > 10) {
    send_to_unum(unum, "*** May only roll from 1 to 10 dice\n", -1);
    return 0;
  }
  if (die < 2 || die > 100) {
    send_to_unum(unum, "*** Die must have from 2 to 100 sides\n", -1);
    return 0;
  }
  strcpy(genbuf, "*** ");
  strcat(genbuf, users[unum].chatid);
  strcat(genbuf, " rolled (");
  for (i=0; i<ndice; i++) {
    roll = diceroll(die);
    if (i > 0) strcat(genbuf, ", ");
    sprintf(diebuf, "%d", roll);
    strcat(genbuf, diebuf);
    total += roll;        
  }
  strcat(genbuf, ") = ");
  sprintf(diebuf, "%d", total);
  strcat(genbuf, diebuf);
  strcat(genbuf, " on ");
  sprintf(diebuf, "%dd%d", ndice, die);
  strcat(genbuf, diebuf);
  strcat(genbuf, "\n");
  send_to_room(users[unum].room, genbuf, unum);
  return 0;  
}  

chat_think(unum, msg)
int unum;
char *msg;
{
  if (*msg) {
    strcpy(genbuf, users[unum].chatid);
    strcat(genbuf, " . o O ( ");
    strncat(genbuf, msg, CHATLINE_TEXT_MAX);
    strcat(genbuf, " )\n");
    send_to_room(users[unum].room, genbuf, unum);
  }
  return 0;
}
 
#endif  /* EXTRA_CHAT_STUFF */

chat_join(unum, msg)
int unum;
char *msg;
{
  char *roomid = nextword(&msg);
  if (RESTRICTED(unum)) {
    send_to_unum(unum, 
		 "*** You do not have permission to join other rooms\n", -1);
    return 0;
  }
  if (*roomid == '\0') {
    send_to_unum(unum, "*** You must specify a room\n", -1);
    return 0;
  }
  if (!is_valid_roomname(roomid)) {
    send_to_unum(unum, "*** Invalid room name\n", -1);
    return 0;
  }
  enter_room(unum, roomid, msg);
  return 0;
}  

chat_kick(unum, msg)
int unum;
char *msg;
{
  char *twit = nextword(&msg);
  int rnum = users[unum].room;
  int recunum;
  if (!OPERATOR(unum) && !MANAGER(unum)) {
    send_to_unum(unum, "*** You're not operator\n", -1);
    return 0;
  }
  if ((recunum = chatid_to_indx(twit)) == -1) {
    /* no such user */
    sprintf(genbuf, "*** No such chatid '%s'\n", twit);
    send_to_unum(unum, genbuf, -1);
    return 0;
  }    
  if (rnum != users[recunum].room) {
    sprintf(genbuf, "*** '%s' is not in this room\n", users[recunum].chatid);
    send_to_unum(unum, genbuf, -1);
    return 0;
  }
  exit_room(recunum, EXIT_KICK, msg);

  if (rnum == 0) logout_user(recunum);        
  else enter_room(recunum, mainroom, (char *)NULL);    

  return 0;  
}  

chat_makeop(unum, msg)
int unum;
char *msg;
{
  char *newop = nextword(&msg);
  int rnum = users[unum].room;
  int recunum;
  if (!OPERATOR(unum)) {
    send_to_unum(unum, "*** You're not operator\n", -1);
    return 0;
  }
  if ((recunum = chatid_to_indx(newop)) == -1) {
    /* no such user */
    sprintf(genbuf, "*** No such chatid '%s'\n", newop);
    send_to_unum(unum, genbuf, -1);
    return 0;
  }    
  if (unum == recunum) {
    sprintf(genbuf, "*** You're already op\n");
    send_to_unum(unum, genbuf, -1);
    return 0;
  }
  if (rnum != users[recunum].room) {
    sprintf(genbuf, "*** '%s' is not in this room\n", users[recunum].chatid);
    send_to_unum(unum, genbuf, -1);
    return 0;
  }
  users[unum].flags &= ~FLG_CHATOP;
  users[recunum].flags |= FLG_CHATOP;
  sprintf(genbuf, "*** %s gave Op to %s\n", users[unum].chatid,
          users[recunum].chatid);
  send_to_room(rnum, genbuf, -1);  
  return 0;  
}  

chat_invite(unum, msg)
int unum;
char *msg;
{
  char *invitee = nextword(&msg);
  int rnum = users[unum].room;
  int recunum;
  if (!OPERATOR(unum)) {
    send_to_unum(unum, "*** You're not operator\n", -1);
    return 0;
  }
  if ((recunum = chatid_to_indx(invitee)) == -1) {
    /* no such user */
    sprintf(genbuf, "*** No such chatid '%s'\n", invitee);
    send_to_unum(unum, genbuf, -1);
    return 0;
  }    
  if (rooms[rnum].invites[recunum] == 1) {
    sprintf(genbuf, "*** %s already has a key\n", users[recunum].chatid);
    send_to_unum(unum, genbuf, -1);
    return 0;
  }
  rooms[rnum].invites[recunum] = 1;
  sprintf(genbuf, "*** %s gave you a key to room '%s'\n", 
          users[unum].chatid, rooms[rnum].name);
  send_to_unum(recunum, genbuf, unum);  
  sprintf(genbuf, "*** Key given to %s\n", users[recunum].chatid);
  send_to_unum(unum, genbuf, -1);
  return 0;  
}  

chat_broadcast(unum, msg)
int unum;
char *msg;
{
  if (!MANAGER(unum)) {
    send_to_unum(unum, "*** You can't do that\n", -1);
    return 0;
  }
  if (*msg == '\0') {
    send_to_unum(unum, "*** No message given\n", -1);
    return 0;
  }
  sprintf(genbuf, "*** Broadcast message from %s:\n", users[unum].chatid);
  send_to_room(ROOM_ALL, genbuf, -1);
  strcpy(genbuf, "*** ");
  strncat(genbuf, msg, CHATLINE_TEXT_MAX);
  send_to_room(ROOM_ALL, genbuf, -1);
  return 0;
}  

/*ARGSUSED*/
_show_ignores(indx, ignoree, unum)
int indx;
char *ignoree;
int *unum;
{
  sprintf(genbuf, "*** You are ignoring %s\n", ignoree);
  send_to_unum(*unum, genbuf, -1);
  return 0;
}

_set_ignores(ignoree, unum, status)
char *ignoree;
int unum;
int status;
{
  int inum;
  for (inum = 0; inum < bbs_max_users; inum++) {
    if (users[inum].sockfd == -1) continue;
    if (!strcasecmp(ignoree, users[inum].userid)) {
      if (users[inum].ignoring == NULL)
	users[inum].ignoring=(char *)calloc(bbs_max_users, sizeof(char));
      if (users[inum].ignoring != NULL)
	users[inum].ignoring[unum] = status;
    }
  }
}

chat_ignore(unum, msg)
int unum;
char *msg;
{
  if (*msg == '\0') {
    if (users[unum].ignorelist == NULL)
      send_to_unum(unum, "*** You're not ignoring anyone\n", -1);
    else {
      apply_namelist(users[unum].ignorelist, _show_ignores, &unum);
    }
  }
  else {
    int inum;
    char *ignoree = nextword(&msg);
    inum = userid_to_indx(ignoree);
    if (inum == -1) {
      sprintf(genbuf, "*** User '%s' is not in chat\n", ignoree);
      send_to_unum(unum, genbuf, -1);
    }
#if NO_IGNORE_CHATOPS
    else if (MANAGER(inum)) {
      sprintf(genbuf, "*** You can't ignore '%s'\n", ignoree);
      send_to_unum(unum, genbuf, -1);
    } 
#endif
    else {
      int ignoring = is_in_namelist(users[unum].ignorelist, ignoree);
      if (ignoring) {
        sprintf(genbuf, "*** Already ignoring user '%s'\n", ignoree);
      }
      else {
        add_namelist(&users[unum].ignorelist, ignoree, NULL);
        sprintf(genbuf, "*** Ignoring user '%s'\n", ignoree);
        _set_ignores(ignoree, unum, 1);
      }
      send_to_unum(unum, genbuf, -1);
    }
  }        
  return 0;  
}  

chat_unignore(unum, msg)
int unum;
char *msg;
{
  if (*msg == '\0') {
    send_to_unum(unum, "*** Must specify a userid\n", -1);
  }
  else {
    char *ignoree = nextword(&msg);
    int removing = is_in_namelist(users[unum].ignorelist, ignoree);
    if (removing) {
      remove_namelist(&users[unum].ignorelist, ignoree);
      sprintf(genbuf, "*** No longer ignoring user '%s'\n", ignoree);
      _set_ignores(ignoree, unum, 0);
    }
    else {
      sprintf(genbuf, "*** You aren't ignoring '%s'\n", ignoree);
    }
    send_to_unum(unum, genbuf, -1);
  }
  return 0;  
}  

chat_goodbye(unum, msg)
int unum;
char *msg;
{
  exit_room(unum, EXIT_LOGOUT, msg);
  return 0;  
}  

struct chatcmd cmdlist[] = 
{ "action", chat_emote, 0,
  "date", chat_time, 0,
  "exit", chat_goodbye, 0,
  "flags", chat_setroom, 0,
  "ignore", chat_ignore, 1,
  "invite", chat_invite, 0,
  "join", chat_join, 0,
  "kick", chat_kick, 0,
  "me", chat_emote, 1,
  "msg", chat_private, 0,
  "nick", chat_nick, 0,
  "operator", chat_makeop, 0,
#if EXTRA_CHAT_STUFF
  "roll", chat_roll_dice, 1,
#endif
  "rooms", chat_list_rooms, 0, 
#if EXTRA_CHAT_STUFF
  "think", chat_think, 0,
#endif
  "unignore", chat_unignore, 1,
  "whoin", chat_list_by_room, 1,
  "whois", chat_query_chatid, 1,
  "wall", chat_broadcast, 1,
  "who", chat_list_users, 0,
  NULL, NULL, 0
};

command_execute(unum)
int unum;
{
  char *msg = users[unum].ibuf;
  char *cmd;
  struct chatcmd *cmdrec;
  int match = 0;

#ifdef DEBUG
  printf("command_execute: %s\n", msg);
#endif

  /* Validation routine */
  if (users[unum].room == -1) {
    /* MUST give special /! command if not in the room yet */
    if (msg[0] != '/' || msg[1] != '!') return -1;    
    else return (login_user(unum, msg+2));
  }

  /* If not a /-command, it goes to the room. */
  if (msg[0] != '/') {
    chat_allmsg(unum, msg);
    return 0;
  }

  msg++;
  cmd = nextword(&msg);

  /* Look up the command in the chat command table. */
  for (cmdrec = cmdlist; !match && cmdrec->cmdstr != NULL; cmdrec++) {
    if (cmdrec->exact) match = !strcasecmp(cmd, cmdrec->cmdstr);
    else match = !strncasecmp(cmd, cmdrec->cmdstr, strlen(cmd));
    if (match) cmdrec->cmdfunc(unum, msg);
  }

  if (!match) {      
    /* invalid input */
    sprintf(genbuf, "*** Unknown command '/%s'\n", cmd);
    send_to_unum(unum, genbuf, -1);
  }

  memset(users[unum].ibuf, 0, sizeof users[unum].ibuf);
  return 0;
}

process_chat_command(unum)
int unum;
{
  register int i;
  int sd, rc, ibufsize;
  CHATLINE recbuf;
  
  sd = users[unum].sockfd;
#ifdef DEBUG
  printf("Incoming on fd %d: ", sd);
#endif
  if ((rc = recv(sd, recbuf, sizeof recbuf, 0)) <= 0) {
    /* disconnected */
    exit_room(unum, EXIT_LOSTCONN, (char *)NULL);
    return -1;
  }
  ibufsize = users[unum].ibufsize;
  for (i=0; i<rc; i++) {
    /* if newline is two characters, throw out the first */
    if (recbuf[i] == '\r') continue;

    /* carriage return signals end of line */
    else if (recbuf[i] == '\n') {
      users[unum].ibuf[ibufsize] = '\0';
      if (command_execute(unum) == -1) return -1;
      ibufsize = 0;
    }

    /* add other chars to input buffer unless size limit exceeded */
    else {
      if (ibufsize < CHATLINE_TEXT_MAX)
        users[unum].ibuf[ibufsize++] = recbuf[i];
    }
  }
  users[unum].ibufsize = ibufsize;
  return 0;
}

/*ARGSUSED*/
exit_chatd(rc, str)
int rc;
char *str;
{
  int i;
#ifdef DEBUG
  printf("Server exit: %s\n", str);
#endif
  close(sock);
  unlink(PATH_CHATPORT);
  unlink(PATH_CHATPID);
  utable_detach(0);
  if (rc) {
    for (i=0; i<bbs_max_users; i++)
      if (users[i].sockfd != -1) close(users[i].sockfd);
  }
  free(users);
  free(rooms);
  exit(rc);
}

void
sig_catcher(sig)
{
  char msg[80];
  sprintf(msg, "shutting down due to signal %d\n", sig);
  signal(sig, SIG_DFL);
  exit_chatd(1, msg);
}  

_write_daemoninfo(fname, number)
char *fname;
unsigned short number;
{
  FILE *fp;
  if ((fp = fopen(fname, "w")) == NULL) {
    fprintf(stderr, "cannot open %s\n", fname);
    perror("fopen");
    exit_chatd(1, "cannot write chatport or chatpid file");
  }
  fprintf(fp, "%04x\n", number);
  fclose(fp);
}

main(argc, argv)
int argc;
char *argv[];
{
  struct sockaddr_in sin;
  register int i;
  int pid, sr, newsock, sinsize;
  fd_set readfds;
  struct timeval *tvptr = NULL;
  if (argc < 2) {
    fprintf(stderr, "Usage: %s numusers\n" ,argv[0]);
    return 1;
  }
  
  bbs_max_users = atoi(argv[1]);

  if (bbs_max_users == 0) {
    fprintf(stderr, "maxusers cannot be zero\n");
    return 1;
  }

  manager_list = restricted_list = NULL;
  strcpy(mainroom, "main");    
  chat_init_config();

  users = (struct chatuser *)calloc(bbs_max_users, sizeof(*users));
  if (users == NULL) {
    perror("calloc");
    return 1;
  }
  for (i=0; i<bbs_max_users; i++) users[i].sockfd = users[i].utent = -1;

  rooms = (struct chatroom *)calloc(bbs_max_users, sizeof(*rooms));
  if (rooms == NULL || 
     (rooms[0].invites = (char *)malloc(bbs_max_users)) == NULL) {
    perror("calloc");
    return 1;
  }
  strcpy(rooms[0].name, mainroom);

  if (utable_attach(bbs_max_users) != S_OK) {
    fprintf(stderr, "cannot attach to utable\n");
    return 1;
  }

  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit_chatd(1, "socket failed");
  }

  sin.sin_family = AF_INET;
  sin.sin_port = 0;
  sin.sin_addr.s_addr = INADDR_ANY;

  if (bind(sock, (struct sockaddr *)&sin, sizeof sin) == -1) {
    perror("bind");
    exit_chatd(1, "bind failed");
  }

  sinsize = sizeof sin;
  if (getsockname(sock, (struct sockaddr *)&sin, &sinsize) == -1) {
    perror("getsockname");
    exit_chatd(1, "getsockname failed");
  }

  if (listen(sock, SOMAXCONN) == -1) {
    perror("listen");
    exit_chatd(1, "listen failed");
  }

  /* race condition here if two daemons start up at once? */
  _write_daemoninfo(PATH_CHATPORT, htons(sin.sin_port));

#ifndef DEBUG
  switch (pid = fork()) {
  case -1:
    perror("fork");
    exit_chatd(1, "fork failed");
  case 0:
    setpgid(0, 0);
    break;
  default:
    /* Assuming that a pid fits into a ushort may be a bad idea. */
    _write_daemoninfo(PATH_CHATPID, (unsigned short)pid);
    return 0;
  }
#else
  printf("Server bound to port %d\n", htons(sin.sin_port));
#endif

  signal(SIGHUP, sig_catcher);
  signal(SIGINT, sig_catcher);
#ifndef DEBUG
  signal(SIGQUIT, sig_catcher);
#endif
  signal(SIGILL, sig_catcher);
  signal(SIGIOT, sig_catcher);
#ifdef SIGEMT
  signal(SIGEMT, sig_catcher);
#endif
  signal(SIGFPE, sig_catcher);
#ifdef SIGBUS
  signal(SIGBUS, sig_catcher);
#endif
  signal(SIGSEGV, sig_catcher);
#ifdef SIGSYS
  signal(SIGSYS, sig_catcher);
#endif
  signal(SIGPIPE, SIG_IGN);
  signal(SIGTERM, sig_catcher);
  signal(SIGALRM, SIG_IGN);
#ifdef SIGIO
  signal(SIGIO, SIG_IGN);
#endif
#ifdef SIGURG
  signal(SIGURG, SIG_IGN);
#endif
#ifdef SIGPWR
  signal(SIGPWR, sig_catcher);
#endif

  FD_ZERO(&allfds);
  FD_SET(sock, &allfds);
  nfds = sock+1;

  while (1) {
    memcpy(&readfds, &allfds, sizeof readfds);
#ifdef DEBUG
    printf("Selecting...");
#endif
    sr = select(nfds, &readfds, NULL, NULL, tvptr);
#ifdef DEBUG
    printf("done: returned %d\n", sr);
#endif
    if (sr == 0) {
      exit_chatd(0, "normal chat server shutdown");      
    }

    if (sr == -1) {
      if (errno == EINTR) continue;
      else {
	exit_chatd(1, "select failed");
      }
    }

    if (tvptr) tvptr = NULL;

    if (FD_ISSET(sock, &readfds)) {
      sinsize = sizeof sin;
      newsock = accept(sock, (struct sockaddr *)&sin, &sinsize);
      if (newsock == -1) {
#ifdef DEBUG
        printf("accept failed: errno %d\n", errno);
#endif	
	continue;
      }
      for (i=0; i<bbs_max_users; i++) {
        if (users[i].sockfd == -1) {
          users[i].sockfd = newsock;
          users[i].room = -1;
	  users[i].ibufsize = 0;
#ifdef DEBUG
	  printf("Connection to slot %2d, fd %d\n", i, newsock);
#endif
          break;
        }
      }
      if (i >= bbs_max_users) {
        /* full -- no more chat users */
        close(newsock);
      }
      else {
#ifndef RELIABLE_SELECT_FOR_WRITE
        int flags = fcntl(newsock, F_GETFL, 0);
        flags |= O_NDELAY;
        fcntl(newsock, F_SETFL, flags);
#endif
        FD_SET(newsock, &allfds);
        if (newsock >= nfds) nfds = newsock+1;
        num_conns++;
      }
    }

    for (i=0; i<bbs_max_users; i++) {
      /* we are done with newsock, so re-use the variable */
      newsock = users[i].sockfd;
      if (newsock != -1 && FD_ISSET(newsock, &readfds)) {
        if (process_chat_command(i) == -1) {
          logout_user(i);  /* this clear the bit in allfds & dec. num_conns */
	}
      }
    }

    if (num_conns <= 0) {
      /* one more pass at select, then we go bye-bye */
      tvptr = &zerotv;
    }
  }
  /*NOTREACHED*/
  return 0;
}

