/* context_replace.c -- add/replace an entry in the context/profile list
 *
 * This code is Copyright (c) 2002, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information.
 */

#include "h/mh.h"
#include "getcpy.h"
#include "context_replace.h"
#include "error.h"
#include "utils.h"
#include "globals.h"


void
context_replace (char *key, char *value)
{
    struct node *np;

    key = FENDNULL(key);

    /*
     * If list is empty, allocate head of profile/context list.
     */
    if (!m_defs) {
	NEW(np);
        m_defs = np;
	np->n_name = mh_xstrdup(key);
	np->n_field = getcpy (value);
	np->n_context = 1;
	np->n_next = NULL;
        context_dirty = true;
	return;
    }

    /*
     * Search list of context/profile entries for
     * this key, and replace its value if found.
     */
    for (np = m_defs;; np = np->n_next) {
	if (!strcasecmp(FENDNULL(np->n_name), key)) {
	    if (strcmp (value, np->n_field)) {
		if (!np->n_context)
		    inform("bug: context_replace(key=\"%s\",value=\"%s\"), continuing...", key, value);
                free(np->n_field);
		np->n_field = mh_xstrdup(value);
                context_dirty = true;
	    }
	    return;
	}
	if (!np->n_next)
	    break;
    }

    /*
     * Else add this new entry at the end
     */
    NEW(np->n_next);
    np = np->n_next;
    np->n_name = mh_xstrdup(key);
    np->n_field = getcpy (value);
    np->n_context = 1;
    np->n_next = NULL;
    context_dirty = true;
}
