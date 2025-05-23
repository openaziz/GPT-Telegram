/* sendsbr.c -- routines to help WhatNow/Send along
 *
 * This code is Copyright (c) 2002, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information.
 */

#include "h/mh.h"
#include "sbr/pidwait.h"
#include "sbr/charstring.h"
#include "sbr/fmt_new.h"
#include "sendsbr.h"
#include "distsbr.h"
#include "annosbr.h"
#include "sbr/m_name.h"
#include "sbr/m_getfld.h"
#include "sbr/concat.h"
#include "sbr/cpydgst.h"
#include "sbr/trimcpy.h"
#include "sbr/uprf.h"
#include "sbr/getcpy.h"
#include "sbr/m_convert.h"
#include "sbr/m_backup.h"
#include "sbr/folder_read.h"
#include "sbr/folder_free.h"
#include "sbr/context_find.h"
#include "sbr/brkstring.h"
#include "sbr/pidstatus.h"
#include "sbr/utils.h"
#include "sbr/arglist.h"
#include "sbr/error.h"
#include "sbr/addrsbr.h"
#include "sbr/fmt_compile.h"
#include "sbr/fmt_scan.h"
#include <signal.h>
#include "sbr/signals.h"
#include <setjmp.h>
#include <fcntl.h>
#include "aliasbr.h"
#include "h/mime.h"
#include "h/tws.h"
#include "sbr/mts.h"

#ifdef HAVE_SYS_TIME_H
# include <sys/time.h>
#endif
#include <time.h>

#ifdef OAUTH_SUPPORT
#include "sbr/oauth.h"
#include "sbr/oauth_prof.h"
#endif
#include "sbr/done.h"
#include "sbr/m_maildir.h"
#include "sbr/m_mktemp.h"
#include "sbr/message_id.h"
#include "sbr/globals.h"

static int expand_alias(const char *, const char **addr, const char **host, const char **);
#ifdef OAUTH_SUPPORT
static int setup_oauth_params(char *[], int *, const char *, const char **);
#endif /* OAUTH_SUPPORT */

bool debugsw;		/* global */
bool forwsw = true;
int inplace = 1;
bool pushsw;
bool unique;
bool verbsw;

char *altmsg   = NULL;		/*  .. */
char *annotext = NULL;
char *distfile = NULL;

static jmp_buf env;

/*
 * static prototypes
 */
static void alert (char *, int);
static int tmp_fd (void);
static void anno (int, struct stat *);
static void annoaux (int);
static int sendaux (char **, int, char *, char *, struct stat *);
static void handle_sendfrom(char **, int *, char *, const char *);
static int get_from_header_info(const char *, const char **, const char **, const char **);
static const char *get_message_header_info(FILE *, char *);
static void merge_profile_entry(const char *, const char *, char *[], int *);
static void armed_done (int) NORETURN;

/*
 * Entry point into (back-end) routines to send message.
 */

int
sendsbr (char **vec, int vecp, char *program, char *draft, struct stat *st,
         int rename_drft, const char *auth_svc)
{
    int status, i;
    pid_t child;
    char buffer[BUFSIZ], file[BUFSIZ];
    char **buildvec, *buildprogram;
    char *volatile drft = draft;
    /* nvecs is volatile to prevent warning from gcc about possible clobbering
       by longjmp. */
    volatile int nvecs = vecp;
    int *nvecsp = (int *) &nvecs;

    /*
     * Run the mimebuildproc (which is by default mhbuild) on the message
     * with the addition of the "-auto" flag
     */

    switch (child = fork()) {
    case NOTOK:
	adios("fork", "unable to");
	break;

    case OK:
        buildvec = argsplit(buildmimeproc, &buildprogram, &i);
	buildvec[i++] = "-auto";
	if (distfile)
	    buildvec[i++] = "-dist";
	buildvec[i++] = (char *) drft;
	buildvec[i] = NULL;
	execvp(buildprogram, buildvec);
	fprintf(stderr, "unable to exec ");
	perror(buildmimeproc);
	_exit(1);
	break;

    default:
	if (pidXwait(child, buildmimeproc))
	    return NOTOK;
	break;
    }

    set_done(armed_done);
    switch (setjmp (env)) {
    case OK:
	/*
	 * If given -push and -unique (which is undocumented), then
	 * rename the draft file.  I'm not quite sure why.
	 */
	if (pushsw && unique) {
	    char *cp = m_mktemp2(drft, invo_name, NULL, NULL);
	    if (cp == NULL) {
		die("unable to create temporary file");
	    }
	    if (rename (drft, strncpy(file, cp, sizeof(file) - 1)) ==
		NOTOK)
		adios (file, "unable to rename %s to", drft);
	    drft = file;
	}

	/*
	 * Add in any necessary profile entries for xoauth
	 */

	if (auth_svc) {
#ifdef OAUTH_SUPPORT
		const char *errmsg;
		if (setup_oauth_params(vec, nvecsp, auth_svc, &errmsg) != OK) {
                        die("%s", errmsg);
		}
#else
                die("send built without OAUTH_SUPPORT, "
                      "so auth_svc %s is not supported", auth_svc);
#endif /* OAUTH_SUPPORT */
	}

        /*
         * Rework the vec based on From: header in draft, as specified
         * by sendfrom-address entries in profile.
         */
        if (context_find_prefix("sendfrom-")) {
            handle_sendfrom(vec, nvecsp, draft, auth_svc);
        }

	status = sendaux (vec, nvecs, program, drft, st) ? NOTOK : OK;

	/* rename the original draft */
	if (rename_drft && status == OK &&
		rename (drft, strncpy (buffer, m_backup (drft),
				 sizeof(buffer) - 1)) == -1)
	    advise (buffer, "unable to rename %s to", drft);
	break;

    default:
	status = DONE;
	break;
    }

    set_done(exit);
    if (distfile)
	(void) m_unlink (distfile);

    return status;
}


/*
 * Annotate original message, and
 * call `postproc' (which is passed down in "program") to send message.
 */

static int
sendaux (char **vec, int vecp, char *program, char *drft, struct stat *st)
{
    pid_t child_id;
    int status, fd, fd2;
    char backup[BUFSIZ], buf[BUFSIZ];

    fd = pushsw ? tmp_fd () : NOTOK;
    fd2 = NOTOK;

    if (annotext) {
	if ((fd2 = tmp_fd ()) != NOTOK) {
	    vec[vecp++] = "-idanno";
	    snprintf (buf, sizeof(buf), "%d", fd2);
	    vec[vecp++] = buf;
	} else {
	    inform("unable to create temporary file in %s for "
		"annotation list, continuing...", get_temp_dir());
	}
    }
    vec[vecp++] = drft;
    if (distfile && distout (drft, distfile, backup) == NOTOK)
	done (1);
    vec[vecp] = NULL;

    child_id = fork();
    switch (child_id) {
    case -1:
	/* oops -- fork error */
	adios ("fork", "unable to");
	break;	/* NOT REACHED */

    case 0:
	/*
	 * child process -- send it
	 *
	 * If fd is OK, then we are pushing and fd points to temp
	 * file, so capture anything on stdout and stderr there.
	 */
	if (fd != NOTOK) {
	    dup2 (fd, fileno (stdout));
	    dup2 (fd, fileno (stderr));
	    close (fd);
	}
	execvp (program, vec);
	fprintf (stderr, "unable to exec ");
	perror (postproc);
	_exit(1);

    default:
	/*
	 * parent process -- wait for it
	 */
	if ((status = pidwait(child_id, NOTOK)) == OK) {
	    if (annotext && fd2 != NOTOK)
		anno (fd2, st);
	} else {
	    /*
	     * If postproc failed, and we have good fd (which means
	     * we pushed), then mail error message (and possibly the
	     * draft) back to the user.
	     */
	    if (fd != NOTOK) {
		alert (drft, fd);
		close (fd);
	    } else {
		inform("message not delivered to anyone");
	    }
	    if (annotext && fd2 != NOTOK)
		close (fd2);
	    if (distfile) {
		(void) m_unlink (drft);
		if (rename (backup, drft) == -1)
		    advise (drft, "unable to rename %s to", backup);
	    }
	}
	break;
    }

    return status;
}


/*
 * Mail error notification (and possibly a copy of the
 * message) back to the user, using the mailproc
 */

static void
alert (char *file, int out)
{
    pid_t child_id;
    int in, argp;
    char buf[BUFSIZ];
    char *program;
    char **arglist;

    child_id = fork();
    switch (child_id) {
	case NOTOK:
	    /* oops -- fork error */
	    advise ("fork", "unable to");
	    /* FALLTHRU */

	case OK:
	    /* child process -- send it */
	    SIGNAL (SIGHUP, SIG_IGN);
	    SIGNAL (SIGINT, SIG_IGN);
	    SIGNAL (SIGQUIT, SIG_IGN);
	    SIGNAL (SIGTERM, SIG_IGN);
	    if (forwsw) {
		if ((in = open (file, O_RDONLY)) == -1) {
		    admonish (file, "unable to re-open");
		} else {
		    lseek(out, 0, SEEK_END);
		    strncpy (buf, "\nMessage not delivered to anyone.\n", sizeof(buf));
		    if (write (out, buf, strlen (buf)) < 0) {
			advise (file, "write");
		    }
		    strncpy (buf, "\n------- Unsent Draft\n\n", sizeof(buf));
		    if (write (out, buf, strlen (buf)) < 0) {
			advise (file, "write");
		    }
		    cpydgst (in, out, file, "temporary file");
		    close (in);
		    strncpy (buf, "\n------- End of Unsent Draft\n", sizeof(buf));
		    if (write (out, buf, strlen (buf)) < 0) {
			advise (file, "write");
		    }
		    if (rename (file, strncpy (buf, m_backup (file),
					       sizeof(buf) - 1)) == -1)
			admonish (buf, "unable to rename %s to", file);
		}
	    }
	    lseek(out, 0, SEEK_SET);
	    dup2 (out, fileno (stdin));
	    close (out);
	    /* create subject for error notification */
	    snprintf (buf, sizeof(buf), "send failed on %s",
			forwsw ? "enclosed draft" : file);

	    arglist = argsplit(mailproc, &program, &argp);

	    arglist[argp++] = getusername(1);
	    arglist[argp++] = "-subject";
	    arglist[argp++] = buf;
	    arglist[argp] = NULL;

	    execvp (program, arglist);
	    fprintf (stderr, "unable to exec ");
	    perror (mailproc);
	    _exit(1);

	default:		/* no waiting... */
	    break;
    }
}


static int
tmp_fd (void)
{
    int fd;
    char *tfile;

    if ((tfile = m_mktemp2(NULL, invo_name, &fd, NULL)) == NULL) return NOTOK;

    if (debugsw)
	inform("temporary file %s selected", tfile);
    else if (m_unlink (tfile) == NOTOK)
        advise (tfile, "unable to remove");

    return fd;
}


static void
anno (int fd, struct stat *st)
{
    pid_t child_id;
    sigset_t set, oset;
    static char *cwd = NULL;
    struct stat st2;

    if (altmsg &&
	    (stat (altmsg, &st2) == -1
		|| st->st_mtime != st2.st_mtime
		|| st->st_dev != st2.st_dev
		|| st->st_ino != st2.st_ino)) {
	if (debugsw)
	    inform("$mhaltmsg mismatch, continuing...");
	return;
    }

    child_id = debugsw ? NOTOK : fork ();
    switch (child_id) {
	case NOTOK:		/* oops */
	    if (!debugsw)
		inform("unable to fork, so doing annotations by hand...");
	    if (cwd == NULL)
		cwd = mh_xstrdup(pwd ());
	    /* FALLTHRU */

	case OK:
	    /* block a few signals */
	    sigemptyset (&set);
	    sigaddset (&set, SIGHUP);
	    sigaddset (&set, SIGINT);
	    sigaddset (&set, SIGQUIT);
	    sigaddset (&set, SIGTERM);
	    sigprocmask (SIG_BLOCK, &set, &oset);

	    unregister_for_removal(0);

	    annoaux (fd);
	    if (child_id == OK)
		_exit (0);

	    /* reset the signal mask */
	    sigprocmask (SIG_SETMASK, &oset, &set);

	    if (chdir (cwd) < 0) {
		advise (cwd, "chdir");
	    }
	    break;

	default:		/* no waiting... */
	    close (fd);
	    break;
    }
}


static void
annoaux (int fd)
{
    int	fd2, fd3, msgnum;
    char *cp, *folder, *maildir;
    char buffer[BUFSIZ], **ap;
    FILE *fp;
    struct msgs *mp;

    if ((folder = getenv ("mhfolder")) == NULL || *folder == 0) {
	if (debugsw)
	    inform("$mhfolder not set, continuing...");
	return;
    }
    maildir = m_maildir (folder);
    if (chdir (maildir) == -1) {
	if (debugsw)
	    admonish (maildir, "unable to change directory to");
	return;
    }
    if (!(mp = folder_read (folder, 0))) {
	if (debugsw)
	    inform("unable to read folder %s, continuing...", folder);
	return;
    }

    /* check for empty folder */
    if (mp->nummsg == 0) {
	if (debugsw)
	    inform("no messages in %s, continuing...", folder);
	goto oops;
    }

    if ((cp = getenv ("mhmessages")) == NULL || *cp == 0) {
	if (debugsw)
	    inform("$mhmessages not set, continuing...");
	goto oops;
    }
    if (!debugsw			/* MOBY HACK... */
	    && pushsw
	    && (fd3 = open ("/dev/null", O_RDWR)) != -1
	    && (fd2 = dup (fileno (stderr))) != -1) {
	dup2 (fd3, fileno (stderr));
	close (fd3);
    } else
	fd2 = -1;
    for (ap = brkstring (cp = mh_xstrdup(cp), " ", NULL); *ap; ap++)
	m_convert (mp, *ap);
    free (cp);
    if (fd2 != -1)
	dup2 (fd2, fileno (stderr));
    if (mp->numsel == 0) {
	if (debugsw)
	    inform("no messages to annotate, continuing...");
	goto oops;
    }

    lseek(fd, 0, SEEK_SET);
    if ((fp = fdopen (fd, "r")) == NULL) {
	if (debugsw)
	    inform("unable to fdopen annotation list, continuing...");
	goto oops;
    }
    cp = NULL;
    while (fgets (buffer, sizeof(buffer), fp) != NULL)
	cp = add (buffer, cp);
    fclose (fp);

    if (debugsw)
	inform("annotate%s with %s: \"%s\"",
		inplace ? " inplace" : "", annotext, cp);
    for (msgnum = mp->lowsel; msgnum <= mp->hghsel; msgnum++) {
	if (is_selected(mp, msgnum)) {
	    if (debugsw)
		inform("annotate message %d", msgnum);
            annotate (m_name (msgnum), annotext, cp, inplace, 1, -2, 0);
	}
    }

    free (cp);

oops:
    folder_free (mp);	/* free folder/message structure */
}


static void
handle_sendfrom(char **vec, int *vecp, char *draft, const char *auth_svc)
{
    const char *addr = NULL, *host = NULL;
    const char *message;

    /* Extract address and host from From: header line in draft. */
    if (get_from_header_info(draft, &addr, &host, &message) != OK) {
        if (addr  &&  strchr(addr, '@')) {
            free((void *) host);
            die("%s for %s", message, addr);
        } else {
            /* Unable to extract address and host.  Try interpreting the
               address as an alias. */
            char *alias = mh_xstrdup(FENDNULL(addr));
            free((void *) addr);
            free((void *) host);
            if (expand_alias(alias, &addr, &host, &message)) {
                die("%s %s", message, alias);
            }

            if (! strcmp(alias, addr)) {
                /* The address was not an alias that we ere able to expand. */
                free(alias);
                die("%s for %s", message, addr);
            }
            free(alias);
        }
    }

    /* Merge in any address or host specific switches to post(1) from profile. */
    merge_profile_entry(addr, host, vec, vecp);
    free((void *) host);
    free((void *) addr);

    vec[*vecp] = NULL;

    {
        char **vp;

        for (vp = vec; *vp; ++vp) {
            if (strcmp(*vp, "xoauth2") == 0) {
#ifdef OAUTH_SUPPORT
                if (setup_oauth_params(vec, vecp, auth_svc, &message) != OK) {
                    die("%s", message);
                }
                break;
#else
                NMH_UNUSED(auth_svc);
                die("send built without OAUTH_SUPPORT, "
                      "so -saslmech xoauth2 is not supported");
#endif /* OAUTH_SUPPORT */
            }
        }
    }
}


/* Return address that an alias expands to, along with the host part. */
static
int
expand_alias(const char *an_alias, const char **addr, const char **host, const char **message)
{
    struct mailname *mp = NULL;
    int count = 0;
    char error[BUFSIZ], *cp;

    /*
     * check for "Aliasfile:" profile entry
     */
    if ((cp = context_find ("Aliasfile"))) {
        char *dp = NULL, **ap;

        for (ap = brkstring(dp = mh_xstrdup(cp), " ", "\n"); ap && *ap; ap++) {
            alias(*ap);
        }
        free(dp);
    }

    *addr = akvalue ((char *) an_alias);  /* do mh alias substitution */
    /* There should only be one From: alias.  But call getname() as usual
       until it returns NULL to reset its internal state. */
    while ((cp = getname (*addr))) {
        if (!(mp = getm (cp, NULL, 0, error, sizeof(error)))) {
            admonish(cp, "%s", error);
            continue;
        }
        if (mp->m_host) {
            *host = mh_xstrdup(mp->m_host);
            ++count;
            mnfree(mp);
        } else {
            *message = "unable to expand alias";
            mnfree(mp);
            return NOTOK;
        }
    }

    *message = "only one sender address allowed";
    return count == 1 ? OK : NOTOK;
}


#ifdef OAUTH_SUPPORT
/*
 * For XOAUTH2, append profile entries so post can do the heavy lifting
 */
static int
setup_oauth_params(char *vec[], int *vecp, const char *auth_svc,
		   const char **message)
{
    const char *saslmech = NULL, *user = NULL;
    mh_oauth_service_info svc;
    char errbuf[256];
    int i;

    /* Make sure we have all the information we need. */
    for (i = 1; i < *vecp; ++i) {
        /* Don't support abbreviated switches, to avoid collisions in the
           future if new ones are added. */
        if (! strcmp(vec[i-1], "-saslmech")) {
            saslmech = vec[i];
        } else if (! strcmp(vec[i-1], "-user")) {
            user = vec[i];
        } else if (! strcmp(vec[i-1], "-authservice")) {
            auth_svc = vec[i];
        }
    }

    if (auth_svc == NULL) {
        if (saslmech  &&  ! strcasecmp(saslmech, "xoauth2")) {
            *message = "must specify -authservice with -saslmech xoauth2";
            return NOTOK;
        }
    } else {
        if (user == NULL) {
            *message = "must specify -user with -saslmech xoauth2";
            return NOTOK;
        }

        if (saslmech  &&  ! strcasecmp(saslmech, "xoauth2")) {
	    if (! mh_oauth_get_service_info(auth_svc, &svc, errbuf,
					    sizeof(errbuf)))
		die("Unable to retrieve oauth profile entries: %s",
		      errbuf);

	    vec[(*vecp)++] = mh_xstrdup("-authservice");
	    vec[(*vecp)++] = mh_xstrdup(auth_svc);
	    vec[(*vecp)++] = mh_xstrdup("-oauthcredfile");
	    vec[(*vecp)++] = mh_oauth_cred_fn(auth_svc);
	    vec[(*vecp)++] = mh_xstrdup("-oauthclientid");
	    vec[(*vecp)++] = getcpy(svc.client_id);
	    vec[(*vecp)++] = mh_xstrdup("-oauthclientsecret");
	    vec[(*vecp)++] = getcpy(svc.client_secret);
	    vec[(*vecp)++] = mh_xstrdup("-oauthauthendpoint");
	    vec[(*vecp)++] = getcpy(svc.auth_endpoint);
	    vec[(*vecp)++] = mh_xstrdup("-oauthredirect");
	    vec[(*vecp)++] = getcpy(svc.redirect_uri);
	    vec[(*vecp)++] = mh_xstrdup("-oauthtokenendpoint");
	    vec[(*vecp)++] = getcpy(svc.token_endpoint);
	    vec[(*vecp)++] = mh_xstrdup("-oauthscope");
	    vec[(*vecp)++] = getcpy(svc.scope);
        }
    }

    return 0;
}
#endif /* OAUTH_SUPPORT */


/*
 * Extract user and domain from From: header line in draft.
 */
static int
get_from_header_info(const char *filename, const char **addr, const char **host, const char **message)
{
    struct stat st;
    FILE *in;

    if (stat (filename, &st) == -1) {
        *message = "unable to stat draft file";
        return NOTOK;
    }

    if ((in = fopen (filename, "r")) != NULL) {
        /* There must be a non-blank Envelope-From or {Resent-}Sender or
           {Resent-}From header. */
        char *addrformat = "%(addr{Envelope-From})";
        char *hostformat = "%(host{Envelope-From})";

        if ((*addr = get_message_header_info (in, addrformat)) == NULL  ||
            !**addr) {
            addrformat = distfile == NULL  ?  "%(addr{Sender})"  :  "%(addr{Resent-Sender})";
            hostformat = distfile == NULL  ?  "%(host{Sender})"  :  "%(host{Resent-Sender})";

            if ((*addr = get_message_header_info (in, addrformat)) == NULL) {
                addrformat = distfile == NULL  ?  "%(addr{From})"  :  "%(addr{Resent-From})";
                hostformat = distfile == NULL  ?  "%(host{From})"  :  "%(host{Resent-From})";

                if ((*addr = get_message_header_info (in, addrformat)) == NULL) {
                    *message = "unable to find sender address in";
                    fclose(in);
                    return NOTOK;
                }
            }
        }

        /* Use the hostformat that corresponds to the successful addrformat. */
        if ((*host = get_message_header_info(in, hostformat)) == NULL) {
            *message = "unable to find sender host";
            fclose(in);
            return NOTOK;
        }
        fclose(in);

        return OK;
    }

    *message = "unable to open";
    return NOTOK;
}


/*
 * Get formatted information from header of a message.
 * Adapted from process_single_file() in uip/fmttest.c.
 */
static const char *
get_message_header_info(FILE *in, char *format)
{
    int dat[5];
    struct format *fmt;
    struct stat st;
    bool parsing_header;
    m_getfld_state_t gstate;
    charstring_t buffer = charstring_create(0);
    char *retval;

    dat[0] = dat[1] = dat[4] = 0;
    dat[2] = fstat(fileno(in), &st) == 0  ?  st.st_size  :  0;
    dat[3] = INT_MAX;

    (void) fmt_compile(new_fs(NULL, NULL, format), &fmt, 1);
    free_fs();

    /*
     * Read in the message and process the header.
     */
    rewind (in);
    parsing_header = true;
    gstate = m_getfld_state_init(in);
    do {
        char name[NAMESZ], rbuf[NMH_BUFSIZ];
        int bufsz = sizeof rbuf;
        int state = m_getfld2(&gstate, name, rbuf, &bufsz);

        switch (state) {
        case FLD:
        case FLDPLUS: {
            int bucket = fmt_addcomptext(name, rbuf);

            if (bucket != -1) {
                while (state == FLDPLUS) {
                    bufsz = sizeof rbuf;
                    state = m_getfld2(&gstate, name, rbuf, &bufsz);
                    fmt_appendcomp(bucket, name, rbuf);
                }
            }

            while (state == FLDPLUS) {
                bufsz = sizeof rbuf;
                state = m_getfld2(&gstate, name, rbuf, &bufsz);
            }
            break;
        }
        default:
            parsing_header = false;
        }
    } while (parsing_header);
    m_getfld_state_destroy(&gstate);

    fmt_scan(fmt, buffer, INT_MAX, dat, NULL);
    fmt_free(fmt, 1);

    /* Trim trailing newline, if any. */
    retval = rtrim(charstring_buffer_copy((buffer)));
    charstring_free(buffer);
    if (*retval)
        return retval;

    free(retval);
    return NULL;
}


/*
 * Look in profile for entry corresponding to addr or host, and add its contents to vec.
 *
 * Could do some of this automatically, by looking for:
 * 1) access-$(mbox{from}) in oauth-svc file using mh_oauth_cred_load(), which isn't
 *    static and doesn't have side effects; free the result with mh_oauth_cred_free())
 * 2) machine $(mbox{from}) in creds
 * If no -server passed in from profile or commandline, could use smtp.<svc>.com for gmail,
 * but that might not generalize for other svcs.
 */
static void
merge_profile_entry(const char *addr, const char *host, char *vec[], int *vecp)
{
    char *addr_entry = concat("sendfrom-", addr, NULL);
    char *profile_entry = context_find(addr_entry);

    free(addr_entry);
    if (profile_entry == NULL) {
        /* No entry for the user.  Look for one for the host. */
        char *host_entry = concat("sendfrom-", host, NULL);

        profile_entry = context_find(host_entry);
        free(host_entry);
    }

    /* Use argsplit() to do the real work of splitting the args in the profile entry. */
    if (profile_entry && *profile_entry) {
        int profile_vecp;
        char *file;
        char **profile_vec = argsplit(profile_entry, &file, &profile_vecp);
        int i;

        for (i = 0; i < profile_vecp; ++i) {
            vec[(*vecp)++] = getcpy(profile_vec[i]);
        }

        arglist_free(file, profile_vec);
    }
}


static void NORETURN
armed_done (int status)
{
    longjmp (env, status ? status : NOTOK);
}
