/* rcvtty.c -- a rcvmail program (a lot like rcvalert) handling IPC ttys
 *
 * This code is Copyright (c) 2002, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information.
 */

/* Changed to use getutent() and friends.  Assumes that when getutent() exists,
 * a number of other things also exist.  Please check.
 * Ruud de Rooij <ruud@ruud.org>  Sun, 28 May 2000 17:28:55 +0200
 */

#include "h/mh.h"
#include "sbr/pidwait.h"
#include "sbr/charstring.h"
#include "sbr/fmt_new.h"
#include "scansbr.h"
#include "sbr/getarguments.h"
#include "sbr/smatch.h"
#include "sbr/ambigsw.h"
#include "sbr/print_version.h"
#include "sbr/print_help.h"
#include "sbr/error.h"
#include <signal.h>
#include "sbr/signals.h"
#include <setjmp.h>
#include "sbr/fmt_compile.h"
#include "sbr/fmt_scan.h"
#include "h/tws.h"
#include "sbr/mts.h"
#include "sbr/done.h"
#include "sbr/utils.h"
#include "sbr/m_mktemp.h"
#include "sbr/globals.h"
#include <fcntl.h>

#ifdef HAVE_GETUTXENT
#include <utmpx.h>
#endif /* HAVE_GETUTXENT */

#define	SCANFMT	\
"%2(hour{dtimenow}):%02(min{dtimenow}): %<(size)%5(size) %>%<{encrypted}E%>\
%<(mymbox{from})%<{to}To:%14(friendly{to})%>%>%<(zero)%17(friendly{from})%>  \
%{subject}%<{body}<<%{body}>>%>"

#define RCVTTY_SWITCHES \
    X("biff", 0, BIFFSW) \
    X("form formatfile", 0, FORMSW) \
    X("format string", 5, FMTSW) \
    X("width columns", 0, WIDTHSW) \
    X("newline", 0, NLSW) \
    X("nonewline", 0, NNLSW) \
    X("bell", 0, BELSW) \
    X("nobell", 0, NBELSW) \
    X("version", 0, VERSIONSW) \
    X("help", 0, HELPSW) \

#define X(sw, minchars, id) id,
DEFINE_SWITCH_ENUM(RCVTTY);
#undef X

#define X(sw, minchars, id) { sw, minchars, id },
DEFINE_SWITCH_ARRAY(RCVTTY, switches);
#undef X

static jmp_buf myctx;
static bool bell = true;
static bool newline = true;
static bool biff;
static int width = -1;
static char *form = NULL;
static char *format = NULL;

/*
 * static prototypes
 */
static void alrmser (int);
static int message_fd (char **);
static int header_fd (void);
#if HAVE_GETUTXENT
static void alert (char *, int);
#endif /* HAVE_GETUTXENT */


int
main (int argc, char **argv)
{
    int md, vecp = 0;
    char *cp, *user, buf[BUFSIZ];
    char **argp, **arguments, *vec[MAXARGS];
    struct utmpx *utp;

    if (nmh_init(argv[0], true, false)) { return 1; }

    mts_init ();
    arguments = getarguments (invo_name, argc, argv, 1);
    argp = arguments;

    while ((cp = *argp++)) {
	if (*cp == '-') {
	    switch (smatch (++cp, switches)) {
		case AMBIGSW:
		    ambigsw (cp, switches);
		    done (1);
		case UNKWNSW:
		    vec[vecp++] = --cp;
		    continue;

		case HELPSW:
		    snprintf (buf, sizeof(buf), "%s [command ...]", invo_name);
		    print_help (buf, switches, 1);
		    done (0);
		case VERSIONSW:
		    print_version(invo_name);
		    done (0);

		case BIFFSW:
		    biff = true;
		    continue;

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
		    width = atoi(cp);
		    continue;
                case NLSW:
                    newline = true;
                    continue;
                case NNLSW:
                    newline = false;
                    continue;
                case BELSW:
                    bell = true;
                    continue;
                case NBELSW:
                    bell = false;
                    continue;

	    }
	}
	vec[vecp++] = cp;
    }
    vec[vecp] = 0;

    if ((md = vecp ? message_fd (vec) : header_fd ()) == NOTOK)
	exit(1);

#if HAVE_GETUTXENT
    user = getusername(1);

    setutxent();
    while ((utp = getutxent()) != NULL) {
        if (utp->ut_type == USER_PROCESS && utp->ut_user[0] != 0
               && utp->ut_line[0] != 0
               && strncmp (user, utp->ut_user, sizeof(utp->ut_user)) == 0) {
	    alert(utp->ut_line, md);
        }
    }
    endutxent();
#else
    NMH_UNUSED (utp);
#endif /* HAVE_GETUTXENT */

    exit(0);
}


static void
alrmser (int i)
{
    NMH_UNUSED (i);

    longjmp (myctx, 1);
}


static int
message_fd (char **vec)
{
    pid_t child_id;
    int bytes, seconds;
    int fd;
    char *tfile;
    struct stat st;

    if ((tfile = m_mktemp2(NULL, invo_name, &fd, NULL)) == NULL) {
	inform("unable to create temporary file in %s", get_temp_dir());
	return NOTOK;
    }
    (void) m_unlink(tfile);  /* Use fd, no longer need the file name. */

    if ((child_id = fork()) == -1) {
	/* fork error */
	close (fd);
	return header_fd ();
    }
    if (child_id) {
	/* parent process */
	if (!setjmp (myctx)) {
	    SIGNAL (SIGALRM, alrmser);
	    bytes = fstat(fileno (stdin), &st) != -1 ? (int) st.st_size : 100;

	    /* amount of time to wait depends on message size */
	    if (bytes <= 100) {
		/* give at least 5 minutes */
		seconds = 300;
	    } else if (bytes >= 90000) {
		/* but 30 minutes should be long enough */
		seconds = 1800;
	    } else {
		seconds = (bytes / 60) + 300;
	    }
	    alarm ((unsigned int) seconds);
	    pidwait(child_id, OK);
	    alarm (0);

	    if (fstat (fd, &st) != -1 && st.st_size > 0)
		return fd;
	} else {
	    /*
	     * Ruthlessly kill the child and anything
	     * else in its process group.
	     */
	    killpg(child_id, SIGKILL);
	}
	close (fd);
	return header_fd ();
    }

    /* child process */
    rewind (stdin);
    if (dup2 (fd, 1) == -1 || dup2 (fd, 2) == -1)
	_exit(1);
    setpgid(0, getpid());	/* put in own process group */
    if (execvp (vec[0], vec) == -1) {
        _exit(1);
    }

    return NOTOK;
}


static int
header_fd (void)
{
    int fd;
    char *nfs;
    char *tfile = NULL;
    charstring_t scanl = NULL;

    if ((tfile = m_mktemp2(NULL, invo_name, &fd, NULL)) == NULL) {
	inform("unable to create temporary file in %s", get_temp_dir());
        return NOTOK;
    }
    (void) m_unlink(tfile);  /* Use fd, no longer need the file name. */

    rewind (stdin);

    /* get new format string */
    nfs = new_fs (form, format, SCANFMT);
    scan (stdin, 0, 0, nfs, width, 0, 0, NULL, 0L, 0, &scanl);
    scan_finished ();
    if (newline) {
	if (write (fd, "\n\r", 2) < 0) {
	    advise (tfile, "write LF/CR");
	}
    }
    if (write (fd, charstring_buffer (scanl), charstring_bytes (scanl)) < 0) {
	advise (tfile, "write");
    }
    charstring_free (scanl);
    if (bell) {
        if (write (fd, "\007", 1) < 0) {
	    advise (tfile, "write BEL");
        }
    }

    return fd;
}


#if HAVE_GETUTXENT
static void
alert (char *tty, int md)
{
    int i, td, mask;
    char buffer[BUFSIZ], ttyspec[BUFSIZ];
    struct stat st;

    snprintf (ttyspec, sizeof(ttyspec), "/dev/%s", tty);

    /*
     * The mask depends on whether we are checking for
     * write permission based on `biff' or `mesg'.
     */
    mask = biff ? S_IEXEC : (S_IWRITE >> 3);
    if (stat (ttyspec, &st) == -1 || (st.st_mode & mask) == 0)
	return;

    if (setjmp (myctx)) {
	alarm (0);
	return;
    }
    SIGNAL (SIGALRM, alrmser);
    alarm (2);
    td = open (ttyspec, O_WRONLY);
    alarm (0);
    if (td == -1)
        return;

    lseek(md, 0, SEEK_SET);

    while ((i = read (md, buffer, sizeof(buffer))) > 0)
	if (write (td, buffer, i) != i)
	    break;

    close (td);
}
#endif /* HAVE_GETUTXENT */
