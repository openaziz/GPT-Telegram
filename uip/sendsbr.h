/* sendsbr.h -- routines to help WhatNow/Send along
 *
 * This code is Copyright (c) 2017, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information. */

int sendsbr(char **, int, char *, char *, struct stat *, int, const char *);

extern char *altmsg;
extern char *annotext;
extern char *distfile;

extern bool forwsw;
extern int inplace;
extern bool pushsw;
extern bool unique;
extern bool verbsw;
