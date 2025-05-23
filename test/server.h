/* server.h - Utilities for fake servers used by the nmh test suite
 *
 * This code is Copyright (c) 2014, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information.
 */

int serve(const char *, const char *);
void putcrlf(int, char *);
