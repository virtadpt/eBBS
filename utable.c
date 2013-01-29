
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
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef NO_FLOCK
# define LOCK(fd)   lockf(fd, F_LOCK, 0)
# define UNLOCK(fd) lockf(fd, F_ULOCK, 0)
#else
# include <sys/file.h> 
# define LOCK(fd)   flock(fd, LOCK_EX)
# define UNLOCK(fd) flock(fd, LOCK_UN)
#endif

#ifndef NO_SHARED_MEMORY
# include <sys/ipc.h>
# include <sys/shm.h>

int _utable_in_shared_memory = 1;
#endif

/* Maximum number of active users, set by utable_attach. */
int utable_sz = 0;

extern SERVERDATA server;

/*
   First the file implementation of the user table. Systems which do not
   support shared memory must use this. Also systems which do support
   it as a kernel option (Linux, SunOS, others?) can fall back to this
   if their kernel isn't set up for it.
*/

#define UTABLEFILE       "etc/.utable"

f_utable_attach(numusers)
int numusers;
{
  int fd;
  if (utable_sz != 0) return S_ATTACHED;
  if ((fd = open(UTABLEFILE, O_RDWR | O_CREAT, 0600)) == -1) {
    bbslog(0, "ERROR f_utable_attach: cannot open %s\n", UTABLEFILE);
    return S_SYSERR;
  }
  utable_sz = numusers;
  close(fd);
  return S_OK;
}

/*ARGSUSED*/
f_utable_detach(destroy)
int destroy;
{
  return S_OK;
}

f_utable_lock_record(precnum)
int *precnum;
{
  int i, fd, rc, maxrec;
  USERDATA data;
  if (utable_sz == 0) return S_NOUTABLE;

  if ((fd = open(UTABLEFILE, O_RDWR)) == -1) {
    bbslog(0, "ERROR f_utable_lock_record: cannot open %s\n", UTABLEFILE);
    return S_SYSERR;
  }

  maxrec = utable_sz;
  if (!_has_access(C_USERESERVED)) maxrec -= server.reservedslots;
  LOCK(fd);
  for (i=0; i<maxrec; i++) {
    rc = read(fd, &data, sizeof data);
    if (rc == sizeof data) {
      if (data.u.mode == M_EMPTY || kill(data.u.pid, 0) == -1) {
        lseek(fd, -sizeof(USERDATA), SEEK_CUR);
        break;
      }
    }
    else if (rc >= 0) {
      lseek(fd, -rc, SEEK_CUR);
      break;
    }
    else {
      bbslog(0, "ERROR f_utable_lock_record: read failed\n");
      UNLOCK(fd);
      close(fd);
      return S_SYSERR;
    }
  }
  
  if (i < maxrec) {
    memset(&data, 0, sizeof data);
    data.u.mode = M_CONNECTING;
    rc = write(fd, &data, sizeof data);
    if (precnum && rc == sizeof data) *precnum = i;
    UNLOCK(fd);
    close(fd);
    return (rc == sizeof data ? S_OK : S_SYSERR);
  }

  UNLOCK(fd);
  close(fd);
  return S_FULL;
}

f_utable_free_record(recnum)
int recnum;
{
  int fd;
  struct stat stbuf;
  USERDATA data;
  int offset = recnum*(sizeof(USERDATA));  

  if (utable_sz == 0) return S_NOUTABLE;
  if (recnum < 0 || recnum > utable_sz-1) return S_OUTOFRANGE;
  memset(&data, 0, sizeof data);
  data.u.mode = M_EMPTY;

  if ((fd = open(UTABLEFILE, O_WRONLY)) == -1) {
    bbslog(0, "ERROR f_utable_free_record: cannot open %s\n", UTABLEFILE);
    return S_SYSERR;
  }
  fstat(fd, &stbuf);
  if (offset >= stbuf.st_size) {
    close(fd);
    return S_OUTOFRANGE;
  }
  lseek(fd, offset, SEEK_SET);
  if (write(fd, &data, sizeof(data)) != sizeof(data)) {
    close(fd);
    return S_SYSERR;
  }
  close(fd);
  return S_OK;
}

f_utable_get_record(recnum, buf)
int recnum;
USERDATA *buf;
{
  int fd;
  struct stat stbuf;
  USERDATA data;
  int offset = recnum*(sizeof(USERDATA));  
  if (utable_sz == 0) return S_NOUTABLE;
  if (recnum < 0 || recnum > utable_sz-1) return S_OUTOFRANGE;
  if ((fd = open(UTABLEFILE, O_RDONLY)) == -1) {
    bbslog(0, "ERROR f_utable_get_record: cannot open %s\n", UTABLEFILE);
    return S_SYSERR;
  }
  fstat(fd, &stbuf);
  if (offset >= stbuf.st_size) {
    close(fd);
    return S_OUTOFRANGE;
  }
  lseek(fd, offset, SEEK_SET);
  if (read(fd, &data, sizeof(data)) != sizeof(data)) {
    close(fd);
    return S_SYSERR;
  }
  close(fd);
  if (data.u.mode == M_EMPTY) return S_EMPTYREC;
  memcpy(buf, &data, sizeof(*buf));
  return S_OK;
}

f_utable_set_record(recnum, buf)
int recnum;
USERDATA *buf;
{
  int fd;
  struct stat stbuf;
  USERDATA data;
  int offset = recnum*(sizeof(USERDATA));  
  if (utable_sz == 0) return S_NOUTABLE;
  if (recnum < 0 || recnum > utable_sz-1) return S_OUTOFRANGE;
  if ((fd = open(UTABLEFILE, O_RDWR)) == -1) {
    bbslog(0, "ERROR f_utable_set_record: cannot open %s\n", UTABLEFILE);
    return S_SYSERR;
  }
  fstat(fd, &stbuf);
  if (offset >= stbuf.st_size) {
    close(fd);
    return S_OUTOFRANGE;
  }
  lseek(fd, offset, SEEK_SET);
  if (read(fd, &data, sizeof(data)) != sizeof(data)) {
    close(fd);
    return S_SYSERR;
  }
  if (data.u.mode == M_EMPTY) {
    close(fd);
    return S_EMPTYREC;
  }
  lseek(fd, -sizeof(USERDATA), SEEK_CUR);
  if (write(fd, buf, sizeof(*buf)) != sizeof(*buf)) {
    close(fd);
    return S_SYSERR;
  }
  close(fd);
  return S_OK;
}

f_utable_find_record(pid, buf)
LONG pid;
USERDATA *buf;
{
  int fd;
  USERDATA data;
  if (utable_sz == 0) return S_NOUTABLE;
  if ((fd = open(UTABLEFILE, O_RDONLY)) == -1) {
    return S_NOSUCHUSER;
  }
  while (read(fd, &data, sizeof data) == sizeof data) {
    if (data.u.mode == M_EMPTY) continue;
    if (data.u.pid == pid) {
      if (buf) memcpy(buf, &data, sizeof(*buf));
      close(fd);
      return S_OK;
    }
  }
  close(fd);
  return S_NOSUCHUSER;
}

f_utable_enumerate(startrec, userid, func, arg)
int startrec;
char *userid;
int (*func)();
void *arg;
{
  int fd, indx, retval = 0;
  USERDATA udata;
  if (utable_sz == 0) return S_NOUTABLE;
  if (startrec < 0 || startrec > utable_sz-1) return S_OUTOFRANGE;
  if ((fd = open(UTABLEFILE, O_RDONLY)) == -1) {
    return S_OK;
  }
  indx = startrec;
  lseek(fd, indx*sizeof(udata), SEEK_SET);
  while (indx < utable_sz && retval != ENUM_QUIT) {
    if (read(fd, &udata, sizeof udata) != sizeof udata) break;
    if (udata.u.mode != M_EMPTY &&
        udata.u.mode != M_CONNECTING &&
        kill(udata.u.pid, 0) == 0 &&
        (userid == NULL || !strcasecmp(userid, udata.u.userid))) { 
      retval = (*func)(indx, &udata, arg);
    }
    indx++;
  }
  close(fd);
  return S_OK;
}

#ifndef NO_SHARED_MEMORY

/* Shared memory implementation of the user table. */

# define KEY_BBS_MIN     90
# define KEY_BBS_MAX     100
# define KEYFILE         "etc/.shmkey"

USERDATA *utableptr = NULL;

int utable_shmid = -1;

_read_shmkey()
{
  FILE *fp;
  char buf[8];
  int key = -1;
  if ((fp = fopen(KEYFILE, "r")) != NULL) {
    if (fgets(buf, sizeof buf, fp) != NULL) key = atoi(buf);
    fclose(fp);
  }
  return key;      
}  

_write_shmkey(key)
int key;
{
  FILE *fp;
  char buf[8];
  if ((fp = fopen(KEYFILE, "w")) != NULL) {
    sprintf(buf, "%05d\n", key);
    fputs(buf, fp);
    fclose(fp);
  }
  return (fp == NULL ? -1 : 0);
}  

utable_attach(numusers)
int numusers;
{
  int key, i, created = 0;
  int memsize;
  if (utableptr) return S_ATTACHED;
  memsize = sizeof(USERDATA)*server.maxutable;
  key = _read_shmkey();
  if (key == -1) {
    key = KEY_BBS_MIN;
    while (key < KEY_BBS_MAX && !created) {
      if ((utable_shmid = shmget(key, memsize, 0600|IPC_CREAT)) == -1) key++;
      else created = 1;
    }
  }
  else {
    utable_shmid = shmget(key, memsize, 0600);
    if (utable_shmid == -1 && errno == ENOENT) {
      created = 1;
      utable_shmid = shmget(key, memsize, 0600|IPC_CREAT);
    }
  }

  if (utable_shmid == -1) {
#if FORCE_SHARED_MEMORY
    bbslog(0, "ERROR utable_attach: shmget failed\n");
    return S_SYSERR;
#else
    _utable_in_shared_memory = 0;
    return (f_utable_attach(numusers));
#endif
  }

  utableptr = (USERDATA *)shmat(utable_shmid, NULL, 0);
  if (utableptr == NULL || utableptr == (USERDATA *)-1) {
    utable_detach(1);
#if FORCE_SHARED_MEMORY
    bbslog(0, "ERROR utable_attach: shmat failed\n");
    return S_SYSERR;
#else
    _utable_in_shared_memory = 0;
    return(f_utable_attach(numusers));
#endif
  }
  utable_sz = numusers;
  if (created) {
    _write_shmkey(key);
    memset(utableptr, 0, memsize);
    for (i=0; i<server.maxutable; i++) utableptr[i].u.mode = M_EMPTY;
  }

  return S_OK;
}

utable_detach(destroy)
int destroy;
{
  if (!_utable_in_shared_memory) 
    return f_utable_detach(destroy);
  if (utableptr == NULL) return S_NOUTABLE;

  if (shmdt((char *)utableptr) == -1) {
    bbslog(0, "ERROR utable_detach: shmdt failed\n");
    return S_SYSERR;
  }

  utableptr = NULL;

  if (destroy) {
    if (shmctl(utable_shmid, IPC_RMID, (struct shmid_ds *)NULL) == -1) {
      bbslog(0, "ERROR utable_detach: shmctl(IPC_RMID) failed\n");
      return S_SYSERR;
    }
  }

  return S_OK;
}

utable_lock_record(precnum)
int *precnum;
{
  int i, maxrec;
  if (!_utable_in_shared_memory) 
    return f_utable_lock_record(precnum);
  if (utableptr == NULL) return S_NOUTABLE;
  maxrec = utable_sz;
  if (!_has_access(C_USERESERVED)) maxrec -= server.reservedslots;
  for (i=0; i<maxrec; i++) {
    if (utableptr[i].u.mode == M_EMPTY || kill(utableptr[i].u.pid, 0) == -1) {
      memset(&utableptr[i], 0, sizeof(utableptr[i]));
      utableptr[i].u.mode = M_CONNECTING;
      if (precnum) *precnum = i;
      return S_OK;
    }
  }

  return S_FULL;
}
            
utable_free_record(recnum)
int recnum;
{
  if (!_utable_in_shared_memory) 
    return f_utable_free_record(recnum);
  if (utableptr == NULL) return S_NOUTABLE;

  if (recnum < 0 || recnum > utable_sz-1) return S_OUTOFRANGE;

  utableptr[recnum].u.mode = M_EMPTY;
  return S_OK;
}

utable_get_record(recnum, buf)
int recnum;
USERDATA *buf;
{
  if (!_utable_in_shared_memory) 
    return f_utable_get_record(recnum, buf);
  if (utableptr == NULL) return S_NOUTABLE;

  if (recnum < 0 || recnum > utable_sz-1) return S_OUTOFRANGE;
  if (utableptr[recnum].u.mode == M_EMPTY) return S_EMPTYREC;
  memcpy(buf, &utableptr[recnum], sizeof(*buf));
  return S_OK;
}

utable_set_record(recnum, buf)
int recnum;
USERDATA *buf;
{
  if (!_utable_in_shared_memory) 
    return f_utable_set_record(recnum, buf);
  if (utableptr == NULL) return S_NOUTABLE;

  if (recnum < 0 || recnum > utable_sz-1) return S_OUTOFRANGE;
  if (utableptr[recnum].u.mode == M_EMPTY) return S_EMPTYREC;
  memcpy(&utableptr[recnum], buf, sizeof(*buf));
  return S_OK;
}

utable_find_record(pid, buf)
LONG pid;
USERDATA *buf;
{
  int recnum;
  if (!_utable_in_shared_memory) 
    return f_utable_find_record(pid, buf);
  if (utableptr == NULL) return S_NOUTABLE;

  for (recnum = 0; recnum < utable_sz; recnum++) {
    if (utableptr[recnum].u.mode == M_EMPTY) continue;
    if (utableptr[recnum].u.pid == pid) {
      if (buf) memcpy(buf, &utableptr[recnum], sizeof(*buf));
      return S_OK;
    }
  }
  return S_NOSUCHUSER;
}

utable_enumerate(startrec, userid, func, arg)
int startrec;
char *userid;
int (*func)();
void *arg;
{
  int indx, retval = 0;
  USERDATA udata;
  if (!_utable_in_shared_memory) 
    return f_utable_enumerate(startrec, userid, func, arg);
  if (utableptr == NULL) return S_NOUTABLE;

  if (startrec < 0 || startrec >= utable_sz) return S_OUTOFRANGE;
  indx = startrec;
  while (indx < utable_sz && retval != ENUM_QUIT) {
    if (utableptr[indx].u.mode != M_EMPTY &&
        utableptr[indx].u.mode != M_CONNECTING &&
        kill(utableptr[indx].u.pid, 0) == 0 &&
        (userid == NULL || !strcasecmp(userid, utableptr[indx].u.userid))) { 
      memcpy(&udata, &utableptr[indx], sizeof udata);
      retval = (*func)(indx, &udata, arg);
    }
    indx++;
  }
  return S_OK;
}

#endif /* NO_SHARED_MEMORY */
