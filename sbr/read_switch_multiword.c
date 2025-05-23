/* read_switch_multiword.c -- get an answer from the user and return a string array
 *
 * This code is Copyright (c) 2002, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information.
 */

#include "h/mh.h"
#include "read_switch_multiword.h"
#include "smatch.h"
#include "brkstring.h"
#include "ambigsw.h"
#include "print_sw.h"
#include <signal.h>
#include "signals.h"
#include <setjmp.h>

static char ansbuf[BUFSIZ];
static sigjmp_buf sigenv;

/*
 * static prototypes
 */
static void intrser (int);


char **
read_switch_multiword (const char *prompt, const struct swit *ansp)
{
    int i;
    SIGNAL_HANDLER istat = NULL;
    char *cp, **cpp;

    if (sigsetjmp(sigenv, 1)) {
	SIGNAL (SIGINT, istat);
	return NULL;
    }
    istat = SIGNAL (SIGINT, intrser);

    for (;;) {
	fputs(prompt, stdout);
	fflush (stdout);
	cp = ansbuf;
	while ((i = getchar ()) != '\n') {
	    if (i == EOF) {
		/*
		 * If we get an EOF, return
		 */
		if (feof(stdin))
		    siglongjmp (sigenv, 1);

		/*
		 * For errors, if we get an EINTR that means that we got
		 * a signal and we should retry.  If we get another error,
		 * then just return.
		 */

		if (ferror(stdin)) {
		    if (errno == EINTR) {
			clearerr(stdin);
			continue;
		    }
		    fprintf(stderr, "\nError %s during read\n",
			    strerror(errno));
		    siglongjmp (sigenv, 1);
		}

                /* Just for completeness's sake... */
                fprintf(stderr, "\nUnknown problem in getchar()\n");
                siglongjmp(sigenv, 1);
	    }
	    if (cp < &ansbuf[sizeof ansbuf - 1])
		*cp++ = i;
	}
	*cp = '\0';
	if (ansbuf[0] == '?' || cp == ansbuf) {
	    puts("Options are:");
	    print_sw("", ansp, "", stdout);
	    continue;
	}
	cpp = brkstring (ansbuf, " ", NULL);
	switch (smatch (*cpp, ansp)) {
	    case AMBIGSW: 
		ambigsw (*cpp, ansp);
		continue;
	    case UNKWNSW: 
		printf (" -%s unknown. Hit <CR> for help.\n", *cpp);
		continue;
	    default: 
		SIGNAL (SIGINT, istat);
		return cpp;
	}
    }
}


static void
intrser (int i)
{
    NMH_UNUSED (i);

    siglongjmp (sigenv, 1);
}
