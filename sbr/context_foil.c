/* context_foil.c -- foil search of profile and context
 *
 * This code is Copyright (c) 2002, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information.
 */

#include "h/mh.h"
#include "error.h"
#include "path.h"
#include "utils.h"
#include "context_foil.h"
#include "globals.h"

/*
 * Foil search of users .mh_profile
 * If error, return -1, else return 0
 */

int
context_foil (char *path)
{
    struct node *np;

    /* In fact, nobody examines defpath in code paths where
     * it's been set by us -- the uses in the source tree are:
     *  1 sbr/context_read.c uses it only after setting it itself
     *  2 uip/install_mh.c uses it only after setting it itself
     *  3 uip/mark.c prints it if given the -debug switch
     * A worthwhile piece of code cleanup would be to make 1 and
     * 2 use a local variable and just delete 3.
     *
     * Similarly, context and ctxpath are not really used
     * outside the context_* routines. It might be worth combining
     * them into one file so the variables can be made static.
     */

    /* We set context to NULL to indicate that no context file
     * is to be read. (Using /dev/null doesn't work because we
     * would try to lock it, which causes timeouts with some
     * locking methods.)
     */
    defpath = context = NULL;

    /*
     * If path is given, create a minimal profile/context list
     */
    if (path) {
	NEW(np);
	m_defs = np;
	if (!(np->n_name = strdup ("Path"))) {
	    inform("strdup failed");
	    return -1;
	}
	if (!(np->n_field = strdup (path))) {
	    inform("strdup failed");
	    return -1;
	}
	np->n_context = 0;
	np->n_next = NULL;

	if (!mypath)
            set_mypath();
    }

    return 0;
}
