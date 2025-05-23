/* mf.h -- mail filter subroutines
 *
 * This code is Copyright (c) 2017, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information. */

struct adrx {
    char *text;
    char *pers;
    char *mbox;
    char *host;
    char *path;
    char *grp;
    int ingrp;
    char *note;
    char *err;
};

/*
 * prototypes
 */
char *legal_person(const char *);
struct adrx *getadrx(const char *, bool);
