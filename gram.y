%token ENVIRONMENT
%token MENU 
%token READMENU
%token STRING
%token COMMAND
%token OBRACE
%token CBRACE
%token COMMA 
%token OPAREN
%token CPAREN
%token ID_NAME
%token INTEGER
%token KEY
%token OPENFLAG
%token ERROR

%{
#include <stdio.h>
#include "client.h"

extern char *get_yytext() ;
extern int yyleng ;
%}

%start commands

%union {
  int ival ;
  char * sval ;
  NMENUITEM *mival ;
  NMENU *mval ;
  NREADMENUITEM *rmival ;
  NREADMENU *rmval ;
}

%type <ival> integer name ident key openflag
%type <sval> string
%type <mival> menulist menuitem command commandentry
%type <rmival> rmenulist rmenuitem rcommand

%%

commands :
    | commands menudef
    | commands rmenudef
    | error

menudef :  MENU string COMMA string COMMA string COMMA string OBRACE menulist CBRACE
{
    NMENU *mkmenu(), *mp ;

    if(mp = mkmenu($10)) {
        mp->menu_id = $2 ;
        mp->menu_title = $4 ;
        mp->menu_prompt = $6 ;
        mp->menu_default = $8 ;
        pushmenu(mp) ;
    }
}
| ENVIRONMENT OPAREN string CPAREN
{
    extern char *default_env ;
    default_env = $3 ;
    parse_default() ;
}

menulist : menuitem
{
    $$ = $1 ;
}
      | menulist menuitem
{
    if($1)
      $2->prev = $1 ;
    $$ = $2 ;
}

menuitem : OPAREN string COMMA string COMMA string COMMA ident COMMA commandentry COMMA string CPAREN
{
    register NMENUITEM *mip ;

    if(mip = $10) {
        mip->name = $2 ;
        mip->enabled = $8 ;
        mip->default_action = $4 ;
        mip->error_action = $6 ;
        mip->help = $12 ;
    }
    $$ = mip ;
}

rmenudef :  READMENU string COMMA string COMMA string COMMA string COMMA string COMMA string COMMA string COMMA string COMMA string COMMA string OBRACE rmenulist CBRACE
{
    NREADMENU *mkreadmenu(), *mp ;

    if(mp = mkreadmenu($2, $22)) {
        mp->menu_helptitle = $20;
        mp->menu_title = $4;
        mp->menu_message = $6;
        mp->menu_line2 = $8;
        mp->menu_line3 = $10;
        mp->menu_field1 = $12;
        mp->menu_field2 = $14;
        mp->menu_field3 = $16;
        mp->menu_field4 = $18;
    }
}

rmenulist : rmenuitem
{
    $$ = $1 ;
}
      | rmenulist rmenuitem
{
    if($1)
      $2->next = $1 ;
    $$ = $2 ;
}

rmenuitem : OPAREN key COMMA rcommand COMMA openflag COMMA ident COMMA string CPAREN
{
    register NREADMENUITEM *mip ;

    if(mip = $4) {
        mip->key = $2 ;
        mip->mainprivs = $8 ;
        mip->boardprivs = $6 ;
        mip->help = $10 ;
    }
    $$ = mip ;
}

ident: integer
{ $$ = $1 ; }
| name
{ $$ = $1 ; }

command : COMMAND
{
    NMENUITEM *mkmenuitem() ;
    char buf[512] ;
    strncpy(buf,get_yytext(),yyleng) ;
    buf[yyleng] = '\0' ;

    $$ = mkmenuitem(buf+1) ;
}

commandentry: command
{ $$ = $1; }
   | command string
{
    NMENUITEM *mip = $1 ;

    if(mip)
      mip->action_arg = $2 ;
    $$ = mip ;
}

rcommand : COMMAND
{
    NREADMENUITEM *mkreadmenuitem() ;
    char buf[512] ;
    strncpy(buf,get_yytext(),yyleng) ;
    buf[yyleng] = '\0' ;

    $$ = mkreadmenuitem(buf+1) ;
}

string : STRING
{
    char *mkstring() ;
    char *ExpandString() ;
    char buf[512] ;
    
    strncpy(buf,get_yytext(),yyleng) ;
    buf[yyleng-1] = '\0' ;
    $$ = mkstring(ExpandString(buf+1)) ;
}

integer : INTEGER
{
    int atoi() ;

    $$ = atoi(get_yytext()) ;
}

openflag : OPENFLAG
{
    int convert_openflag_to_int() ;
    char buf[512] ;
    
    strncpy(buf,get_yytext(),yyleng) ;
    buf[yyleng] = '\0' ;
    $$ = convert_openflag_to_int(buf+2) ;
} | integer { $$ = $1; }

name : ID_NAME
{
    int convert_cmd_to_int() ;
    char buf[512] ;
    
    strncpy(buf,get_yytext(),yyleng) ;
    buf[yyleng] = '\0' ;
    $$ = convert_cmd_to_int(buf) ;
}

key : KEY
{
    int convert_key_to_int() ;
    char buf[512] ;
    
    strncpy(buf,get_yytext(),yyleng) ;
    buf[yyleng] = '\0' ;
    $$ = convert_key_to_int(buf) ;
} 
%%

char *mkstring(s)
register char *s ;
{
    register char *p ;

    p = (char *) malloc(strlen(s)+1) ;
    strcpy(p,s) ;
    return p ;
}

menuerror()
{
    do_echo("WARNING, invalid function in Menu definition\n") ;
    return 0 ;
}

rmenuerror()
{
    do_echo("WARNING, invalid function in ReadMenu definition\n") ;
    return 0 ;
}

NMENUITEM *
mkmenuitem(s)
char *s ;
{
    NMENUITEM *mip ;
    extern int line_num ;
    extern int (*findfunc())() ;
    if(!(mip = (NMENUITEM *) malloc(sizeof(NMENUITEM))))
      return NULL ;
    memset(mip,0,sizeof(NMENUITEM)) ;
    if(!(mip->action_func = findfunc(s))) {
        char buf[512] ;
        sprintf(buf,"WARNING, invalid function in menu file\nfunction = '%s' LINE %d\n",s,line_num) ;
        do_echo(buf) ;
        mip->action_func = menuerror ;
    }
    return mip ;
}

char
getmenuletter(s)
char *s ;
{
    register char firstch;
    for (firstch = *s; *s; s++)
        if (*s >= 'A' && *s <= 'Z') return *s;

    return firstch;
}

int
getmenuindex(t)
unsigned int t ;
{
    t = (t | 0x20) - 'a' ;
    return t % MAXMENUSZ ;
}

NMENU *
mkmenu(mip)
NMENUITEM *mip ;
{
    NMENU *mp ;
    NMENUITEM *tp ;

    if(!(mp = (NMENU *)malloc(sizeof(NMENU))))
      return NULL ;
    memset(mp,0,sizeof(NMENU)) ;
    mp->commlist = mip ;
    for(tp = mip;tp;tp = tp->prev) {
        register char menuletter = getmenuletter(tp->name) ;
        register int menuindex = getmenuindex(menuletter) ;
        if(mp->menucommands[menuindex]) {
            char buf[512] ;
            extern int line_num ;
            sprintf(buf,
         "Warning Menu command %s attempting to use filled slot!\nLINE %d\n",
                    tp->name,line_num) ;
            do_echo(buf) ;
            continue ;
        }
        mp->menucommands[menuindex] = tp ;
    }
    return mp ;
}

NMENU *bigMenuList = NULL ;

pushmenu(mp)
NMENU *mp ;
{
    NMENUITEM *mip = NULL ;
    NMENUITEM *tmp, *next ;
    
    for(tmp=mp->commlist;tmp;tmp = next) {
        next = tmp->prev ;
        tmp->next = mip ;
        mip = tmp ;
    }
    mp->commlist = mip ;
    mp->next = bigMenuList ;
    bigMenuList = mp ;
}

NREADMENU *PostReadMenu = NULL;
NREADMENU *MailReadMenu = NULL;
NREADMENU *FileReadMenu = NULL;

NREADMENU *
mkreadmenu(name, mip)
char *name ;
NREADMENUITEM *mip ;
{
    NREADMENU *mp ;

    if(!(mp = (NREADMENU *)malloc(sizeof(NREADMENU))))
      return NULL ;
    if (!strcasecmp(name, "Main")) PostReadMenu = mp;
    else if (!strcasecmp(name, "Mail")) MailReadMenu = mp;
    else if (!strcasecmp(name, "File")) FileReadMenu = mp;
    else {
        char buf[512] ;
        extern int line_num ;
        sprintf(buf,
        "Warning Invalid ReadMenu '%s'!\nLINE %d\n", name, line_num) ;
        do_echo(buf) ;
        free(mp);
        return NULL;
    }
    memset(mp,0,sizeof(NREADMENU)) ;
    mp->commlist = mip ;
    return mp ;
}

NREADMENUITEM *
mkreadmenuitem(s)
char *s ;
{
    NREADMENUITEM *mip ;
    extern int line_num ;
    extern int (*findrfunc())() ;
    if(!(mip = (NREADMENUITEM *) malloc(sizeof(NREADMENUITEM))))
      return NULL ;
    memset(mip,0,sizeof(NREADMENUITEM)) ;
    if(!(mip->action_func = findrfunc(s))) {
        char buf[512] ;
        sprintf(buf,"WARNING, invalid function in menu file\nfunction = '%s' LINE %d\n",s,line_num) ;
        do_echo(buf) ;
        mip->action_func = rmenuerror ;
    }
    return mip ;
}



