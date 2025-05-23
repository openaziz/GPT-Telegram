/* oauth_prof.h -- OAuth 2.0 implementation for XOAUTH2 in SMTP and POP3.
 *
 * This code is Copyright (c) 2017, by the authors of nmh.  See the
 * COPYRIGHT file in the root directory of the nmh distribution for
 * complete copyright information. */

/*
 * Retrieve the various entries for the OAuth mechanism
 */
bool mh_oauth_get_service_info(const char *, mh_oauth_service_info *, char *, size_t);

/*
 * Return the null-terminated file name for storing this service's OAuth tokens.
 *
 * Accesses global m_defs via context_find.
 *
 * Never returns NULL.
 */
char *mh_oauth_cred_fn(const char *);
