/* seq_print.c -- Routines to print sequence information.
 *
 * This code is Copyright (c) 2002, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information.
 */

#include "h/mh.h"
#include "seq_getnum.h"
#include "seq_print.h"
#include "error.h"
#include "globals.h"


/*
 * Print the sequence membership for the selected messages, for just
 * the given sequence.
 */

int
seq_print_msgs (struct msgs *mp, int i, char *seqname, bool emptyok, bool rangeok)
{
    int msgnum, r;
    int found = 0;

    if (i < 0)
	i = seq_getnum (mp, seqname);  /* may not exist */

    /*
     * Special processing for "cur" sequence.  We assume that the
     * "cur" sequence and mp->curmsg are in sync (see seq_add.c).
     * This is returned, even if message doesn't exist or the
     * folder is empty.
     */
    if (!strcmp (current, seqname)) {
	if (mp->curmsg)
	    printf("%s: %d\n", current, mp->curmsg);
        return 1;
    }

    /*
     * Now look for the sequence bit on all selected messages
     */
    if (i >= 0) {
	for (msgnum = mp->lowsel; msgnum <= mp->hghsel; msgnum++) {
	    if (!is_selected (mp, msgnum) || !in_sequence (mp, i, msgnum))
		continue;

	    if (!found) {
		printf("%s%s:", seqname,
			is_seq_private(mp, i) ? " (private)" : "");
		found = 1;
	    }

	    printf(" %d", msgnum);
	    if (rangeok) {  /* turn "8 9 10 11" into "8-11" */
		r = msgnum;  /* start of range? */
		for (++msgnum; msgnum <= mp->hghsel &&
		    (is_selected (mp, msgnum) && in_sequence (mp, i, msgnum));
			msgnum++)
		    /* loop to end of range */ ;

		if (msgnum - r > 1)
		    printf("-%d", msgnum - 1);
	    }
	}
    }
    if (found) {
	printf("\n");
    } else if (emptyok) {
	printf("%s%s: \n", seqname,
		(i == -1) ? "" : (is_seq_private(mp, i) ? " (private)" : ""));
    }

    return 1;
}
