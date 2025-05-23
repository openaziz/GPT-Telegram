/* print_version.c -- print a version string
 *
 * This code is Copyright (c) 2002, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information.
 */

#include "config/version.h"
#include "h/mh.h"
#include "print_version.h"


void
print_version (char *invo_name)
{
    printf("%s -- %s\n", invo_name, version_str);
}
