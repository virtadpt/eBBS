
#include "client.h"

char *default_env = "TERM=dumb,SHELL=/bin/false" ;


char *ExpandString(s)
char *s ;
{
    static char buf[1024] ;
    char *sr = buf ;
    int cnt = 0 ;
    static ACCOUNT acct ;
    static int acct_filled = 0 ;
    
    for(cnt=0;cnt<sizeof(buf);cnt++,sr++) {
        *sr = *s ;
        if(*s == '\0') break ;
        if(*s == '%') {
            if(!acct_filled) {
                bbs_owninfo(&acct) ;
                acct_filled = 1 ;
            }
            switch(*(s+1)) {
                int len ;
              case 'T':
              case 't':
                len =strlen(acct.terminal) ;
                if(len+cnt<sizeof(buf)) {
                    strcpy(sr,acct.terminal) ;
                    sr += len-1 ;
                    cnt += len-1 ;
                }
                s++ ;
                break ;
              case 'I':
              case 'i':
                len = strlen(acct.userid) ;
                if(len+cnt<sizeof(buf)) {
                    strcpy(sr,acct.userid) ;
                    sr += len-1 ;
                    cnt += len-1 ;
                }
                s++ ;
                break ;
              case 'U':
              case 'u':
                len = strlen(acct.username) ;
                if(len+cnt<sizeof(buf)) {
                    strcpy(sr,acct.username) ;
                    sr += len-1 ;
                    cnt += len-1 ;
                }
                s++ ;
                break ;
              case 'E':
              case 'e':
                len = strlen(acct.editor) ;
                if(len+cnt<sizeof(buf)) {
                    strcpy(sr,acct.editor) ;
                    sr += len-1 ;
                    cnt += len-1 ;
                }
                s++ ;
                break ;
              case 'H':
              case 'h':
                len = strlen(acct.fromhost) ;
                if(len+cnt<sizeof(buf)) {
                    strcpy(sr,acct.fromhost) ;
                    sr += len-1 ;
                    cnt += len-1 ;
                }
                s++ ;
                break ;
              default:
                sr-- ;
                break ;
            }
        }
        s++ ;
    }
        
    return buf ;
}


parse_environment(s)
char *s ;
{
    char buf[4096] ;
    char *p1, *p2, *p3 ;
    strncpy(buf,s,sizeof(buf)) ;
    buf[sizeof(buf)-1] = '\0' ;
    p1 = buf ;
    while(p1!=NULL) {
        p2 = strchr(p1,'=') ;
        p3 = strchr(p1,',') ;
        if(p2 == NULL)
          break ;
        *p2 = '\0' ;
        if(p3!=NULL)
          *p3 = '\0' ;
        bbssetenv(p1,p2+1) ;
        if(p3== NULL)
          break ;
        p1 = p3 + 1 ;
        
    }
}


parse_default()
{
    parse_environment(default_env) ;
}

