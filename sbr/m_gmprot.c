/* m_gmprot.c -- return the msg-protect value
 *
 * This code is Copyright (c) 2002, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information.
 */

#include "h/mh.h"
#include "m_gmprot.h"
#include "context_find.h"
#include "atooi.h"
#include "globals.h"


int
m_gmprot (void)
{
    char *cp;

    return atooi ((cp = context_find ("msg-protect")) && *cp ? cp : msgprot);
}
