/* maildir_read_and_sort.c -- returns a sorted list of msgs in a Maildir.
 *
 * This code is Copyright (c) 2020, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information.
 */

#include "h/mh.h"

#include "sbr/maildir_read_and_sort.h"

#include "sbr/concat.h"
#include "sbr/error.h"

static int maildir_srt(const void *va, const void *vb) PURE;

static int
maildir_srt(const void *va, const void *vb)
{
    const struct Maildir_entry *a = va, *b = vb;
    if (a->mtime > b->mtime)
      return 1;
    if (a->mtime < b->mtime)
      return -1;
    return 0;
}

static void read_one_dir (char *dirpath, char *dirpath_base,
                          struct Maildir_entry **maildir, int *maildir_size,
                          int *nmsgs) {
    DIR *md;
    struct dirent *de;
    struct stat ms;
    char *cp;

    cp = concat (dirpath_base, dirpath, NULL);
    if ((md = opendir(cp)) == NULL)
	die("unable to open %s", cp);
    while ((de = readdir (md)) != NULL) {
	if (de->d_name[0] == '.')
	    continue;
	if (*nmsgs >= *maildir_size) {
          if ((*maildir = realloc(*maildir, sizeof(**maildir) * (2*(*nmsgs)+16))) == NULL)
            die("not enough memory for %d messages", 2*(*nmsgs)+16);
          *maildir_size = 2*(*nmsgs)+16;
	}
	(*maildir)[*nmsgs].filename = concat (cp, "/", de->d_name, NULL);
	if (stat((*maildir)[*nmsgs].filename, &ms) != 0)
	    adios ((*maildir)[*nmsgs].filename, "couldn't get delivery time");
	(*maildir)[*nmsgs].mtime = ms.st_mtime;
	(*nmsgs)++;
    }
    free (cp);
    closedir (md);
}

/*
 * On return, maildir_out will be NULL or point to memory the caller may free.
 */
void maildir_read_and_sort (char *maildirpath,
			    struct Maildir_entry **maildir_out,
			    int *num_maildir_entries_out) {
    int num_maildir_entries = 0;
    struct Maildir_entry *Maildir = NULL;
    int msize = 0;

    read_one_dir("/new", maildirpath, &Maildir, &msize, &num_maildir_entries);
    read_one_dir("/cur", maildirpath, &Maildir, &msize, &num_maildir_entries);
    if (num_maildir_entries == 0)
	die("no mail to incorporate");
    qsort (Maildir, num_maildir_entries, sizeof(*Maildir), maildir_srt);

    *num_maildir_entries_out = num_maildir_entries;
    *maildir_out = Maildir;
}
