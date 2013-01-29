
#############################################################################
#
# Makefile for Eagles BBS version 3.1
# 
#############################################################################

# This is the configurable part. Put anything OS-dependent here.
# You do not need to worry with LEX, YACC, or YFLAGS unless you want to
# modify the menu parser.

# your C compiler of choice
CC=gcc

# your C linker of choice
LD=gcc

# archiver of choice
AR=ar

# ranlib, if applicable. 
# SunOS, OSF/1, NeXT, A/UX, MachTen, FreeBSD use RANLIB=ranlib.
RANLIB=echo Not using ranlib:

# special include paths?
INCLUDES=

# special libs?
# Solaris needs -lsocket -lnsl -L/usr/ccs/lib
# MachTen needs -lcompat
# Unixware needs -lsocket -lnsl (and maybe -L/usr/ucblib -lucb)
# FreeBSD needs -lcrypt -lcompat -lipc
LIBS=

# flags for the compiler
# NeXT wants -pipe -s -O4 -arch m68040; FreeBSD wants -O2 -m486 -pipe
# might want to try -DMENU_STANDOUT too if you're a dialup bbs
# might want -DEXTRA_CHAT_STUFF or -DNO_IGNORE_CHATOPS, see docs
CCFLAGS=-O -DCOLOR

# flags for the linker -- NeXT wants -s -O4; FreeBSD wants -s
LDFLAGS=-O

# flags for the archiver -- remove s if using ranlib, add c for NeXT, FreeBSD
ARFLAGS=rs

# zipper for srcdist and bindist targets, if desired
ZIP=gzip

# scanner generator: lex or flex
LEX=flex

# parser: yacc or bison
YACC=bison

# flags for parser
YFLAGS=-d -o y.tab.c

# default install directory for the install script
INSTALLDIR=

# The rest shouldn't need messing with.

##############################################################################

CFLAGS=$(CCFLAGS) $(INCLUDES)

BINS=lbbs chatd addacct delacct bbslog bbfinger bbsmaild

DOCS=README ChangeLog EBBS-Guide ReleaseNotes

all: $(BINS)

##############################################################################
# BBS Library make
##############################################################################

LIBOBJS=record.o util.o name.o log.o utable.o login.o passwd.o home.o init.o \
acct.o misc.o headers.o readbits.o board.o files.o exec.o uldl.o chat.o \
talk.o netmail.o edit.o conv.o

libbbs.a: $(LIBOBJS)
	-rm -f libbbs.a
	$(AR) $(ARFLAGS) libbbs.a $(LIBOBJS)
	$(RANLIB) libbbs.a

# Here are the makes for the external utility programs.

addacct.o: addacct.c server.h common.h
	$(CC) -c $(CFLAGS) addacct.c

addacct: addacct.o libbbs.a 
	$(CC) $(LDFLAGS) -o addacct addacct.o libbbs.a $(LIBS)

delacct.o: delacct.c server.h common.h
	$(CC) -c $(CFLAGS) delacct.c

delacct: delacct.o libbbs.a 
	$(CC) $(LDFLAGS) -o delacct delacct.o libbbs.a $(LIBS)

bbslog.o: bbslog.c server.h common.h
	$(CC) -c $(CFLAGS) bbslog.c

bbslog: bbslog.o libbbs.a 
	$(CC) $(LDFLAGS) -o bbslog bbslog.o libbbs.a $(LIBS)

bbfinger.o: bbfinger.c server.h common.h
	$(CC) -c $(CFLAGS) bbfinger.c

bbfinger: bbfinger.o modes.o libbbs.a 
	$(CC) $(LDFLAGS) -o bbfinger bbfinger.o modes.o libbbs.a $(LIBS)

bbsmaild.o: bbsmaild.c server.h common.h
	$(CC) -c $(CFLAGS) bbsmaild.c

bbsmaild: bbsmaild.o libbbs.a 
	$(CC) $(LDFLAGS) -o bbsmaild bbsmaild.o libbbs.a $(LIBS)

############################################################################
# BBS daemon make (not quite finished)
############################################################################

#bbsd:
#	cd server; make CC="$(CC)" CFLAGS="$(CFLAGS)" LD="$(LD)" LDFLAGS="$(LDFLAGS)" LIBS="$(LIBS)" bbsd

############################################################################
# Chat daemon make
############################################################################

chatserv.o: chatserv.c
	$(CC) -c $(CFLAGS) chatserv.c

chatconf.o: chatconf.c
	$(CC) -c $(CFLAGS) chatconf.c

chatd: chatserv.o chatconf.o libbbs.a
	$(LD) $(LDFLAGS) -o chatd chatserv.o chatconf.o libbbs.a $(LIBS)

############################################################################
# BBS client make (not quite finished)
############################################################################

#bbs:
#	cd client; make CC="$(CC)" CFLAGS="$(CFLAGS)" LD="$(LD)" LDFLAGS="$(LDFLAGS)" AR="$(AR)" ARFLAGS="$(ARFLAGS)" RANLIB="$(RANLIB)" LIBS="$(LIBS)" bbs

############################################################################
# Local client make
############################################################################

LLIBS=libbbs.a pbbs/libpbbs.a

LOBJS=client.o menus.o complete.o system.o c_users.o readmenu.o c_mail.o \
modes.o c_boards.o c_post.o readnew.o c_files.o c_chat.o c_talk.o c_lists.o \
nmenus.o env.o y.tab.o lex.yy.o

PLIBOBJS=pbbs/term.o pbbs/screen.o pbbs/io.o pbbs/stuff.o \
pbbs/more.o pbbs/vedit.o

pbbs/term.o: pbbs/term.c
	cd pbbs; $(CC) -c -I.. $(CFLAGS) term.c

pbbs/screen.o: pbbs/screen.c
	cd pbbs; $(CC) -c -I.. $(CFLAGS) screen.c

pbbs/io.o: pbbs/io.c
	cd pbbs; $(CC) -c -I.. $(CFLAGS) io.c

pbbs/stuff.o: pbbs/stuff.c
	cd pbbs; $(CC) -c -I.. $(CFLAGS) stuff.c

pbbs/more.o: pbbs/more.c
	cd pbbs; $(CC) -c -I.. $(CFLAGS) more.c

pbbs/vedit.o: pbbs/vedit.c
	cd pbbs; $(CC) -c -I.. $(CFLAGS) vedit.c

pbbs/libpbbs.a: $(PLIBOBJS)
	-rm -f pbbs/libpbbs.a
	$(AR) $(ARFLAGS) pbbs/libpbbs.a $(PLIBOBJS)
	$(RANLIB) pbbs/libpbbs.a

lex.yy.c:  menu.l
	$(LEX) menu.l

lex.yy.o: lex.yy.c y.tab.o

y.tab.c: gram.y
	$(YACC) $(YFLAGS) gram.y

lbbs: $(LOBJS) $(LLIBS)
	$(LD) $(LDFLAGS) -o lbbs $(LOBJS) $(LLIBS) $(LIBS) -ltermcap

srcdist:
	rm -f ebbssrc.tar ebbssrc.tar.gz 
	tar -cvf ebbssrc.tar [A-Z]* *.[chly] pbbs/*.c pbbs/*.h config/*
	$(ZIP) ebbssrc.tar

bindist: all 
	rm -f ebbsbin.tar ebbsbin.tar.gz
	if [ ! -d deliv ]; then \
	  mkdir deliv deliv/bin deliv/boards deliv/etc deliv/tmp; \
	  mkdir deliv/home deliv/home/SYSOP deliv/home/SYSOP/mail; \
	fi
	cp $(DOCS) deliv
	cp $(BINS) deliv/bin
	chmod 6755 deliv/bin/lbbs deliv/bin/bbsmaild
	cp config/* deliv/etc
	(cd deliv/bin; strip $(BINS))	
	(cd deliv; tar -cvf ../ebbsbin.tar .)
	$(ZIP) ebbsbin.tar        

install: all
	./Install.sh $(INSTALLDIR)

clean:
	rm -f *.o pbbs/*.o *~ pbbs/*~ config/*~ *# y.output

clobber: clean
	rm -f $(BINS) libbbs.a pbbs/libpbbs.a
