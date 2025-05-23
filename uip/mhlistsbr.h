/* mhlistsbr.h -- routines to list information about the
 *             -- contents of MIME messages
 *
 * This code is Copyright (c) 2017, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information. */

/*
 * Given a list of messages, display information about them on standard
 * output.
 *
 * Arguments are:
 *
 * cts		- An array of CT elements of messages that need to be
 *		  displayed.  Array is terminated by a NULL.
 * headsw	- If 1, display a column header.
 * sizesw	- If 1, display the size of the part.
 * verbosw	- If 1, display verbose information
 * debugsw	- If true, turn on debugging for the output.
 * disposw	- If 1, display MIME part disposition information.
 *
 */
void list_all_messages(CT *, int, int, int, bool, int);

/*
 * Display content-appropriate information on MIME parts, descending recursively
 * into multipart content if appropriate.  Uses list_content() for displaying
 * generic information.
 *
 * Arguments and return value are the same as list_content().
 */
int list_switch(CT, int, int, int, bool, int);

/*
 * List the content information of a single MIME part on stdout.
 *
 * Arguments are:
 *
 * ct		- MIME Content structure to display.
 * toplevel	- If set, we're at the top level of a message
 * realsize	- If set, determine the real size of the content
 * verbose	- If set, output verbose information
 * debug	- If true, turn on debugging for the output
 * dispo	- If set, display MIME part disposition information.
 *
 * Returns OK on success, NOTOK otherwise.
 */
int list_content(CT, int, int, int, bool, int);
