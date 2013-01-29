%{
#include "client.h"
#include "y.tab.h"
char *get_yytext() { return yytext ; }
extern int line_num ;
%}

%%
[Ee]nvironment |
ENVIRONMENT    {return ENVIRONMENT;}
[Mm]enu |
MENU           {return MENU;}
[Rr]eadmenu |
READMENU       {return READMENU;}
\,             {return COMMA;}
\{             {return OBRACE;}
\}             {return CBRACE;}
\(             {return OPAREN;}
\)             {return CPAREN;}
\"[^"\n]*\"    {return STRING;}
\'[a-zA-Z]'    {return KEY;}
\'^[a-zA-Z]'   {return KEY;}
\$[a-zA-Z.]+   {return COMMAND;}
\$$[MP]        {return OPENFLAG;}
[a-zA-Z_]+     {return ID_NAME;}
[0-9]+         {return INTEGER;}
[ \t] ;
\n             {line_num++; }
\/\/.* ;
\#.* ;
.              {return ERROR ; }
%%


