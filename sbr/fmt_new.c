/* fmt_new.c -- read format file/string and normalize
 *
 * This code is Copyright (c) 2002, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information.
 */

#include "h/mh.h"
#include "fmt_new.h"
#include "getcpy.h"
#include "error.h"
#include "utils.h"
#include "path.h"

#define QUOTE '\\'

static char *formats = 0;

/*
 * static prototypes
 */
static void normalize (char *);


/*
 * Get new format string
 */

char *
new_fs (char *form, char *format, char *default_fs)
{
    struct stat st;
    FILE *fp;

    free(formats);

    if (form) {
        char *path = etcpath(form);
        if ((fp = fopen(path, "r")) == NULL)
            adios(path, "unable to open format file");

	if (fstat (fileno (fp), &st) == -1)
            adios(path, "unable to stat format file");

	formats = mh_xmalloc ((size_t) st.st_size + 1);

	if (read (fileno(fp), formats, (int) st.st_size) != st.st_size)
            adios(path, "error reading format file");

	formats[st.st_size] = '\0';

	fclose (fp);
    } else {
	formats = getcpy (format ? format : default_fs);
    }

    normalize (formats);	/* expand escapes */

    return formats;
}


void
free_fs(void)
{
    free (formats);
    formats = 0;
}


/*
 * Expand escapes in format strings
 */

static void
normalize (char *cp)
{
    char *dp;

    for (dp = cp; *cp; cp++) {
	if (*cp != QUOTE) {
	    *dp++ = *cp;
	} else {
	    switch (*++cp) {
		case 'b':
		    *dp++ = '\b';
		    break;

		case 'f':
		    *dp++ = '\f';
		    break;

		case 'n':
		    *dp++ = '\n';
		    break;

		case 'r':
		    *dp++ = '\r';
		    break;

		case 't':
		    *dp++ = '\t';
		    break;

		case '\n':
		    break;

		case 0: 
		    cp--;
		    /* FALLTHRU */
		default: 
		    *dp++ = *cp;
		    break;
	    }
	}
    }
    *dp = '\0';
}
