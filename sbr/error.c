/* error.c -- main error handling routines
 *
 * This code is Copyright (c) 2002, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information.
 */

#include "h/mh.h"
#include "error.h"

#include <sys/types.h>
#include <sys/uio.h>
#include "done.h"
#include "globals.h"

/* inform calls advertise() with no what and no tail.
 * Thus the simple "[invo_name: ]fmt\n" results. */
void
inform(char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    advertise(NULL, NULL, fmt, ap);
    va_end(ap);
}


/* advise calls advertise() with no tail to print fmt, and perhaps what,
 * to stderr.
 * Thus "[invo_name: ]fmt[[ what]: errno]\n" results. */
void
advise (const char *what, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    advertise (what, NULL, fmt, ap);
    va_end(ap);
}


/* adios is the same as advise(), but then "ends" the program.
 * It calls advertise() with no tail to print fmt, and perhaps what, to
 * stderr, and exits the program with an error status.
 * Thus "[invo_name: ]fmt[[ what]: errno]\n" results.
 *
 * The route to exit is via the done function pointer and may not be
 * straightforward, e.g. longjmp(3), but it must not return to adios().
 * If it does then it's a bug and adios() will abort(3) as callers do
 * not expect execution to continue. */
void NORETURN
adios (const char *what, const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    advertise (what, NULL, fmt, ap);
    va_end(ap);
    done (1);
    abort();
}


/* die is the same as adios(), but without the what as that's commonly
 * NULL.
 * Thus "[invo_name: ]fmt\n" results. */
void NORETURN
die(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    advertise(NULL, NULL, fmt, ap);
    va_end(ap);
    done(1);
    abort();
}


/* admonish calls advertise() with a tail indicating the program
 * continues.
 * Thus "[invo_name: ]fmt[[ what]: errno], continuing...\n" results. */
void
admonish (char *what, char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    advertise (what, "continuing...", fmt, ap);
    va_end(ap);
}


/* advertise prints fmt and ap to stderr after flushing stdout.
 * The print happens in a single syscall from a buffer.
 * If invo_name isn't NULL or empty     then "invo_name: " is buffered.
 *                                      Then fmt           is buffered.
 * If what isn't NULL or empty          then " what"       is buffered.
 * If what isn't NULL and errno isn't 0 then ": errno"     is buffered.
 * If tail isn't NULL or empty          then ", tail"      is buffered.
 *                                      Then "\n"          is buffered.
 *
 * Note, a what of "" doesn't print it or the preceding space, but does
 * trigger the printing of a non-zero errno; such is the cunning design.
 *
 * In summary: "[invo_name: ]fmt[[ what]: errno][, tail]\n". */
void
advertise (const char *what, char *tail, const char *fmt, va_list ap)
{
    int	eindex = errno;
    char buffer[NMH_BUFSIZ];
    struct iovec iob[10], *iov;
    size_t niov;

    iov = iob;

#define APPEND_IOV(p, len) \
    iov->iov_base = (p); \
    iov->iov_len = (len); \
    iov++

#define ADD_LITERAL(s) APPEND_IOV((s), LEN(s))
#define ADD_VAR(s) APPEND_IOV((s), strlen(s))

    if (invo_name && *invo_name) {
        ADD_VAR(invo_name);
	ADD_LITERAL(": ");
    }
    
    vsnprintf (buffer, sizeof(buffer), fmt, ap);
    ADD_VAR(buffer);
    if (what) {
	if (*what) {
            ADD_LITERAL(" ");
            ADD_VAR((void *)what);
	}
        if (eindex) {
            ADD_LITERAL(": ");
            char *err = strerror(eindex);
            ADD_VAR(err);
        }
    }
    if (tail && *tail) {
        ADD_LITERAL(", ");
        ADD_VAR(tail);
    }
    ADD_LITERAL("\n");

#undef ADD_LITERAL
#undef ADD_VAR

    niov = iov - iob;
    assert(niov <= DIM(iob));

    fflush (stdout);
    fflush (stderr);
    if (writev(fileno(stderr), iob, niov) == -1) {
        snprintf(buffer, sizeof buffer, "%s: write stderr failed: %d\n",
            invo_name && *invo_name ? invo_name : "nmh", errno);
        if (write(2, buffer, strlen(buffer)) == -1) {
            /* Ignore.  if-statement needed to shut up compiler. */
        }
    }
}
