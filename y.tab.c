
/*  A Bison parser, made from gram.y with Bison version GNU Bison version 1.22
  */

#define YYBISON 1  /* Identify Bison output.  */

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

#line 17 "gram.y"

#include <stdio.h>
#include "client.h"

extern char *get_yytext() ;
extern int yyleng ;

#line 27 "gram.y"
typedef union {
  int ival ;
  char * sval ;
  NMENUITEM *mival ;
  NMENU *mval ;
  NREADMENUITEM *rmival ;
  NREADMENU *rmval ;
} YYSTYPE;

#ifndef YYLTYPE
typedef
  struct yyltype
    {
      int timestamp;
      int first_line;
      int first_column;
      int last_line;
      int last_column;
      char *text;
   }
  yyltype;

#define YYLTYPE yyltype
#endif

#include <stdio.h>

#ifndef __cplusplus
#ifndef __STDC__
#define const
#endif
#endif



#define	YYFINAL		84
#define	YYFLAG		-32768
#define	YYNTBASE	18

#define YYTRANSLATE(x) ((unsigned)(x) <= 272 ? yytranslate[x] : 34)

static const char yytranslate[] = {     0,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
     2,     2,     2,     2,     2,     1,     2,     3,     4,     5,
     6,     7,     8,     9,    10,    11,    12,    13,    14,    15,
    16,    17
};

#if YYDEBUG != 0
static const short yyprhs[] = {     0,
     0,     1,     4,     7,     9,    21,    26,    28,    31,    45,
    69,    71,    74,    86,    88,    90,    92,    94,    97,    99,
   101,   103,   105,   107,   109
};

static const short yyrhs[] = {    -1,
    18,    19,     0,    18,    22,     0,     1,     0,     4,    29,
    10,    29,    10,    29,    10,    29,     8,    20,     9,     0,
     3,    11,    29,    12,     0,    21,     0,    20,    21,     0,
    11,    29,    10,    29,    10,    29,    10,    25,    10,    27,
    10,    29,    12,     0,     5,    29,    10,    29,    10,    29,
    10,    29,    10,    29,    10,    29,    10,    29,    10,    29,
    10,    29,    10,    29,     8,    23,     9,     0,    24,     0,
    23,    24,     0,    11,    33,    10,    28,    10,    31,    10,
    25,    10,    29,    12,     0,    30,     0,    32,     0,     7,
     0,    26,     0,    26,    29,     0,     7,     0,     6,     0,
    14,     0,    16,     0,    30,     0,    13,     0,    15,     0
};

#endif

#if YYDEBUG != 0
static const short yyrline[] = { 0,
    43,    44,    45,    46,    48,    60,    67,    71,    78,    92,
   109,   113,   120,   133,   135,   138,   148,   150,   159,   169,
   180,   187,   195,   197,   207
};

static const char * const yytname[] = {   "$","error","$illegal.","ENVIRONMENT",
"MENU","READMENU","STRING","COMMAND","OBRACE","CBRACE","COMMA","OPAREN","CPAREN",
"ID_NAME","INTEGER","KEY","OPENFLAG","ERROR","commands","menudef","menulist",
"menuitem","rmenudef","rmenulist","rmenuitem","ident","command","commandentry",
"rcommand","string","integer","openflag","name","key",""
};
#endif

static const short yyr1[] = {     0,
    18,    18,    18,    18,    19,    19,    20,    20,    21,    22,
    23,    23,    24,    25,    25,    26,    27,    27,    28,    29,
    30,    31,    31,    32,    33
};

static const short yyr2[] = {     0,
     0,     2,     2,     1,    11,     4,     1,     2,    13,    23,
     1,     2,    11,     1,     1,     1,     1,     2,     1,     1,
     1,     1,     1,     1,     1
};

static const short yydefact[] = {     0,
     4,     0,     0,     0,     0,     2,     3,     0,    20,     0,
     0,     0,     0,     0,     6,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,     0,     0,     0,     0,     7,
     0,     0,     5,     8,     0,     0,     0,     0,     0,     0,
     0,     0,     0,     0,     0,    24,    21,     0,    14,    15,
     0,     0,     0,    16,    17,     0,     0,    18,     0,     0,
     0,     0,     9,     0,     0,    11,    25,     0,    10,    12,
     0,    19,     0,     0,    22,    23,     0,     0,     0,     0,
     0,    13,     0,     0
};

static const short yydefgoto[] = {     2,
     6,    29,    30,     7,    65,    66,    48,    55,    56,    73,
    10,    49,    77,    50,    68
};

static const short yypact[] = {     1,
-32768,     7,     9,    22,    22,-32768,-32768,    22,-32768,    19,
    23,    20,    22,    22,-32768,    26,    27,    22,    22,    29,
    30,    22,    22,    33,    32,    34,    22,    22,    10,-32768,
    37,    38,-32768,-32768,    22,    22,    39,    41,    22,    22,
    43,    45,    22,     2,    46,-32768,-32768,    47,-32768,-32768,
    22,    36,    48,-32768,    22,    49,    22,-32768,    22,    52,
    50,    53,-32768,    51,    15,-32768,-32768,    55,-32768,-32768,
    54,-32768,    57,    11,-32768,-32768,    58,     2,    59,    22,
    60,-32768,    44,-32768
};

static const short yypgoto[] = {-32768,
-32768,-32768,    42,-32768,-32768,    -2,    -8,-32768,-32768,-32768,
    -5,    -1,-32768,-32768,-32768
};


#define	YYLAST		75


static const short yytable[] = {    11,
    -1,     1,    12,    -1,    -1,    -1,    83,    16,    17,     3,
     4,     5,    20,    21,    46,    47,    24,    25,    33,     8,
    28,    31,    32,    69,    47,    64,    75,     9,    13,    37,
    38,    15,    14,    41,    42,    18,    19,    45,    22,    23,
    26,    27,    54,    84,    28,    53,    35,    36,    39,    58,
    40,    60,    43,    61,    44,    51,    52,    57,    59,    62,
    72,    63,    70,    64,    71,    67,    74,    78,    80,    79,
    34,    82,    76,     0,    81
};

static const short yycheck[] = {     5,
     0,     1,     8,     3,     4,     5,     0,    13,    14,     3,
     4,     5,    18,    19,    13,    14,    22,    23,     9,    11,
    11,    27,    28,     9,    14,    11,    16,     6,    10,    35,
    36,    12,    10,    39,    40,    10,    10,    43,    10,    10,
     8,    10,     7,     0,    11,    51,    10,    10,    10,    55,
    10,    57,    10,    59,    10,    10,    10,    10,    10,     8,
     7,    12,    65,    11,    10,    15,    10,    10,    10,    78,
    29,    12,    74,    -1,    80
};
/* -*-C-*-  Note some compilers choke on comments on `#line' lines.  */
#line 3 "/usr/lib/bison.simple"

/* Skeleton output parser for bison,
   Copyright (C) 1984, 1989, 1990 Bob Corbett and Richard Stallman

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 1, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */


#ifndef alloca
#ifdef __GNUC__
#define alloca __builtin_alloca
#else /* not GNU C.  */
#if (!defined (__STDC__) && defined (sparc)) || defined (__sparc__) || defined (__sparc) || defined (__sgi)
#include <alloca.h>
#else /* not sparc */
#if defined (MSDOS) && !defined (__TURBOC__)
#include <malloc.h>
#else /* not MSDOS, or __TURBOC__ */
#if defined(_AIX)
#include <malloc.h>
 #pragma alloca
#else /* not MSDOS, __TURBOC__, or _AIX */
#ifdef __hpux
#ifdef __cplusplus
extern "C" {
void *alloca (unsigned int);
};
#else /* not __cplusplus */
void *alloca ();
#endif /* not __cplusplus */
#endif /* __hpux */
#endif /* not _AIX */
#endif /* not MSDOS, or __TURBOC__ */
#endif /* not sparc.  */
#endif /* not GNU C.  */
#endif /* alloca not defined.  */

/* This is the parser code that is written into each bison parser
  when the %semantic_parser declaration is not specified in the grammar.
  It was written by Richard Stallman by simplifying the hairy parser
  used when %semantic_parser is specified.  */

/* Note: there must be only one dollar sign in this file.
   It is replaced by the list of actions, each action
   as one case of the switch.  */

#define yyerrok		(yyerrstatus = 0)
#define yyclearin	(yychar = YYEMPTY)
#define YYEMPTY		-2
#define YYEOF		0
#define YYACCEPT	return(0)
#define YYABORT 	return(1)
#define YYERROR		goto yyerrlab1
/* Like YYERROR except do call yyerror.
   This remains here temporarily to ease the
   transition to the new meaning of YYERROR, for GCC.
   Once GCC version 2 has supplanted version 1, this can go.  */
#define YYFAIL		goto yyerrlab
#define YYRECOVERING()  (!!yyerrstatus)
#define YYBACKUP(token, value) \
do								\
  if (yychar == YYEMPTY && yylen == 1)				\
    { yychar = (token), yylval = (value);			\
      yychar1 = YYTRANSLATE (yychar);				\
      YYPOPSTACK;						\
      goto yybackup;						\
    }								\
  else								\
    { yyerror ("syntax error: cannot back up"); YYERROR; }	\
while (0)

#define YYTERROR	1
#define YYERRCODE	256

#ifndef YYPURE
#define YYLEX		yylex()
#endif

#ifdef YYPURE
#ifdef YYLSP_NEEDED
#define YYLEX		yylex(&yylval, &yylloc)
#else
#define YYLEX		yylex(&yylval)
#endif
#endif

/* If nonreentrant, generate the variables here */

#ifndef YYPURE

int	yychar;			/*  the lookahead symbol		*/
YYSTYPE	yylval;			/*  the semantic value of the		*/
				/*  lookahead symbol			*/

#ifdef YYLSP_NEEDED
YYLTYPE yylloc;			/*  location data for the lookahead	*/
				/*  symbol				*/
#endif

int yynerrs;			/*  number of parse errors so far       */
#endif  /* not YYPURE */

#if YYDEBUG != 0
int yydebug;			/*  nonzero means print parse trace	*/
/* Since this is uninitialized, it does not stop multiple parsers
   from coexisting.  */
#endif

/*  YYINITDEPTH indicates the initial size of the parser's stacks	*/

#ifndef	YYINITDEPTH
#define YYINITDEPTH 200
#endif

/*  YYMAXDEPTH is the maximum size the stacks can grow to
    (effective only if the built-in stack extension method is used).  */

#if YYMAXDEPTH == 0
#undef YYMAXDEPTH
#endif

#ifndef YYMAXDEPTH
#define YYMAXDEPTH 10000
#endif

/* Prevent warning if -Wstrict-prototypes.  */
#ifdef __GNUC__
int yyparse (void);
#endif

#if __GNUC__ > 1		/* GNU C and GNU C++ define this.  */
#define __yy_bcopy(FROM,TO,COUNT)	__builtin_memcpy(TO,FROM,COUNT)
#else				/* not GNU C or C++ */
#ifndef __cplusplus

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_bcopy (from, to, count)
     char *from;
     char *to;
     int count;
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#else /* __cplusplus */

/* This is the most reliable way to avoid incompatibilities
   in available built-in functions on various systems.  */
static void
__yy_bcopy (char *from, char *to, int count)
{
  register char *f = from;
  register char *t = to;
  register int i = count;

  while (i-- > 0)
    *t++ = *f++;
}

#endif
#endif

#line 184 "/usr/lib/bison.simple"
int
yyparse()
{
  register int yystate;
  register int yyn;
  register short *yyssp;
  register YYSTYPE *yyvsp;
  int yyerrstatus;	/*  number of tokens to shift before error messages enabled */
  int yychar1 = 0;		/*  lookahead token as an internal (translated) token number */

  short	yyssa[YYINITDEPTH];	/*  the state stack			*/
  YYSTYPE yyvsa[YYINITDEPTH];	/*  the semantic value stack		*/

  short *yyss = yyssa;		/*  refer to the stacks thru separate pointers */
  YYSTYPE *yyvs = yyvsa;	/*  to allow yyoverflow to reallocate them elsewhere */

#ifdef YYLSP_NEEDED
  YYLTYPE yylsa[YYINITDEPTH];	/*  the location stack			*/
  YYLTYPE *yyls = yylsa;
  YYLTYPE *yylsp;

#define YYPOPSTACK   (yyvsp--, yyssp--, yylsp--)
#else
#define YYPOPSTACK   (yyvsp--, yyssp--)
#endif

  int yystacksize = YYINITDEPTH;

#ifdef YYPURE
  int yychar;
  YYSTYPE yylval;
  int yynerrs;
#ifdef YYLSP_NEEDED
  YYLTYPE yylloc;
#endif
#endif

  YYSTYPE yyval;		/*  the variable used to return		*/
				/*  semantic values from the action	*/
				/*  routines				*/

  int yylen;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Starting parse\n");
#endif

  yystate = 0;
  yyerrstatus = 0;
  yynerrs = 0;
  yychar = YYEMPTY;		/* Cause a token to be read.  */

  /* Initialize stack pointers.
     Waste one element of value and location stack
     so that they stay on the same level as the state stack.
     The wasted elements are never initialized.  */

  yyssp = yyss - 1;
  yyvsp = yyvs;
#ifdef YYLSP_NEEDED
  yylsp = yyls;
#endif

/* Push a new state, which is found in  yystate  .  */
/* In all cases, when you get here, the value and location stacks
   have just been pushed. so pushing a state here evens the stacks.  */
yynewstate:

  *++yyssp = yystate;

  if (yyssp >= yyss + yystacksize - 1)
    {
      /* Give user a chance to reallocate the stack */
      /* Use copies of these so that the &'s don't force the real ones into memory. */
      YYSTYPE *yyvs1 = yyvs;
      short *yyss1 = yyss;
#ifdef YYLSP_NEEDED
      YYLTYPE *yyls1 = yyls;
#endif

      /* Get the current used size of the three stacks, in elements.  */
      int size = yyssp - yyss + 1;

#ifdef yyoverflow
      /* Each stack pointer address is followed by the size of
	 the data in use in that stack, in bytes.  */
#ifdef YYLSP_NEEDED
      /* This used to be a conditional around just the two extra args,
	 but that might be undefined if yyoverflow is a macro.  */
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yyls1, size * sizeof (*yylsp),
		 &yystacksize);
#else
      yyoverflow("parser stack overflow",
		 &yyss1, size * sizeof (*yyssp),
		 &yyvs1, size * sizeof (*yyvsp),
		 &yystacksize);
#endif

      yyss = yyss1; yyvs = yyvs1;
#ifdef YYLSP_NEEDED
      yyls = yyls1;
#endif
#else /* no yyoverflow */
      /* Extend the stack our own way.  */
      if (yystacksize >= YYMAXDEPTH)
	{
	  yyerror("parser stack overflow");
	  return 2;
	}
      yystacksize *= 2;
      if (yystacksize > YYMAXDEPTH)
	yystacksize = YYMAXDEPTH;
      yyss = (short *) alloca (yystacksize * sizeof (*yyssp));
      __yy_bcopy ((char *)yyss1, (char *)yyss, size * sizeof (*yyssp));
      yyvs = (YYSTYPE *) alloca (yystacksize * sizeof (*yyvsp));
      __yy_bcopy ((char *)yyvs1, (char *)yyvs, size * sizeof (*yyvsp));
#ifdef YYLSP_NEEDED
      yyls = (YYLTYPE *) alloca (yystacksize * sizeof (*yylsp));
      __yy_bcopy ((char *)yyls1, (char *)yyls, size * sizeof (*yylsp));
#endif
#endif /* no yyoverflow */

      yyssp = yyss + size - 1;
      yyvsp = yyvs + size - 1;
#ifdef YYLSP_NEEDED
      yylsp = yyls + size - 1;
#endif

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Stack size increased to %d\n", yystacksize);
#endif

      if (yyssp >= yyss + yystacksize - 1)
	YYABORT;
    }

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Entering state %d\n", yystate);
#endif

  goto yybackup;
 yybackup:

/* Do appropriate processing given the current state.  */
/* Read a lookahead token if we need one and don't already have one.  */
/* yyresume: */

  /* First try to decide what to do without reference to lookahead token.  */

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* yychar is either YYEMPTY or YYEOF
     or a valid token in external form.  */

  if (yychar == YYEMPTY)
    {
#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Reading a token: ");
#endif
      yychar = YYLEX;
    }

  /* Convert token to internal form (in yychar1) for indexing tables with */

  if (yychar <= 0)		/* This means end of input. */
    {
      yychar1 = 0;
      yychar = YYEOF;		/* Don't call YYLEX any more */

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Now at end of input.\n");
#endif
    }
  else
    {
      yychar1 = YYTRANSLATE(yychar);

#if YYDEBUG != 0
      if (yydebug)
	{
	  fprintf (stderr, "Next token is %d (%s", yychar, yytname[yychar1]);
	  /* Give the individual parser a way to print the precise meaning
	     of a token, for further debugging info.  */
#ifdef YYPRINT
	  YYPRINT (stderr, yychar, yylval);
#endif
	  fprintf (stderr, ")\n");
	}
#endif
    }

  yyn += yychar1;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != yychar1)
    goto yydefault;

  yyn = yytable[yyn];

  /* yyn is what to do for this token type in this state.
     Negative => reduce, -yyn is rule number.
     Positive => shift, yyn is new state.
       New state is final state => don't bother to shift,
       just return success.
     0, or most negative number => error.  */

  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrlab;

  if (yyn == YYFINAL)
    YYACCEPT;

  /* Shift the lookahead token.  */

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting token %d (%s), ", yychar, yytname[yychar1]);
#endif

  /* Discard the token being shifted unless it is eof.  */
  if (yychar != YYEOF)
    yychar = YYEMPTY;

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  /* count tokens shifted since error; after three, turn off error status.  */
  if (yyerrstatus) yyerrstatus--;

  yystate = yyn;
  goto yynewstate;

/* Do the default action for the current state.  */
yydefault:

  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;

/* Do a reduction.  yyn is the number of a rule to reduce with.  */
yyreduce:
  yylen = yyr2[yyn];
  if (yylen > 0)
    yyval = yyvsp[1-yylen]; /* implement default value of the action */

#if YYDEBUG != 0
  if (yydebug)
    {
      int i;

      fprintf (stderr, "Reducing via rule %d (line %d), ",
	       yyn, yyrline[yyn]);

      /* Print the symbols being reduced, and their result.  */
      for (i = yyprhs[yyn]; yyrhs[i] > 0; i++)
	fprintf (stderr, "%s ", yytname[yyrhs[i]]);
      fprintf (stderr, " -> %s\n", yytname[yyr1[yyn]]);
    }
#endif


  switch (yyn) {

case 5:
#line 49 "gram.y"
{
    NMENU *mkmenu(), *mp ;

    if(mp = mkmenu(yyvsp[-1].mival)) {
        mp->menu_id = yyvsp[-9].sval ;
        mp->menu_title = yyvsp[-7].sval ;
        mp->menu_prompt = yyvsp[-5].sval ;
        mp->menu_default = yyvsp[-3].sval ;
        pushmenu(mp) ;
    }
;
    break;}
case 6:
#line 61 "gram.y"
{
    extern char *default_env ;
    default_env = yyvsp[-1].sval ;
    parse_default() ;
;
    break;}
case 7:
#line 68 "gram.y"
{
    yyval.mival = yyvsp[0].mival ;
;
    break;}
case 8:
#line 72 "gram.y"
{
    if(yyvsp[-1].mival)
      yyvsp[0].mival->prev = yyvsp[-1].mival ;
    yyval.mival = yyvsp[0].mival ;
;
    break;}
case 9:
#line 79 "gram.y"
{
    register NMENUITEM *mip ;

    if(mip = yyvsp[-3].mival) {
        mip->name = yyvsp[-11].sval ;
        mip->enabled = yyvsp[-5].ival ;
        mip->default_action = yyvsp[-9].sval ;
        mip->error_action = yyvsp[-7].sval ;
        mip->help = yyvsp[-1].sval ;
    }
    yyval.mival = mip ;
;
    break;}
case 10:
#line 93 "gram.y"
{
    NREADMENU *mkreadmenu(), *mp ;

    if(mp = mkreadmenu(yyvsp[-21].sval, yyvsp[-1].rmival)) {
        mp->menu_helptitle = yyvsp[-3].sval;
        mp->menu_title = yyvsp[-19].sval;
        mp->menu_message = yyvsp[-17].sval;
        mp->menu_line2 = yyvsp[-15].sval;
        mp->menu_line3 = yyvsp[-13].sval;
        mp->menu_field1 = yyvsp[-11].sval;
        mp->menu_field2 = yyvsp[-9].sval;
        mp->menu_field3 = yyvsp[-7].sval;
        mp->menu_field4 = yyvsp[-5].sval;
    }
;
    break;}
case 11:
#line 110 "gram.y"
{
    yyval.rmival = yyvsp[0].rmival ;
;
    break;}
case 12:
#line 114 "gram.y"
{
    if(yyvsp[-1].rmival)
      yyvsp[0].rmival->next = yyvsp[-1].rmival ;
    yyval.rmival = yyvsp[0].rmival ;
;
    break;}
case 13:
#line 121 "gram.y"
{
    register NREADMENUITEM *mip ;

    if(mip = yyvsp[-7].rmival) {
        mip->key = yyvsp[-9].ival ;
        mip->mainprivs = yyvsp[-3].ival ;
        mip->boardprivs = yyvsp[-5].ival ;
        mip->help = yyvsp[-1].sval ;
    }
    yyval.rmival = mip ;
;
    break;}
case 14:
#line 134 "gram.y"
{ yyval.ival = yyvsp[0].ival ; ;
    break;}
case 15:
#line 136 "gram.y"
{ yyval.ival = yyvsp[0].ival ; ;
    break;}
case 16:
#line 139 "gram.y"
{
    NMENUITEM *mkmenuitem() ;
    char buf[512] ;
    strncpy(buf,get_yytext(),yyleng) ;
    buf[yyleng] = '\0' ;

    yyval.mival = mkmenuitem(buf+1) ;
;
    break;}
case 17:
#line 149 "gram.y"
{ yyval.mival = yyvsp[0].mival; ;
    break;}
case 18:
#line 151 "gram.y"
{
    NMENUITEM *mip = yyvsp[-1].mival ;

    if(mip)
      mip->action_arg = yyvsp[0].sval ;
    yyval.mival = mip ;
;
    break;}
case 19:
#line 160 "gram.y"
{
    NREADMENUITEM *mkreadmenuitem() ;
    char buf[512] ;
    strncpy(buf,get_yytext(),yyleng) ;
    buf[yyleng] = '\0' ;

    yyval.rmival = mkreadmenuitem(buf+1) ;
;
    break;}
case 20:
#line 170 "gram.y"
{
    char *mkstring() ;
    char *ExpandString() ;
    char buf[512] ;
    
    strncpy(buf,get_yytext(),yyleng) ;
    buf[yyleng-1] = '\0' ;
    yyval.sval = mkstring(ExpandString(buf+1)) ;
;
    break;}
case 21:
#line 181 "gram.y"
{
    int atoi() ;

    yyval.ival = atoi(get_yytext()) ;
;
    break;}
case 22:
#line 188 "gram.y"
{
    int convert_openflag_to_int() ;
    char buf[512] ;
    
    strncpy(buf,get_yytext(),yyleng) ;
    buf[yyleng] = '\0' ;
    yyval.ival = convert_openflag_to_int(buf+2) ;
;
    break;}
case 23:
#line 195 "gram.y"
{ yyval.ival = yyvsp[0].ival; ;
    break;}
case 24:
#line 198 "gram.y"
{
    int convert_cmd_to_int() ;
    char buf[512] ;
    
    strncpy(buf,get_yytext(),yyleng) ;
    buf[yyleng] = '\0' ;
    yyval.ival = convert_cmd_to_int(buf) ;
;
    break;}
case 25:
#line 208 "gram.y"
{
    int convert_key_to_int() ;
    char buf[512] ;
    
    strncpy(buf,get_yytext(),yyleng) ;
    buf[yyleng] = '\0' ;
    yyval.ival = convert_key_to_int(buf) ;
;
    break;}
}
   /* the action file gets copied in in place of this dollarsign */
#line 465 "/usr/lib/bison.simple"

  yyvsp -= yylen;
  yyssp -= yylen;
#ifdef YYLSP_NEEDED
  yylsp -= yylen;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

  *++yyvsp = yyval;

#ifdef YYLSP_NEEDED
  yylsp++;
  if (yylen == 0)
    {
      yylsp->first_line = yylloc.first_line;
      yylsp->first_column = yylloc.first_column;
      yylsp->last_line = (yylsp-1)->last_line;
      yylsp->last_column = (yylsp-1)->last_column;
      yylsp->text = 0;
    }
  else
    {
      yylsp->last_line = (yylsp+yylen-1)->last_line;
      yylsp->last_column = (yylsp+yylen-1)->last_column;
    }
#endif

  /* Now "shift" the result of the reduction.
     Determine what state that goes to,
     based on the state we popped back to
     and the rule number reduced by.  */

  yyn = yyr1[yyn];

  yystate = yypgoto[yyn - YYNTBASE] + *yyssp;
  if (yystate >= 0 && yystate <= YYLAST && yycheck[yystate] == *yyssp)
    yystate = yytable[yystate];
  else
    yystate = yydefgoto[yyn - YYNTBASE];

  goto yynewstate;

yyerrlab:   /* here on detecting error */

  if (! yyerrstatus)
    /* If not already recovering from an error, report this error.  */
    {
      ++yynerrs;

#ifdef YYERROR_VERBOSE
      yyn = yypact[yystate];

      if (yyn > YYFLAG && yyn < YYLAST)
	{
	  int size = 0;
	  char *msg;
	  int x, count;

	  count = 0;
	  /* Start X at -yyn if nec to avoid negative indexes in yycheck.  */
	  for (x = (yyn < 0 ? -yyn : 0);
	       x < (sizeof(yytname) / sizeof(char *)); x++)
	    if (yycheck[x + yyn] == x)
	      size += strlen(yytname[x]) + 15, count++;
	  msg = (char *) malloc(size + 15);
	  if (msg != 0)
	    {
	      strcpy(msg, "parse error");

	      if (count < 5)
		{
		  count = 0;
		  for (x = (yyn < 0 ? -yyn : 0);
		       x < (sizeof(yytname) / sizeof(char *)); x++)
		    if (yycheck[x + yyn] == x)
		      {
			strcat(msg, count == 0 ? ", expecting `" : " or `");
			strcat(msg, yytname[x]);
			strcat(msg, "'");
			count++;
		      }
		}
	      yyerror(msg);
	      free(msg);
	    }
	  else
	    yyerror ("parse error; also virtual memory exceeded");
	}
      else
#endif /* YYERROR_VERBOSE */
	yyerror("parse error");
    }

  goto yyerrlab1;
yyerrlab1:   /* here on error raised explicitly by an action */

  if (yyerrstatus == 3)
    {
      /* if just tried and failed to reuse lookahead token after an error, discard it.  */

      /* return failure if at end of input */
      if (yychar == YYEOF)
	YYABORT;

#if YYDEBUG != 0
      if (yydebug)
	fprintf(stderr, "Discarding token %d (%s).\n", yychar, yytname[yychar1]);
#endif

      yychar = YYEMPTY;
    }

  /* Else will try to reuse lookahead token
     after shifting the error token.  */

  yyerrstatus = 3;		/* Each real token shifted decrements this */

  goto yyerrhandle;

yyerrdefault:  /* current state does not do anything special for the error token. */

#if 0
  /* This is wrong; only states that explicitly want error tokens
     should shift them.  */
  yyn = yydefact[yystate];  /* If its default is to accept any token, ok.  Otherwise pop it.*/
  if (yyn) goto yydefault;
#endif

yyerrpop:   /* pop the current state because it cannot handle the error token */

  if (yyssp == yyss) YYABORT;
  yyvsp--;
  yystate = *--yyssp;
#ifdef YYLSP_NEEDED
  yylsp--;
#endif

#if YYDEBUG != 0
  if (yydebug)
    {
      short *ssp1 = yyss - 1;
      fprintf (stderr, "Error: state stack now");
      while (ssp1 != yyssp)
	fprintf (stderr, " %d", *++ssp1);
      fprintf (stderr, "\n");
    }
#endif

yyerrhandle:

  yyn = yypact[yystate];
  if (yyn == YYFLAG)
    goto yyerrdefault;

  yyn += YYTERROR;
  if (yyn < 0 || yyn > YYLAST || yycheck[yyn] != YYTERROR)
    goto yyerrdefault;

  yyn = yytable[yyn];
  if (yyn < 0)
    {
      if (yyn == YYFLAG)
	goto yyerrpop;
      yyn = -yyn;
      goto yyreduce;
    }
  else if (yyn == 0)
    goto yyerrpop;

  if (yyn == YYFINAL)
    YYACCEPT;

#if YYDEBUG != 0
  if (yydebug)
    fprintf(stderr, "Shifting error token, ");
#endif

  *++yyvsp = yylval;
#ifdef YYLSP_NEEDED
  *++yylsp = yylloc;
#endif

  yystate = yyn;
  goto yynewstate;
}
#line 216 "gram.y"


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



