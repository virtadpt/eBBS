
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
#include <ctype.h>

#define MAILERFILE "etc/mailers"

/* For the bbs name and tempfile */
extern SERVERDATA server;

is_valid_address(addr)
char *addr;
{
  if (*addr == '\0') return 0;   /* blank */
  if (*addr == '-') return 0;    /* cannot begin with '-' */
  while (*addr) {
    if (!isalnum(*addr) && strchr(".%!@:;-_+[]", *addr) == NULL)
      return 0;
    addr++;
  }
  return 1;
}

spec_to_mailer(rec, mailer)
char *rec;
char *mailer;
{
  NAME prefix;
  rec = _extract_quoted(rec, prefix, sizeof(prefix));
  rec = _extract_quoted(rec, mailer, sizeof(ADDR));
  return S_OK;
}
 
char *
lookup_mailer(addr, mailer)
char *addr;
char *mailer;
{
  char *colon;
  int rc;

  if ((colon = strchr(addr, ':')) == NULL) return NULL;
  *colon = '\0';
  rc = _record_find(MAILERFILE, _match_first, addr, spec_to_mailer, mailer);
  if (rc != S_OK) return NULL;
  return colon+1;  
}

ok_for_from_header(str)
char *str;
{
  for (; str && *str; str++)
    if (strchr("<>;\"'\\|[]{}()%@!", *str)) return 0;

  return 1;
}

LONG
mail_file_to_outside(fname, subject, addrspec, is_forward, is_binary)
char *fname;
char *subject;
char *addrspec;
int is_forward;
int is_binary;
{
  FILE *fp;
  ACCOUNT acct;
  ADDR addrbuf;
  PATH execbuf;
  PATH mailer;
  int rc;
  char *address;
  char *base;

  if ((fp = fopen(fname, "r")) == NULL) return S_NOSUCHFILE;
  fclose(fp);

  if (local_bbs_owninfo(&acct) != S_OK) return S_SYSERR;

  if (addrspec == NULL) {
    sprintf(addrbuf, "%s:%s", MAILER_PREFIX, acct.email);
  }
  else {
    strncpy(addrbuf, addrspec, sizeof addrbuf);
  }

  if ((address = lookup_mailer(addrbuf, mailer)) == NULL) return S_BADADDRESS;

  if ((fp = fopen(server.tempfile, "w")) == NULL) return S_SYSERR;

  /* Write headers! */
  if (ok_for_from_header(acct.username)) 
    fprintf(fp, "From: %s.bbs (%s)\n", acct.userid, acct.username);
  else fprintf(fp, "From: %s.bbs (%s)\n", acct.userid, acct.userid);
  if (is_forward) fprintf(fp, "Subject: %s (fwd)\n", subject);
  else fprintf(fp, "Subject: %s\n", subject);
  fprintf(fp, "To: %s\n", address);
  fprintf(fp, "X-Disclaimer: %s is not responsible for the contents of this message.\n", server.name);  

  fprintf(fp, "\n");
  if (is_forward) fprintf(fp, "*** Forwarded file follows ***\n\n");

  fflush(fp);
  if (is_binary) {
    fclose(fp);
    base = strrchr(fname, '/');
    if (base) base++;
    else base = fname;
    strcpy(execbuf, server.encodebin);
    strcat(execbuf, " ");
    strcat(execbuf, base);
    rc = execute(execbuf, NULL, fname, server.tempfile, "/dev/null", NULL, 1);
  }
  else {
    rc = append_file(fileno(fp), fname);
    fclose(fp);
  }
  
  if (rc != 0) {
    unlink(server.tempfile);
    return S_SYSERR;
  }

  sprintf(execbuf, "%s %s", mailer, address);

  bbslog(3, "FORWARD '%s' to %s by %s\n", subject, address, acct.userid);

  rc = execute(execbuf, NULL, server.tempfile, "/dev/null", "/dev/null", NULL);

  unlink(server.tempfile);
  return (rc == 0 ? S_OK : S_SYSERR);
}

LONG
forward_file_to_outside(fname, title, is_binary)
char *fname;
char *title;
int is_binary;
{
  LONG rc;
  
  rc = mail_file_to_outside(fname, title, NULL, 1, is_binary);

  return rc;
}


