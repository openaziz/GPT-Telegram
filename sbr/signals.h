/* signals.h -- general signals interface for nmh
 *
 * This code is Copyright (c) 2017, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information. */

/*
 * The type for a signal handler
 */
typedef void (*SIGNAL_HANDLER)(int);

/*
 * prototypes
 */
SIGNAL_HANDLER SIGNAL(int, SIGNAL_HANDLER);
SIGNAL_HANDLER SIGNAL2(int, SIGNAL_HANDLER);
int setup_signal_handlers(void);
