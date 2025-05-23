/* print_sw.h -- print switches
 *
 * This code is Copyright (c) 2017, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information. */

/* print_sw() prints the switches in swp which start with substr to the
 * file fp.  An empty substr prints all switches and compresses a
 * ‘nofoo’ switch into the preceding ‘foo’ switch.  prefix is printed
 * before each switch name, e.g. ‘-’. */
void print_sw(const char *substr, const struct swit *swp, char *prefix, FILE *fp);
