
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
   Defines the bits that correspond to each permission. The permission bits
   used by the system are defined here, and their names are in the permstrs
   file.
*/

#define PERMBIT(n)	(1 << n)
#define PERMBIT_MAX     31

#define PERM_NONE       0
#define PERM_ALL        ~0

/* PERM_DEFAULT is given to each new account upon creation */
#define PERM_DEFAULT    PERMBIT(0) | PERMBIT(1) | PERMBIT(2) | PERMBIT(3)

/* The bit to denote Sysop permission */
#define PERM_SYSOP	PERMBIT(4)
