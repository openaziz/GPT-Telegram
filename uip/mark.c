/* mark.c -- add message(s) to sequences in given folder
 *        -- delete messages (s) from sequences in given folder
 *        -- list sequences in given folder
 *
 * This code is Copyright (c) 2002, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information.
 */

#include "h/mh.h"
#include "sbr/getarguments.h"
#include "sbr/seq_save.h"
#include "sbr/smatch.h"
#include "sbr/snprintb.h"
#include "sbr/m_convert.h"
#include "sbr/getfolder.h"
#include "sbr/folder_read.h"
#include "sbr/folder_free.h"
#include "sbr/context_save.h"
#include "sbr/context_replace.h"
#include "sbr/context_find.h"
#include "sbr/ambigsw.h"
#include "sbr/path.h"
#include "sbr/print_version.h"
#include "sbr/print_help.h"
#include "sbr/seq_bits.h"
#include "sbr/seq_del.h"
#include "sbr/seq_print.h"
#include "sbr/seq_add.h"
#include "sbr/error.h"
#include "sbr/done.h"
#include "sbr/utils.h"
#include "sbr/m_maildir.h"
#include "sbr/globals.h"

#define MARK_SWITCHES \
    X("add", 0, ADDSW) \
    X("delete", 0, DELSW) \
    X("list", 0, LSTSW) \
    X("sequence name", 0, SEQSW) \
    X("public", 0, PUBLSW) \
    X("nopublic", 0, NPUBLSW) \
    X("zero", 0, ZEROSW) \
    X("nozero", 0, NZEROSW) \
    X("empty", 0, EMPTYSW) \
    X("noempty", 0, NEMPTYSW) \
    X("range", 0, RANGESW) \
    X("norange", 0, NRANGESW) \
    X("version", 0, VERSIONSW) \
    X("help", 0, HELPSW) \
    X("debug", -5, DEBUGSW) \

#define X(sw, minchars, id) id,
DEFINE_SWITCH_ENUM(MARK);
#undef X

#define X(sw, minchars, id) { sw, minchars, id },
DEFINE_SWITCH_ARRAY(MARK, switches);
#undef X

/*
 * static prototypes
 */
static void print_debug (struct msgs *);
static void seq_printdebug (struct msgs *);


int
main (int argc, char **argv)
{
    bool addsw = false;
    bool deletesw = false;
    bool debugsw = false;
    bool listsw = false;
    int publicsw = -1;
    bool zerosw = false;
    bool emptysw = true;
    bool rangesw = true;
    int msgnum;
    unsigned int seqp = 0;
    char *cp, *maildir, *folder = NULL, buf[BUFSIZ];
    char **argp, **arguments;
    svector_t seqs = svector_create (0);
    struct msgs_array msgs = { 0, 0, NULL };
    struct msgs *mp;

    if (nmh_init(argv[0], true, true)) { return 1; }

    arguments = getarguments (invo_name, argc, argv, 1);
    argp = arguments;

    /*
     * Parse arguments
     */
    while ((cp = *argp++)) {
	if (*cp == '-') {
	    switch (smatch (++cp, switches)) {
	    case AMBIGSW: 
		ambigsw (cp, switches);
		done (1);
	    case UNKWNSW: 
		die("-%s unknown\n", cp);

	    case HELPSW: 
		snprintf (buf, sizeof(buf), "%s [+folder] [msgs] [switches]",
			  invo_name);
		print_help (buf, switches, 1);
		done (0);
	    case VERSIONSW:
		print_version(invo_name);
		done (0);

	    case ADDSW: 
		addsw = true;
		deletesw = false;
                listsw = false;
		continue;
	    case DELSW: 
		deletesw = true;
		addsw = false;
                listsw = false;
		continue;
	    case LSTSW: 
		listsw = true;
		addsw = false;
                deletesw = false;
		continue;

	    case SEQSW: 
		if (!(cp = *argp++) || *cp == '-')
		    die("missing argument to %s", argp[-2]);

		svector_push_back (seqs, cp);
		seqp++;
		continue;

	    case PUBLSW: 
		publicsw = 1;
		continue;
	    case NPUBLSW: 
		publicsw = 0;
		continue;

	    case DEBUGSW: 
		debugsw = true;
		continue;

	    case ZEROSW: 
		zerosw = true;
		continue;
	    case NZEROSW: 
		zerosw = false;
		continue;

	    case EMPTYSW: 
		emptysw = true;
		continue;
	    case NEMPTYSW: 
		emptysw = false;
		continue;

	    case RANGESW: 
		rangesw = true;
		continue;
	    case NRANGESW: 
		rangesw = false;
		continue;
	    }
	}
	if (*cp == '+' || *cp == '@') {
	    if (folder)
		die("only one folder at a time!");
            folder = pluspath (cp);
	} else
            app_msgarg(&msgs, cp);
    }

    /*
     * If we haven't specified -add, -delete, or -list,
     * then use -add if a sequence was specified, else
     * use -list.
     */
    if (!addsw && !deletesw && !listsw) {
	if (seqp)
	    addsw = true;
	else
	    listsw = true;
    }

    if (!context_find ("path"))
	free (path ("./", TFOLDER));
    if (!msgs.size)
	app_msgarg(&msgs, listsw ? "all" :"cur");
    if (!folder)
	folder = getfolder (1);
    maildir = m_maildir (folder);

    if (chdir (maildir) == -1)
	adios (maildir, "unable to change directory to");

    /* read folder and create message structure */
    if (!(mp = folder_read (folder, 1)))
	die("unable to read folder %s", folder);

    /* print some general debugging info */
    if (debugsw)
	print_debug(mp);

    /* check for empty folder */
    if (mp->nummsg == 0)
	die("no messages in %s", folder);

    /* parse all the message ranges/sequences and set SELECTED */
    for (msgnum = 0; msgnum < msgs.size; msgnum++)
	if (!m_convert (mp, msgs.msgs[msgnum]))
	    done (1);

    if (publicsw == 1 && is_readonly(mp))
	die("folder %s is read-only, so -public not allowed", folder);

    /*
     * Make sure at least one sequence has been
     * specified if we are adding or deleting.
     */
    if (seqp == 0 && (addsw || deletesw))
	die("-%s requires at least one -sequence argument",
	       addsw ? "add" : "delete");

    /* Adding messages to sequences */
    if (addsw) {
	for (seqp = 0; seqp < svector_size (seqs); seqp++)
	    if (!seq_addsel (mp, svector_at (seqs, seqp), publicsw, zerosw))
		done (1);
    }

    /* Deleting messages from sequences */
    if (deletesw) {
	for (seqp = 0; seqp < svector_size (seqs); seqp++)
	    if (!seq_delsel (mp, svector_at (seqs, seqp), publicsw, zerosw))
		done (1);
    }

    /* Listing messages in sequences */
    if (listsw) {
	if (seqp) {
	    for (seqp = 0; seqp < svector_size (seqs); seqp++)
		seq_print_msgs (mp, -1, svector_at (seqs, seqp),
		    emptysw, rangesw);
	} else {
	    size_t i;
	    for (i = 0; i < svector_size (mp->msgattrs); i++)
		seq_print_msgs (mp, i, svector_at (mp->msgattrs, i),
		    emptysw, rangesw);
	}

	/* print debugging info about SELECTED messages */
	if (debugsw)
	    seq_printdebug (mp);
    }

    svector_free (seqs);
    seq_save (mp);			/* synchronize message sequences */
    context_replace (pfolder, folder);	/* update current folder         */
    context_save ();			/* save the context file         */
    folder_free (mp);			/* free folder/message structure */
    done (0);
    return 1;
}


/*
 * Print general debugging info
 */
static void
print_debug (struct msgs *mp)
{
    char buf[100];

    printf ("invo_name     = %s\n", invo_name);
    printf ("mypath        = %s\n", mypath);
    printf ("defpath       = %s\n", defpath);
    printf ("ctxpath       = %s\n", ctxpath);
    printf("context_dirty = %d\n", context_dirty);
    printf ("foldpath      = %s\n", mp->foldpath);
    printf ("folder flags  = %s\n\n", snprintb(buf, sizeof(buf),
		(unsigned) mp->msgflags, FBITS));
    printf ("lowmsg=%d hghmsg=%d nummsg=%d curmsg=%d\n",
	mp->lowmsg, mp->hghmsg, mp->nummsg, mp->curmsg);
    printf ("lowsel=%d hghsel=%d numsel=%d\n",
	mp->lowsel, mp->hghsel, mp->numsel);
    printf ("lowoff=%d hghoff=%d\n\n", mp->lowoff, mp->hghoff);
}


/*
 * Print debugging info about all the SELECTED
 * messages and the sequences they are in.
 * Due to limitations of snprintb(), only a limited
 * number of sequences will be printed.  See the
 * comments in sbr/seq_bits.c.
 */
static void
seq_printdebug (struct msgs *mp)
{
    int msgnum;
    char buf[BUFSIZ];

    putchar('\n');
    for (msgnum = mp->lowsel; msgnum <= mp->hghsel; msgnum++) {
	if (is_selected (mp, msgnum))
	    printf ("%*d: %s\n", DMAXFOLDER, msgnum,
		    snprintb (buf, sizeof buf,
			      (unsigned) bvector_first_bits (msgstat (mp, msgnum)),
			      seq_bits (mp)));
    }
}
