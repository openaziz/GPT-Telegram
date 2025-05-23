/* globals.c -- global variable definitions
 *
 * This code is Copyright (c) 2002, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information.
 */

#include "h/mh.h"


/* 
 * Standard yes/no switches structure
 */

struct swit anoyes[] = {
    { "no", 0, 0 },
    { "yes", 0, 1 },
    { NULL, 0, 0 }
};

/* 
 * nmh constants
 */

/* initial profile for new users */
char *mh_defaults = NMHETCDIR "/mh.profile";

/* default name of user profile */
char *mh_profile = ".mh_profile";

/* name of credentials file, defaults to .netrc in either Path or $HOME. */
char *credentials_file;

/* if set to 1, do not check permissions on credentials file */
int credentials_no_perm_check = 0;

/* name of current message "sequence" */
char *current = "cur";

/* standard component files */
char *components = "components";		/* comp         */
char *replcomps = "replcomps";			/* repl         */
char *replgroupcomps = "replgroupcomps";	/* repl -group  */
char *forwcomps = "forwcomps";			/* forw         */
char *distcomps = "distcomps";			/* dist         */
char *rcvdistcomps = "rcvdistcomps";		/* rcvdist      */
char *digestcomps = "digestcomps";		/* forw -digest */

/* standard format (filter) files */
char *mhlformat = "mhl.format";			/* show         */
char *mhlreply = "mhl.reply";			/* repl -filter */
char *mhlforward = "mhl.forward";		/* forw -filter */

char *draft = "draft";

char *inbox = "Inbox";
char *defaultfolder = "inbox";

char *pfolder = "Current-Folder";
char *usequence = "Unseen-Sequence";
char *psequence = "Previous-Sequence";
char *nsequence = "Sequence-Negation";

/* profile entries for storage locations */
char *nmhstorage = "nmh-storage";
char *nmhcache = "nmh-cache";
char *nmhprivcache = "nmh-private-cache";

/* profile entry for external ftp access command */
char *nmhaccessftp = "nmh-access-ftp";

/* profile entry for external url access command */
char *nmhaccessurl = "nmh-access-url";

char *mhbindir = NMHBINDIR;
char *mhlibexecdir = NMHLIBEXECDIR;
char *mhetcdir = NMHETCDIR;
char *mhdocdir = NMHDOCDIR;

/* 
 * nmh not-so constants
 */

/*
 * Default name for the nmh context file.
 */
char *context = "context";

/*
 * Default name of file for public sequences.  If "\0" (an empty
 * "mh-sequences" profile entry), then nmh will use private sequences by
 * default.
 */
char *mh_seq = ".mh_sequences";

/* 
 * nmh globals
 */

/* The in-memory copy of the context has been modified. */
bool context_dirty;

char *invo_name;	/* command invocation name    */
char *mypath;		/* user's $HOME               */
char *defpath;		/* pathname of user's profile */
char *ctxpath;		/* pathname of user's context */
struct node *m_defs;	/* profile/context structure  */

/* 
 * nmh processes
 */

/*
 * This is the program to process MIME composition files
 */
char *buildmimeproc = NMHBINDIR "/mhbuild";
/*
 * This is the program to `cat' a file.
 */
char *catproc = "/bin/cat";

/*
 * This program is usually called directly by users, but it is
 * also invoked by the post program to process an "Fcc", or by
 * comp/repl/forw/dist to refile a draft message.
 */

char *fileproc = NMHBINDIR "/refile";

/*
 * This program is used to optionally format the bodies of messages by
 * "mhl".
 */

char *formatproc = NULL;

/* 
 * This program is called to incorporate messages into a folder.
 */

char *incproc = NMHBINDIR "/inc";

/*
 * This is the default program invoked by a "list" response
 * at the "What now?" prompt.  It is also used by the draft
 * folder facility in comp/dist/forw/repl to display the
 * draft message.
 */

char *lproc = NULL;

/*
 * This is the path for the Bell equivalent mail program.
 */

char *mailproc = NMHBINDIR "/mhmail";

/*
 * This is used by mhl as a front-end.  It is also used
 * by mhn as the default method of displaying message bodies
 * or message parts of type text/plain.
 */

char *moreproc = NULL;

/* 
 * This is the program (mhl) used to filter messages.  It is
 * used by mhn to filter and display the message headers of
 * MIME messages.  It is used by repl/forw (with -filter)
 * to filter the message to which you are replying/forwarding.
 * It is used by send/post (with -filter) to filter the message
 * for "Bcc:" recipients.
 */

char *mhlproc = NMHLIBEXECDIR "/mhl";

/* 
 * This program is called to pack a folder.  
 */

char *packproc = NMHBINDIR "/packf";

/*
 * This is the delivery program called by send to actually
 * deliver mail to users.  This is the interface to the MTS.
 */

char *postproc = NMHLIBEXECDIR "/post";

/*
 * This is program is called by slocal to handle
 * the action `folder' or `+'.
 */

char *rcvstoreproc = NMHLIBEXECDIR "/rcvstore";

/* 
 * This program is called to remove a message by rmm or refile -nolink.
 * It's usually empty, which means to rename the file to a backup name.
 */

char *rmmproc = NULL;

/*
 * This program is usually called by the user's whatnowproc, but it
 * may also be called directly to send a message previously composed.
 */

char *sendproc = NMHBINDIR "/send";

/*
 * This is the path to the program used by "show"
 * to display non-text (MIME) messages.
 */

char *showmimeproc = NMHBINDIR "/mhshow";

/*
 * This is the default program called by "show" to filter
 * and display standard text (non-MIME) messages.  It can be
 * changed to a pager (such as "more" or "less") if you prefer
 * that such message not be filtered in any way.
 */

char *showproc = NMHLIBEXECDIR "/mhl";

/* 
 * This program is called after comp, et. al., have built a draft
 */

char *whatnowproc = NMHBINDIR "/whatnow";

/* 
 * This program is called to list/validate the addresses in a message.
 */

char *whomproc = NMHBINDIR "/whom";

/* 
 * This is the global nmh alias file.  It is somewhat obsolete, since
 * global aliases should be handled by the Mail Transport Agent (MTA).
 */

char *AliasFile = NMHETCDIR "/MailAliases";

/* 
 * File protections
 */

/*
 * Folders (directories) are created with this protection (mode)
 */

char *foldprot = "700";

/*
 * Every NEW message will be created with this protection.  When a
 * message is filed it retains its protection, so this only applies
 * to messages coming in through inc.
 */

char *msgprot = "600";
