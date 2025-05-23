/* dropsbr.c -- create/read/manipulate mail drops
 *
 * This code is Copyright (c) 2002, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information.
 */

#include "h/nmh.h"
#include "h/mh.h"
#include "sbr/dtime.h"
#include "sbr/concat.h"
#include "sbr/stringdex.h"
#include "sbr/error.h"
#include "sbr/utils.h"
#include "dropsbr.h"
#include "sbr/mts.h"
#include "h/tws.h"
#include "sbr/lock_file.h"
#include "sbr/m_mktemp.h"
#include <fcntl.h>

/*
 * static prototypes
 */
static int mbx_chk_mbox (int);
static int mbx_chk_mmdf (int);


/*
 * Main entry point to open/create and lock
 * a file or maildrop.
 */

int
mbx_open (char *file, int mbx_style, uid_t uid, gid_t gid, mode_t mode)
{
    int j, count, fd = NOTOK;
    struct stat st;

    j = 0;

    /* attempt to open and lock file */
    for (count = 4; count > 0; count--) {
        int failed_to_lock = 0;

	if ((fd = lkopenspool (file, O_RDWR | O_CREAT | O_NONBLOCK,
            mode, &failed_to_lock)) != NOTOK)
            break;

        if (!failed_to_lock)
            return NOTOK;

        j = errno;
        sleep (5);
    }

    errno = j;

    /*
     * Return if we still failed after 4 attempts,
     * or we just want to skip the sanity checks.
     */
    if (fd == NOTOK || mbx_style == OTHER_FORMAT)
	return fd;

    /*
     * Do sanity checks on maildrop.
     */
    if (fstat (fd, &st) == -1) {
	/*
	 * The stat failed.  So we make sure file
	 * has right ownership/modes
	 */
	if (chown (file, uid, gid) < 0) {
	    advise (file, "chown");
	}
	if (chmod (file, mode) < 0) {
	    advise (file, "chmod");
	}
    } else if (st.st_size > 0) {
	int status;

	/* check the maildrop */
	switch (mbx_style) {
	    case MMDF_FORMAT: 
	    default: 
		status = mbx_chk_mmdf (fd);
		break;

	    case MBOX_FORMAT: 
		status = mbx_chk_mbox (fd);
		break;
	}

	/* if error, attempt to close it */
	if (status == NOTOK) {
	    close (fd);
	    return NOTOK;
	}
    }

    return fd;
}


/*
 * Check/prepare MBOX style maildrop for appending.
 */

static int
mbx_chk_mbox (int fd)
{
    /* just seek to the end */
    if (lseek(fd, 0, SEEK_END) == (off_t) -1)
	return NOTOK;

    return OK;
}


/*
 * Check/prepare MMDF style maildrop for appending.
 */

static int
mbx_chk_mmdf (int fd)
{
    ssize_t count;
    char ldelim[BUFSIZ];

    count = LEN(MMDF_DELIM);

    if (lseek (fd, -count, SEEK_END) == (off_t) -1)
	return NOTOK;
    if (read (fd, ldelim, count) != count)
	return NOTOK;

    ldelim[count] = 0;

    if (strcmp (ldelim, MMDF_DELIM)
	    && write (fd, "\n", 1) != 1
	    && write (fd, MMDF_DELIM, count) != count)
	return NOTOK;

    return OK;
}


/*
 * Append message to end of file or maildrop.
 */

int
mbx_copy (char *mailbox, int mbx_style, int md, int fd,
          char *text)
{
    int i, j, size;
    char *cp, buffer[BUFSIZ + 1];   /* Space for NUL. */
    FILE *fp;

    size = 0;

    switch (mbx_style) {
	case MMDF_FORMAT: 
	default: 
	    j = LEN(MMDF_DELIM);
	    if (write (md, MMDF_DELIM, j) != j)
		return NOTOK;

	    if (text) {
		i = strlen (text);
		if (write (md, text, i) != i)
		    return NOTOK;
		for (cp = text; *cp++; size++)
		    if (*cp == '\n')
			size++;
	    }
		    
	    while ((i = read (fd, buffer, sizeof buffer - 1)) > 0) {
                buffer[i] = '\0';   /* Terminate for stringdex(). */

		for ( ;	(j = stringdex (MMDF_DELIM, buffer)) >= 0; buffer[j]++)
		    continue;
		for ( ;	(j = stringdex (MMDF_DELIM, buffer)) >= 0; buffer[j]++)
		    continue;
		if (write (md, buffer, i) != i)
		    return NOTOK;
	    }

	    j = LEN(MMDF_DELIM);
	    if (write (md, MMDF_DELIM, j) != j)
		return NOTOK;

            return i == -1 ? -1 : OK;

	case MBOX_FORMAT:
	    if ((j = dup (fd)) == -1)
		return NOTOK;
	    if ((fp = fdopen (j, "r")) == NULL) {
		close (j);
		return NOTOK;
	    }

	    /* If text is given, we add it to top of message */
	    if (text) {
		i = strlen (text);
		if (write (md, text, i) != i)
		    return NOTOK;
		for (cp = text; *cp++; size++)
		    if (*cp == '\n')
			size++;
	    }
		    
	    for (j = 0; fgets (buffer, sizeof(buffer), fp) != NULL; j++) {

		/*
		 * Check the first line, and make some changes.
		 */
		if (j == 0 && !text) {
		    /*
		     * Change the "Return-Path:" field (if in first line)
		     * back to "From ".
		     */
                    if (has_prefix(buffer, "Return-Path:")) {
			char tmpbuffer[sizeof buffer];
			char *tp, *ep, *fp;

			strncpy(tmpbuffer, buffer, sizeof(tmpbuffer));
			ep = tmpbuffer + 13;
			if (!(fp = strchr(ep + 1, ' ')))
			    fp = strchr(ep + 1, '\n');
			tp = dctime(dlocaltimenow());
			snprintf (buffer, sizeof(buffer), "From %.*s  %s",
				(int)(fp - ep), ep, tp);
		    } else if (has_prefix(buffer, "X-Envelope-From:")) {
			/*
			 * Change the "X-Envelope-From:" field
			 * (if first line) back to "From ".
			 */
			char tmpbuffer[sizeof buffer];
			char *ep;

			strncpy(tmpbuffer, buffer, sizeof(tmpbuffer));
			ep = tmpbuffer + 17;
			snprintf (buffer, sizeof(buffer), "From %s", ep);
		    } else if (!has_prefix(buffer, "From ")) {
                        /* There is no "From " line; add one. */
                        char from[] = "From nobody@nowhere\t";
                        char prefixed[LEN(from) + DCTIME_SIZEOF - 1 + sizeof buffer];
                        snprintf(prefixed, sizeof prefixed, "%s%s%s",
                            from, dctime(dlocaltimenow()), buffer);
                        ABORTCPY(buffer, prefixed);
		    }
		}

		/*
		 * If this is not first line, and begins with
		 * "From ", then prepend line with ">".
		 */
		if (j != 0 && has_prefix(buffer, "From ")) {
		    if (write (md, ">", 1) < 0) {
			advise (mailbox, "write");
		    }
		    size++;
		}
		i = strlen (buffer);
		if (write (md, buffer, i) != i) {
		    fclose (fp);
		    return NOTOK;
		}
	    }
	    if (write (md, "\n", 1) != 1) {
		fclose (fp);
		return NOTOK;
	    }

	    fclose (fp);
	    lseek(fd, 0, SEEK_END);

	    return OK;
    }
}


/*
 * Close and unlock file/maildrop.
 */

int
mbx_close (char *mailbox, int md)
{
    if (lkclosespool (md, mailbox) == 0)
        return OK;
    return NOTOK;
}
