/* mhstoresbr.h -- routines to save/store the contents of MIME messages
 *
 * This code is Copyright (c) 2017, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information. */

typedef struct mhstoreinfo *mhstoreinfo_t;

mhstoreinfo_t mhstoreinfo_create(CT *, char *, const char *, int, int);
void mhstoreinfo_free(mhstoreinfo_t);
int mhstoreinfo_files_not_clobbered(const mhstoreinfo_t) PURE;
void store_all_messages(mhstoreinfo_t);
