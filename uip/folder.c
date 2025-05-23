/* folder.c -- set/list the current message and/or folder
 *             -- push/pop a folder onto/from the folder stack
 *             -- list the folder stack
 *
 * This code is Copyright (c) 2002, 2008, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information.
 */

#include "h/mh.h"
#include "sbr/getarguments.h"
#include "sbr/concat.h"
#include "sbr/seq_setprev.h"
#include "sbr/seq_setcur.h"
#include "sbr/seq_save.h"
#include "sbr/smatch.h"
#include "sbr/getcpy.h"
#include "sbr/m_convert.h"
#include "sbr/getfolder.h"
#include "sbr/folder_read.h"
#include "sbr/folder_pack.h"
#include "sbr/folder_free.h"
#include "sbr/context_save.h"
#include "sbr/context_replace.h"
#include "sbr/context_del.h"
#include "sbr/context_find.h"
#include "sbr/brkstring.h"
#include "sbr/ambigsw.h"
#include "sbr/path.h"
#include "sbr/print_version.h"
#include "sbr/print_help.h"
#include "sbr/error.h"
#include "sbr/crawl_folders.h"
#include "sbr/done.h"
#include "sbr/utils.h"
#include "sbr/m_maildir.h"
#include "sbr/globals.h"

#define FOLDER_SWITCHES \
    X("all", 0, ALLSW) \
    X("noall", 0, NALLSW) \
    X("create", 0, CREATSW) \
    X("nocreate", 0, NCREATSW) \
    X("fast", 0, FASTSW) \
    X("nofast", 0, NFASTSW) \
    X("header", 0, HDRSW) \
    X("noheader", 0, NHDRSW) \
    X("pack", 0, PACKSW) \
    X("nopack", 0, NPACKSW) \
    X("verbose", 0, VERBSW) \
    X("noverbose", 0, NVERBSW) \
    X("recurse", 0, RECURSW) \
    X("norecurse", 0, NRECRSW) \
    X("total", 0, TOTALSW) \
    X("nototal", 0, NTOTLSW) \
    X("list", 0, LISTSW) \
    X("nolist", 0, NLISTSW) \
    X("print", 0, PRNTSW) \
    X("noprint", 0, NPRNTSW) \
    X("push", 0, PUSHSW) \
    X("pop", 0, POPSW) \
    X("version", 0, VERSIONSW) \
    X("help", 0, HELPSW) \

#define X(sw, minchars, id) id,
DEFINE_SWITCH_ENUM(FOLDER);
#undef X

#define X(sw, minchars, id) { sw, minchars, id },
DEFINE_SWITCH_ARRAY(FOLDER, switches);
#undef X

static bool fshort;	        /* output only folder names                 */
static int fcreat   = 0;	/* should we ask to create new folders?     */
static bool fpack;		/* are we packing the folder?               */
static bool fverb;		/* print actions taken while packing folder */
static int fheader  = 0;	/* should we output a header?               */
static bool frecurse;		/* recurse through subfolders               */
static int ftotal   = 0;	/* should we output the totals?             */
static bool all;		/* should we output all folders             */

static int total_folders = 0;	/* total number of folders                  */

static char *nmhdir;
static char *stack = "Folder-Stack";
static char folder[BUFSIZ];

/*
 * Structure to hold information about
 * folders as we scan them.
 */
struct FolderInfo {
    char *name;
    int nummsg;
    int curmsg;
    int lowmsg;
    int hghmsg;
    int others;		/* others == 1 if other files in folder */
    int error;		/* error == 1 for unreadable folder     */
};

/*
 * Dynamically allocated space to hold
 * all the folder information.
 */
static struct FolderInfo *fi;
static int maxFolderInfo;

/*
 * static prototypes
 */
static int get_folder_info (char *, char *);
static crawl_callback_t get_folder_info_callback;
static void print_folders (void);
static int sfold (struct msgs *, char *);
static void readonly_folders (void);

/*
 * Function for printing error message if folder does not exist with
 * -nocreate.
 */
static void
nonexistent_folder (int status)
{
    NMH_UNUSED (status);
    die("folder %s does not exist", folder);
}


int
main (int argc, char **argv)
{
    int printsw = -1;
    bool listsw = false;
    bool pushsw = false;
    bool popsw = false;
    char *cp, *dp, *msg = NULL, *argfolder = NULL;
    char **ap, **argp, buf[BUFSIZ], **arguments;

    if (nmh_init(argv[0], true, true)) { return 1; }

    /*
     * If program was invoked with name ending
     * in `s', then add switch `-all'.
     */
    all = has_suffix_c(argv[0], 's');

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
		    snprintf (buf, sizeof(buf), "%s [+folder] [msg] [switches]",
			invo_name);
		    print_help (buf, switches, 1);
		    done (0);
		case VERSIONSW:
		    print_version(invo_name);
		    done (0);

		case ALLSW: 
		    all = true;
		    continue;

		case NALLSW:
		    all = false;
		    continue;

		case CREATSW: 
		    fcreat = 1;
		    continue;
		case NCREATSW: 
		    fcreat = -1;
		    continue;

		case FASTSW: 
		    fshort = true;
		    continue;
		case NFASTSW: 
		    fshort = false;
		    continue;

		case HDRSW: 
		    fheader = 1;
		    continue;
		case NHDRSW: 
		    fheader = -1;
		    continue;

		case PACKSW: 
		    fpack = true;
		    continue;
		case NPACKSW: 
		    fpack = false;
		    continue;

		case VERBSW:
		    fverb = true;
		    continue;
		case NVERBSW:
		    fverb = false;
		    continue;

		case RECURSW: 
		    frecurse = true;
		    continue;
		case NRECRSW: 
		    frecurse = false;
		    continue;

		case TOTALSW: 
		    ftotal = 1;
		    continue;
		case NTOTLSW: 
		    ftotal = -1;
		    continue;

		case PRNTSW: 
		    printsw = 1;
		    continue;
		case NPRNTSW: 
		    printsw = 0;
		    continue;

		case LISTSW: 
		    listsw = true;
		    continue;
		case NLISTSW: 
		    listsw = false;
		    continue;

		case PUSHSW: 
		    pushsw = true;
		    listsw = true;
		    popsw  = false;
		    continue;
		case POPSW: 
		    popsw  = true;
		    listsw = true;
		    pushsw = false;
		    continue;
	    }
	}
	if (*cp == '+' || *cp == '@') {
	    if (argfolder)
		die("only one folder at a time!");
            argfolder = pluspath (cp);
	} else {
	    if (msg)
		die("only one (current) message at a time!");
            msg = cp;
	}
    }

    if (!context_find ("path"))
	free (path ("./", TFOLDER));
    nmhdir = concat (m_maildir (""), "/", NULL);

    /*
     * If not directed via -print/-noprint, we print folder summary
     * info unless if we're working with the folder stack (i.e.,
     * -push, -pop, or -list).
     */
    if (printsw == -1) {
	printsw = !(pushsw || popsw || listsw);
    }

    /* Pushing a folder onto the folder stack */
    if (pushsw) {
	if (!argfolder) {
	    /* If no folder is given, the current folder and */
	    /* the top of the folder stack are swapped.      */
	    if ((cp = context_find (stack))) {
		dp = mh_xstrdup(cp);
		ap = brkstring (dp, " ", "\n");
		argfolder = getcpy(*ap++);
	    } else {
		die("no other folder");
	    }
	    for (cp = mh_xstrdup(getfolder(1)); *ap; ap++)
		cp = add (*ap, add (" ", cp));
	    free (dp);
	    context_replace (stack, cp);	/* update folder stack */
	} else {
	    /* update folder stack */
	    context_replace (stack,
		    (cp = context_find (stack))
		    ? concat (getfolder (1), " ", cp, NULL)
		    : mh_xstrdup(getfolder(1)));
	}
    }

    /* Popping a folder off of the folder stack */
    if (popsw) {
	if (argfolder)
	    die("sorry, no folders allowed with -pop");
	if ((cp = context_find (stack))) {
	    dp = mh_xstrdup(cp);
	    ap = brkstring (dp, " ", "\n");
	    argfolder = getcpy(*ap++);
	} else {
	    die("folder stack empty");
	}
	if (*ap) {
	    /* if there's anything left in the stack */
	    cp = getcpy (*ap++);
	    for (; *ap; ap++)
		cp = add (*ap, add (" ", cp));
	    context_replace (stack, cp);	/* update folder stack */
	} else {
	    context_del (stack);	/* delete folder stack entry from context */
	}
	free (dp);
    }
    if (pushsw || popsw) {
	cp = m_maildir(argfolder);
	if (access (cp, F_OK) == -1)
	    adios (cp, "unable to find folder");
	context_replace (pfolder, argfolder);	/* update current folder   */
	context_save ();		/* save the context file   */
	argfolder = NULL;
    }

    /* Listing the folder stack */
    if (listsw) {
	fputs(argfolder ? argfolder : getfolder (1), stdout);
	if ((cp = context_find (stack))) {
	    dp = mh_xstrdup(cp);
	    for (ap = brkstring (dp, " ", "\n"); *ap; ap++)
		printf (" %s", *ap);
	    free (dp);
	}
	putchar('\n');

	if (!printsw)
	    done (0);
    }

    /* Allocate initial space to record folder information */
    maxFolderInfo = CRAWL_NUMFOLDERS;
    fi = mh_xmalloc (maxFolderInfo * sizeof(*fi));

    /*
     * Scan the folders
     */
    /* change directory to base of nmh directory for crawl_folders */
    if (chdir (nmhdir) == -1)
	adios (nmhdir, "unable to change directory to");
    if (all || ftotal > 0) {
	/*
	 * If no folder is given, do them all
	 */
	if (!argfolder) {
	    if (msg)
		inform("no folder given for message %s, continuing...", msg);
	    readonly_folders (); /* do any readonly folders */
	    cp = context_find(pfolder);
	    strncpy (folder, FENDNULL(cp), sizeof(folder) - 1);
	    crawl_folders (".", get_folder_info_callback, NULL);
	} else {
	    strncpy (folder, argfolder, sizeof(folder) - 1);
	    if (get_folder_info (argfolder, msg)) {
		context_replace (pfolder, argfolder);/* update current folder */
		context_save ();		     /* save the context file */
	    }
	    /*
	     * Since recurse wasn't done in get_folder_info(),
	     * we still need to list all level-1 sub-folders.
	     */
	    if (!frecurse)
		crawl_folders (folder, get_folder_info_callback, NULL);
	}
    } else {
	strncpy (folder, argfolder ? argfolder : getfolder (1),
		 sizeof(folder) - 1);

	/*
	 * Check if folder exists.  If not, then see if
	 * we should create it, or just exit.
	 */
        create_folder (m_maildir (folder), fcreat, nonexistent_folder);

	if (get_folder_info (folder, msg) && argfolder) {
	    /* update current folder */
	    context_replace (pfolder, argfolder);
	    }
    }

    /*
     * Print out folder information
     */
    if (printsw)
	print_folders();

    context_save ();	/* save the context file */
    done (0);
    return 1;
}

static int
get_folder_info_body (char *fold, char *msg, bool *crawl_children)
{
    int	i, retval = 1;
    struct msgs *mp = NULL;

    i = total_folders++;

    /*
     * if necessary, reallocate the space
     * for folder information
     */
    if (total_folders >= maxFolderInfo) {
	maxFolderInfo += CRAWL_NUMFOLDERS;
	fi = mh_xrealloc (fi, maxFolderInfo * sizeof(*fi));
    }

    fi[i].name   = fold;
    fi[i].nummsg = 0;
    fi[i].curmsg = 0;
    fi[i].lowmsg = 0;
    fi[i].hghmsg = 0;
    fi[i].others = 0;
    fi[i].error  = 0;

    if ((ftotal > 0) || !fshort || msg || fpack) {
	/*
	 * create message structure and get folder info
	 */
	if (!(mp = folder_read (fold, fpack))) {
	    inform("unable to read folder %s, continuing...", fold);
	    *crawl_children = false;
	    return 0;
	}

	/* set the current message */
	if (msg && !sfold (mp, msg))
	    retval = 0;

	if (fpack) {
	    if (folder_pack (&mp, fverb) == -1) {
		*crawl_children = false; /* to please clang static analyzer */
		done (1);
	    }
	    seq_save (mp);		/* synchronize the sequences */
	    context_save ();	/* save the context file     */
	}

	/* record info for this folder */
	if ((ftotal > 0) || !fshort) {
	    fi[i].nummsg = mp->nummsg;
	    fi[i].curmsg = mp->curmsg;
	    fi[i].lowmsg = mp->lowmsg;
	    fi[i].hghmsg = mp->hghmsg;
	    fi[i].others = other_files (mp);
	}

	folder_free (mp); /* free folder/message structure */
    }

    *crawl_children = (frecurse && (fshort || fi[i].others)
		       && (fi[i].error == 0));
    return retval;
}

static bool
get_folder_info_callback (char *fold, void *baton)
{
    bool crawl_children;
    NMH_UNUSED (baton);

    get_folder_info_body (fold, NULL, &crawl_children);
    fflush (stdout);
    return crawl_children;
}

static int
get_folder_info (char *fold, char *msg)
{
    bool crawl_children;
    int retval;

    retval = get_folder_info_body (fold, msg, &crawl_children);

    if (crawl_children) {
	crawl_folders (fold, get_folder_info_callback, NULL);
    }

    return retval;
}

/*
 * Print folder information
 */

static void
print_folders (void)
{
    int i, len;
    bool hasempty = false;
    bool curprinted;
    int maxlen = 0, maxnummsg = 0, maxlowmsg = 0;
    int maxhghmsg = 0, maxcurmsg = 0, total_msgs = 0;
    int nummsgdigits, lowmsgdigits;
    int hghmsgdigits, curmsgdigits;

    /*
     * compute a few values needed to for
     * printing various fields
     */
    for (i = 0; i < total_folders; i++) {
	/* length of folder name */
	len = strlen (fi[i].name);
	if (len > maxlen)
	    maxlen = len;

	/* If folder has error, skip the rest */
	if (fi[i].error)
	    continue;

	/* calculate total number of messages */
	total_msgs += fi[i].nummsg;

	/* maximum number of messages */
	if (fi[i].nummsg > maxnummsg)
	    maxnummsg = fi[i].nummsg;

	/* maximum low message */
	if (fi[i].lowmsg > maxlowmsg)
	    maxlowmsg = fi[i].lowmsg;

	/* maximum high message */
	if (fi[i].hghmsg > maxhghmsg)
	    maxhghmsg = fi[i].hghmsg;

	/* maximum current message */
	if (fi[i].curmsg >= fi[i].lowmsg &&
	    fi[i].curmsg <= fi[i].hghmsg &&
	    fi[i].curmsg > maxcurmsg)
	    maxcurmsg = fi[i].curmsg;

	/* check for empty folders */
	if (fi[i].nummsg == 0)
	    hasempty = true;
    }
    nummsgdigits = num_digits (maxnummsg);
    lowmsgdigits = num_digits (maxlowmsg);
    hghmsgdigits = num_digits (maxhghmsg);
    curmsgdigits = num_digits (maxcurmsg);

    if (hasempty && nummsgdigits < 2)
	nummsgdigits = 2;

    /*
     * Print the header
     */
    if (fheader > 0 || (all && !fshort && fheader >= 0))
	printf ("%-*s %*s %-*s; %-*s %*s\n",
		maxlen+1, "FOLDER",
		nummsgdigits + 13, "# MESSAGES",
		lowmsgdigits + hghmsgdigits + 4, " RANGE",
		curmsgdigits + 4, "CUR",
		9, "(OTHERS)");

    /*
     * Print folder information
     */
    if (all || fshort || ftotal < 1) {
	for (i = 0; i < total_folders; i++) {
	    char *name = fi[i].name;

	    if (fshort) {
		puts(name);
		continue;
	    }

	    if (!strcmp(name, folder)) {
		int spaces = maxlen - strlen(name);
		printf("%s+%*s ", name, spaces, "");
	    } else
		printf("%-*s  ", maxlen, name);

	    if (fi[i].error) {
                puts("is unreadable");
		continue;
	    }

	    curprinted = false; /* remember if we print cur */
	    if (fi[i].nummsg == 0) {
		printf ("has %*s messages%*s",
			nummsgdigits, "no",
			fi[i].others ? lowmsgdigits + hghmsgdigits + 5 : 0, "");
	    } else {
		printf ("has %*d message%1s  (%*d-%*d)",
			nummsgdigits, fi[i].nummsg,
			PLURALS(fi[i].nummsg),
			lowmsgdigits, fi[i].lowmsg,
			hghmsgdigits, fi[i].hghmsg);
		if (fi[i].curmsg >= fi[i].lowmsg && fi[i].curmsg <= fi[i].hghmsg) {
		    curprinted = true;
		    printf ("; cur=%*d", curmsgdigits, fi[i].curmsg);
		}
	    }

	    if (fi[i].others)
		printf (";%*s (others)", curprinted ? 0 : curmsgdigits + 6, "");
	    puts(".");
	}
    }

    /*
     * Print folder/message totals
     */
    if (ftotal > 0 || (all && !fshort && ftotal >= 0)) {
	if (all)
	    putchar('\n');
	printf ("TOTAL = %d message%s in %d folder%s.\n",
		total_msgs, PLURALS(total_msgs),
		total_folders, PLURALS(total_folders));
    }

    fflush (stdout);
}

/*
 * Set the current message and synchronize sequences
 */

static int
sfold (struct msgs *mp, char *msg)
{
    /* parse the message range/sequence/name and set SELECTED */
    if (!m_convert (mp, msg))
	return 0;

    if (mp->numsel > 1) {
	inform("only one message at a time!, continuing...");
	return 0;
    }
    seq_setprev (mp);		/* set the previous-sequence     */
    seq_setcur (mp, mp->lowsel);/* set current message           */
    seq_save (mp);		/* synchronize message sequences */
    context_save ();		/* save the context file         */

    return 1;
}


/*
 * Do the read only folders
 */

static void
readonly_folders (void)
{
    int	atrlen;
    char atrcur[BUFSIZ];
    struct node *np;

    snprintf (atrcur, sizeof(atrcur), "atr-%s-", current);
    atrlen = strlen (atrcur);

    for (np = m_defs; np; np = np->n_next)
	if (has_prefix(np->n_name, atrcur)
		&& !has_prefix(np->n_name + atrlen, nmhdir))
	    get_folder_info (np->n_name + atrlen, NULL);
}
