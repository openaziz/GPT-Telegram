/* path.c -- return a pathname
 *
 * This code is Copyright (c) 2002, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information.
 */

#include "h/mh.h"
#include <pwd.h>
#include "error.h"
#include "getfolder.h"
#include "path.h"
#include "utils.h"
#include "m_maildir.h"
#include "context_read.h"
#include "globals.h"

#define	CWD	"./"
#define	DOT	"."
#define	DOTDOT	".."
#define	PWD	"../"

static char *pwds;

/*
 * static prototypes
 */
static char *expath(char *,int);
static void compath(char *);


/* set_mypath sets the global mypath to the HOME environment variable if
 * it is set and not empty, or else the password entry's home-directory
 * field if it's found and not empty.  Otherwise, the program exits
 * with an error.  The value found is copied with mh_xstrdup() as later
 * library calls may invalidate returned values. */
void
set_mypath(void)
{
    char *var = getenv("HOME");

    if (var && *var) {
        mypath = mh_xstrdup(var);
        return;
    }

    errno = 0;
    struct passwd *pw = getpwuid(getuid());
    if (!pw) {
        if (errno)
            adios("", "getpwuid() failed");   /* "" prints errno! */
        die("password entry not found");
    }
    if (!*pw->pw_dir)
        die("password entry has empty home directory");

    mypath = mh_xstrdup(pw->pw_dir);
}


/* Return value must be free(3)'d. */
char *
pluspath(char *name)
{
	return path(name + 1, *name == '+' ? TFOLDER : TSUBCWF);
}


/* Return value must be free(3)'d. */
char *
path(char *name, int flag)
{
    char *p, *last;

    p = expath(name, flag);
    last = p + strlen(p) - 1;
    if (last > p && *last == '/')
	*last = '\0';

    return p;
}


/* Return value must be free(3)'d. */
static char *
expath (char *name, int flag)
{
    char *cp, *ep;
    char buffer[BUFSIZ];

    if (flag == TSUBCWF) {
	snprintf (buffer, sizeof(buffer), "%s/%s", getfolder (1), name);
	name = m_mailpath (buffer);
	compath (name);
	snprintf (buffer, sizeof(buffer), "%s/", m_maildir (""));
	if (has_prefix(name, buffer)) {
	    cp = name;
	    name = mh_xstrdup(name + strlen(buffer));
	    free (cp);
	}
	flag = TFOLDER;
    }

    if (*name == '/'
	    || (flag == TFOLDER
		&& (!has_prefix(name, CWD)
		    && strcmp (name, DOT)
		    && strcmp (name, DOTDOT)
		    && !has_prefix(name, PWD))))
	return mh_xstrdup(name);

    if (pwds == NULL)
	pwds = pwd ();

    if (strcmp (name, DOT) == 0 || strcmp (name, CWD) == 0)
	return mh_xstrdup(pwds);

    ep = pwds + strlen (pwds);
    if ((cp = strrchr(pwds, '/')) == NULL)
	cp = ep;
    else if (cp == pwds)
        cp++;

    if (has_prefix(name, CWD))
	name += LEN(CWD);

    if (strcmp (name, DOTDOT) == 0 || strcmp (name, PWD) == 0) {
	snprintf (buffer, sizeof(buffer), "%.*s", (int)(cp - pwds), pwds);
	return mh_xstrdup(buffer);
    }

    if (has_prefix(name, PWD))
	name += LEN(PWD);
    else
	cp = ep;

    snprintf (buffer, sizeof(buffer), "%.*s/%s", (int)(cp - pwds), pwds, name);
    return mh_xstrdup(buffer);
}


static void
compath (char *f)
{
    char *cp, *dp;

    if (*f != '/')
	return;

    for (cp = f; *cp;) {
	if (*cp != '/') {
	    cp++;
            continue;
        }

        switch (*++cp) {
            case 0:
                if (--cp > f)
                    *cp = '\0';
                return;

            case '/':
                for (dp = cp; *dp == '/'; dp++)
                    continue;
                strcpy (cp--, dp);
                continue;

            case '.':
                if (strcmp (cp, DOT) == 0) {
                    if (cp > f + 1)
                        cp--;
                    *cp = '\0';
                    return;
                }
                if (strcmp (cp, DOTDOT) == 0) {
                    for (cp -= 2; cp > f; cp--)
                        if (*cp == '/')
                            break;
                    if (cp <= f)
                        cp = f + 1;
                    *cp = '\0';
                    return;
                }
                if (has_prefix(cp, PWD)) {
                    for (dp = cp - 2; dp > f; dp--)
                        if (*dp == '/')
                            break;
                    if (dp <= f)
                        dp = f;
                    strcpy (dp, cp + LEN(PWD) - 1);
                    cp = dp;
                    continue;
                }
                if (has_prefix(cp, CWD)) {
                    strcpy (cp - 1, cp + LEN(CWD) - 1);
                    cp--;
                    continue;
                }
                continue;

            default:
                cp++;
                continue;
        }
    }
}


/*
 * Find the location of a format or configuration
 * file, and return its absolute pathname.
 *
 * etcpath() does get pass string constants, so don't modify
 * the argument.
 *
 * 1) If already absolute pathname, then leave unchanged.
 * 2) Next, if it begins with ~user, then expand it.
 * 3) Next, check in nmh Mail directory.
 * 4) Next, check in nmh `etc' directory.
 *
 * Does not return NULL.
 */
char *
etcpath (char *file)
{
    static char epath[PATH_MAX];
    char *dir = NULL;
    char *base = NULL;
    char *cp;
    int count;

    context_read();

    switch (*file) {
        case '/':
            /* If already absolute pathname, return it */
            return file;

        case '~': {
            /* Expand ~username */
            bool unknown_user = false;
            char *pp;

            if ((cp = strchr(pp = file + 1, '/'))) {
                dir = strdup(pp);
                dir[cp-pp] = '\0';
                base = strdup(cp + 1);
            }

            if (file[1] == '/') {
                pp = mypath;
            } else {
                struct passwd *pw;
                if (dir && (pw = getpwnam (dir))) {
                    pp = pw->pw_dir;
                } else {
                    unknown_user = true;
                }
            }

            if (! unknown_user) {
                count = snprintf (epath, sizeof(epath), "%s/%s", pp, FENDNULL(base));
                if ((size_t) count >= sizeof(epath)) {
                    inform("etcpath(%s) overflow, continuing", file);
                    goto failed;
                }
                if (access (epath, R_OK) != -1) {
                    goto succeeded;
                }
            }
        }
            /* FALLTHRU */
        default:
            /* Check nmh Mail directory */
            cp = m_mailpath(file);
            size_t need = strlen(cp) + 1;
            if (need > sizeof epath) {
                inform ("etcpath(%s) overflow when checking Mail directory, continuing", cp);
                free (cp);
                goto failed;
            }
            memcpy(epath, cp, need);
            free (cp);
            if (access (epath, R_OK) != -1) {
                goto succeeded;
            }
    }

    /* Check nmh `etc' directory */
    count = snprintf (epath, sizeof(epath), NMHETCDIR "/%s", file);
    if ((size_t) count >= sizeof(epath)) {
        inform ("etcpath(%s/%s) overflow when checking etc directory, continuing",
                NMHETCDIR, file);
        goto failed;
    } else if (access (epath, R_OK) == -1) {
        goto failed;
    }
succeeded:
    free(base);
    free(dir);
    return epath;
failed:
    free(base);
    free(dir);
    return file;
}
