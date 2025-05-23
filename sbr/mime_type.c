/* mime_type.c -- routine to determine the MIME Content-Type of a file
 *
 * This code is Copyright (c) 2014, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information.
 */

#include "h/mh.h"
#include "concat.h"
#include "readconfig.h"
#include "error.h"
#include "utils.h"
#include "h/tws.h"
#include "mime_type.h"
#include "path.h"
#include "globals.h"

#if defined(MIMETYPEPROC) || defined(MIMEENCODINGPROC)
static char *get_file_info(const char *, const char *);
#endif /* MIMETYPEPROC || MIMEENCODINGPROC */

/*
 * Try to use external command to determine mime type, and possibly
 * encoding.  If that fails try using the filename extension.  Caller
 * is responsible for free'ing returned memory.
 */
char *
mime_type(const char *filename)
{
    char *content_type = NULL;  /* mime content type */
    char *p;

#ifdef MIMETYPEPROC
    char *mimetype;

    if ((mimetype = get_file_info(MIMETYPEPROC, filename))) {
#ifdef MIMEENCODINGPROC
        /* Try to append charset for text content. */
        char *mimeencoding;

        if (!strncasecmp(mimetype, "text", 4) &&
            (mimeencoding = encoding(filename))) {
            content_type = concat(mimetype, "; charset=", mimeencoding, NULL);
            free(mimeencoding);
            free(mimetype);
        } else
            content_type = mimetype;
#else  /* MIMEENCODINGPROC */
        content_type = mimetype;
#endif /* MIMEENCODINGPROC */
    }
#endif /* MIMETYPEPROC */

    /*
     * If we didn't get the MIME type from the contents (or we don't support
     * the necessary command) then use the mhshow suffix.
     */

    if (content_type == NULL) {
	struct node *np;		/* Content scan node pointer */
	FILE *fp;			/* File pointer for mhn.defaults */

        static bool loaded_defaults;
	if (! loaded_defaults &&
			(fp = fopen(p = etcpath("mhn.defaults"), "r"))) {
	    loaded_defaults = true;
	    readconfig(NULL, fp, p, 0);
	    fclose(fp);
	}

	if ((p = strrchr(filename, '.')) != NULL) {
	    for (np = m_defs; np; np = np->n_next) {
		if (strncasecmp(np->n_name, "mhshow-suffix-", 14) == 0 &&
		    strcasecmp(p, FENDNULL(np->n_field)) == 0) {
		    content_type = mh_xstrdup(np->n_name + 14);
		    break;
		}
	    }
	}

	/*
	 * If we didn't match any filename extension, try to infer the
	 * content type. If we have binary, assume application/octet-stream;
	 * otherwise, assume text/plain.
	 */

	if (content_type == NULL) {
	    FILE *fp;
            int c;

	    if (!(fp = fopen(filename, "r"))) {
		inform("unable to access file \"%s\"", filename);
		return NULL;
	    }

	    bool binary = false;
	    while ((c = getc(fp)) != EOF) {
		if (! isascii(c)  ||  c == 0) {
		    binary = true;
		    break;
		}
	    }

	    fclose(fp);

	    content_type =
		mh_xstrdup(binary ? "application/octet-stream" : "text/plain");
	}
    }

    return content_type;
}


char *
encoding(const char *filename)
{
    char *mimeencoding = NULL;

#ifdef MIMEENCODINGPROC
    mimeencoding = get_file_info(MIMEENCODINGPROC, filename);
#else
    NMH_UNUSED(filename);
#endif /* MIMEENCODINGPROC */

    return mimeencoding;
}


#if defined(MIMETYPEPROC) || defined(MIMEENCODINGPROC)
/*
 * Get information using proc about a file.
 * Non-null return value must be free(3)'d. */
static char *
get_file_info(const char *proc, const char *filename)
{
    char *quotec;
    char *cmd;
    FILE *fp;
    bool ok;
    char buf[max(BUFSIZ, 2048)];
    char *info;
    char *needle;

    if (strchr(filename, '\'')) {
        if (strchr(filename, '"')) {
            inform("filenames containing both single and double quotes "
                "are unsupported for attachment");
            return NULL;
        }
        quotec = "\"";
    } else
        quotec = "'";

    cmd = concat(proc, " ", quotec, filename, quotec, NULL);
    if (!cmd) {
        inform("concat with \"%s\" failed, out of memory?", proc);
        return NULL;
    }

    if ((fp = popen(cmd, "r")) == NULL) {
        inform("no output from %s", cmd);
        free(cmd);
        return NULL;
    }

    ok = fgets(buf, sizeof buf, fp);
    free(cmd);
    (void)pclose(fp);
    if (!ok)
        return NULL;

    /* s#^[^:]*:[ \t]*##. */
    info = buf;
    if ((needle = strchr(info, ':'))) {
        info = needle + 1;
        while (isblank((unsigned char)*info))
            info++;
    }

    /* s#[\n\r].*##. */
    if ((needle = strpbrk(info, "\n\r")))
        *needle = '\0';

    return mh_xstrdup(info);
}
#endif /* MIMETYPEPROC || MIMEENCODINGPROC */
