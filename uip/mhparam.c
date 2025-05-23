/* mhparam.c -- print mh_profile values
 *
 * Originally contributed by
 * Jeffrey C Honig <Jeffrey_C_Honig@cornell.edu>
 *
 * This code is Copyright (c) 2002, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information.
 */

#include "config/version.h"
#include "h/mh.h"
#include "sbr/getarguments.h"
#include "sbr/smatch.h"
#include "sbr/context_find.h"
#include "sbr/ambigsw.h"
#include "sbr/print_version.h"
#include "sbr/print_help.h"
#include "sbr/error.h"
#include "sbr/mts.h"
#include "sbr/done.h"
#include "sbr/utils.h"
#include "sbr/globals.h"

#define MHPARAM_SWITCHES \
    X("components", 0, COMPSW) \
    X("nocomponents", 0, NCOMPSW) \
    X("all", 0, ALLSW) \
    X("version", 0, VERSIONSW) \
    X("help", 0, HELPSW) \
    X("debug", 5, DEBUGSW) \

#define X(sw, minchars, id) id,
DEFINE_SWITCH_ENUM(MHPARAM);
#undef X

#define X(sw, minchars, id) { sw, minchars, id },
DEFINE_SWITCH_ARRAY(MHPARAM, switches);
#undef X

extern char *mhbindir;
extern char *mhlibexecdir;
extern char *mhetcdir;
extern char *mhdocdir;

static char *sbackup = BACKUP_PREFIX;

static char *datalocking = "fcntl";
static char *localmbox = "";
static bool localmbox_primed;

static char *sasl =
#ifdef CYRUS_SASL
    "cyrus_sasl";
#else
    "";
#endif

static char *tls =
#ifdef TLS_SUPPORT
    "tls";
#else
    "";
#endif

static char *mimetypeproc =
#ifdef MIMETYPEPROC
    MIMETYPEPROC;
#else
    "";
#endif

static char *mimeencodingproc =
#ifdef MIMEENCODINGPROC
    MIMEENCODINGPROC;
#else
    "";
#endif

static char *iconv =
#ifdef HAVE_ICONV
    "iconv";
#else
    "";
#endif

static char *oauth =
#ifdef OAUTH_SUPPORT
    "oauth";
#else
    "";
#endif

struct proc {
    char *p_name;
    char **p_field;
};

static struct proc procs [] = {
     { "context",          &context },
     { "mh-sequences",     &mh_seq },
     { "buildmimeproc",    &buildmimeproc },
     { "fileproc",         &fileproc },
     { "foldprot",         &foldprot },
     { "formatproc",	   &formatproc },
     { "incproc",          &incproc },
     { "lproc",            &lproc },
     { "mailproc",         &mailproc },
     { "mhlproc",          &mhlproc },
     { "mimetypeproc",     &mimetypeproc },
     { "mimeencodingproc", &mimeencodingproc },
     { "moreproc",         &moreproc },
     { "msgprot",          &msgprot },
     { "packproc",         &packproc },
     { "postproc",         &postproc },
     { "rmmproc",          &rmmproc },
     { "sendproc",         &sendproc },
     { "showmimeproc",     &showmimeproc },
     { "showproc",         &showproc },
     { "version",          &version_num },
     { "whatnowproc",      &whatnowproc },
     { "whomproc",         &whomproc },
     { "bindir",           &mhbindir },
     { "libexecdir",       &mhlibexecdir },
     { "etcdir",           &mhetcdir },
     { "docdir",           &mhdocdir },
     { "localmbox",	   &localmbox },
     { "sbackup",          &sbackup },
     { "datalocking",      &datalocking },
     { "spoollocking",     &spoollocking },
     { "iconv",		   &iconv },
     { "oauth",		   &oauth },
     { "sasl",             &sasl },
     { "tls",              &tls },
     { NULL,               NULL },
};


/*
 * static prototypes
 */
static char *p_find(char *) PURE;


int
main(int argc, char **argv)
{
    int i, compp = 0;
    bool missed;
    bool all = false;
    bool debug = false;
    int components = -1;
    char *cp, buf[BUFSIZ], **argp;
    char **arguments, *comps[MAXARGS];

    if (nmh_init(argv[0], true, false)) { return 1; }

    arguments = getarguments (invo_name, argc, argv, 1);
    argp = arguments;

    while ((cp = *argp++)) {
	if (*cp == '-') {
	    switch (smatch (++cp, switches)) {
		case AMBIGSW: 
		    ambigsw (cp, switches);
		    done (1);
		case UNKWNSW: 
		    die("-%s unknown", cp);

		case HELPSW: 
		    snprintf (buf, sizeof(buf), "%s [profile-components] [switches]",
			invo_name);
		    print_help (buf, switches, 1);
		    done (0);
		case VERSIONSW:
		    print_version(invo_name);
		    done (0);

		case COMPSW:
		    components = 1;
		    break;
		case NCOMPSW:
		    components = 0;
		    break;

		case ALLSW:
		    all = true;
		    break;

		case DEBUGSW:
		    debug = true;
		    break;
	    }
	} else {
	    comps[compp++] = cp;
	    if (strcmp("localmbox", cp) == 0 && ! localmbox_primed) {
		localmbox = getlocalmbox();
		localmbox_primed = true;
	    }
	}
    }

    if (all) {
        struct node *np;

	if (compp)
	    inform("profile-components ignored with -all");

	if (components >= 0)
	    inform("-%scomponents ignored with -all",
		   components ? "" : "no");

	/* Print all entries in context/profile list.  That does not
	   include entries in mts.conf, such as spoollocking. */
	for (np = m_defs; np; np = np->n_next)
	    printf("%s: %s\n", np->n_name, np->n_field);

    }

    if (debug) {
	struct proc *ps;

	/* In case datalocking was set in profile. */
	if ((cp = context_find("datalocking"))) { datalocking = cp; }

	/* In case spoollocking was set in mts.conf. */
	mts_init();

	/* Also set localmbox here */
	if (! localmbox_primed) {
	    localmbox = getlocalmbox();
	    localmbox_primed = true;
	}

	/*
	 * Print the current value of everything in
	 * procs array.  This will show their current
	 * value (as determined after context is read).
         */
	for (ps = procs; ps->p_name; ps++)
	    printf ("%s: %s\n", ps->p_name, FENDNULL(*ps->p_field));

    }

    missed = false;
    if (! all) {
        if (components < 0)
	    components = compp > 1;

	for (i = 0; i < compp; i++)  {
	    char *value;

	    if (! strcmp ("spoollocking", comps[i])) {
		/* In case spoollocking was set in mts.conf. */
		mts_init();
            }

	    value = context_find (comps[i]);
	    if (!value)
		value = p_find (comps[i]);
	    if (value) {
	        if (components)
		    printf("%s: ", comps[i]);

		puts(value);
	    } else
                missed = true;
	}
    }

    done(missed);
}


static char *
p_find(char *str)
{
    struct proc *ps;

    for (ps = procs; ps->p_name; ps++)
	if (!strcasecmp (ps->p_name, str))
	    return *ps->p_field;

    return NULL;
}
