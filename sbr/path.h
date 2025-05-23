/* path.h -- return a pathname
 *
 * This code is Copyright (c) 2017, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information. */

void set_mypath(void);
char *pluspath(char *);
char *path(char *, int);
char *etcpath(char *) NONNULL(1);
