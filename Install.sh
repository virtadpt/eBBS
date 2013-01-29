#!/bin/sh
# 
# Eagles BBS 3.1 install script
#

homedir=$1
if [ "$homedir" = "" ]
then
    homedir=$BBSHOME;
    if [ "$homedir" = "" ]
    then
        homedir=`grep \^bbs: /etc/passwd | cut -d: -f6`
        if [ "$homedir" = "" ]
        then
    	    echo Cannot determine bbs home directory\!
	    echo You must do one of the following\:
	    echo \ \ \ \ \-\- define INSTALLDIR in the Makefile
	    echo \ \ \ \ \-\- define BBSHOME in the environment
	    echo \ \ \ \ \-\- add an entry for 'bbs' into /etc/passwd.
	    exit 1
	fi
    fi
fi

echo Checking directories
if [ ! -d "$homedir" ]
then
    echo Creating "$homedir"
    mkdir $homedir
    chmod 770 $homedir
fi

if [ ! -d "$homedir/bin" ]
then
    echo Creating $homedir/bin
    mkdir $homedir/bin
fi

if [ -f "$homedir/bin/lbbs" ]
then
    echo Moving existing lbbs to lbbs.old
    mv $homedir/bin/lbbs $homedir/bin/lbbs.old
fi

if [ -f "$homedir/bin/chatd" ]
then
    echo Moving existing chatd to chatd.old
    mv $homedir/bin/chatd $homedir/bin/chatd.old
fi

echo Copying new executables
cp lbbs chatd addacct delacct bbslog bbfinger bbsmaild $homedir/bin
(cd $homedir/bin; strip lbbs chatd addacct delacct bbslog bbfinger bbsmaild)
chmod 6755 $homedir/bin/lbbs $homedir/bin/bbsmaild

if [ ! -d "$homedir/etc" ]
then
    echo Creating $homedir/etc
    mkdir $homedir/etc
    echo Copying configuration files
    cp config/* $homedir/etc    
fi

if [ ! -d "$homedir/tmp" ]
then 
    echo Creating $homedir/tmp
    mkdir $homedir/tmp
fi

if [ ! -d "$homedir/home" ]
then 
    echo Creating $homedir/home
    mkdir $homedir/home
fi

if [ ! -d "$homedir/home/SYSOP" ]
then 
    echo Creating $homedir/home/SYSOP
    mkdir $homedir/home/SYSOP
fi

if [ ! -d "$homedir/home/SYSOP/mail" ]
then 
    echo Creating $homedir/home/SYSOP/mail
    mkdir $homedir/home/SYSOP/mail
fi

if [ ! -d "$homedir/boards" ]
then 
    echo Creating $homedir/boards
    mkdir $homedir/boards
fi
