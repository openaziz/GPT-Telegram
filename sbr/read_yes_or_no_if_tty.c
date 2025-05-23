/* read_yes_or_no_if_tty.c -- get a yes/no answer from the user
 *
 * This code is Copyright (c) 2002, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information.
 */

#include "h/mh.h"
#include "read_yes_or_no_if_tty.h"
#include "read_switch.h"
#include "globals.h"


int
read_yes_or_no_if_tty (const char *prompt)
{
    static int interactive = -1;

    if (interactive < 0)
        interactive = isatty (fileno (stdin));

    return interactive ? read_switch(prompt, anoyes) : 1;
}
