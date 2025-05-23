/* rcvpack.c -- append message to a file
 *
 * This code is Copyright (c) 2002, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information.
 */

#include "h/mh.h"
#include "sbr/m_gmprot.h"
#include "sbr/getarguments.h"
#include "sbr/smatch.h"
#include "sbr/ambigsw.h"
#include "sbr/print_version.h"
#include "sbr/print_help.h"
#include "sbr/error.h"
#include "dropsbr.h"
#include "h/tws.h"
#include "sbr/mts.h"
#include "sbr/done.h"
#include "sbr/utils.h"
#include "sbr/globals.h"

#define RCVPACK_SWITCHES \
    X("mbox", 0, MBOXSW) \
    X("mmdf", 0, MMDFSW) \
    X("version", 0, VERSIONSW) \
    X("help", 0, HELPSW) \

#define X(sw, minchars, id) id,
DEFINE_SWITCH_ENUM(RCVPACK);
#undef X

#define X(sw, minchars, id) { sw, minchars, id },
DEFINE_SWITCH_ARRAY(RCVPACK, switches);
#undef X

/*
 * default format in which to save messages
 */
static int mbx_style = MBOX_FORMAT;


int
main (int argc, char **argv)
{
    int md;
    char *cp, *file = NULL, buf[BUFSIZ];
    char **argp, **arguments;

    if (nmh_init(argv[0], true, false)) { return 1; }

    mts_init ();
    arguments = getarguments (invo_name, argc, argv, 1);
    argp = arguments;

    /* parse arguments */
    while ((cp = *argp++)) {
	if (*cp == '-') {
	    switch (smatch (++cp, switches)) {
		case AMBIGSW: 
		    ambigsw (cp, switches);
		    done (1);
		case UNKWNSW: 
		    die("-%s unknown", cp);

		case HELPSW: 
		    snprintf (buf, sizeof(buf), "%s [switches] file", invo_name);
		    print_help (buf, switches, 1);
		    done (0);
		case VERSIONSW:
		    print_version(invo_name);
		    done (0);

		case MBOXSW:
		    mbx_style = MBOX_FORMAT;
		    continue;
		case MMDFSW:
		    mbx_style = MMDF_FORMAT;
		    continue;
	    }
	}
	if (file)
	    die("only one file at a time!");
        file = cp;
    }

    if (!file)
	die("%s [switches] file", invo_name);

    rewind (stdin);

    /* open and lock the file */
    if ((md = mbx_open (file, mbx_style, getuid(), getgid(), m_gmprot())) == NOTOK)
	done(1);

    /* append the message */
    if (mbx_copy (file, mbx_style, md, fileno(stdin), NULL) == NOTOK) {
	mbx_close (file, md);
	done(1);
    }

    /* close and unlock the file */
    if (mbx_close (file, md) == NOTOK)
	done(1);

    done(0);
    return 1;
}
