/* seq_setprev.c -- set the Previous-Sequence
 *
 * This code is Copyright (c) 2002, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information.
 */

#include "h/mh.h"
#include "seq_setprev.h"
#include "context_find.h"
#include "brkstring.h"
#include "seq_add.h"
#include "utils.h"
#include "globals.h"

/*
 * Add all the messages currently SELECTED to
 * the Previous-Sequence.  This way, when the next
 * command is given, there is a convenient way to
 * selected all the messages used in the previous
 * command.
 */

void
seq_setprev (struct msgs *mp)
{
    char **ap, *cp, *dp;

    /*
     * Get the list of sequences for Previous-Sequence
     * and split them.
     */
    if (!(cp = context_find(psequence)))
        return;
    dp = mh_xstrdup(cp);
    if (!(ap = brkstring(dp, " ", "\n")) || !*ap) {
        free(dp);
        return;
    }

    /* Now add all SELECTED messages to each sequence */
    for (; *ap; ap++)
	seq_addsel (mp, *ap, -1, 1);

    free (dp);
}
