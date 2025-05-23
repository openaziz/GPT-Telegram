/* globals.h -- global variable declarations
 *
 * This code is Copyright (c) 2021, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information.
 */

/* See the definitions for commentary; tags should take you there. */

extern struct swit anoyes[];
extern char *mh_defaults;
extern char *mh_profile;
extern char *credentials_file;
extern int credentials_no_perm_check;
extern char *current;
extern char *components;
extern char *replcomps;
extern char *replgroupcomps;
extern char *forwcomps;
extern char *distcomps;
extern char *rcvdistcomps;
extern char *digestcomps;
extern char *mhlformat;
extern char *mhlreply;
extern char *mhlforward;
extern char *draft;
extern char *inbox;
extern char *defaultfolder;
extern char *pfolder;
extern char *usequence;
extern char *psequence;
extern char *nsequence;
extern char *nmhstorage;
extern char *nmhcache;
extern char *nmhprivcache;
extern char *nmhaccessftp;
extern char *nmhaccessurl;
extern char *mhbindir;
extern char *mhlibexecdir;
extern char *mhetcdir;
extern char *mhdocdir;
extern char *context;
extern char *mh_seq;
extern bool context_dirty;
extern char *invo_name;
extern char *mypath;
extern char *defpath;
extern char *ctxpath;
extern struct node *m_defs;
extern char *buildmimeproc;
extern char *catproc;
extern char *fileproc;
extern char *formatproc;
extern char *incproc;
extern char *lproc;
extern char *mailproc;
extern char *moreproc;
extern char *mhlproc;
extern char *packproc;
extern char *postproc;
extern char *rcvstoreproc;
extern char *rmmproc;
extern char *sendproc;
extern char *showmimeproc;
extern char *showproc;
extern char *whatnowproc;
extern char *whomproc;
extern char *AliasFile;
extern char *foldprot;
extern char *msgprot;
