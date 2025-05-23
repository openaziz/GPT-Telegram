/* mhbuildsbr.h -- routines to expand/translate MIME composition files
 *
 * This code is Copyright (c) 2017, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information. */

/*
 * Translate a composition file into a MIME data structure.  Arguments are:
 *
 * infile	- Name of input filename
 * autobuild    - A flag to indicate if the composition file parser is
 *		  being run in automatic mode or not.  In auto mode,
 *		  if a MIME-Version header is encountered it is assumed
 *		  that the composition file is already in MIME format
 *		  and will not be processed further.  Otherwise, an
 *		  error is generated.
 * dist		- A flag to indicate if we are being run by "dist".  In
 *		  that case, add no MIME headers to the message.  Existing
 *		  headers will still be encoded by RFC 2047.
 * directives	- A flag to control whether or not build directives are
 *		  processed by default.
 * encoding	- The default encoding to use when doing RFC 2047 header
 *		  encoding.  Must be one of CE_UNKNOWN, CE_BASE64, or
 *		  CE_QUOTED.
 * contentid    - Whether a ‘Content-ID’ field should be added.
 * rfc934	- Whether the multipart/digest for ‘#forw’ should
 *		  attempt compliance with RFC 934.
 * maxunencoded	- The maximum line length before the default encoding for
 *		  text parts is quoted-printable.
 * listsw	- Whether to list the MIME contents.
 * verbose	- If 1, output verbose information during message composition
 *
 * Returns a CT structure describing the resulting MIME message.  If the
 * -auto flag is set and a MIME-Version header is encountered, the return
 * value is NULL.
 */
CT build_mime(char *, bool, int, int, int, bool, bool, size_t, bool, int);
