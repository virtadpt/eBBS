
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

#include "common.h"
#include "perms.h"
#include <stdio.h>

#define CLIENT_VERSION_MAJ	0
#define CLIENT_VERSION_MIN	1

/* 
   One day there will be a different libbbs.a that will be the client side
   of a remote bbs service. The entry points will just pack the parameters
   into network form and ship them across to the server, who will unpack
   them and call the local_bbs_* functions to do the work.

   For the local libbbs.a, we can just #define all the entry points to the
   library to be the local_bbs_* functions.
*/

#ifndef REMOTE_CLIENT

#define bbs_initialize local_bbs_initialize
#define bbs_connect local_bbs_connect 
#define bbs_disconnect local_bbs_disconnect 
#define bbs_get_issue local_bbs_get_issue 
#define bbs_get_welcome local_bbs_get_welcome 
#define bbs_newlogin local_bbs_newlogin 
#define bbs_login local_bbs_login 
#define bbs_add_account local_bbs_add_account 
#define bbs_delete_account local_bbs_delete_account 
#define bbs_query local_bbs_query 
#define bbs_owninfo local_bbs_owninfo 
#define bbs_get_userinfo local_bbs_get_userinfo 
#define bbs_set_mode local_bbs_set_mode 
#define bbs_set_passwd local_bbs_set_passwd 
#define bbs_set_username local_bbs_set_username 
#define bbs_set_terminal local_bbs_set_terminal 
#define bbs_set_charset local_bbs_set_charset
#define bbs_set_email local_bbs_set_email 
#define bbs_modify_perms local_bbs_modify_perms 
#define bbs_modify_account local_bbs_modify_account 
#define bbs_enum_accounts local_bbs_enum_accounts 
#define bbs_enum_users local_bbs_enum_users 
#define bbs_acctnames local_bbs_acctnames 
#define bbs_usernames local_bbs_usernames 
#define bbs_get_info local_bbs_get_info 
#define bbs_get_license local_bbs_get_license 
#define bbs_get_plan local_bbs_get_plan 
#define bbs_set_plan local_bbs_set_plan 
#define bbs_toggle_cloak local_bbs_toggle_cloak 
#define bbs_toggle_exempt local_bbs_toggle_exempt 
#define bbs_get_permstrings local_bbs_get_permstrings 
#define bbs_check_mail local_bbs_check_mail 
#define bbs_mail local_bbs_mail
#define bbs_open_mailbox local_bbs_open_mailbox 
#define bbs_close_board local_bbs_close_board 
#define bbs_enum_headers local_bbs_enum_headers 
#define bbs_read_message local_bbs_read_message 
#define bbs_delete_message local_bbs_delete_message 
#define bbs_delete_range local_bbs_delete_range 
#define bbs_mark_message local_bbs_mark_message 
#define bbs_set_welcome local_bbs_set_welcome
#define bbs_add_board local_bbs_add_board
#define bbs_delete_board local_bbs_delete_board
#define bbs_get_board local_bbs_get_board
#define bbs_modify_board local_bbs_modify_board
#define bbs_enum_boards local_bbs_enum_boards
#define bbs_boardnames local_bbs_boardnames
#define bbs_visit_board local_bbs_visit_board
#define bbs_post local_bbs_post
#define bbs_open_board local_bbs_open_board
#define bbs_test_board local_bbs_test_board
#define bbs_update_message local_bbs_update_message
#define bbs_zap_board local_bbs_zap_board
#define bbs_get_boardmgrs local_bbs_get_boardmgrs
#define bbs_set_boardmgrs local_bbs_set_boardmgrs
#define bbs_enum_fileboards local_bbs_enum_fileboards
#define bbs_fileboardnames local_bbs_fileboardnames
#define bbs_open_fileboard local_bbs_open_fileboard
#define bbs_protonames local_bbs_protonames
#define bbs_set_protocol local_bbs_set_protocol
#define bbs_upload local_bbs_upload
#define bbs_download local_bbs_download
#define bbs_chat local_bbs_chat
#define bbs_exit_chat local_bbs_exit_chat
#define bbs_chat_send local_bbs_chat_send
#define bbs_kick_user local_bbs_kick_user
#define bbs_talk local_bbs_talk
#define bbs_exit_talk local_bbs_exit_talk
#define bbs_get_talk_request local_bbs_get_talk_request
#define bbs_accept_page local_bbs_accept_page
#define bbs_refuse_page local_bbs_refuse_page
#define bbs_set_pager local_bbs_set_pager
#define bbs_get_overrides local_bbs_get_overrides
#define bbs_set_overrides local_bbs_set_overrides
#define bbs_forward_message local_bbs_forward_message
#define bbs_forward_file local_bbs_forward_file
#define bbs_set_editor local_bbs_set_editor
#define bbs_get_editor local_bbs_get_editor
#define bbs_enum_editors local_bbs_enum_editors
#define bbs_get_signature local_bbs_get_signature
#define bbs_set_signature local_bbs_set_signature
#define bbs_get_modestrings local_bbs_get_modestrings
#define bbs_get_modechars local_bbs_get_modechars
#define bbs_set_cliopts local_bbs_set_cliopts
#define bbs_change_fileboard_dir local_bbs_change_fileboard_dir
#define bbs_move_message local_bbs_move_message

#endif /* !REMOTE_CLIENT */

/* Function prototypes for libbbs entry points. */

int bbs_initialize __P((INITINFO *));
int bbs_connect __P((char *, SHORT, BBSINFO *));
int bbs_disconnect __P((void));
int bbs_get_issue __P((char *));
int bbs_get_welcome __P((char *));
int bbs_newlogin __P((ACCOUNT *, LOGININFO *));
int bbs_login __P((char *, char *, SHORT, LOGININFO *));
int bbs_add_account __P((ACCOUNT *, SHORT));
int bbs_delete_account __P((char *));
int bbs_query __P((char *, ACCOUNT *));
int bbs_owninfo __P((ACCOUNT *));
int bbs_get_userinfo __P((char *, ACCOUNT *));
int bbs_set_mode __P((SHORT));
int bbs_set_passwd __P((char *));
int bbs_set_username __P((char *));
int bbs_set_terminal __P((char *));
int bbs_set_charset __P((char *));
int bbs_set_email __P((char *));
int bbs_modify_perms __P((char *, LONG));
int bbs_modify_account __P((char *, ACCOUNT *, SHORT));
int bbs_enum_accounts __P((SHORT, SHORT, int(), void *));
int bbs_enum_users __P((SHORT, SHORT, char *, int(), void *));
int bbs_acctnames __P((NAMELIST *, char *));
int bbs_usernames __P((NAMELIST *, char *));
int bbs_get_info __P((char *));
int bbs_get_license __P((char *));
int bbs_get_plan __P((char *, char *));
int bbs_set_plan __P((char *));
int bbs_toggle_cloak __P((void));
int bbs_toggle_exempt __P((char *));
int bbs_get_permstrings __P((char **));
int bbs_check_mail __P((void));
int bbs_mail __P((char *, char *, NAMELIST, char *, char *, LONG *));
int bbs_open_mailbox __P((OPENINFO *));
int bbs_close_board __P((void));
int bbs_enum_headers __P((SHORT, SHORT, SHORT, int(), void *));
int bbs_read_message __P((SHORT, char *));
int bbs_delete_message __P((SHORT));
int bbs_delete_range __P((SHORT, SHORT, SHORT *));
int bbs_mark_message __P((SHORT, SHORT));
int bbs_set_welcome __P((char *));
int bbs_add_board __P((BOARD *));
int bbs_delete_board __P((char *));
int bbs_get_board __P((char *, BOARD *));
int bbs_modify_board __P((char *, BOARD *, SHORT));
int bbs_enum_boards __P((SHORT, SHORT, SHORT, int(), void *));
int bbs_boardnames __P((NAMELIST *, char *));
int bbs_visit_board __P((char *));
int bbs_post __P((char *, char *, char *));
int bbs_open_board __P((char *, OPENINFO *));
int bbs_test_board __P((char *, SHORT *));
int bbs_update_message __P((SHORT, char *));
int bbs_zap_board __P((char *, SHORT));
int bbs_get_boardmgrs __P((char *, NAMELIST *));
int bbs_set_boardmgrs __P((char *, NAMELIST));
int bbs_enum_fileboards __P((SHORT, SHORT, int(), void *));
int bbs_fileboardnames __P((NAMELIST *, char *));
int bbs_open_fileboard __P((char *, OPENINFO *));
int bbs_protonames __P((NAMELIST *, char *));
int bbs_set_protocol __P((char *));
int bbs_upload __P((char *, char *, char *));
int bbs_download __P((char *, char *, char *));
int bbs_chat __P((char *, LONG *));
int bbs_exit_chat __P((void));
int bbs_chat_send __P((char *));
int bbs_kick_user __P((LONG));
int bbs_talk __P((LONG, LONG, LONG *));
int bbs_exit_talk __P((void));
int bbs_get_talk_request __P((USEREC *, LONG *, SHORT *));
int bbs_accept_page __P((LONG, SHORT, LONG *));
int bbs_refuse_page __P((LONG, SHORT));
int bbs_set_pager __P((SHORT, SHORT));
int bbs_get_overrides __P((NAMELIST *));
int bbs_set_overrides __P((NAMELIST));
int bbs_forward_message __P((SHORT));
int bbs_forward_file __P((char *));
int bbs_set_editor __P((char *));
int bbs_get_editor __P((char *, char *, char *));
int bbs_enum_editors __P((NAMELIST *, char *));
int bbs_get_signature __P((char *));
int bbs_set_signature __P((char *));
int bbs_get_modestrings __P((char **));
int bbs_get_modechars __P((char *));
int bbs_set_cliopts __P((SHORT));
int bbs_change_fileboard_dir __P((char *, OPENINFO *));
int bbs_move_message __P((SHORT, char *));

/* Now for the UI-dependent stuff. */

#include "clientui.h"
