
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

#include "common.h"
#include <ctype.h>
#ifdef NeXT
# include <sys/dir.h>
# include <sys/dirent.h>
#else
# include <dirent.h>
#endif
#include <fcntl.h>
#include <time.h>
#include <sys/stat.h>
#ifdef NeXT
# define S_ISDIR(mode)   (((mode) & (_S_IFMT)) == (_S_IFDIR))
#endif

/* Miscellaneous useful functions */

#ifdef NO_STRCASECMP
strcasecmp(s1,s2)
register char *s1,*s2 ;
{
  for(;;s1++,s2++) {
    if(*s1=='\0' || *s2 == '\0')
      return (*s1=='\0' && *s2=='\0' ? 0 : 1);
    if((isalpha(*s1)?*s1|0x20:*s1) != (isalpha(*s2)?*s2|0x20:*s2))
      return 1;
  }
  /*NOTREACHED*/
  return 1;
}

strncasecmp(s1,s2,n)
register char *s1,*s2 ;
register int n ;
{
  for(;n;s1++,s2++,n--) {
    if(*s1=='\0' && *s2 == '\0')
      break ;
    if((isalpha(*s1)?*s1|0x20:*s1) != (isalpha(*s2)?*s2|0x20:*s2))
      return 1;
  }
  return 0;
}
#endif

void
strip_trailing_space(str)
char *str;
{
  char *eos;
  if (str == NULL || *str == '\0') return;
  eos = str+strlen(str)-1;
  while (eos >= str && isspace(*eos))
    *eos-- = '\0';
}

is_directory(fname)
char *fname;
{
  struct stat stbuf;
  if (stat(fname, &stbuf) == 0 && S_ISDIR(stbuf.st_mode)) return 1;
  return 0;
}

recursive_rmdir(dir)
char *dir;
{
  PATH fname;
  DIR *dp;
  struct dirent *dent;

  if ((dp = opendir(dir)) == NULL) {
    return -1;
  }
              
  while ((dent = readdir(dp)) != NULL) {
    if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))
      continue;
    sprintf(fname, "%s/%s", dir, dent->d_name);
    if (is_directory(fname)) recursive_rmdir(fname);
    else unlink(fname);
  }

  closedir(dp);
  if (rmdir(dir)) return -1;
  else return 0;
}

/* I want to strangle whoever decided that ctime should return a string 
   with a FREAKING NEWLINE on the end. */

char *
Ctime(t)
time_t *t;
{
  static char ctbuf[40];
  char *ct;
  if (ct = ctime(t)) strncpy(ctbuf, ct, strlen(ct)-1);
  else ctbuf[0] = '\0';
  return ctbuf;
}

is_valid_userid(userid)
char *userid;
{
  int len = strlen(userid);
  if (len < 2) return 0;
  if (!isalpha(userid[0])) return 0;
  if (!isalnum(userid[len-1])) return 0;
  while (*userid) {
    if (!isalnum(*userid)) {
      if (*userid != '-' && *userid != '_') return 0;
      if (!isalnum(*(userid-1))) return 0;
    }    
    userid++;
  }
  return 1;
}
    
is_valid_password(passwd)
char *passwd;
{
  if (!passwd || !*passwd) return 0;
  return 1;
}

is_valid_boardname(bname)
char *bname;
{
  int len = strlen(bname);
  if (len < 1) return 0;
  while (*bname) {
    if (!isalnum(*bname)) return 0;
    bname++;
  }
  return 1;
}

/* 
   Append contents of fname to open file descriptor fd.
   fd is assumed to be pointing at end-of-file.
   fd is left open by this function.
*/

append_file(fd, fname)
int fd;
char *fname;
{
  char buf[4096];
  int bytes;
  int nfd = open(fname, O_RDONLY);
  if (nfd == -1) return -1;
  
  while ((bytes = read(nfd, buf, sizeof buf)) && bytes != -1) {
    write(fd, buf, bytes);
  }
  close(nfd);
  return 0;
}

copy_file(src, dest, fmode, append)
char *src, *dest;
int fmode, append;
{
  int omode, fd, rc;
  omode = (append ? O_WRONLY|O_APPEND|O_CREAT : O_WRONLY|O_TRUNC|O_CREAT);
  if ((fd = open(dest, omode, fmode)) == -1) return -1;
  rc = append_file(fd, src);
  close(fd);
  return rc;
}

LONG
hex2LONG(str)
char *str;
{
  char c;  
  int i;
  LONG val = 0;
  for (i=0; i<8; i++) {
    val <<= 4;
    c = tolower(str[i]);
    if (c >= '0' && c <= '9') val += (c-'0');
    else if (c >= 'a' && c <= 'f') val += (c-'a'+10);
    else return 0;
  }
  return val;
}        

SHORT
hex2SHORT(str)
char *str;
{
  char c;  
  int i;
  SHORT val = 0;
  for (i=0; i<4; i++) {
    val <<= 4;
    c = tolower(str[i]);
    if (c >= '0' && c <= '9') val += (c-'0');
    else if (c >= 'a' && c <= 'f') val += (c-'a'+10);
    else return 0;
  }
  return val;
}        

char *
LONGcpy(str, lval)
char *str;
LONG lval;
{
  char nbuf[9];
  sprintf(nbuf, "%08x", lval);
  memcpy(str, nbuf, 8);
  return (str+8);
}
  
char *
SHORTcpy(str, sval)
char *str;
SHORT sval;
{
  char nbuf[5];
  sprintf(nbuf, "%04x", sval);
  memcpy(str, nbuf, 4);
  return (str+4);
}

char *ascii_exts [] = {
   "c", "h", "txt", "pl", "cpp", "p", "f", "pas", "bas", "cxx", NULL
};

char *binary_exts [] = {
   "z", "gz", "tar", "zip", "zoo", "arc", "arj", "gif", "o", "obj", "a",
   "exe", "com", "lib", "dll", "wav", "cur", "ani", "jpg", "tif", "bin",
   NULL
};

is_text_file(path)
char *path;
{
  int fd, cc, i;
  char buf[1024];
  char *dot, **cp;

  /* Look for common extensions first and make assumptions, wise we hope */
  if (dot = strrchr(path, '.')) {
    dot++;
    for (cp = ascii_exts; *cp != NULL; cp++)
      if (!strcasecmp(dot, *cp)) return 1;
    for (cp = binary_exts; *cp != NULL; cp++)
      if (!strcasecmp(dot, *cp)) return 0;
  }

  if ((fd = open(path, O_RDONLY)) == -1) return 0;
  cc = read(fd, buf, sizeof buf);
  close(fd);
  if (cc == -1) return 0;
  for (i=0; i<cc; i++) {
    if (!isprint(buf[i]) && !isspace(buf[i])) return 0;
  }
  return 1;  
}        
