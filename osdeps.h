
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

/* 
   All operating system dependent stuff should go here.
   If your OS has already been ported to, pick it below and you do not
   have to edit any other source files.
   If not you'll have to figure out the right settings and maybe add some.
*/

#define LINUX		1 	/* Linux 1.x or 2.x */
#define SUNOS           0	/* SunOS version 4.x */
#define SOLARIS         0       /* SunOS version 5.x */
#define AIX             0       /* IBM AIX 3.2.x */
#define OSF             0       /* Digital Unix (LINUX works too) */
#define NEXTSTEP        0       /* NeXTStep 3.2 -- also uses #ifdef NeXT */
#define AUX             0       /* Apple A/UX */
#define UNIXWARE        0       /* Novell's UnixWare (SVR4) */
#define ULTRIX          0       /* Ultrix 4.3 (LINUX works too) */
#define MACHTEN         0       /* MachTen */
#define HPUX            0       /* HP-UX 9.05 */
#define FREEBSD         0       /* FreeBSD */
#define IRIX            0       /* SGI IRIX 5.3 */

/* 
   LONG must be a 4-byte quantity, SHORT must be a 2-byte quantity 
*/

typedef unsigned LONG;		/* 32-bit unsigned */
typedef unsigned short SHORT;	/* 16-bit unsigned */

/*
   If your C library does not have setlocale(3) define NO_LOCALE.
*/

#if 0
# define NO_LOCALE 1            /* No setlocale, locale in bbconfig is */
#endif                          /* meaningless. */

/* 
   If your C library does not have flock(2) define NO_FLOCK.
*/

#if SOLARIS || AIX || UNIXWARE || HPUX
# define NO_FLOCK 1             /* No flock, must use lockf */
#endif

/*
   If your C library does not have vfork(2) define NO_VFORK.
*/

#if AIX || IRIX || AUX
# define NO_VFORK 1
#endif

/* 
   If your system does not support the System V termio interface
   (you need BSD sgtty.h etc.), define NO_TERMIO.
*/

#if NEXTSTEP || MACHTEN || FREEBSD
# define NO_TERMIO 1
#endif

/* 
   If your C library does not have the setpgid(3) function, define 
   NO_SETPGID to make it use setpgrp(3).
*/

#if NEXTSTEP || MACHTEN
# define NO_SETPGID 1
#endif

/* 
   If your C library does not have strcasecmp(3) and strncasecmp(3)
   you can define this. Or, #define strcasecmp and strncasecmp to
   whatever case-insensitive strcmp functions you do have.
*/

#if UNIXWARE
# define NO_STRCASECMP 1
#endif

/* 
   Do you want varargs or stdarg? Stdarg is the default, define this
   if your compiler uses varargs.
*/

#if SUNOS || SOLARIS || NEXTSTEP || IRIX
# define WANTS_VARARGS_H 1
#endif

/*
   Do you have a <sys/select.h> header file and need it?
*/

#if AIX || FREEBSD
# define USES_SYS_SELECT_H 1
#endif

/* 
   Are you missing the <malloc.h> header file?
*/

#if MACHTEN || FREEBSD
# define LACKS_MALLOC_H 1
#endif

/* 
   Does your system support shared memory? (shmget(2), shmat(2), shmdt(2))
   If no, define this symbol. If shared memory is optional in your kernel
   and you don't have it, it's ok to leave this undefined. 
*/

#if NEXTSTEP || MACHTEN
# define NO_SHARED_MEMORY 1
#endif

/* 
   If you can obtain the remote host name from an environment variable,
   define it here and forget about utmp/utmpx. This will require
   hacking /bin/login on most systems.
*/
#if 0
# define HOST_ENV_VAR "REMOTE_HOST"
#endif

/* 
   System V usually doesn't give the remote host in the utmp file.
   Many of them have a utmpx file which does include this field.
*/

#if SOLARIS || UNIXWARE || IRIX
# define USES_UTMPX 1
#endif

/*
   HPUX (perhaps among others) doesn't have UP, BC, PC, and ospeed
   in its termcap library. If you get unresolved symbols for those,
   try turning on this define.
*/

#if HPUX
# define HPUX_TERMCAP
#endif

/*
   If crypt(3) is declared in unistd.h, good. If it's missing use this.
*/

#if SUNOS || SOLARIS || AIX || AUX || IRIX
# define NEEDS_CRYPT_DECLARED 1
#endif

/* Function prototyping */

#ifndef __P
# define __P(x) ()
#endif
