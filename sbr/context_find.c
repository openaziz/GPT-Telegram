/* context_find.c -- find an entry in the context/profile list
 *
 * This code is Copyright (c) 2002, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information.
 */

#include "h/mh.h"
#include "concat.h"
#include "context_find.h"
#include "globals.h"


char *
context_find (const char *str)
{
    struct node *np;

    str = FENDNULL(str);
    for (np = m_defs; np; np = np->n_next)
	if (!strcasecmp(FENDNULL(np->n_name), str))
	    return np->n_field;

    return NULL;
}


/*
 * Helper function to search first, if subtype is non-NULL, for
 * invoname-string-type/subtype and then, if not yet found,
 * invoname-string-type.  If entry is found but is empty, it is
 * treated as not found.
 */
char *
context_find_by_type (const char *string, const char *type,
                      const char *subtype)
{
    char *key, *value;

    if (subtype) {
        key = concat(invo_name, "-", string, "-", type, "/", subtype, NULL);
        value = context_find(key);
        free(key);
        if (value && *value)
            return value;
    }

    key = concat(invo_name, "-", string, "-", type, NULL);
    value = context_find(key);
    free(key);
    if (value && *value)
        return value;

    return NULL;
}


/*
 * Helper function to search profile an entry with name beginning with prefix.
 * The search is case insensitive.
 */
int
context_find_prefix (const char *prefix)
{
    struct node *np;
    size_t len;

    len = strlen(prefix);
    for (np = m_defs; np; np = np->n_next) {
	if (np->n_name  &&  ! strncasecmp (np->n_name, prefix, len)) {
	    return 1;
        }
    }

    return 0;
}
