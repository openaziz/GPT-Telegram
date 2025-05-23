/* maildir_read_and_sort.h -- returns a sorted list of msgs in a Maildir.
 *
 * This code is Copyright (c) 2020, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information.
 */

struct Maildir_entry {
	char *filename;
	time_t mtime;
};

void maildir_read_and_sort (char *maildirpath,
			    struct Maildir_entry **maildir_out,
			    int *num_msgs_out);
