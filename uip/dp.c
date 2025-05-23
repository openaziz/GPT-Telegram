/* dp.c -- parse dates 822-style
 *
 * This code is Copyright (c) 2002, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information.
 */

#include "h/mh.h"
#include "sbr/charstring.h"
#include "sbr/fmt_new.h"
#include "sbr/getarguments.h"
#include "sbr/smatch.h"
#include "sbr/context_save.h"
#include "sbr/ambigsw.h"
#include "sbr/print_version.h"
#include "sbr/print_help.h"
#include "sbr/error.h"
#include "sbr/done.h"
#include "sbr/utils.h"
#include "sbr/fmt_compile.h"
#include "sbr/fmt_scan.h"
#include "h/tws.h"
#include "sbr/terminal.h"
#include "sbr/globals.h"

#define	NDATES 100

#define	WIDTH 78

#define	FORMAT "%<(nodate{text})error: %{text}%|%(putstr(pretty{text}))%>"

#define DP_SWITCHES \
    X("form formatfile", 0, FORMSW) \
    X("format string", 5, FMTSW) \
    X("width columns", 0, WIDTHSW) \
    X("version", 0, VERSIONSW) \
    X("help", 0, HELPSW) \

#define X(sw, minchars, id) id,
DEFINE_SWITCH_ENUM(DP);
#undef X

#define X(sw, minchars, id) { sw, minchars, id },
DEFINE_SWITCH_ARRAY(DP, switches);
#undef X

static struct format *fmt;

static int dat[5];

/*
 * static prototypes
 */
static int process (char *, int);


int
main (int argc, char **argv)
{
    int datep = 0, width = -1, status = 0;
    char *cp, *form = NULL, *format = NULL, *nfs;
    char buf[BUFSIZ], **argp;
    char *dates[NDATES + 1]; /* Includes terminating NULL. */

    if (nmh_init(argv[0], true, false)) { return 1; }

    argp = getarguments (invo_name, argc, argv, 1);
    while ((cp = *argp++)) {
	if (*cp == '-') {
	    switch (smatch (++cp, switches)) {
		case AMBIGSW: 
		    ambigsw (cp, switches);
		    done (1);
		case UNKWNSW: 
		    die("-%s unknown", cp);

		case HELPSW: 
		    snprintf (buf, sizeof(buf), "%s [switches] dates ...",
			invo_name);
		    print_help (buf, switches, 1);
		    done (0);
		case VERSIONSW:
		    print_version(invo_name);
		    done (0);

		case FORMSW: 
		    if (!(form = *argp++) || *form == '-')
			die("missing argument to %s", argp[-2]);
		    format = NULL;
		    continue;
		case FMTSW: 
		    if (!(format = *argp++) || *format == '-')
			die("missing argument to %s", argp[-2]);
		    form = NULL;
		    continue;

		case WIDTHSW: 
		    if (!(cp = *argp++) || *cp == '-')
			die("missing argument to %s", argp[-2]);
		    width = atoi (cp);
		    continue;
	    }
	}
	if (datep == NDATES)
	    die("more than %d dates", NDATES);
        dates[datep++] = cp;
    }
    dates[datep] = NULL;

    if (datep == 0)
	die("usage: %s [switches] dates ...", invo_name);

    /* get new format string */
    nfs = new_fs (form, format, FORMAT);

    if (width == -1) {
	if ((width = sc_width ()) < WIDTH / 2) {
	    /* Default:  width of the terminal, but at least WIDTH/2. */
	    width = WIDTH / 2;
	}
	width -= 2;
    } else if (width == 0) {
	/* Unlimited width.  */
	width = INT_MAX;
    }
    fmt_compile (nfs, &fmt, 1);

    dat[0] = 0;
    dat[1] = 0;
    dat[2] = 0;
    dat[3] = width;
    dat[4] = 0;

    for (datep = 0; dates[datep]; datep++)
	status += process (dates[datep], width);

    context_save ();	/* save the context file */
    fmt_free (fmt, 1);
    done(!!status);
    return 1;
}


static int
process (char *date, int length)
{
    int status = 0;
    charstring_t scanl =
	charstring_create (length < NMH_BUFSIZ ? length : NMH_BUFSIZ);
    struct comp *cptr;

    cptr = fmt_findcomp ("text");
    if (cptr) {
        free(cptr->c_text);
	cptr->c_text = mh_xstrdup(date);
    }
    fmt_scan (fmt, scanl, length, dat, NULL);
    fputs (charstring_buffer (scanl), stdout);
    charstring_free (scanl);

    return status;
}
