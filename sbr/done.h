/* done.h -- terminate the program
 *
 * This code is Copyright (c) 2017, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information. */

void set_done(void (*)(int) NORETURN);
void done(int) NORETURN;
