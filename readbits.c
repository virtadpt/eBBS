
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

/* readbits file format:
bname       :ordr:stampxxx:10101010...

if bname is empty, the readbits are for Mail.
*/

#define BIT_BNAME_OFFSET  0
#define BIT_ORDER_OFFSET  (BIT_BNAME_OFFSET+NAMELEN+1)
#define BIT_STAMP_OFFSET  (BIT_ORDER_OFFSET+5)
#define BIT_DATA_OFFSET   (BIT_STAMP_OFFSET+9)
 
#define READBITFILE "readbits"

get_readbit_file(userid, buf)
char *userid;
char *buf;
{
  get_home_directory(userid, buf);
  strcat(buf, "/");
  strcat(buf, READBITFILE);
}

expand_bitent(bitent, ba)
char *bitent;    /* The readbit data */
char *ba;        /* char array of size READBITSIZE, filled */
{
  char *eob;
  char currval;
  int currct = 0;
  char scratch[10];

  for (eob = ba+READBITSIZE-1; *eob == '0' && eob > ba; eob--);
  eob++;

  for (currval = *ba; ba <= eob; ba++) {
    if (*ba == currval) currct++;
    else {
      if (currct <= 4) {
        memset(scratch, 0, sizeof scratch);
        for (; currct; currct--) scratch[currct-1] = currval;
      }
      else sprintf(scratch, "%c[%d]", currval, currct);
      strcpy(bitent, scratch);
      bitent += strlen(scratch);
      currct = 1;
      currval = *ba;
    }
  }
  *bitent = '\0';
}

compress_bitent(bitent, ba)
char *bitent;     /* The readbit data */
char *ba;         /* char array of size READBITSIZE, empty */
{
  int ct;
  char *rbrkt;

  memset(ba, '0', READBITSIZE);

  while (*bitent && *bitent != '\n') {
    if (*(bitent+1) == '[') {
      if (rbrkt = strchr(bitent, ']')) *rbrkt = '\0';
      for (ct = atoi(bitent+2); ct; ct--) *ba++ = *bitent;
      if (rbrkt) {
	*rbrkt = ']';
        bitent = rbrkt;
      }
    }
    else {
      *ba++ = *bitent;
    }
    bitent++;
  }
}

parse_readinfo(rec, rinfo)
char *rec;
READINFO *rinfo;
{
  rinfo->stamp = hex2LONG(rec+BIT_STAMP_OFFSET);
  compress_bitent(rec+BIT_DATA_OFFSET, rinfo->bits);
  return S_OK;
}

parse_readorder(rec, order)
char *rec;
SHORT *order;
{
  *order = hex2SHORT(rec+BIT_ORDER_OFFSET);
  return S_OK;
}

get_bitfile_ent(bname, rinfo)
char *bname;
READINFO *rinfo;
{
  int rc;
  PATH bitfile;

  get_readbit_file(my_userid(), bitfile);

  memset(rinfo->bits, '0', READBITSIZE);
  rinfo->stamp = 0;

  rc = _record_find(bitfile, _match_first, bname, parse_readinfo, rinfo);
  return rc;
}

struct _bitsetstruct {
  char *bname;
  SHORT *order;
  READINFO *rinfo;
};

format_readinfo(rec, info)
char *rec;
struct _bitsetstruct *info;
{
  if (info->bname) {
    memset(rec+BIT_BNAME_OFFSET, ' ', NAMELEN);
    memcpy(rec+BIT_BNAME_OFFSET, info->bname, strlen(info->bname));
    rec[BIT_ORDER_OFFSET-1] = ':';
  }

  if (info->order) {
    SHORTcpy(rec+BIT_ORDER_OFFSET, *(info->order));
    rec[BIT_STAMP_OFFSET-1] = ':';
  }
    
  if (info->rinfo) {
    LONGcpy(rec+BIT_STAMP_OFFSET, info->rinfo->stamp);
    rec[BIT_DATA_OFFSET-1] = ':';
    expand_bitent(rec+BIT_DATA_OFFSET, info->rinfo->bits);
    strcat(rec, "\n");
  }
  return S_OK;
}              

/* This can GO AWAY if I ever fix _record_replace to only pass two args */

update_readinfo(newrec, oldrec, info)
char *newrec;
char *oldrec;
struct _bitsetstruct *info;
{
  strcpy(newrec, oldrec);
  return (format_readinfo(newrec, info));
}

set_bitfile_ent(bname, rinfo)
char *bname;
READINFO *rinfo;
{
  int rc;
  struct _bitsetstruct bs;
  PATH bitfile;

  if (my_flag(FLG_SHARED)) return 0;

  get_readbit_file(my_userid(), bitfile);

  bs.bname = NULL;
  bs.order = NULL;
  bs.rinfo = rinfo;

  rc = _record_replace(bitfile, _match_first, bname, update_readinfo, &bs);
  
  if (rc == S_NOSUCHREC) {
    SHORT order = READ_ORDER_UNSET;
    bs.bname = bname;    
    bs.order = &order;
    rc = _record_add(bitfile, _match_first, bname, format_readinfo, &bs);
  }

  return rc;
}
    
set_readbit(rinfo, fileid)
READINFO *rinfo;
SHORT fileid;
{
  if (fileid <= 0 || fileid > READBITSIZE) return S_OUTOFRANGE;
  rinfo->bits[fileid-1] = '1';
  return S_OK;
}

clear_all_readbits(rinfo)
READINFO *rinfo;
{
  memset(rinfo->bits, '0', sizeof(rinfo->bits));
  return S_OK;
}

test_readbit(rinfo, fileid)
READINFO *rinfo;
SHORT fileid;
{
  if (fileid <= 0 || fileid > READBITSIZE) return 0;
  return (rinfo->bits[fileid-1] == '1');
}

get_read_order(bname, order)
char *bname;
SHORT *order;
{
  int rc;
  PATH bitfile;

  get_readbit_file(my_userid(), bitfile);
  *order = READ_ORDER_UNSET;

  rc = _record_find(bitfile, _match_first, bname, parse_readorder, order);
  return rc;
}

set_read_order(bname, order)
char *bname;
SHORT order;
{
  int rc;
  struct _bitsetstruct bs;
  PATH bitfile;

  if (my_flag(FLG_SHARED)) return 0;

  get_readbit_file(my_userid(), bitfile);

  bs.bname = NULL;
  bs.order = &order;
  bs.rinfo = NULL;

  rc = _record_replace(bitfile, _match_first, bname, update_readinfo, &bs);
  
  if (rc == S_NOSUCHREC) {
    READINFO rinfo;
    rinfo.stamp = 0;
    clear_all_readbits(&rinfo);
    bs.bname = bname;    
    bs.rinfo = &rinfo;
    rc = _record_add(bitfile, _match_first, bname, format_readinfo, &bs);
  }

  return rc;
}
    
/*ARGSUSED*/
fix_readbit_entry(indx, rec, ncs)
int indx;
char *rec;
struct namechange *ncs;
{
  int rc;
  PATH bitfile;
  NAME userid;
  struct _bitsetstruct bs;

  /* Sleazy way to get the userid out of the passfile record */
  memset(userid, 0, sizeof userid);
  strncpy(userid, rec, NAMELEN);
  strip_trailing_space(userid);

  get_readbit_file(userid, bitfile);
  
  if (ncs->newname == NULL)
    rc = _record_delete(bitfile, _match_first, ncs->oldname);
  else {
    bs.rinfo = NULL;
    bs.order = NULL;
    bs.bname = ncs->newname;
    rc = _record_replace(bitfile, _match_first, ncs->oldname,
		         update_readinfo, &bs);
  }
 
  return rc;
}    

#define NOZAP_FILE "nozap"

local_bbs_zap_board(bname, dozap)
char *bname;
SHORT dozap;
{
  int rc;
  PATH nzfile;
  FILE *fp;
  SHORT neworder = dozap ? READ_ORDER_ZAPPED : READ_ORDER_UNSET;

  get_board_directory(bname, nzfile);
  strcat(nzfile, "/");
  strcat(nzfile, NOZAP_FILE);
  if (dozap && ((fp = fopen(nzfile, "r")) != NULL)) {
    fclose(fp);
    return S_CANNOTZAP;
  }

  rc = set_read_order(bname, neworder);

  return (rc == S_OK ? S_OK : S_NOSUCHBOARD);
}
