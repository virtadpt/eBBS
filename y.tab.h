typedef union {
  int ival ;
  char * sval ;
  NMENUITEM *mival ;
  NMENU *mval ;
  NREADMENUITEM *rmival ;
  NREADMENU *rmval ;
} YYSTYPE;
#define	ENVIRONMENT	258
#define	MENU	259
#define	READMENU	260
#define	STRING	261
#define	COMMAND	262
#define	OBRACE	263
#define	CBRACE	264
#define	COMMA	265
#define	OPAREN	266
#define	CPAREN	267
#define	ID_NAME	268
#define	INTEGER	269
#define	KEY	270
#define	OPENFLAG	271
#define	ERROR	272


extern YYSTYPE yylval;
