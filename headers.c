
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
#include <time.h>

#define HEADER_TAG_SIZE 11
#define HEADERSIZE      128  /* must be greater than HEADER_TAG_SIZE+ADDRLEN */

read_headers(fname, hdr)
char *fname;
HEADER *hdr;
{
  char buf[HEADERSIZE];
  FILE *fp;
  char *hdrdata, *space;

  memset(hdr->owner, '\0', sizeof(hdr->owner));
  memset(hdr->title, '\0', sizeof(hdr->title));
  
  fp = fopen(fname, "r");
  if (fp == NULL) return S_SYSERR;

  hdrdata = buf+HEADER_TAG_SIZE;
  while (fgets(buf, sizeof buf, fp)) {
    strip_trailing_space(buf);
    if (buf[0] == '\0') break;
    else if (!strncmp(buf, "From: ", 6)) {
      if (space = strchr(hdrdata, ' ')) *space = '\0';
      strncpy(hdr->owner, hdrdata, sizeof(hdr->owner)-1);
    }
    else if (!strncmp(buf, "Posted By: ", 11)) {
      if (space = strchr(hdrdata, ' ')) *space = '\0';
      strncpy(hdr->owner, hdrdata, sizeof(hdr->owner)-1);
    }
    else if (!strncmp(buf, "Title: ", 7))
      strncpy(hdr->title, hdrdata, sizeof(hdr->title)-1);
    else if (!strncmp(buf, "Subject: ", 9))
      strncpy(hdr->title, hdrdata, sizeof(hdr->title)-1);
  }

  fclose(fp);
  return S_OK;
}

struct writetostruct {
  int fd;
  char *buf;
};

write_to_header(indx, userid, info)
int indx;
char *userid;
struct writetostruct *info;
{
  int i;
  int len = strlen(info->buf);
  int needed = strlen(userid)+3;
  if ((80 - len) < needed) {
    strcat(info->buf, ",\n");
    write(info->fd, info->buf, len+2);
    for (i=0; i<HEADER_TAG_SIZE; i++) info->buf[i] = ' ';
    info->buf[HEADER_TAG_SIZE] = '\0';
  }
  else if (len > HEADER_TAG_SIZE) strcat(info->buf, ", ");
  strcat(info->buf, userid);
  return S_OK;
}  

/* What a mess. I should use stdio. */

write_mail_headers(fd, hdr, username, list)
int fd;
HEADER *hdr;
char *username;
NAMELIST list;
{
  char fmt[40];
  char hdrline[HEADERSIZE];
  time_t now = time(NULL);
  struct writetostruct tostruct;
  
  sprintf(fmt, "%%-%ds", HEADER_TAG_SIZE);

  sprintf(hdrline, fmt, "From:");
  strcat(hdrline, hdr->owner);
  strcat(hdrline, " (");
  strcat(hdrline, username);         /* hdrline is big enough */
  strcat(hdrline, ")\n");
  write(fd, hdrline, strlen(hdrline));

  sprintf(hdrline, fmt, "Subject:");
  strcat(hdrline, hdr->title);       /* again, hdrline is big enough */
  strcat(hdrline, "\n");
  write(fd, hdrline, strlen(hdrline));

  sprintf(hdrline, fmt, "Date:");
  strcat(hdrline, ctime(&now));
  write(fd, hdrline, strlen(hdrline));

  sprintf(hdrline, fmt, "To:");
  tostruct.fd = fd;
  tostruct.buf = hdrline;
  apply_namelist(list, write_to_header, &tostruct);
  if (strlen(hdrline) > HEADER_TAG_SIZE) {
    strcat(hdrline, "\n");
    write(fd, hdrline, strlen(hdrline));
  }
  write(fd, "\n", 1);
}

write_post_headers(fd, hdr, username, bname)
int fd;
HEADER *hdr;
char *username;
char *bname;
{
  char fmt[40];
  char hdrline[HEADERSIZE];
  time_t now = time(NULL);
  struct writetostruct tostruct;
  
  sprintf(fmt, "%%-%ds", HEADER_TAG_SIZE);

  sprintf(hdrline, fmt, "Posted By:");
  strcat(hdrline, hdr->owner);
  strcat(hdrline, " (");
  strcat(hdrline, username);         /* hdrline is big enough */
  strcat(hdrline, ") on '");
  strcat(hdrline, bname);
  strcat(hdrline, "'\n");
  write(fd, hdrline, strlen(hdrline));

  sprintf(hdrline, fmt, "Title:");
  strcat(hdrline, hdr->title);       /* again, hdrline is big enough */
  strcat(hdrline, "\n");
  write(fd, hdrline, strlen(hdrline));

  sprintf(hdrline, fmt, "Date:");
  strcat(hdrline, ctime(&now));
  write(fd, hdrline, strlen(hdrline));

  write(fd, "\n", 1);
}

/* This one is used by the client side */

parse_to_list(list, fname, myname)
NAMELIST *list;
char *fname;
char *myname;
{
  FILE *fp;
  char header[HEADERSIZE];
  char *ptr, *id;
  int gotit = 0;

  if ((fp = fopen(fname, "r")) == NULL) return;
  while (fgets(header, sizeof header, fp) && header[0] != '\n') {
    if (!gotit && strncmp(header, "To: ", 4)) continue;
    if (gotit && strncmp(header, "    ", 4)) break;
    gotit = 1;
    ptr = header+4;
    while (id = strtok(ptr, " \t\n,")) {
      ptr = NULL;
      if (strcmp(id, myname) && !is_in_namelist(*list, id))
        add_namelist(list, id, NULL);
    }
  }
  fclose(fp);
}

