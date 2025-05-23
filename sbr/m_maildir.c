/* m_maildir.c -- get the path for the mail directory
 *
 * This code is Copyright (c) 2002, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information.
 */

#include "h/mh.h"
#include "getfolder.h"
#include "context_find.h"
#include "path.h"
#include "utils.h"
#include "m_maildir.h"
#include "globals.h"

#define	CWD	"./"
#define	DOT	"."
#define	DOTDOT	".."
#define	PWD	"../"

static char mailfold[BUFSIZ];

/*
 * static prototypes
 */
static char *exmaildir (char *);


/* Returns static char[], never NULL. */
char *
m_maildir (char *folder)
{
    char *cp, *ep;

    if ((cp = exmaildir (folder))
	    && (ep = cp + strlen (cp) - 1) > cp
	    && *ep == '/')
	*ep = '\0';

    return cp;
}


/* Return value must be free(3)'d. */
char *
m_mailpath (char *folder)
{
    char *cp;
    char maildir[BUFSIZ];

    if (*folder != '/'
	    && !has_prefix(folder, CWD)
	    && strcmp (folder, DOT)
	    && strcmp (folder, DOTDOT)
	    && !has_prefix(folder, PWD)) {
	strncpy (maildir, mailfold, sizeof(maildir));	/* preserve... */
	cp = mh_xstrdup(m_maildir(folder));
	strncpy (mailfold, maildir, sizeof(mailfold));
    } else {
	cp = path (folder, TFOLDER);
    }

    return cp;
}


/* Returns static char[], never NULL. */
static char *
exmaildir (char *folder)
{
    char *cp, *pp;

    /* use current folder if none is specified */
    if (folder == NULL)
	folder = getfolder(1);

    if (!(*folder != '/'
	    && !has_prefix(folder, CWD)
	    && strcmp (folder, DOT)
	    && strcmp (folder, DOTDOT)
	    && !has_prefix(folder, PWD))) {
	strncpy (mailfold, folder, sizeof(mailfold) - 1);
	return mailfold;
    }

    cp = mailfold;
    if ((pp = context_find ("path")) && *pp) {
	if (*pp != '/') {
	    snprintf(cp, sizeof mailfold, "%s/", mypath);
	    cp += strlen (cp);
	}
	cp = stpcpy(cp, pp);
    } else {
        char *p = path("./", TFOLDER);
	cp = stpcpy(cp, p);
        free(p);
    }
    if (cp[-1] != '/')
	*cp++ = '/';
    strcpy (cp, folder);

    return mailfold;
}
