
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
   Adapted from Pirates BBS 1.8, namecomplete.c
   Copyright (C) 1990, Edward Luke, lush@Athena.EE.MsState.EDU
*/

#include "client.h"
#include <ctype.h>
#if LACKS_MALLOC_H
# include <stdlib.h>
#else
# include <malloc.h>
#endif

NumInList(list)
NAMELIST list;
{
  int num = 0;
  for (;list; list=list->next, num++);
  return num;
}

ListMaxLen(list, lines)
NAMELIST list;
int lines;
{
  int len, max = 0;
  for (;list && lines; list=list->next, lines--) {
    len = strlen(list->word);
    if (len > max) max = len;
  }
  return max;
}

chkstr(tag,name)
char *tag, *name;
{
  register char c1, c2;
  while(*tag != '\0') {
    c1 = *tag++;
    c2 = *name++;
    if(toupper(c1) != toupper(c2))
      return 0;
  }
  return 1;
}

NAMELIST
GetSubList(tag, list)
register char *tag ;
register NAMELIST list;
{
  NAMELIST wlist, wcurr;
  
  wlist = NULL ;
  wcurr = NULL ;
  while(list != NULL) {
    if(chkstr(tag,list->word)) {
      register NAMENODE *node ;
      node = (NAMENODE *)malloc(sizeof(NAMENODE)) ;
      node->word = list->word ;
      node->next = NULL ;
      if(wlist)
	wcurr->next = node ;
      else {
	wlist = node ;
      }
      wcurr = node ;
    }
    list = list->next ;
  }
  return wlist ;
}

void
ClearSubList(list)
NAMELIST *list;
{
  NAMENODE *trav = *list, *next;
  while (trav) {
    next = trav->next;
    free(trav);
    trav = next;
  }
  *list = NULL;
}

#define NUMLINES (t_lines-5)

/*
   namecomplete can use two methods for completion.
   Primarily, namecomplete uses the completefn to obtain the completion
   list given the partial string. If completefn is NULL, the complist 
   is used to find the possible completions.
*/

namecomplete(completefn, complist, prompt, data, datasize)
int (*completefn)();
NAMELIST complist;
char *prompt, *data ;
int datasize;
{
  char *temp;
  int ch ;
  int count = 0 ;
  int clearbot = 0;
  
  if(scrint) {
    NAMELIST cwlist, morelist ;
    int x,y ;
    int origx, origy;
    
    if(prompt != NULL) {
      prints("%s",prompt) ;
      clrtoeol() ;
    }
    temp = data ;
    cwlist = NULL;
    morelist = NULL;
    getyx(&y,&x) ;
    getyx(&origy, &origx);
    
    while((ch = igetch()) != EOF)
      {
	if(ch == '\n' || ch == '\r') {
	  *temp = '\0' ;
	  prints("\n") ;
	  if (temp != data) {
            ClearSubList(&cwlist);
            if (completefn) completefn(&cwlist, data) ;
            else cwlist = GetSubList(data, complist) ;
            if (NumInList(cwlist) == 1) {
	      strncpy(data,cwlist->word,datasize) ;
            }
          }
	  break ;
	}
	if(ch == ' ') {
	  int col,len ;
          if (!morelist) {
            ClearSubList(&cwlist);
            if (completefn) completefn(&cwlist, data) ;
            else cwlist = GetSubList(data, complist) ;
          }
          if(NumInList(cwlist) == 0) {
            bell();
            continue;
          }            
	  if(NumInList(cwlist) == 1) {
	    strncpy(data,cwlist->word,datasize) ;
            ClearSubList(&cwlist);
	    move(origy,origx) ;
	    prints("%s",data) ;
	    count = strlen(data) ;
	    temp = data + count ;
	    getyx(&y,&x) ;
	    continue ;
	  }
	  clearbot = 1 ;
	  col = 0 ;
	  if(!morelist) morelist = cwlist ;
	  len = ListMaxLen(morelist,NUMLINES) ;
	  move(3,0) ;
	  clrtobot() ;
	  standout() ;
	  prints(
		 "------------------------------- Completion List -------------------------------") ;
	  standend() ;
	  while(len+col < 80) {
	    int i ;
	    for(i=NUMLINES;(morelist)&&(i>0);i--,morelist=morelist->next) {
	      move(4+(NUMLINES - i),col) ;
	      prints("%s",morelist->word) ;
	    }
	    col += len+2 ;
	    if(!morelist)
	      break ;
	    len = ListMaxLen(morelist,NUMLINES) ;
	  }
	  if(morelist) {
	    move(23,0) ;
	    standout() ;
	    prints("-- More --") ;
	    standend() ;
	  }
	  move(y,x) ;
	  continue ;
	}
	if(ch == '\177' || ch == '\010') {
	  if(temp == data)
	    continue ;
          morelist = NULL;
	  temp-- ;
	  count-- ;
	  *temp = '\0' ;
	  x-- ;
	  move(y,x) ;
	  addch(' ') ;
	  move(y,x) ;
	  continue ;
	}
	if(count < datasize) {
          morelist = NULL;
	  *temp++ = ch ;
	  count++ ;
	  *temp = '\0' ;
	  move(y,x) ;
	  addch(ch) ;
	  x++ ;
	}
      }
    if(ch == EOF)
      generic_abort() ;
    if (*data) {
      move(origy,origx);
      prints("%s", data);
    }
    prints("\n") ;
    refresh() ;
    if(clearbot) {
      move(3,0) ;
      clrtobot() ;
    }
    ClearSubList(&cwlist);
    return 0 ;
  }
  if(prompt != NULL) {
    printf("%s",prompt) ;
    fflush(stdout) ;
  }
  if(!fgets(data,datasize,stdin))
    generic_abort() ;
  data[datasize] = '\0';
  if(temp = strchr(data,'\n'))
    *temp = '\0' ;
  return 0 ;
}

#if 0

namecomplete(list, prompt, data)
NAMELIST list;
char *prompt, *data ;
{
  NAMENODE dummynode;
  char *temp;
  int ch ;
  int count = 0 ;
  int clearbot = 0;
  
  if(scrint) {
    NAMELIST cwlist, morelist ;
    int x,y ;
    int origx, origy;
    
    if(prompt != NULL) {
      prints("%s",prompt) ;
      clrtoeol() ;
    }
    temp = data ;
    
    if (list == NULL) {
      dummynode.word = "";
      dummynode.next = NULL;
      list = &dummynode;
    }    
    cwlist = GetSubList("", list) ;
    morelist = NULL ;
    getyx(&y,&x) ;
    getyx(&origy, &origx);
    
    while((ch = igetch()) != EOF)
      {
	if(ch == '\n' || ch == '\r') {
	  *temp = '\0' ;
	  prints("\n") ;
	  if(NumInList(cwlist) == 1 && temp != data)
	    strncpy(data,cwlist->word,WORDSIZE) ;
	  ClearSubList(cwlist) ;
	  break ;
	}
	if(ch == ' ') {
	  int col,len ;
	  if(NumInList(cwlist) == 1) {
	    strncpy(data,cwlist->word,WORDSIZE) ;
	    move(origy,origx) ;
	    prints("%s",data) ;
	    count = strlen(data) ;
	    temp = data + count ;
	    getyx(&y,&x) ;
	    continue ;
	  }
	  clearbot = 1 ;
	  col = 0 ;
	  if(!morelist)
	    morelist = cwlist ;
	  len = ListMaxLen(morelist,NUMLINES) ;
	  move(3,0) ;
	  clrtobot() ;
	  standout() ;
	  prints(
		 "------------------------------- Completion List -------------------------------") ;
	  standend() ;
	  while(len+col < 80) {
	    int i ;
	    for(i=NUMLINES;(morelist)&&(i>0);i--,morelist=morelist->next) {
	      move(4+(NUMLINES - i),col) ;
	      prints("%s",morelist->word) ;
	    }
	    col += len+2 ;
	    if(!morelist)
	      break ;
	    len = ListMaxLen(morelist,NUMLINES) ;
	  }
	  if(morelist) {
	    move(23,0) ;
	    standout() ;
	    prints("-- More --") ;
	    standend() ;
	  }
	  move(y,x) ;
	  continue ;
	}
	if(ch == '\177' || ch == '\010') {
	  if(temp == data)
	    continue ;
	  temp-- ;
	  count-- ;
	  *temp = '\0' ;
	  ClearSubList(cwlist) ;
	  cwlist = GetSubList(data,list) ;
	  morelist = NULL ;
	  x-- ;
	  move(y,x) ;
	  addch(' ') ;
	  move(y,x) ;
	  continue ;
	}
	if(count < WORDSIZE) {
	  NAMENODE *node ;
	  *temp++ = ch ;
	  count++ ;
	  *temp = '\0' ;
	  node = GetSubList(data,cwlist) ;
	  if(node == NULL) {
	    bell() ;
	    temp-- ;
	    *temp = '\0' ;
	    count-- ;
	    continue ;
	  }
	  ClearSubList(cwlist) ;
	  cwlist = node ;
	  morelist = NULL ;
	  move(y,x) ;
	  addch(ch) ;
	  x++ ;
	}
      }
    if(ch == EOF)
      generic_abort() ;
    if (*data) {
      move(origy,origx);
      prints("%s", data);
    }
    prints("\n") ;
    refresh() ;
    if(clearbot) {
      move(3,0) ;
      clrtobot() ;
    }
    return 0 ;
  }
  if(prompt != NULL) {
    printf("%s",prompt) ;
    fflush(stdout) ;
  }
  if(!fgets(data,WORDSIZE,stdin))
    generic_abort() ;
  data[WORDSIZE] = '\0';
  if(temp = strchr(data,'\n'))
    *temp = '\0' ;
  return 0 ;
}

#endif

